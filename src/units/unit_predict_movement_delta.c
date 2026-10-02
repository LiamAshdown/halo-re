// unit_predict_movement_delta  (Ghidra: unit_predict_movement_delta, renamed)
// address 0x55cca0, size 783 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: the flat local copy of the local player's controlled unit is read back at the exact
//   same offsets biped_update (0x5590a0) itself pre-processes before calling its movement
//   solver -- object.parent_object/vitality_flags/position/forward/up (0x11c/0x106/0x05c/0x074/
//   0x080, objects.h), unit_data.animation_state/control_flags/throttle/desired_facing_vector
//   (0x2a3/0x208/0x278/0x224, types/units.h), biped_data.flags/unknown_501/unknown_502
//   (0x4cc/0x501/0x502) -- confirming this runs the same pre-solve steps on a scratch copy
//   before calling biped_integrate_movement (0x55bea0), then diffs the copy's post-solve
//   position/forward/up against the live object's, scaled by a time fraction, to produce a
//   prediction delta. player_data's unit-handle offset (+0x34) matches types/units.h.
// FIXED (objdump 0x55cd8b, 0x55cdd1): the planar aim's k is zeroed before it is normalized and the copy's movement_state
//   byte (+0x4d2) is set from the animation state as in biped_update; the draft omitted both.
// VERIFIED against disassembly 0x55cca0..0x55cfaf (2026-09-30): data_iterator_next (EDI), the 0x550-byte copy, the pre-solve
//   steps, biped_integrate_movement(unit, copy, flags) and the clamped delta scaling (29.999998).
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)
// reconciled: R16 the local iterator (int32 next_index, no signature) is now the 0x10-byte types/memory.h data_iterator, signature stored as at 0x55cce1

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480, types/units.h
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_point3d *global_origin3d_pointer;   // 0x00696714

extern real vector3d_normalize_with_length(real_vector3d *v);
extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void biped_integrate_movement(uint32_t object_index, uint8_t *working_copy,
                                      uint8_t *output_flags); // 0x55bea0, this batch
extern void *memcpy(void *dst, const void *src, uint32_t n);

