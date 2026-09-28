// unit_update_aiming_overlay_angles  (Ghidra: unit_update_aiming_overlay_angles)
// address 0x563b50, size 1352 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN (objdump 0x563b50..0x56409a; the draft called the aiming blend with a table pointer and two angles).
//   Stack: unit, the node orientations being built. Graph = the unit tag's animation graph (+0x44), animations at
//   graph +0x78 (0xb4 each).
//   1. The three overlay slots layer onto the orientations: +0x2aa replaces (frame +0x2ac), +0x2ae and +0x2b2
//      overlay (frames +0x2b0, +0x2b4) -- ESI = the animation, stack (frame, orientations). The aim/look flags
//      +0x2b6/+0x2b7 are cleared.
//   2. Units whose tag has flag 0x800 (+0x17c) or no unit block (+0x2a0 == -1) stop here. The unit block is graph
//      units (+0x10) [+0x2a0] (0x64 each; animations +0x40 count / +0x44 int16 slots).
//      An emotion frame (+0x2a8 unless -1) overlays the emotion animation (slot 11, or +0x21e when set) when the
//      frame is below its frame count (+0x22). A positive weight +0x2e8 overlays slot 10 at frame 0 with that
//      weight. With +0x298 bit 1, slots 2..4 overlay interpolated at (frame count - 1) * the weights +0x364..+0x36c.
//   3. Tags with flag 0x400, units in states 0x17..0x23 or 0x29, or with +0x2a4 set stop here. The aim angles start
//      as *global_zero_vector2d (0x00696730). With an aiming overlay (+0x29a): the aiming vector (+0x23c) in the
//      unit's own frame (object_get_orientation, left = forward x up, origin) gives yaw = atan2(y, x) and pitch =
//      atan2(z, |xy|); the weapon's aiming screen (unit block weapons +0x5c [+0x2a1], 0xbc each, +0x60) sets the
//      limits +0x2b8..+0x2c4 (-left, right, -down, up: frames times per-frame angles) and animation_aiming_screen_blend
//      (EDI animation, stack: screen, yaw, pitch, orientations) poses it. With a weapon or a player and a looking
//      overlay (+0x29c): the looking vector (+0x260) gives angles relative to the aim, the unit block's own screen
//      (+0x20) sets +0x2c8..+0x2d4 and poses the looking overlay the same way.
// blam-cc: stack -> unit_index, orientations (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern float *global_zero_vector2d_pointer;      // 0x00696730
extern real_point3d *global_zero_vector3d_pointer;  // 0x006966f8

extern void animation_replace_frame_orientations(void *animation, int16_t frame, void *out_orientations); // 0x4d4dd0, ESI, stack
extern void animation_overlay_frame_orientations(void *animation, int16_t frame, void *out_orientations); // 0x4d4f90, ESI, stack
extern void animation_overlay_frame_orientations_weighted(void *animation, int16_t frame, float weight,
    void *out_orientations); // 0x4d51a0, EDI, stack
extern void animation_overlay_interpolated_frame_orientations(void *animation, float frame, void *out_orientations);
    // 0x4d53f0, EDI, stack
extern void animation_aiming_screen_blend(void *animation, void *screen, real yaw, real pitch, void *orientation_out);
    // 0x4d5c00, EDI, stack
extern void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up); // 0x4f6970
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
extern void matrix4x3_inverse_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cc080
extern double atan2(double y, double x);
extern double sqrt(double x);

// Builds the unit's own frame (scale 1, orientation, left = forward x up, the origin) and returns `direction`
// expressed in it as (yaw, pitch).
static void aiming_angles_in_unit_frame(uint32_t unit_index, real_vector3d *direction, float *yaw, float *pitch)
{
    real_matrix4x3 frame;
    real_vector3d local;

    frame.scale = 1.0f;
    object_get_orientation(&frame.forward, unit_index, &frame.up);
    vector3d_cross_product(&frame.left, &frame.forward, &frame.up);
    frame.position = *global_zero_vector3d_pointer;
    matrix4x3_inverse_transform_normal(&local, direction, &frame);
    *yaw = (float)atan2((double)local.j, (double)local.i);
    *pitch = (float)atan2((double)local.k, sqrt((double)(local.i * local.i + local.j * local.j)));
}

