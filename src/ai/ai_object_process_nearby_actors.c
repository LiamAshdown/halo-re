// ai_object_process_nearby_actors  (Ghidra: ai_object_process_nearby_actors; named for this rewrite)
// address 0x433cc0, size 335 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: Ghidra badly misjudges this function's stack frame (declaring two ~390KB
// phantom locals); the real locals are a reference position (object_get_position), a
// 0x40-entry candidate array of {actor_index, distance_squared, is_type_9} records (stride
// 0xc, matching object_sort_by_flag_then_distance's own +4/+8 reads -- that comparator is
// library math code per out/phase4/ai_types_notes.md and is not rewritten in this batch),
// and a 32-byte buffer unit_find_seats_matching_name_and_flags (outside this rewrite's range) fills in. Reproduced here
// with an explicit local struct instead of chasing Ghidra's phantom stack layout. Gathers
// every actor named by a packed ai reference, sorts them by (is-type-9-target, ascending
// distance to the current object), and processes them nearest-first via
// actor_play_first_valid_vocalization-style callee actor_play_first_valid_vocalization (outside this batch,
// declared with the signature its other call site in actor_squad_action_execute.c already
// established), stopping early at the first type-9 candidate unless allow_type_9 is set.
// register convention: Ghidra fully resolved param_1/param_2/param_3 and left the current
// object index in EAX.
//   // blam-cc: EAX -> object_index, stack -> query_a, query_b, allow_type_9

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_get_position(real_point3d *out, uint32_t object_index);       // 0x4f6900
extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern int object_sort_by_flag_then_distance(const void *a, const void *b); // 0x433c70, library code, not rewritten in this batch
extern void _qsort(void *base, int32_t count, int32_t size, int (*compare)(const void *, const void *)); // 0x623410
extern int16_t unit_find_seats_matching_name_and_flags(uint32_t query_a, uint32_t query_b, uint32_t unused, void *out_buffer, int32_t buffer_size); // 0x56a310, outside this rewrite's range, UNSURE signature
extern uint8_t actor_play_first_valid_vocalization(); // 0x40e260, this module, not in this rewrite's range; SIGNATURE-CONFLICT with
                                // actor_squad_action_execute.c's own call site, left unprototyped, see src/ai/README.md

// blam-cc: EAX -> object_index, stack -> query_a, query_b, allow_type_9
void ai_object_process_nearby_actors(datum_index object_index, uint32_t query_a, uint32_t query_b,
                                      char allow_type_9)
{
    object *obj = object_try_and_get(object_index, 3); // biped or vehicle

    if (obj != 0) {
        real_point3d reference_position;
        uint8_t query_buffer[0x10];
        int16_t category;
        int16_t candidate_count = 0;
        ai_nearby_actor_candidate candidates[0x40];

        object_get_position(&reference_position, object_index);
        category = unit_find_seats_matching_name_and_flags(query_a, query_b, (uint32_t)k_datum_index_none, query_buffer, 0x10);

        if (category > 0) {
            ai_reference_actor_iterator iterator;
            actor *a;

            ai_reference_actor_iterator_new(query_a, &iterator);
            a = ai_reference_actor_iterator_next(&iterator);
            while (a != 0) {
                if (candidate_count < 0x40) {
                    float dx = reference_position.x - a->body_position.x;
                    float dy = reference_position.y - a->body_position.y;
                    float dz = reference_position.z - a->body_position.z;
                    ai_nearby_actor_candidate *c = &candidates[candidate_count];
                    c->actor_index = iterator.actor_index;
                    c->distance_squared = dy * dy + dx * dx + dz * dz;
                    c->is_type_9 = (a->mode == 9);
                    candidate_count = candidate_count + 1;
                }
                a = ai_reference_actor_iterator_next(&iterator);
            }

            _qsort(candidates, candidate_count, sizeof(ai_nearby_actor_candidate), object_sort_by_flag_then_distance);

            {
                int16_t i;
                for (i = 0; i < candidate_count; i++) {
                    if (candidates[i].is_type_9 != 0 && allow_type_9 == 0) {
                        return;
                    }
                    actor_play_first_valid_vocalization(candidates[i].actor_index, 0, (uint32_t)k_datum_index_none, category);
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
