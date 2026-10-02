// player_effect_mark_damage_direction  (Ghidra: FUN_00456cf0)
// address 0x456cf0, size 779 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x456cf0..0x456ffa and its callers (object_apply_damage 0x4eeb0f, the network dispatch
//   0x456ba1): EAX is the damaged player, the stack (damage_data, direction = damage_data +0x34, random blend,
//   damage amount). Counted by player_effect_reentry_count for the whole call. For a player with a local index,
//   the damage effect tag's screen flash (+0x24), camera impulse (+0x98) and camera shake (+0xcc) are started with
//   the blend as intensity, and its sound (+0x120) is played. With a positive amount and a responsible object:
//   tag flag 0x100 lights indicator 2; otherwise, from the unit's eye (0x568f50) to the responsible object's
//   position, d is projected on (up x forward, forward, up) of the local camera (0x4479a0) and normalized; a
//   vertical part over 0.5 lights indicator 0 (above) or 2, and atan2(forward, side) outside [pi/4, 3pi/4] lights
//   indicator 1 (|angle| > pi/2) or 3.
// blam-cc: EAX -> player_index, stack -> (dd, direction, random_blend, damage_amount)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"
#include "camera.h"
#include "sound.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;                               // 0x0087a480
extern player_effect_globals *player_effect_globals_pointer;  // 0x006f1884
extern tag_instance *tag_instances;                           // 0x0087bc14
extern int32_t player_effect_reentry_count;                   // 0x00719ccc

extern double atan2(double y, double x); // fpatan is a single x87 FPATAN instruction
extern double fabs(double x);
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern datum_index local_player_to_player_index(int16_t local_player_index); // 0x474d30, AX
extern observer_camera *observer_get_camera(int16_t player_index); // 0x4479a0, CX
extern void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out); // 0x568f50, ECX, ESI
extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size,
    uint32_t first_person_hint); // 0x549af0

extern void player_effect_set_screen_flash(player_effect *self, player_screen_flash *descriptor,
    float intensity_falloff, float duration_scale); // 0x4578a0, stack, EBX, stack
extern void player_effect_set_camera_impulse(player_effect *self, int16_t local_player_index,
    real *tag_descriptor, real *direction, float intensity_falloff, float duration_scale); // 0x4579b0, EBX, stack
extern void player_effect_set_camera_shake(player_effect *self, player_camera_shake *descriptor,
    float intensity_falloff, float duration_scale); // 0x457d50, EBX, EAX, stack

void player_effect_mark_damage_direction(datum_index player_index, const damage_data *dd,
    const real_vector3d *direction, float random_blend, float damage_amount)
{
    int16_t local_player_index = ((player *)player_data->data)[player_index & 0xffff].local_player_index;
    player_effect *self;
    uint8_t *tag;

    player_effect_reentry_count++;
    if (local_player_index == -1) {
        player_effect_reentry_count--;
        return;
    }
    self = (player_effect *)((uint8_t *)player_effect_globals_pointer + local_player_index * 0xec);
    tag = (uint8_t *)tag_instances[dd->damage_effect_tag & 0xffff].data;
    player_effect_set_screen_flash(self, (player_screen_flash *)(tag + 0x24), random_blend, 1.0f);
    player_effect_set_camera_impulse(self, local_player_index, (real *)(tag + 0x98), (real *)direction,
        random_blend, 1.0f);
    player_effect_set_camera_shake(self, (player_camera_shake *)(tag + 0xcc), random_blend, 1.0f);
    if (*(datum_index *)(tag + 0x120) != k_datum_index_none) {
        sound_location location;

        memset(&location, 0, sizeof(location));
        location.scale = 1.0f;
        location.gain = 1.0f;
        sound_play_new(*(datum_index *)(tag + 0x120), &location, k_datum_index_none, 0, 0, 0, 0);
    }
    if (damage_amount > 0.0f && dd->responsible_object != k_datum_index_none) {
        datum_index controlling_player;
        datum_index unit_index;
        observer_camera *camera;
        real_point3d eye;
        real_point3d source;
        real_vector3d delta;
        real_vector3d side;
        real_vector3d projected;
        double angle;
        float abs_angle;

        if (*(uint32_t *)(tag + 0x1c8) & 0x100) {
            self->damage_indicator_alpha[2] = 1;
            player_effect_reentry_count--;
            return;
        }
        controlling_player = local_player_to_player_index(local_player_index);
        unit_index = (controlling_player == k_datum_index_none) ? k_datum_index_none :
            ((player *)player_data->data)[controlling_player & 0xffff].unit;
        if (object_try_and_get(unit_index, 3) == 0 ||
            object_try_and_get(dd->responsible_object, 0xffffffff) == 0) {
            player_effect_reentry_count--;
            return;
        }
        camera = observer_get_camera(local_player_index);
        if (camera == 0) {
            player_effect_reentry_count--;
            return;
        }
        unit_get_primary_eye_marker_position(unit_index, &eye);
        object_get_position(&source, dd->responsible_object);
        delta.i = source.x - eye.x;
        delta.j = source.y - eye.y;
        delta.k = source.z - eye.z;
        vector3d_cross_product(&side, (const real_vector3d *)&camera->up, (const real_vector3d *)&camera->forward);
        projected.i = side.k * delta.k + side.j * delta.j + side.i * delta.i;
        projected.j = delta.k * camera->forward.k + delta.j * camera->forward.j + delta.i * camera->forward.i;
        projected.k = delta.k * camera->up.k + delta.j * camera->up.j + delta.i * camera->up.i;
        if (vector3d_normalize_with_length(&projected) == 0.0f) {
            player_effect_reentry_count--;
            return;
        }
        if (fabs(projected.k) > 0.5) {
            if (projected.k > 0.0f) {
                self->damage_indicator_alpha[0] = 1;
            } else {
                self->damage_indicator_alpha[2] = 1;
            }
        }
        angle = atan2(projected.j, projected.i);
        abs_angle = (float)fabs(angle);
        if (angle < 0.78539819f || angle > 2.3561945f) {
            if (abs_angle > 1.5707964f) {
                self->damage_indicator_alpha[1] = 1;
            } else {
                self->damage_indicator_alpha[3] = 1;
            }
        }
    }
    player_effect_reentry_count--;
}

