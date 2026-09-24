// player_effect_mark_damage_direction  (Ghidra: FUN_00456cf0, still unnamed there; named
//   directly by out/phase4/effects_types_notes.md: "player_effect_mark_damage_direction 0x456cf0
//   sets one of them [damage_indicator_alpha] to 1 from the angle between the camera forward and
//   the damage source")
// address 0x456cf0, size 779 bytes
// name confidence: 0.5   rewrite confidence: 0.25 (heavily UNSURE: the DamageEffect tag offsets
//   this reads -- +0x98 camera impulse block, +0x120 a sound reference, +0x1c8 a flags word with
//   bit 0x100 -- are not established anywhere else in this batch, so they are kept as raw offset
//   arithmetic rather than named fields; the camera/geometry tail that decides which of the four
//   damage_indicator_alpha slots to light is likewise best-effort)
// evidence: types/game.h player.local_player_index (+0x02); types/effects.h player_effect
//   (damage_indicator_alpha[4] +0xe4, matching the four single-byte writes at +0xe4..+0xe7);
//   this module's player_effect_set_screen_flash, player_effect_set_camera_impulse and
//   player_effect_set_camera_shake, all invoked here in sequence; global 0x00719ccc
//   player_effect_reentry_count (types/effects.h globals list).
// register convention: object index in EAX (in_EAX); a small descriptor pointer (tag reference
//   at +0x00, a fourth-object index at +0x0c) as the recognized param_1; param_2/param_3 forward
//   straight into the camera-impulse and screen-flash/shake calls; falloff distance as param_4.
//   // blam-cc: in_EAX -> object_index, stack -> (descriptor, param_2, param_3, falloff)
// UNSURE: object_try_and_get is called here with only its type-mask argument visible at two of
//   its three call sites (the object index itself is dropped by Ghidra); modeled as probing the
//   local player's own unit for the first and the responsible/parent object for the second,
//   which is the only reading consistent with "the camera forward vs. damage source" framing.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"

extern data_array *player_data;                               // 0x0087a480
extern player_effect_globals *player_effect_globals_pointer;  // 0x006f1884
extern tag_instance *tag_instances;                           // 0x0087bc14
extern int32_t player_effect_reentry_count;                   // 0x00719ccc

extern double atan2(double y, double x); // fpatan is a single x87 FPATAN instruction
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *a, real_vector3d *b); // 0x4052c0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, objects module
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module
extern datum_index local_player_to_player_index(int16_t local_player_index); // 0x474d30, game module
extern void *observer_get_camera(uint32_t player_index); // 0x4479a0, UNSURE signature
extern void unit_get_primary_eye_marker_position(void *camera_globals, real_point3d *out_position); // 0x568f50, UNSURE
extern void sound_play_new(uint32_t sound_tag, void *descriptor, int32_t a3, int32_t a4, int32_t a5,
    int32_t a6, int32_t a7); // 0x549af0, sound module, UNSURE

extern void player_effect_set_screen_flash(player_effect *self, player_screen_flash *descriptor,
    float intensity_falloff, float duration_scale); // 0x4578a0, this module
extern void player_effect_set_camera_impulse(player_effect *self, int16_t local_player_index,
    real *tag_descriptor, real *direction, float intensity_falloff, float duration_scale); // 0x4579b0,
                                    // this module
extern void player_effect_set_camera_shake(player_effect *self, player_camera_shake *descriptor,
    float intensity_falloff, float duration_scale); // 0x457d50, this module

