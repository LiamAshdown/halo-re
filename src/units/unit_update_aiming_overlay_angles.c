// unit_update_aiming_overlay_angles  (Ghidra: unit_update_aiming_overlay_angles)
// address 0x563b50, size 1352 bytes
// name confidence: 0.4 (phase2 candidate)   rewrite confidence: 0.15
// evidence: types/units.h unit_data.overlays[3] (0x2aa/0x2ae/0x2b2), .aiming_bounds_valid/
//   .looking_bounds_valid (0x2b6/0x2b7), .animation_definition_index/.emotion_animation_frame
//   (0x2a0/0x2a8), .emotion_animation_index (0x21e), .unknown_2a4, .animation_blend_weight
//   (0x2e8, ">0 blends animation 10 of the graph unit block"), .animation_state_flags (0x298,
//   _unit_animation_flag_aiming_enabled), .animation_controls_smoothed[3] (0x364),
//   .animation_state (0x2a3), .current_weapon_index (0x2f2), .desired_weapon_index... (see
//   body); types/tags.h Unit.unit_flags (tag+0x17c, simple_creature bit 0x800, has_no_aiming
//   bit 0x400); the same ModelAnimationsAnimationGraphUnitSeat (animations TagReflexive at
//   0x40) chain used by unit_try_set_animation_state.
// register convention: unit index in EAX, an output/blend pointer forwarded to the animation
//   helpers in the stack parameter.
//   // blam-cc: param_1 (EAX) -> unit_index, param_2 -> output
// UNSURE: this is the least-recovered function in the whole batch. The three overlay-advance
//   calls, the emotion-animation trigger and the animation_blend_weight-gated vertex-frame loop
//   are reproduced with reasonable confidence (their destination fields and gating conditions
//   all match named header fields exactly). The final two aiming/looking-angle blocks compute
//   yaw/pitch via object_get_orientation, vector3d_cross_product and
//   matrix4x3_inverse_transform_normal into locals (local_54/local_50/local_58) that are never
//   visibly assigned in this decompilation -- they are almost certainly populated through
//   overlapping stack slots the decompiler could not attribute to the right call, the same
//   class of artifact as the "hidden output" calls in unit_find_weapon_marker_transform.c.
//   Rather than invent a plausible-looking vector algebra to fill that gap, the two blocks are
//   reproduced only up to the point where they write the four aiming_bounds/looking_bounds
//   floats (0x2b8/0x2c8, each an int16 frame count times a float scale from the Biped/graph
//   data, matching the header's description) and set the *_bounds_valid flags; the fpatan/
//   animation_aiming_screen_blend calls that derive the two angles feeding those
//   floats are left as a best-effort sketch and are very likely inexact in the small details.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t default_aim_angles[8]; // 0x00696730, PTR_DAT_00696730, UNSURE shape (two floats read)
extern real_vector3d *global_up3d_and_neighbors_pointer; // 0x006966f8, PTR_DAT_006966f8, UNSURE identity

extern void animation_replace_frame_orientations(uint32_t overlay_frame_and_index, void *output);       // 0x4d4dd0, UNSURE signature
extern void animation_overlay_frame_orientations(uint32_t overlay_frame_and_index, void *output);       // 0x4d4f90, UNSURE signature
extern void animation_overlay_frame_orientations_weighted(uint32_t zero, float blend_weight, void *output);      // 0x4d51a0, UNSURE signature
extern void animation_overlay_interpolated_frame_orientations(float frame, void *output);   // 0x4d53f0, UNSURE signature
extern void animation_aiming_screen_blend(float *table, float u, float v, void *output); // 0x4d5c00, UNSURE signature
extern void object_get_orientation(void *out);                                 // 0x4f6970, UNSURE: implicit unit_index
  // real signature (object_get_orientation.c): void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up); Ghidra recovered 1 of 3 args at this call site
// vector3d_cross_product (0x4052c0) computes  *out = stack_operand x ecx_operand,  with out
// in EAX, ecx_operand in ECX and stack_operand pushed -- read out of the callee own
// decompilation (in_EAX / in_ECX / param_1) and matching
// src/objects/object_set_position_and_orientation.c. Ghidra binds only the stack operand at
// the call sites below, so the declaration is left unprototyped.
extern void vector3d_cross_product(); // 0x4052c0
extern void matrix4x3_inverse_transform_normal(void *v);                       // 0x4cc080
  // real signature (matrix4x3_inverse_transform_normal.c): void matrix4x3_inverse_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); Ghidra recovered 1 of 3 args at this call site

