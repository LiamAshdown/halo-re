// ai_object_list_remap_units_and_children  (Ghidra: ai_object_list_remap_units_and_children; named for this rewrite)
// address 0x433a70, size 296 bytes
// name confidence: 0.35   rewrite confidence: 0.8 (FIXED: the remap calls pass notify 0 (the binary pushes constants; EDX is not an input))
// evidence: walks an object_list (types/hs.h) and, for every biped or vehicle object in it
// (object_try_and_get with type mask 3, already established), calls
// ai_unit_remap_actor_to_squad (0x433970, this batch) on it and then recursively on every
// biped/vehicle child object (object.next_object/first_child_object, types/objects.h
// 0x114/0x118). This function has zero callers in this build, so the third register
// argument (assumed here to be `notify`, forwarded unchanged to every
// ai_unit_remap_actor_to_squad call, matching the "flag this function itself never touches"
// pattern already seen in ai_reference_notify_actors, this batch) is a guess.
// register convention: UNSURE overall; Ghidra left the object_list header in ECX and the
// packed reference in EBX unresolved.
//   // blam-cc: ECX -> object_list_header, EBX -> packed_reference, EDX -> notify (guessed)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "hs.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;                // 0x008603b0
extern data_array *object_list_reference_data; // 0x0087a468

extern datum_index object_list_get_first(datum_index header_index, object_list_iterator *iterator_out); // 0x48b2f0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void ai_unit_remap_actor_to_squad(datum_index unit_index, uint32_t packed_reference, char notify); // 0x433970, this batch
extern void ai_recompute_all_relationship_flags(void); // 0x42bbb0, outside this rewrite's range, UNSURE signature
extern void encounters_recompute_dirty(void); // 0x435f00, this batch

// blam-cc: ECX -> object_list_header, EBX -> packed_reference, EDX -> notify (guessed)
void ai_object_list_remap_units_and_children(datum_index object_list_header, uint32_t packed_reference, char notify)
{
    if (object_list_header != (datum_index)k_datum_index_none && packed_reference != (uint32_t)k_datum_index_none) {
        object_list_iterator iterator;
        datum_index object_index = object_list_get_first(object_list_header, &iterator);

        while (object_index != (datum_index)k_datum_index_none) {
            object *obj = object_try_and_get(object_index, 3); // biped or vehicle

            if (obj != 0) {
                datum_index child;
                ai_unit_remap_actor_to_squad(object_index, packed_reference, 0); // FIXED: the binary pushes (reference, 0, 0) (0x433af2 / 0x433b34)

                child = obj->first_child_object;
                while (child != (datum_index)k_datum_index_none) {
                    object_header *child_header = &((object_header *)object_data->data)[child & 0xffff];
                    if ((1 << (child_header->type & 0x1f) & 3) != 0) {
                        ai_unit_remap_actor_to_squad(child, packed_reference, 0); // FIXED: the binary pushes (reference, 0, 0) (0x433af2 / 0x433b34)
                    }
                    child = ((object *)child_header->data)->next_object;
                }
            }

            if (iterator == (datum_index)k_datum_index_none) {
                object_index = (datum_index)k_datum_index_none;
            } else {
                object_list_reference *node =
                    (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                               (iterator & 0xffff) * 0x0c);
                iterator = node->next;
                object_index = node->object_index;
            }
        }

        ai_recompute_all_relationship_flags();
        encounters_recompute_dirty();
    }
}

#if 0
Original Ghidra decompilation (0x433a70):

void FUN_00433a70(void)

{
  uint uVar1;
  short sVar2;
  int iVar3;
  uint in_ECX;
  short *psVar4;
  int unaff_EBX;
  int iVar5;
  short sVar6;
  short *psVar7;

  if ((in_ECX != 0xffffffff) && (unaff_EBX != -1)) {
    iVar3 = object_list_get_first();
    iVar5 = DAT_008603b0;
    while (iVar3 != -1) {
      psVar7 = (short *)0x0;
      if (((iVar3 != -1) && (sVar2 = (short)iVar3, -1 < sVar2)) &&
         (sVar2 < *(short *)(iVar5 + 0x20))) {
        psVar4 = (short *)((int)*(short *)(iVar5 + 0x22) * (int)sVar2 + *(int *)(iVar5 + 0x34));
        sVar2 = *psVar4;
        if ((sVar2 != 0) && ((sVar6 = (short)((uint)iVar3 >> 0x10), sVar6 == 0 || (sVar2 == sVar6)))
           ) {
          psVar7 = psVar4;
        }
      }
      if (((psVar7 != (short *)0x0) && ((1 << (*(byte *)((int)psVar7 + 3) & 0x1f) & 3U) != 0)) &&
         (iVar3 = *(int *)(psVar7 + 4), iVar3 != 0)) {
        FUN_00433970();
        uVar1 = *(uint *)(iVar3 + 0x118);
        iVar5 = DAT_008603b0;
        while (uVar1 != 0xffffffff) {
          iVar3 = *(int *)(*(int *)(iVar5 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
          if ((1 << (*(byte *)(iVar3 + 0xb4) & 0x1f) & 3U) != 0) {
            FUN_00433970();
            iVar5 = DAT_008603b0;
          }
          uVar1 = *(uint *)(iVar3 + 0x114);
        }
      }
      if (in_ECX == 0xffffffff) {
        iVar3 = -1;
      }
      else {
        iVar3 = *(int *)(DAT_0087a468 + 0x34) + (in_ECX & 0xffff) * 0xc;
        in_ECX = *(uint *)(iVar3 + 8);
        iVar3 = *(int *)(iVar3 + 4);
      }
    }
    FUN_0042bbb0();
    FUN_00435f00();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