// Screen limits: -left, right, -down, up (the frame counts at +0x08/+0x0a/+0x14/+0x16 times the per-frame angles at
// +0x00/+0x04/+0x0c/+0x10).
static void aiming_screen_limits(const uint8_t *screen, float *out)
{
    out[0] = -((float)*(int16_t *)(screen + 0x08) * *(float *)(screen + 0x00));
    out[1] = (float)*(int16_t *)(screen + 0x0a) * *(float *)(screen + 0x04);
    out[2] = -((float)*(int16_t *)(screen + 0x14) * *(float *)(screen + 0x0c));
    out[3] = (float)*(int16_t *)(screen + 0x16) * *(float *)(screen + 0x10);
}

void unit_update_aiming_overlay_angles(uint32_t unit_index, void *output)
{
    uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 0xc + 8);
    uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)(unit_tag + 0x44) & 0xffff].data;
    uint8_t *animations = *(uint8_t **)(graph + 0x78);
    uint8_t *block;
    float aim_yaw;
    float aim_pitch;
    int8_t state;

    if (*(int16_t *)(unit + 0x2aa) != -1) {
        animation_replace_frame_orientations(animations + *(int16_t *)(unit + 0x2aa) * 0xb4,
            (int16_t)*(uint16_t *)(unit + 0x2ac), output);
    }
    if (*(int16_t *)(unit + 0x2ae) != -1) {
        animation_overlay_frame_orientations(animations + *(int16_t *)(unit + 0x2ae) * 0xb4,
            (int16_t)*(uint16_t *)(unit + 0x2b0), output);
    }
    if (*(int16_t *)(unit + 0x2b2) != -1) {
        animation_overlay_frame_orientations(animations + *(int16_t *)(unit + 0x2b2) * 0xb4,
            (int16_t)*(uint16_t *)(unit + 0x2b4), output);
    }
    unit[0x2b6] = 0;
    unit[0x2b7] = 0;
    if ((*(uint32_t *)(unit_tag + 0x17c) & 0x800) || unit[0x2a0] == 0xff) {
        return;
    }
    block = *(uint8_t **)(graph + 0x10) + (int8_t)unit[0x2a0] * 0x64;

    if (unit[0x2a8] != 0xff) {
        int16_t emotion = (*(int32_t *)(block + 0x40) > 0xb) ? (*(int16_t **)(block + 0x44))[0xb] : -1;

        if (((unit_object *)unit)->unit.emotion_animation_index != -1) {
            emotion = ((unit_object *)unit)->unit.emotion_animation_index;
        }
        if (emotion != -1) {
            uint8_t *record = animations + emotion * 0xb4;
            int8_t frame = (int8_t)unit[0x2a8];

            if (frame >= 0 && frame < *(int16_t *)(record + 0x22)) {
                animation_overlay_frame_orientations(record, frame, output);
            }
        }
    }
    if (((unit_object *)unit)->unit.animation_blend_weight > 0.0f && *(int32_t *)(block + 0x40) > 0xa &&
        (*(int16_t **)(block + 0x44))[0xa] != -1) {
        animation_overlay_frame_orientations_weighted(animations + (*(int16_t **)(block + 0x44))[0xa] * 0xb4, 0,
            ((unit_object *)unit)->unit.animation_blend_weight, output);
    }
    if (unit[0x298] & 2) {
        int32_t slot;

        for (slot = 2; slot < 5; slot++) {
            if (slot < *(int32_t *)(block + 0x40) && (*(int16_t **)(block + 0x44))[slot] != -1) {
                uint8_t *record = animations + (*(int16_t **)(block + 0x44))[slot] * 0xb4;
                int32_t last_frame = *(int16_t *)(record + 0x22) - 1;

                animation_overlay_interpolated_frame_orientations(record,
                    (float)last_frame * *(float *)(unit + 0x364 + (slot - 2) * 4), output);
            }
        }
    }

    if (*(uint32_t *)(unit_tag + 0x17c) & 0x400) {
        return;
    }
    state = (int8_t)unit[0x2a3];
    if ((state >= 0x17 && state <= 0x23) || state == 0x29 || unit[0x2a4] != 0) {
        return;
    }

    aim_yaw = global_zero_vector2d_pointer[0];
    aim_pitch = global_zero_vector2d_pointer[1];
    if (((unit_object *)unit)->unit.animation_instance != -1) {
        uint8_t *screen = *(uint8_t **)(block + 0x5c) + (int8_t)unit[0x2a1] * 0xbc + 0x60;

        aiming_angles_in_unit_frame(unit_index, (real_vector3d *)(unit + 0x23c), &aim_yaw, &aim_pitch);
        unit[0x2b6] = 1;
        aiming_screen_limits(screen, (float *)(unit + 0x2b8));
        animation_aiming_screen_blend(animations + ((unit_object *)unit)->unit.animation_instance * 0xb4, screen, aim_yaw, aim_pitch, output);
    }

    if (((unit_object *)unit)->unit.current_weapon_index == -1 && ((unit_object *)unit)->unit.controlling_player == k_datum_index_none) {
        return;
    }
    if (*(int16_t *)(unit + 0x29c) != -1) {
        uint8_t *screen = block + 0x20;
        float look_yaw;
        float look_pitch;

        aiming_angles_in_unit_frame(unit_index, (real_vector3d *)(unit + 0x260), &look_yaw, &look_pitch);
        unit[0x2b7] = 1;
        look_yaw -= aim_yaw;
        look_pitch -= aim_pitch;
        aiming_screen_limits(screen, (float *)(unit + 0x2c8));
        animation_aiming_screen_blend(animations + *(int16_t *)(unit + 0x29c) * 0xb4, screen, look_yaw, look_pitch,
            output);
    }
}