#if 0
Original Ghidra decompilation (0x456cf0):

void FUN_00456cf0(uint *param_1,undefined4 param_2,undefined4 param_3,float param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  uint in_EAX;
  int iVar9;
  int iVar10;
  float10 fVar11;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  undefined2 local_40 [2];
  undefined4 local_3c;
  undefined4 local_38;

  sVar4 = *(short *)((in_EAX & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34));
  DAT_00719ccc = DAT_00719ccc + 1;
  if (sVar4 != -1) {
    iVar10 = sVar4 * 0xec + DAT_006f1884;
    iVar9 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    FUN_004578a0(iVar10,param_3,0x3f800000);
    FUN_004579b0(sVar4,iVar9 + 0x98,param_2,param_3,0x3f800000);
    FUN_00457d50(param_3,0x3f800000);
    if (*(int *)(iVar9 + 0x120) != -1) {
      local_40[0] = 0;
      local_3c = 0x3f800000;
      local_38 = 0x3f800000;
      FUN_00549af0(*(int *)(iVar9 + 0x120),local_40,0xffffffff,0,0,0,0);
    }
    if ((0.0 < param_4) && (param_1[3] != 0xffffffff)) {
      if ((*(uint *)(iVar9 + 0x1c8) & 0x100) != 0) {
        *(undefined1 *)(iVar10 + 0xe6) = 1;
        DAT_00719ccc = DAT_00719ccc + -1;
        return;
      }
      iVar9 = local_player_to_player_index();
      if (iVar9 != -1) {
        local_player_to_player_index();
      }
      iVar9 = object_try_and_get(3);
      if (((iVar9 != 0) && (iVar9 = object_try_and_get(0xffffffff), iVar9 != 0)) &&
         (iVar9 = camera_get_globals_for_player(), iVar9 != 0)) {
        FUN_00568f50();
        object_get_position();
        fVar5 = local_58 - local_4c;
        fVar6 = local_54 - local_48;
        fVar7 = local_50 - local_44;
        vector3d_cross_product((float *)(iVar9 + 0x20));
        fVar1 = *(float *)(iVar9 + 0x28);
        fVar2 = *(float *)(iVar9 + 0x24);
        fVar3 = *(float *)(iVar9 + 0x20);
        fVar8 = fVar5 * *(float *)(iVar9 + 0x2c) +
                fVar6 * *(float *)(iVar9 + 0x30) + fVar7 * *(float *)(iVar9 + 0x34);
        fVar11 = (float10)vector3d_normalize_with_length();
        if ((float10)0.0 != fVar11) {
          if (0.5 < ABS(fVar8)) {
            if (fVar8 <= 0.0) {
              *(undefined1 *)(iVar10 + 0xe6) = 1;
            }
            else {
              *(undefined1 *)(iVar10 + 0xe4) = 1;
            }
          }
          fVar11 = (float10)fpatan((float10)(fVar5 * fVar3 + fVar6 * fVar2 + fVar7 * fVar1),
                                   (float10)(local_4c * fVar5 + local_48 * fVar6 + local_44 * fVar7)
                                  );
          if ((fVar11 < (float10)0.7853982) || ((float10)2.3561945 < fVar11)) {
            if (1.5707964 < (float)ABS(fVar11)) {
              *(undefined1 *)(iVar10 + 0xe5) = 1;
              DAT_00719ccc = DAT_00719ccc + -1;
              return;
            }
            *(undefined1 *)(iVar10 + 0xe7) = 1;
          }
        }
      }
    }
  }
  DAT_00719ccc = DAT_00719ccc + -1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
