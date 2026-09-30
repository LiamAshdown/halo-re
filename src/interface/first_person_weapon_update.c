// first_person_weapon_update  (Ghidra: first_person_weapon_update, already named)
// address 0x493150, size 1510 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/interface.h first_person_weapon_interface names this address directly;
// out/phase4/interface_functions.md "Main per-frame update for a local player's first-person
// weapon: advances state, idle animation selection, aim-sway smoothing, and interface timers.";
// reuses first_person_weapon_update_state.c (0x492d20), first_person_weapon_snapshot_pose.c
// (0x4930b0), first_person_weapon_interface_tick_reset.c (0x4942e0), first_person_weapon_
// set_state.c (0x492e60); src/math/real_seek_toward_clamped.c for 0x4cf360's registers.
// register convention: local_player_index is the one stack parameter.
// Review pass (phase 4): rebuilt from the disassembly (0x493150..0x493735); the first rewrite
// had the seek calls writing to scratch, guessed set_state values and the wrong tag.
//  - The tag read through weapon tag +0x478 is Weapon.first_person_animations.tag_id, so the
//    tag data is ModelAnimations: +0x48/+0x4c is first_person_weapons, whose element +0x10/+0x14
//    is the int16 animation-index list (entry 3 moving, entry 15 overcharged jitter), and
//    +0x78 is the animations block (stride 0xb4, frame_count at +0x22).
//  - animation_state_advance advances an {int16 animation, int16 frame} pair: EAX = animation graph tag,
//    ESI = the pair, EBX = optional out for the frame sound tag (0x4d4918), one stack dword.
//    It returns 0 to 4; 2 means the last frame of a non-looping animation. The Ghidra local_10
//    is that sound out-parameter.
//  - The new_state values in AX for 0x492e60 are 0x16 (weapon flag 2), 0 (weapon flag 1 clear,
//    and leaving state 5) and 5 (idle fidget); snapshot_pose gets DX = 6.
//  - The four sway seeks are value/velocity pairs 0x30/0x38, 0x34/0x3c (targets unit throttle
//    x and y), 0x40/0x48, 0x44/0x4c (targets the clamped aim deltas), and 0x28/0x2c always
//    seeks to 0. The idle timing reads Globals.player_information (+0x174 is its pointer, not
//    player_control): first_person_idle_time[2] at +0x9c and first_person_skip_fraction at +0xa4.
//  - FUN_00628cca is the CRT _CIfmod (x in ST1, y in ST0), 0x6391b4 is __ftol.
// UNSURE: sound_start_at_object_marker (sound start on an object: ESI = object, ECX and EAX the two constant
// origin pointers at 0x6966f8 and 0x696718, then sound tag, marker -1, gain 1.0 and a flag) is
// foreign; modelled with the register arguments as leading parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_math.h"
#include "fn_interface.h"

extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern Globals *global_globals;     // 0x00746fa0
extern real_vector3d *global_forward3d_pointer; // 0x00696718, types/math.h
extern void *global_zero_vector3d_pointer;  // 0x006966f8, types/devices.h (points at global_origin3d 0x0065c230)

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0; blam-cc: ECX -> object_index
extern int16_t camera_get_type_for_player(int16_t player_index); // 0x445ac0; blam-cc: CX -> player_index
extern uint8_t biped_is_idle_eligible(datum_index unit_index); // 0x55e8e0; blam-cc: EAX -> unit_index
extern int16_t animation_state_advance(datum_index animation_graph, int16_t *animation_state,
                            datum_index *out_frame_sound, int32_t unknown); // 0x4d48d0
    // blam-cc: EAX -> animation_graph, ESI -> animation_state, EBX -> out_frame_sound (may be 0)
extern int32_t sound_start_at_object_marker(datum_index object_index, void *position, void *forward,
                            datum_index sound, int32_t marker, float gain, uint8_t flag); // 0x543ce0
    // blam-cc: ESI -> object_index, ECX -> position, EAX -> forward, stack -> sound, marker, gain, flag
    // (objdump call sites 0x492ac4, 0x4932a6, 0x4c4866 all load ECX from 0x006966f8 and EAX from 0x00696718)


