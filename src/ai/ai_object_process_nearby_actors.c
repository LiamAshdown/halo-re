// ai_object_process_nearby_actors  (Ghidra: FUN_00433cc0; really: the engine side of ai_go_to_vehicle /
//   ai_go_to_vehicle_override, hs evaluators 0x47d9d0 / 0x47da20)
// address 0x433cc0, size 335 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// VERIFIED against disassembly 0x433cc0..0x433e1b (2026-09-30); rewritten from it (the draft took the ai reference from the stack and passed the
//   seat search the wrong object). EAX: the packed ai reference (-1 = nothing); stack (vehicle, seat name,
//   allow actors already boarding). For a biped or vehicle, the seats matching the name (any flags, at most 16)
//   are collected (0x56a310); the referenced actors (at most 0x40) are sorted by (already boarding, distance to
//   the vehicle) and each is sent to the first still-free listed seat it can use (0x40e260, which strikes the
//   seat from the shared list). Without the override the first already-boarding actor ends the pass.
// blam-cc: EAX -> ai_reference, stack -> (vehicle_index, seat_name, allow_boarding_actors)
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_get_position(real_point3d *out, uint32_t object_index);       // 0x4f6900
extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern int object_sort_by_flag_then_distance(const void *a, const void *b); // 0x433c70, library code, not rewritten in this batch
extern int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector,
                                                       int16_t *out_indices, int16_t max_indices); // 0x56a310
extern uint8_t actor_play_first_valid_vocalization(int16_t *seat_list, datum_index vehicle_index, datum_index actor_index,
                                                   char *seat_name, int16_t seat_flags, int16_t count); // 0x40e260, EAX, ECX, stack

void ai_object_process_nearby_actors(uint32_t ai_reference, datum_index vehicle_index, char *seat_name,
                                      char allow_boarding_actors)
{
    object *obj;

    if (ai_reference == 0xffffffff) {
        return;
    }
    obj = object_try_and_get(vehicle_index, 3); // biped or vehicle
    if (obj != 0) {
        real_point3d reference_position;
        int16_t seat_list[16];
        int16_t seat_count;
        int16_t candidate_count = 0;
        ai_nearby_actor_candidate candidates[0x40];

        object_get_position(&reference_position, vehicle_index);
        seat_count = unit_find_seats_matching_name_and_flags(vehicle_index, seat_name, 0xffff, seat_list, 0x10);

        if (seat_count > 0) {
            ai_reference_actor_iterator iterator;
            actor *a;

            ai_reference_actor_iterator_new(ai_reference, &iterator);
            a = ai_reference_actor_iterator_next(&iterator);
            while (a != 0) {
                if (candidate_count < 0x40) {
                    float dx = reference_position.x - a->body_position.x;
                    float dy = reference_position.y - a->body_position.y;
                    float dz = reference_position.z - a->body_position.z;
                    ai_nearby_actor_candidate *c = &candidates[candidate_count];
                    c->actor_index = iterator.actor_index;
                    c->distance_squared = dz * dz + dx * dx + dy * dy; // 0x433d76..0x433d84
                    c->is_type_9 = (a->mode == 9);
                    candidate_count = candidate_count + 1;
                }
                a = ai_reference_actor_iterator_next(&iterator);
            }

            qsort(candidates, candidate_count, sizeof(ai_nearby_actor_candidate), object_sort_by_flag_then_distance);

            {
                int16_t i;
                for (i = 0; i < candidate_count; i++) {
                    if (candidates[i].is_type_9 != 0 && allow_boarding_actors == 0) {
                        return;
                    }
                    actor_play_first_valid_vocalization(seat_list, vehicle_index, candidates[i].actor_index, 0, -1,
                                                        seat_count);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x433cc0):

void FUN_00433cc0(undefined4 param_1,undefined4 param_2,char param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int in_EAX;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  ushort uVar7;
  short sVar8;
  float afStackY_60300 [2];
  char acStackY_602f8 [393104];
  float local_344;
  float local_340;
  float local_33c;
  float local_328;
  undefined1 local_320 [32];
  float local_300 [2];
  char acStack_2f8 [760];

  if ((in_EAX != -1) && (iVar4 = object_try_and_get(3), iVar4 != 0)) {
    uVar7 = 0;
    object_get_position();
    uVar5 = FUN_0056a310(param_1,param_2,0xffffffff,local_320,0x10);
    if (0 < (short)uVar5) {
      FUN_00432650();
      iVar4 = FUN_004326d0();
      while (iVar4 != 0) {
        if (uVar7 < 0x40) {
          iVar6 = (short)uVar7 * 0xc;
          local_300[(short)uVar7 * 3] = local_328;
          fVar1 = local_344 - *(float *)(iVar4 + 300);
          fVar2 = local_340 - *(float *)(iVar4 + 0x130);
          fVar3 = local_33c - *(float *)(iVar4 + 0x134);
          *(float *)(acStack_2f8 + iVar6 + -4) = fVar2 * fVar2 + fVar1 * fVar1 + fVar3 * fVar3;
          acStack_2f8[iVar6] = *(short *)(iVar4 + 0x6c) == 9;
          uVar7 = uVar7 + 1;
        }
        iVar4 = FUN_004326d0();
      }
      _qsort(local_300,(int)(short)uVar7,0xc,object_sort_by_flag_then_distance);
      sVar8 = 0;
      if (0 < (short)uVar7) {
        do {
          if ((acStack_2f8[sVar8 * 0xc] != '\0') && (param_3 == '\0')) {
            return;
          }
          FUN_0040e260(local_300[sVar8 * 3],0,0xffffffff,uVar5);
          sVar8 = sVar8 + 1;
        } while (sVar8 < (short)uVar7);
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