void player_effect_mark_damage_direction(uint32_t object_index, uint32_t *descriptor,
    void *direction_block, void *rotation_block, float falloff) // blam-cc: in_EAX, stack, stack,
                                    // stack, stack
{
    player *record = &((player *)player_data->data)[object_index & 0xffff];
    int16_t local_player_index = record->local_player_index;

    player_effect_reentry_count++;

    if (local_player_index != -1) {
        player_effect *self = &player_effect_globals_pointer->players[local_player_index];
        uint8_t *tag = (uint8_t *)tag_instances[descriptor[0] & 0xffff].data;

        player_effect_set_screen_flash(self, (player_screen_flash *)0, falloff, 1.0f); // UNSURE:
                                    // descriptor arg dropped, see file header
        player_effect_set_camera_impulse(self, local_player_index, (real *)(tag + 0x98),
            (real *)direction_block, *(real *)&rotation_block, 1.0f); // UNSURE: this function's
                                    // own 3rd/4th parameters were previously guessed as
                                    // "direction_block"/"rotation_block" pointers, but
                                    // player_effect_set_camera_impulse 0x4579b0's real signature
                                    // needs a direction pointer and a falloff *value* here, so
                                    // the 4th parameter is now believed to be a float reinterpreted
                                    // through a void*, not a second vector -- see that file's header
        player_effect_set_camera_shake(self, (player_camera_shake *)0, falloff, 1.0f); // UNSURE

        if (*(int32_t *)(tag + 0x120) != -1) {
            uint16_t sound_descriptor[6] = {0, 0, 0, 0, 0, 0};
            sound_descriptor[0] = 0;
            *(float *)&sound_descriptor[2] = 1.0f;
            *(float *)&sound_descriptor[4] = 1.0f;
            sound_play_new(*(uint32_t *)(tag + 0x120), sound_descriptor, -1, 0, 0, 0, 0);
        }

        if (0.0f < falloff && descriptor[3] != 0xffffffff) {
            if ((*(uint32_t *)(tag + 0x1c8) & 0x100) != 0) {
                self->damage_indicator_alpha[2] = 1;
                player_effect_reentry_count--;
                return;
            }

            {
                datum_index player_id = local_player_to_player_index(local_player_index);
                object *unit;

                if (player_id != (datum_index)0xffffffff) {
                    local_player_to_player_index(local_player_index); // UNSURE: called twice with
                                    // no visible use of the second result, see decompile
                }

                unit = object_try_and_get(object_index, _object_mask_unit);
                if (unit != (object *)0) {
                    object *responsible = object_try_and_get(descriptor[3], 0xffffffff);
                    if (responsible != (object *)0) {
                        void *camera = observer_get_camera(0); // UNSURE: player index
                                    // argument not recovered
                        if (camera != (void *)0) {
                            real_point3d source_position, camera_position;
                            real_vector3d to_source, up, cross;
                            float side, length;

                            unit_get_primary_eye_marker_position(camera, &camera_position);
                            object_get_position(&source_position, object_index);

                            to_source.i = source_position.x - camera_position.x;
                            to_source.j = source_position.y - camera_position.y;
                            to_source.k = source_position.z - camera_position.z;

                            vector3d_cross_product(&cross, (real_vector3d *)((uint8_t *)camera + 0x20),
                                                    &to_source); // UNSURE: camera forward/up
                                    // layout at +0x20 not established elsewhere

                            side = to_source.i * *(float *)((uint8_t *)camera + 0x2c) +
                                   to_source.j * *(float *)((uint8_t *)camera + 0x30) +
                                   to_source.k * *(float *)((uint8_t *)camera + 0x34);
                            length = vector3d_normalize_with_length(&to_source);

                            if (length != 0.0f) {
                                double angle;

                                if (0.5f < (side < 0.0f ? -side : side)) {
                                    if (side <= 0.0f) {
                                        self->damage_indicator_alpha[2] = 1;
                                    } else {
                                        self->damage_indicator_alpha[0] = 1;
                                    }
                                }

                                angle = atan2(
                                    (double)(to_source.i * *(float *)((uint8_t *)camera + 0x20) +
                                             to_source.j * *(float *)((uint8_t *)camera + 0x24) +
                                             to_source.k * *(float *)((uint8_t *)camera + 0x28)),
                                    (double)(camera_position.x * to_source.i +
                                             camera_position.y * to_source.j +
                                             camera_position.z * to_source.k)); // UNSURE: this
                                    // second argument reuses local_4c/48/44 as though they were
                                    // the camera position, matching the decompile literally

                                if (angle < 0.7853982 || 2.3561945 < angle) {
                                    if (1.5707964 < (angle < 0.0 ? -angle : angle)) {
                                        self->damage_indicator_alpha[1] = 1;
                                        player_effect_reentry_count--;
                                        return;
                                    }
                                    self->damage_indicator_alpha[3] = 1;
                                }
                            }
                        }
                    }
                }
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