extern real random_range_real(real minimum, real maximum); // 0x444af0
extern float effect_random_fraction(void);                  // 0x4505b0
extern float angle_delta_wrapped(float a, float b);         // 0x470d10, returns b - a wrapped
extern double fmod(double x, double y);                     // 0x628cca, CRT _CIfmod
extern double sqrt(double x);                               // inline x87 fsqrt
extern int32_t __ftol(double x);                            // 0x6391b4

#define FP_FLOAT(fp, offset) (*(float *)((uint8_t *)(fp) + (offset)))

static object *object_get(datum_index object_index)
{
    return *(object **)((char *)object_data->data + 8 + (object_index & 0xffff) * 0xc);
}

static ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *first_person_weapon_list(
    ModelAnimations *animations)
{
    if (animations->first_person_weapons.count == 0) {
        return (ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *)0;
    }
    return (ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *)
        animations->first_person_weapons.pointer;
}


void first_person_weapon_update(int16_t local_player_index)
{
    debug_play_diagnostics(); // TEMPORARY
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];

    if (fp->weapon_index != (datum_index)-1 && object_try_and_get(fp->weapon_index, 4) == 0) {
        fp->weapon_index = (datum_index)-1;
    }

    if (fp->unit_index != (datum_index)-1 && fp->weapon_index != (datum_index)-1) {
        object *weapon_obj = object_get(fp->weapon_index);
        object *unit_obj = object_get(fp->unit_index);
        Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
        datum_index animation_graph = *(datum_index *)&weapon_tag->first_person_animations.tag_id;
        ModelAnimations *animations = (ModelAnimations *)tag_instances[animation_graph & 0xffff].data;
        ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *list;
        uint32_t *weapon_flags = (uint32_t *)((uint8_t *)weapon_obj + 0x22c); // weapon_data.flags
        datum_index frame_sound;
        uint8_t is_moving;
        float target_yaw;
        float target_pitch;

        if (fp->state == 3 || fp->state == 1) {
            if ((*weapon_flags & 2) != 0) {
                first_person_weapon_set_state(local_player_index, 1, 0x16);
            }
            if ((*weapon_flags & 1) == 0) {
                first_person_weapon_set_state(local_player_index, 1, 0);
            }
        }

        // the tag id is reloaded from the weapon tag here, as the original does
        if (animation_state_advance(*(datum_index *)&weapon_tag->first_person_animations.tag_id, &fp->animation_block_index,
                         &frame_sound, 0) == 2) {
            first_person_weapon_update_state(local_player_index);
        }

        if (frame_sound != (datum_index)-1 && camera_get_type_for_player(local_player_index) == 0) {
            fp->frame_sound_impulse = sound_start_at_object_marker(fp->weapon_index, global_zero_vector3d_pointer,
                                            global_forward3d_pointer, frame_sound, -1, 1.0f,
                                            local_player_index != -1);
            fp->frame_sound_state = fp->state;
        }

        {
            real_vector3d *throttle = (real_vector3d *)((uint8_t *)unit_obj + 0x278);
            is_moving = 1;
            if (!(sqrt(throttle->k * throttle->k + throttle->j * throttle->j +
                       throttle->i * throttle->i) > 0.1f)) {
                is_moving = 0;
            }
        }
        if (object_get(fp->unit_index)->type == 0 && biped_is_idle_eligible(fp->unit_index) != 0) {
            is_moving = 0;
        }

        // moving overlay: {animation 0x1a, frame 0x1c}
        if (fp->moving_animation_index != -1) {
            animation_state_advance(*(datum_index *)&weapon_tag->first_person_animations.tag_id, &fp->moving_animation_index,
                         (datum_index *)0, 0);
            if (!is_moving) {
                if (fp->state == 0) {
                    first_person_weapon_snapshot_pose(local_player_index, 6);
                }
                fp->moving_animation_index = -1;
            }
        } else if (is_moving) {
            list = first_person_weapon_list(animations);
            *(int16_t *)fp->unknown_1c = 0;
            if ((int32_t)list->animations.count > 3) {
                fp->moving_animation_index = ((int16_t *)list->animations.pointer)[3];
            } else {
                fp->moving_animation_index = -1;
            }
        }

        // state 4 overlay: {animation 0x20, float frame 0x24}
        if (fp->charging_animation_index == -1) {
            if (fp->state == 4) {
                list = first_person_weapon_list(animations);
                FP_FLOAT(fp, 0x24) = 0.0f;
                if ((int32_t)list->animations.count > 0xf) {
                    fp->charging_animation_index = ((int16_t *)list->animations.pointer)[0xf];
                } else {
                    fp->charging_animation_index = -1;
                }
            }
        } else if (fp->state == 4) {
            ModelAnimationsAnimation *animation =
                &((ModelAnimationsAnimation *)animations->animations.pointer)[fp->charging_animation_index];
            float charged_fraction = *(float *)((uint8_t *)weapon_obj + 0x244);
            FP_FLOAT(fp, 0x24) = (float)fmod((charged_fraction + 1.0f) + (charged_fraction + 1.0f) +
                                             FP_FLOAT(fp, 0x24),
                                             (double)(int16_t)animation->frame_count);
        } else {
            fp->charging_animation_index = -1;
        }

        if (fp->unknown_30[0x20] != 0) {  // 0x50
            real_seek_toward_clamped(0, &FP_FLOAT(fp, 0x38), &FP_FLOAT(fp, 0x30),
                                     *(float *)((uint8_t *)unit_obj + 0x278), 0.08f, 0.5f, -1.0f, 1.0f);
            real_seek_toward_clamped(0, &FP_FLOAT(fp, 0x3c), &FP_FLOAT(fp, 0x34),
                                     *(float *)((uint8_t *)unit_obj + 0x27c), 0.08f, 0.5f, -1.0f, 1.0f);
            target_yaw = angle_delta_wrapped(FP_FLOAT(fp, 0x68), FP_FLOAT(fp, 0x60)) * 30.0f;
            target_pitch = angle_delta_wrapped(FP_FLOAT(fp, 0x6c), FP_FLOAT(fp, 0x64)) * -30.0f;
            if (target_yaw < -1.0f) {
                target_yaw = -1.0f;
            } else if (target_yaw > 1.0f) {
                target_yaw = 1.0f;
            }
            if (target_pitch < -1.0f) {
                target_pitch = -1.0f;
            } else if (target_pitch > 1.0f) {
                target_pitch = 1.0f;
            }
            real_seek_toward_clamped(0, &FP_FLOAT(fp, 0x48), &FP_FLOAT(fp, 0x40), target_yaw,
                                     0.03f, 0.2f, -1.0f, 1.0f);
            real_seek_toward_clamped(0, &FP_FLOAT(fp, 0x4c), &FP_FLOAT(fp, 0x44), target_pitch,
                                     0.03f, 0.2f, -1.0f, 1.0f);
        }
        real_seek_toward_clamped(0, &fp->charge, &fp->charge_blend_weight, 0.0f, 0.01f, 0.2f, 0.0f, 1.0f);
        if (fp->charge_blend_weight == 1.0f) {
            fp->charge = 0.0f;
        }

        if (fp->blend_end > 0) {
            fp->blend_start++;
            if (fp->blend_start >= fp->blend_end) {
                fp->blend_end = 0;
            }
        }

        {
            // player_control_globals_ptr + index * 0x40 + 0x3c and + 0x34
            local_player_control *control =
                &player_control_globals_ptr->local_players[local_player_index];

            if (control->nameplate_weight == 0.0f &&
                (local_player_index == -1 || control->desired_zoom_level == -1) &&
                fp->charge_blend_weight == 0.0f &&
                FP_FLOAT(fp, 0x30) == 0.0f && FP_FLOAT(fp, 0x34) == 0.0f &&
                FP_FLOAT(fp, 0x40) == 0.0f && FP_FLOAT(fp, 0x44) == 0.0f) {
                if (fp->state == 0) {
                    GlobalsPlayerInformation *player_information =
                        (GlobalsPlayerInformation *)global_globals->player_information.pointer;
                    if (fp->idle_ticks_target == 0) {
                        fp->idle_ticks_target = (int16_t)__ftol(
                            random_range_real(player_information->first_person_idle_time[0],
                                              player_information->first_person_idle_time[1]) * 30.0f);
                    }
                    fp->idle_ticks++;
                    if (fp->idle_ticks > fp->idle_ticks_target) {
                        fp->idle_ticks_target = 0;
                        if (!(effect_random_fraction() <
                              player_information->first_person_skip_fraction)) {
                            first_person_weapon_set_state(local_player_index, 1, 5);
                        }
                    }
                } else {
                    fp->idle_ticks = 0;
                }
            } else {
                fp->idle_ticks = 0;
                if (fp->state == 5) {
                    first_person_weapon_set_state(local_player_index, 1, 0);
                }
            }
        }
    }

    fp->shutdown_countdown--;
    if (fp->shutdown_countdown <= 0) {
        first_person_weapon_interface_tick_reset(local_player_index);
    }
}