// Predicts a network-simulated unit's movement over a fraction of a tick without touching the
// live object: finds the local player's controlled unit, runs the same pre-solve bookkeeping
// biped_update itself does on a scratch copy of it, drives biped_integrate_movement against that
// copy, and returns the (time-scaled) position/forward/up deltas between the copy's result and
// the live object's current state. Returns 0 (leaving the outputs untouched) if the tick already
// ran, no connected local player has a controlled unit, or that unit is attached to a parent.
uint32_t unit_predict_movement_delta(real_vector3d *out_position_delta, real_vector3d *out_forward_delta,
                                      real_vector3d *out_up_delta, float time_fraction)
{
    if (game_time->paused != 0) {
        return 0;
    }

    {
        // The inline data_iterator over player_data (0x55ccc1..0x55cce1: data, WORD next_index
        // = 0, index = -1, signature = data ^ 'iter').
        data_iterator iterator;
        void *entry;

        iterator.data = player_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

        entry = data_iterator_next(&iterator);
        if (entry == 0) {
            return 0;
        }
        while (*(int16_t *)((uint8_t *)entry + 2) == -1) {
            entry = data_iterator_next(&iterator);
            if (entry == 0) {
                return 0;
            }
        }

        {
            datum_index unit_index = *(datum_index *)((uint8_t *)entry + 0x34);
            if (unit_index == k_datum_index_none) {
                return 0;
            }

            {
                object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
                void *tag_data = tag_instances[obj->definition_tag & 0xffff].data;
                uint8_t working_copy[0x550];
                object *copy = (object *)working_copy;
                unit_data *copy_unit = (unit_data *)(working_copy + 0x1f4);
                biped_data *copy_biped = (biped_data *)(working_copy + 0x4cc);
                uint8_t output_flags[2] = {0, 0};
                float t;

                memcpy(working_copy, obj, sizeof(working_copy));

                if (copy->parent_object != k_datum_index_none) {
                    return 0;
                }

                if ((copy->vitality_flags & 4) != 0 || (*(uint8_t *)((uint8_t *)tag_data + 0x2f4) & 0x44) == 0) {
                    copy_unit->desired_facing_vector.k = 0.0f; // 0x55cd8b: the planar aim
                    if (vector3d_normalize_with_length(&copy_unit->desired_facing_vector) == 0.0f) {
                        copy_unit->desired_facing_vector = *global_forward3d_pointer;
                    }
                }

                switch (copy_unit->animation_state) { // 0x55cdd1: byte table 0x55cfbc -> jump table 0x55cfb0
                case 0: case 2: case 3:
                    copy_biped->movement_state = 0;
                    break;
                case 4: case 5: case 6: case 7:
                    copy_biped->movement_state = 1;
                    break;
                default:
                    copy_biped->movement_state = 2;
                    break;
                }

                if (copy_unit->throttle.i * copy_unit->throttle.i + copy_unit->throttle.j * copy_unit->throttle.j +
                    copy_unit->throttle.k * copy_unit->throttle.k < 0.010000001f) {
                    copy_unit->throttle.i = global_origin3d_pointer->x;
                    copy_unit->throttle.j = global_origin3d_pointer->y;
                    copy_unit->throttle.k = global_origin3d_pointer->z;
                }

                copy_biped->airborne_ticks = (copy_biped->flags & 1) ?
                    ((copy_biped->airborne_ticks < 0x7f) ? copy_biped->airborne_ticks + 1 : copy_biped->airborne_ticks) : 0;
                copy_biped->slipping_ticks = (copy_biped->flags & 2) ?
                    ((copy_biped->slipping_ticks < 0x7f) ? copy_biped->slipping_ticks + 1 : copy_biped->slipping_ticks) : 0;

                output_flags[1] = (uint8_t)(copy_unit->control_flags & 1);
                output_flags[0] = 0;

                biped_integrate_movement(unit_index, working_copy, output_flags);

                t = time_fraction * 29.999998f;
                if (t > 1.0f) t = 1.0f;
                else if (t < 0.0f) t = 0.0f;

                out_position_delta->i = (copy->position.x - obj->position.x) * t;
                out_position_delta->j = (copy->position.y - obj->position.y) * t;
                out_position_delta->k = (copy->position.z - obj->position.z) * t;
                out_forward_delta->i = (copy->forward.i - obj->forward.i) * t;
                out_forward_delta->j = (copy->forward.j - obj->forward.j) * t;
                out_forward_delta->k = (copy->forward.k - obj->forward.k) * t;
                out_up_delta->i = (copy->up.i - obj->up.i) * t;
                out_up_delta->j = (copy->up.j - obj->up.j) * t;
                out_up_delta->k = (copy->up.k - obj->up.k) * t;
                return 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x55cca0):

undefined1 FUN_0055cca0(float *param_1,float *param_2,float *param_3,float param_4)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  undefined1 uVar4;
  uint *puVar5;
  uint *puVar6;
  float10 fVar7;
  undefined1 local_570;
  byte local_56f;
  uint local_56c;
  uint local_568;
  undefined2 local_564;
  undefined4 local_560;
  uint local_55c;
  uint local_558 [23];
  float local_4fc;
  float local_4f8;
  float local_4f4;
  float local_4e4;
  float local_4e0;
  float local_4dc;
  float local_4d8;
  float local_4d4;
  float local_4d0;
  byte local_452;
  int local_43c;
  byte local_350;
  undefined4 local_334;
  undefined4 local_330;
  undefined4 local_32c;
  float local_2e0;
  float local_2dc;
  float local_2d8;
  undefined1 local_2b5;
  byte local_8c;
  undefined1 local_86;
  char local_57;
  char local_56;

  uVar4 = 0;
  if (*(char *)(DAT_006f1d6c + 2) == '\0') {
    local_568 = DAT_0087a480;
    local_55c = DAT_0087a480 ^ 0x69746572;
    local_564 = 0;
    local_560 = 0xffffffff;
    iVar2 = data_iterator_next();
    if (iVar2 != 0) {
      while (*(short *)(iVar2 + 2) == -1) {
        iVar2 = data_iterator_next();
        if (iVar2 == 0) {
          return 0;
        }
      }
      local_56c = *(uint *)(iVar2 + 0x34);
      if (local_56c != 0xffffffff) {
        puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_56c & 0xffff) * 0xc);
        iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        puVar5 = puVar1;
        puVar6 = local_558;
        for (iVar3 = 0x154; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar6 = *puVar5;
          puVar5 = puVar5 + 1;
          puVar6 = puVar6 + 1;
        }
        local_570 = 0;
        local_56f = 0;
        if (local_43c != -1) {
          return 0;
        }
        if (((local_452 & 4) != 0) || ((*(byte *)(iVar2 + 0x2f4) & 0x44) == 0)) {
          local_32c = 0;
          fVar7 = (float10)vector3d_normalize_with_length();
          if ((float10)0.0 == fVar7) {
            local_334 = *(undefined4 *)PTR_DAT_00696718;
            local_330 = *(undefined4 *)(PTR_DAT_00696718 + 4);
            local_32c = *(undefined4 *)(PTR_DAT_00696718 + 8);
          }
        }
        switch(local_2b5) {
        case 0:
        case 2:
        case 3:
          local_86 = 0;
          break;
        default:
          local_86 = 2;
          break;
        case 4:
        case 5:
        case 6:
        case 7:
          local_86 = 1;
        }
        if (local_2d8 * local_2d8 + local_2dc * local_2dc + local_2e0 * local_2e0 < 0.010000001) {
          local_2e0 = *(float *)PTR_DAT_00696714;
          local_2dc = *(float *)(PTR_DAT_00696714 + 4);
          local_2d8 = *(float *)(PTR_DAT_00696714 + 8);
        }
        if ((local_8c & 1) == 0) {
          local_57 = '\0';
        }
        else if (local_57 < '\x7f') {
          local_57 = local_57 + '\x01';
        }
        if ((local_8c & 2) == 0) {
          local_56 = '\0';
        }
        else if (local_56 < '\x7f') {
          local_56 = local_56 + '\x01';
        }
        local_56f = local_350 & 1;
        local_570 = 0;
        FUN_0055bea0(local_56c,local_558,&local_570);
        param_4 = param_4 * 29.999998;
        if (param_4 <= 1.0) {
          if (param_4 < 0.0) {
            param_4 = 0.0;
          }
        }
        else {
          param_4 = 1.0;
        }
        *param_1 = (local_4fc - (float)puVar1[0x17]) * param_4;
        param_1[1] = (local_4f8 - (float)puVar1[0x18]) * param_4;
        param_1[2] = (local_4f4 - (float)puVar1[0x19]) * param_4;
        *param_2 = (local_4e4 - (float)puVar1[0x1d]) * param_4;
        param_2[1] = (local_4e0 - (float)puVar1[0x1e]) * param_4;
        param_2[2] = (local_4dc - (float)puVar1[0x1f]) * param_4;
        *param_3 = (local_4d8 - (float)puVar1[0x20]) * param_4;
        param_3[1] = (local_4d4 - (float)puVar1[0x21]) * param_4;
        uVar4 = 1;
        param_3[2] = (local_4d0 - (float)puVar1[0x22]) * param_4;
      }
    }
  }
  return uVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