#if 0
Original Ghidra decompilation (0x563b50):

void FUN_00563b50(uint param_1,undefined4 param_2)

{
  char cVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  float *pfVar7;
  float10 fVar8;
  float10 fVar9;
  int local_64;
  int local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  undefined4 local_40 [7];
  undefined1 local_24 [12];
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *(int *)((*(uint *)(iVar3 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(short *)((int)puVar2 + 0x2aa) != -1) {
    FUN_004d4dd0((short)puVar2[0xab],param_2);
  }
  if (*(short *)((int)puVar2 + 0x2ae) != -1) {
    FUN_004d4f90((short)puVar2[0xac],param_2);
  }
  if (*(short *)((int)puVar2 + 0x2b2) != -1) {
    FUN_004d4f90((short)puVar2[0xad],param_2);
  }
  *(undefined1 *)((int)puVar2 + 0x2b6) = 0;
  *(undefined1 *)((int)puVar2 + 0x2b7) = 0;
  if (((*(uint *)(iVar3 + 0x17c) & 0x800) == 0) && ((char)puVar2[0xa8] != -1)) {
    iVar6 = (char)puVar2[0xa8] * 100 + *(int *)(iVar4 + 0x10);
    cVar1 = (char)puVar2[0xaa];
    if (cVar1 != -1) {
      if (*(int *)(iVar6 + 0x40) < 0xc) {
        sVar5 = -1;
      }
      else {
        sVar5 = *(short *)(*(int *)(iVar6 + 0x44) + 0x16);
      }
      if (*(short *)((int)puVar2 + 0x21e) != -1) {
        sVar5 = *(short *)((int)puVar2 + 0x21e);
      }
      if (((sVar5 != -1) && (-1 < cVar1)) &&
         ((short)cVar1 < *(short *)(sVar5 * 0xb4 + *(int *)(iVar4 + 0x78) + 0x22))) {
        FUN_004d4f90(CONCAT22((short)((uint)*(int *)(iVar4 + 0x10) >> 0x10),(short)cVar1),param_2);
      }
    }
    if (((0.0 < (float)puVar2[0xba]) && (10 < *(int *)(iVar6 + 0x40))) &&
       (*(short *)(*(int *)(iVar6 + 0x44) + 0x14) != -1)) {
      FUN_004d51a0(0,puVar2[0xba],param_2);
    }
    if ((puVar2[0xa6] & 2) != 0) {
      local_5c = 2;
      local_64 = 4;
      pfVar7 = (float *)(puVar2 + 0xd9);
      local_58 = 4.2039e-45;
      do {
        if (((-1 < local_5c) && (local_5c < *(int *)(iVar6 + 0x40))) &&
           (sVar5 = *(short *)(local_64 + *(int *)(iVar6 + 0x44)), sVar5 != -1)) {
          model_vertices_get_interpolated_frame
                    ((float)(*(short *)(sVar5 * 0xb4 + 0x22 + *(int *)(iVar4 + 0x78)) + -1) *
                     *pfVar7,param_2);
        }
        local_64 = local_64 + 2;
        pfVar7 = pfVar7 + 1;
        local_5c = local_5c + 1;
        local_58 = (float)((int)local_58 + -1);
      } while (local_58 != 0.0);
      local_58 = 0.0;
    }
    if ((*(uint *)(iVar3 + 0x17c) & 0x400) == 0) {
      cVar1 = *(char *)((int)puVar2 + 0x2a3);
      if (((cVar1 < '\x17') || (('#' < cVar1 && (cVar1 != ')')))) && ((char)puVar2[0xa9] == '\0')) {
        local_4c = *(float *)PTR_DAT_00696730;
        local_48 = *(float *)(PTR_DAT_00696730 + 4);
        if (*(short *)((int)puVar2 + 0x29a) != -1) {
          pfVar7 = (float *)(*(char *)((int)puVar2 + 0x2a1) * 0xbc + 0x60 + *(int *)(iVar6 + 0x5c));
          local_40[0] = 0x3f800000;
          object_get_orientation(local_24);
          vector3d_cross_product(local_24);
          local_18 = *(undefined4 *)PTR_DAT_006966f8;
          local_14 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
          local_10 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
          matrix4x3_inverse_transform_normal(local_40);
          *(undefined1 *)((int)puVar2 + 0x2b6) = 1;
          fVar8 = (float10)fpatan((float10)local_54,(float10)local_58);
          local_4c = (float)fVar8;
          fVar8 = (float10)fpatan((float10)local_50,
                                  SQRT((float10)local_54 * (float10)local_54 +
                                       (float10)local_58 * (float10)local_58));
          local_48 = (float)fVar8;
          puVar2[0xae] = (uint)-((float)(int)*(short *)(pfVar7 + 2) * *pfVar7);
          puVar2[0xaf] = (uint)((float)(int)*(short *)((int)pfVar7 + 10) * pfVar7[1]);
          puVar2[0xb0] = (uint)-((float)(int)*(short *)(pfVar7 + 5) * pfVar7[3]);
          puVar2[0xb1] = (uint)((float)(int)*(short *)((int)pfVar7 + 0x16) * pfVar7[4]);
          model_vertices_bilinear_interpolate_2d_frame(pfVar7,local_4c,local_48,param_2);
        }
        if (((*(short *)((int)puVar2 + 0x2f2) != -1) || (puVar2[0x86] != 0xffffffff)) &&
           ((short)puVar2[0xa7] != -1)) {
          local_40[0] = 0x3f800000;
          object_get_orientation(local_24);
          vector3d_cross_product(local_24);
          local_18 = *(undefined4 *)PTR_DAT_006966f8;
          local_14 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
          local_10 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
          matrix4x3_inverse_transform_normal(local_40);
          *(undefined1 *)((int)puVar2 + 0x2b7) = 1;
          fVar8 = (float10)fpatan((float10)local_54,(float10)local_58);
          fVar9 = (float10)fpatan((float10)local_50,
                                  SQRT((float10)local_58 * (float10)local_58 +
                                       (float10)local_54 * (float10)local_54));
          puVar2[0xb2] = (uint)-((float)(int)*(short *)(iVar6 + 0x28) * *(float *)(iVar6 + 0x20));
          puVar2[0xb3] = (uint)((float)(int)*(short *)(iVar6 + 0x2a) * *(float *)(iVar6 + 0x24));
          puVar2[0xb4] = (uint)-((float)(int)*(short *)(iVar6 + 0x34) * *(float *)(iVar6 + 0x2c));
          puVar2[0xb5] = (uint)((float)(int)*(short *)(iVar6 + 0x36) * *(float *)(iVar6 + 0x30));
          model_vertices_bilinear_interpolate_2d_frame
                    ((float *)(iVar6 + 0x20),(float)(fVar8 - (float10)local_4c),
                     (float)(fVar9 - (float10)local_48),param_2);
        }
      }
    }
  }
  return;
}
#endif