void unit_update_aiming_overlay_angles(uint32_t unit_index, void *output) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    void *graph = tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)((uint8_t *)graph + 0x10);

    if (unit->overlays[0].animation_index != -1) {
        animation_replace_frame_orientations(unit->overlays[0].frame, output);
    }
    if (unit->overlays[1].animation_index != -1) {
        animation_overlay_frame_orientations(unit->overlays[1].frame, output);
    }
    if (unit->overlays[2].animation_index != -1) {
        animation_overlay_frame_orientations(unit->overlays[2].frame, output);
    }
    unit->aiming_bounds_valid = 0;
    unit->looking_bounds_valid = 0;

    if ((((Unit *)obj_tag)->unit_flags & 0x800) != 0 || unit->animation_definition_index == -1) {
        return; // simple_creature, or no animation-graph unit block: nothing else to do
    }

    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);

    if (unit->emotion_animation_frame != -1) {
        int16_t emotion_animation = -1;
        if ((int32_t)unit_seat->animations.count >= 0xc) {
            emotion_animation = *(int16_t *)((uint8_t *)unit_seat->animations.pointer + 0x16);
        }
        if (unit->emotion_animation_index != -1) {
            emotion_animation = unit->emotion_animation_index;
        }
        if (emotion_animation != -1 && unit->emotion_animation_frame >= 0) {
            uint8_t *animations = *(uint8_t **)((uint8_t *)graph + 0x78);
            ModelAnimationsAnimation *anim =
                (ModelAnimationsAnimation *)(animations + emotion_animation * 0xb4);
            if (unit->emotion_animation_frame < anim->frame_count) {
                // UNSURE: CONCAT22 packs the unit-block pointer's high 16 bits with the frame
                // index in the original; reproduced as passing the frame index alone.
                animation_overlay_frame_orientations((uint32_t)(uint16_t)unit->emotion_animation_frame, output);
            }
        }
    }

    if (unit->animation_blend_weight > 0.0f && (int32_t)unit_seat->animations.count > 10 &&
        *(int16_t *)((uint8_t *)unit_seat->animations.pointer + 0x14) != -1) {
        animation_overlay_frame_orientations_weighted(0, unit->animation_blend_weight, output);
    }

    if ((unit->animation_state_flags & _unit_animation_flag_aiming_enabled) != 0) {
        uint8_t *animations = *(uint8_t **)((uint8_t *)graph + 0x78);
        for (int32_t i = 0; i < 3; i++) {
            int16_t anim_index = *(int16_t *)((uint8_t *)unit_seat->animations.pointer + 4 + i * 2);
            if (anim_index != -1) {
                ModelAnimationsAnimation *anim = (ModelAnimationsAnimation *)(animations + anim_index * 0xb4);
                float frame = (float)(anim->frame_count - 1) * unit->animation_controls_smoothed[i];
                animation_overlay_interpolated_frame_orientations(frame, output);
            }
        }
    }

    if ((((Unit *)obj_tag)->unit_flags & 0x400) != 0) {
        return; // has_no_aiming
    }

    if ((unit->animation_state < 0x17 || (0x23 < unit->animation_state && unit->animation_state != 0x29)) &&
        unit->unknown_2a4 == 0) {
        float aim_yaw = *(float *)default_aim_angles;
        float aim_pitch = *(float *)(default_aim_angles + 4);

        if (unit->animation_instance != -1) {
            ModelAnimationsAnimationGraphWeapon *weapon_anim =
                (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)unit_seat->weapons.pointer +
                                                         unit->animation_weapon_index * 0xbc);
            // UNSURE: aim_yaw/aim_pitch derivation, see file header
            unit->aiming_bounds_valid = 1;
            unit->aiming_bounds[0] = -((float)(int16_t)weapon_anim->right_frame_count * weapon_anim->right_yaw_per_frame);
            unit->aiming_bounds[1] = (float)(int16_t)weapon_anim->left_frame_count * weapon_anim->left_yaw_per_frame;
            unit->aiming_bounds[2] = -((float)(int16_t)weapon_anim->down_pitch_frame_count * weapon_anim->down_pitch_per_frame);
            unit->aiming_bounds[3] = (float)(int16_t)weapon_anim->up_pitch_frame_count * weapon_anim->up_pitch_per_frame;
            animation_aiming_screen_blend(&weapon_anim->right_yaw_per_frame, aim_yaw, aim_pitch, output);
        }

        // Ghidra: (short)puVar2[0xa7], a *dword index* -> object + 0x29c =
        // unit_data.unknown_29c (the seat / turret overlay gate types/units.h documents),
        // NOT base_animation_state at 0x2a7. Corrected in the phase-4 review pass.
        if ((unit->current_weapon_index != -1 || unit->controlling_player != (datum_index)-1) &&
            unit->unknown_29c != -1) {
            unit->looking_bounds_valid = 1;
            unit->looking_bounds[0] = -((float)(int16_t)unit_seat->right_frame_count * unit_seat->right_yaw_per_frame);
            unit->looking_bounds[1] = (float)(int16_t)unit_seat->left_frame_count * unit_seat->left_yaw_per_frame;
            unit->looking_bounds[2] = -((float)(int16_t)unit_seat->down_pitch_frame_count * unit_seat->down_pitch_per_frame);
            unit->looking_bounds[3] = (float)(int16_t)unit_seat->up_pitch_frame_count * unit_seat->up_pitch_per_frame;
            // UNSURE: yaw/pitch derivation relative to aim_yaw/aim_pitch, see file header
            animation_aiming_screen_blend(&unit_seat->right_yaw_per_frame, 0.0f, 0.0f, output);
        }
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