#if 0
Original Ghidra decompilation (0x493150):

void first_person_weapon_update(undefined4 param_1)

{
  undefined2 *puVar1;
  uint *puVar2;
  char cVar3;
  short sVar4;
  undefined2 uVar5;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  bool bVar11;
  float10 fVar12;
  int local_10;
  float local_c;
  float local_8;

  iVar10 = (short)param_1 * 0x1ea0 + DAT_006b2d98;
  if ((*(int *)(iVar10 + 8) != -1) && (iVar6 = object_try_and_get(4), iVar6 == 0)) {
    *(undefined4 *)(iVar10 + 8) = 0xffffffff;
  }
  if ((*(uint *)(iVar10 + 4) != 0xffffffff) && (*(uint *)(iVar10 + 8) != 0xffffffff)) {
    puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar10 + 8) & 0xffff) * 0xc);
    iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar10 + 4) & 0xffff) * 0xc);
    iVar8 = *(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x478) &
                     0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if ((*(short *)(iVar10 + 0xc) == 3) || (*(short *)(iVar10 + 0xc) == 1)) {
      if ((puVar2[0x8b] & 2) != 0) {
        first_person_weapon_set_state(param_1,1);
      }
      if ((puVar2[0x8b] & 1) == 0) {
        first_person_weapon_set_state(param_1,1);
      }
    }
    sVar4 = FUN_004d48d0(0);
    if ((sVar4 != 1) && (sVar4 == 2)) {
      FUN_00492d20();
    }
    if ((local_10 != -1) && (sVar4 = camera_get_type_for_player(), sVar4 == 0)) {
      uVar7 = FUN_00543ce0(local_10,0xffffffff,0x3f800000,(short)param_1 != -1);
      *(undefined4 *)(iVar10 + 0x1e98) = uVar7;
      *(undefined2 *)(iVar10 + 0x1e9c) = *(undefined2 *)(iVar10 + 0xc);
    }
    bVar11 = 0.1 < SQRT(*(float *)(iVar6 + 0x280) * *(float *)(iVar6 + 0x280) +
                        *(float *)(iVar6 + 0x27c) * *(float *)(iVar6 + 0x27c) +
                        *(float *)(iVar6 + 0x278) * *(float *)(iVar6 + 0x278));
    if ((*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                            (*(uint *)(iVar10 + 4) & 0xffff) * 0xc) + 0xb4) == 0) &&
       (cVar3 = FUN_0055e8e0(), cVar3 != '\0')) {
      bVar11 = false;
    }
    puVar1 = (undefined2 *)(iVar10 + 0x1a);
    if (*(short *)(iVar10 + 0x1a) == -1) {
      if (bVar11) {
        if (*(int *)(iVar8 + 0x48) == 0) {
          iVar9 = 0;
        }
        else {
          iVar9 = *(int *)(iVar8 + 0x4c);
        }
        *(undefined2 *)(iVar10 + 0x1c) = 0;
        if (*(int *)(iVar9 + 0x10) < 4) {
          *puVar1 = 0xffff;
        }
        else {
          *puVar1 = *(undefined2 *)(*(int *)(iVar9 + 0x14) + 6);
        }
      }
    }
    else {
      FUN_004d48d0(0);
      if (!bVar11) {
        if (*(short *)(iVar10 + 0xc) == 0) {
          FUN_004930b0();
        }
        *puVar1 = 0xffff;
      }
    }
    if (*(short *)(iVar10 + 0x20) == -1) {
      if (*(short *)(iVar10 + 0xc) == 4) {
        if (*(int *)(iVar8 + 0x48) == 0) {
          iVar8 = 0;
        }
        else {
          iVar8 = *(int *)(iVar8 + 0x4c);
        }
        *(undefined4 *)(iVar10 + 0x24) = 0;
        if (*(int *)(iVar8 + 0x10) < 0x10) {
          *(undefined2 *)(iVar10 + 0x20) = 0xffff;
        }
        else {
          *(undefined2 *)(iVar10 + 0x20) = *(undefined2 *)(*(int *)(iVar8 + 0x14) + 0x1e);
        }
      }
    }
    else if (*(short *)(iVar10 + 0xc) == 4) {
      fVar12 = (float10)FUN_00628cca();
      *(float *)(iVar10 + 0x24) = (float)fVar12;
    }
    else {
      *(undefined2 *)(iVar10 + 0x20) = 0xffff;
    }
    if (*(char *)(iVar10 + 0x50) != '\0') {
      real_seek_toward_clamped
                (*(undefined4 *)(iVar6 + 0x278),0x3da3d70a,0x3f000000,0xbf800000,0x3f800000);
      real_seek_toward_clamped
                (*(undefined4 *)(iVar6 + 0x27c),0x3da3d70a,0x3f000000,0xbf800000,0x3f800000);
      local_c = angle_delta_wrapped(*(float *)(iVar10 + 0x68),*(float *)(iVar10 + 0x60));
      local_c = local_c * 30.0;
      local_8 = angle_delta_wrapped(*(float *)(iVar10 + 0x6c),*(float *)(iVar10 + 100));
      local_8 = local_8 * -30.0;
      if (-1.0 <= local_c) {
        if (1.0 < local_c) {
          local_c = 1.0;
        }
      }
      else {
        local_c = -1.0;
      }
      if (-1.0 <= local_8) {
        if (1.0 < local_8) {
          local_8 = 1.0;
        }
      }
      else {
        local_8 = -1.0;
      }
      real_seek_toward_clamped(local_c,0x3cf5c28f,0x3e4ccccd,0xbf800000,0x3f800000);
      real_seek_toward_clamped(local_8,0x3cf5c28f,0x3e4ccccd,0xbf800000,0x3f800000);
    }
    real_seek_toward_clamped(0,0x3c23d70a,0x3e4ccccd,0,0x3f800000);
    if (*(float *)(iVar10 + 0x28) == 1.0) {
      *(undefined4 *)(iVar10 + 0x2c) = 0;
    }
    if ((0 < *(short *)(iVar10 + 0x8a)) &&
       (*(short *)(iVar10 + 0x88) = *(short *)(iVar10 + 0x88) + 1,
       *(short *)(iVar10 + 0x8a) <= *(short *)(iVar10 + 0x88))) {
      *(undefined2 *)(iVar10 + 0x8a) = 0;
    }
    iVar6 = (short)param_1 * 0x40;
    if (((*(float *)(DAT_006b145c + 0x3c + iVar6) == 0.0) &&
        ((((short)param_1 == -1 || (*(short *)(DAT_006b145c + iVar6 + 0x34) == -1)) &&
         (*(float *)(iVar10 + 0x28) == 0.0)))) &&
       ((((*(float *)(iVar10 + 0x30) == 0.0 && (*(float *)(iVar10 + 0x34) == 0.0)) &&
         (*(float *)(iVar10 + 0x40) == 0.0)) && (*(float *)(iVar10 + 0x44) == 0.0)))) {
      if (*(short *)(iVar10 + 0xc) == 0) {
        iVar6 = *(int *)(DAT_00746fa0 + 0x174);
        if (*(short *)(iVar10 + 0xe) == 0) {
          random_range_real(*(float *)(iVar6 + 0x9c),*(float *)(iVar6 + 0xa0));
          uVar5 = __ftol();
          *(undefined2 *)(iVar10 + 0xe) = uVar5;
        }
        *(short *)(iVar10 + 0x10) = *(short *)(iVar10 + 0x10) + 1;
        if (*(short *)(iVar10 + 0xe) < *(short *)(iVar10 + 0x10)) {
          *(undefined2 *)(iVar10 + 0xe) = 0;
          fVar12 = (float10)effect_random_fraction();
          if ((float10)*(float *)(iVar6 + 0xa4) <= fVar12) {
            first_person_weapon_set_state(param_1,1);
          }
        }
      }
      else {
        *(undefined2 *)(iVar10 + 0x10) = 0;
      }
    }
    else {
      *(undefined2 *)(iVar10 + 0x10) = 0;
      if (*(short *)(iVar10 + 0xc) == 5) {
        first_person_weapon_set_state(param_1,1);
      }
    }
  }
  *(short *)(iVar10 + 0x12) = *(short *)(iVar10 + 0x12) + -1;
  if (*(short *)(iVar10 + 0x12) < 1) {
    FUN_004942e0();
  }
  return;
}
#endif
