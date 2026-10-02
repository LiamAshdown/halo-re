// ai_reference_build_object_list  (Ghidra: ai_reference_build_object_list; named for this rewrite)
// address 0x432740, size 371 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: allocates an object_list_header (types/hs.h, "object list header" data_array at
// 0x0087a464) and, for every actor named by a packed ai reference (walked via
// ai_reference_actor_iterator_new/_next, this batch), pushes its controlled unit
// (actor.unit_index) plus every unit chained off actor.cluster_unit_index through
// object+0x1fc -- exactly the chain types/ai.h's own comment on actor.cluster_unit_index
// describes ("head of the unit cluster list, chained through object+0x1fc"). The push logic
// is object_list_reference_add (0x48b2a0, src/hs/object_list_reference_add.c) inlined by the
// compiler (it is absent from this function's own callee list); called directly here
// instead of reproducing its body a second time.
// register convention: Ghidra fully resolved the one parameter.
//   // blam-cc: stack -> packed_reference (EAX per the sibling ai_reference_* functions'
//   convention; not independently confirmed with objdump for this file)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;             // 0x008603b0
extern data_array *object_list_header_data; // 0x0087a464

extern datum_index datum_new(data_array *array); // 0x4d0480
extern void object_list_reference_add(datum_index header_index, datum_index object_index); // 0x48b2a0, src/hs
extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch

// blam-cc: stack -> packed_reference
// Builds a fresh object_list (see types/hs.h) of every unit associated with a packed ai
// reference: for each matching actor, its own controlled unit and every unit chained off
// its unit cluster (a vehicle and its other occupants). Returns the new list's header
// handle, or none if the reference is none or the header allocation failed.
datum_index ai_reference_build_object_list(uint32_t packed_reference)
{
    datum_index header_index = (datum_index)k_datum_index_none;

    if (packed_reference != (uint32_t)k_datum_index_none) {
        header_index = datum_new(object_list_header_data);
        if (header_index != (datum_index)k_datum_index_none) {
            object_list_header *header =
                (object_list_header *)((uint8_t *)object_list_header_data->data + (header_index & 0xffff) * 0x0c);
            ai_reference_actor_iterator iterator;
            actor *a;

            header->count = 0;
            header->first_reference = (datum_index)k_datum_index_none;

            ai_reference_actor_iterator_new(packed_reference, &iterator);
            a = ai_reference_actor_iterator_next(&iterator);
            while (a != 0) {
                datum_index passenger;

                if (a->unit_index != (datum_index)k_datum_index_none) {
                    object_list_reference_add(header_index, a->unit_index);
                }

                passenger = a->cluster_unit_index;
                while (passenger != (datum_index)k_datum_index_none) {
                    object_header *passenger_header = &((object_header *)object_data->data)[passenger & 0xffff];
                    object_list_reference_add(header_index, passenger);
                    passenger = *(datum_index *)((uint8_t *)passenger_header->data + 0x1fc); // UNSURE: units.h names this swarm_next_unit_index but never establishes it; here it is the next unit-cluster passenger
                }

                a = ai_reference_actor_iterator_next(&iterator);
            }
        }
    }

    return header_index;
}

#if 0
Original Ghidra decompilation (0x432740):

uint FUN_00432740(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;

  iVar1 = DAT_0087a464;
  uVar5 = 0xffffffff;
  if ((param_1 != -1) && (uVar5 = datum_new(), uVar5 != 0xffffffff)) {
    iVar6 = *(int *)(iVar1 + 0x34) + (uVar5 & 0xffff) * 0xc;
    *(undefined2 *)(iVar6 + 6) = 0;
    *(undefined4 *)(iVar6 + 8) = 0xffffffff;
    if (uVar5 != 0xffffffff) {
      FUN_00432650(param_1);
      iVar6 = FUN_004326d0();
      iVar4 = DAT_0087a468;
      while (iVar6 != 0) {
        iVar3 = *(int *)(iVar6 + 0x18);
        if (iVar3 != -1) {
          iVar1 = *(int *)(iVar1 + 0x34) + (uVar5 & 0xffff) * 0xc;
          uVar7 = datum_new();
          if (uVar7 != 0xffffffff) {
            iVar2 = *(int *)(iVar4 + 0x34) + (uVar7 & 0xffff) * 0xc;
            *(int *)(iVar2 + 4) = iVar3;
            *(undefined4 *)(iVar2 + 8) = *(undefined4 *)(iVar1 + 8);
            *(uint *)(iVar1 + 8) = uVar7;
          }
          *(short *)(iVar1 + 6) = *(short *)(iVar1 + 6) + 1;
        }
        uVar7 = *(uint *)(iVar6 + 0x24);
        if (uVar7 != 0xffffffff) {
          do {
            iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
            iVar6 = *(int *)(DAT_0087a464 + 0x34) + (uVar5 & 0xffff) * 0xc;
            uVar8 = datum_new();
            if (uVar8 != 0xffffffff) {
              iVar3 = *(int *)(iVar4 + 0x34) + (uVar8 & 0xffff) * 0xc;
              *(uint *)(iVar3 + 4) = uVar7;
              *(undefined4 *)(iVar3 + 8) = *(undefined4 *)(iVar6 + 8);
              *(uint *)(iVar6 + 8) = uVar8;
            }
            *(short *)(iVar6 + 6) = *(short *)(iVar6 + 6) + 1;
            uVar7 = *(uint *)(iVar1 + 0x1fc);
          } while (uVar7 != 0xffffffff);
        }
        iVar6 = FUN_004326d0();
        iVar1 = DAT_0087a464;
      }
    }
  }
  return uVar5;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
