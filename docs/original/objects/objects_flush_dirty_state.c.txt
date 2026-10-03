// objects_flush_dirty_state
// address 0x4f4cc0, size 237 bytes
// name confidence: 0.85 (still FUN_004f4cc0 in Ghidra; types/objects.h's
//   object_type_definition.flush field comment names this exact function: "0x20
//   objects_flush_dirty_state")
// rewrite confidence: 0.6
// evidence: types/objects.h globals list (light_data 0x00860b14, light_object_references
//   0x00860b28, light_cluster_references 0x00860b24, object_data, the four
//   collideable/noncollideable cluster/object-reference globals); data_array.valid at 0x24;
//   object_type_definition chain (next at 0xc0, flush hook at 0x20). The datum_next loop over
//   object_data is restored to a real call, following the precedent in
//   src/hs/hs_object_list_collect_player_units.c, whose evidence comment establishes that this
//   exact inlined tail is datum_next's body re-inlined by the compiler.
// register convention: none (void), matches the callers seen elsewhere in this batch.
// UNSURE: object_block_data_free's argument is not visible in the decompilation (called with no visible
//   parentheses args); by analogy with the datum_next loop it almost certainly receives the
//   current object index, but that is not proven here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern object_type_definition *object_type_definition_list; // 0x008603dc
extern data_array *light_data; // 0x00860b14
extern data_array *light_object_references; // 0x00860b28
extern data_array *light_cluster_references; // 0x00860b24
extern data_array *object_data; // 0x008603b0
extern void *collideable_cluster_partition; // 0x008603d8
extern data_array *collideable_object_references; // 0x008603d4
extern void *noncollideable_cluster_partition; // 0x008603c8
extern data_array *noncollideable_object_references; // 0x008603c4

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern void object_block_data_free(data_array *array, datum_index object_index); // 0x4f7de0, UNSURE: argument inferred by analogy
extern void widgets_dispose_clear_flag(void); // 0x4ffa50

void objects_flush_dirty_state(void)
{
    object_type_definition *def;
    datum_index index;

    widgets_dispose_clear_flag();

    for (def = object_type_definition_list; def != 0; def = def->next) {
        if (def->flush != 0) {
            ((void (*)(void))def->flush)();
        }
    }

    light_data->valid = 0;
    if (light_object_references->valid != 0) {
        light_object_references->valid = 0;
    }
    if (light_cluster_references->valid != 0) {
        light_cluster_references->valid = 0;
    }

    if (object_data->valid != 0) {
        index = datum_next(-1, object_data);
        while (index != k_datum_index_none) {
            object_block_data_free(object_data, index); // 0x4f4d26 mov eax,edi / mov edx,esi
            index = datum_next((int16_t)index, object_data);
        }
        object_data->valid = 0;
    }

    if (((data_array *)collideable_cluster_partition)->valid != 0) {
        ((data_array *)collideable_cluster_partition)->valid = 0;
    }
    if (collideable_object_references->valid != 0) {
        collideable_object_references->valid = 0;
    }
    if (((data_array *)noncollideable_cluster_partition)->valid != 0) {
        ((data_array *)noncollideable_cluster_partition)->valid = 0;
    }
    if (noncollideable_object_references->valid != 0) {
        noncollideable_object_references->valid = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4f4cc0):

void FUN_004f4cc0(void)

{
  int iVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  short *psVar5;

  widgets_dispose_clear_flag();
  for (iVar1 = DAT_008603dc; iVar1 != 0; iVar1 = *(int *)(iVar1 + 0xc0)) {
    if (*(code **)(iVar1 + 0x20) != (code *)0x0) {
      (**(code **)(iVar1 + 0x20))();
    }
  }
  *(undefined1 *)(DAT_00860b14 + 0x24) = 0;
  if (*(char *)(DAT_00860b28 + 0x24) != '\0') {
    *(undefined1 *)(DAT_00860b28 + 0x24) = 0;
  }
  if (*(char *)(DAT_00860b24 + 0x24) != '\0') {
    *(undefined1 *)(DAT_00860b24 + 0x24) = 0;
  }
  iVar1 = DAT_008603b0;
  if (*(char *)(DAT_008603b0 + 0x24) != '\0') {
    uVar3 = FUN_004d0630();
joined_r0x004f4d21:
    if (uVar3 != 0xffffffff) {
      FUN_004f7de0();
      iVar4 = uVar3 + 1;
      uVar3 = 0xffffffff;
      sVar2 = (short)iVar4;
      if ((-1 < sVar2) && (sVar2 < *(short *)(iVar1 + 0x2e))) {
        psVar5 = (short *)((int)sVar2 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
        do {
          if (*psVar5 != 0) {
            uVar3 = (int)*psVar5 << 0x10 | (int)(short)iVar4;
            break;
          }
          iVar4 = iVar4 + 1;
          psVar5 = (short *)((int)psVar5 + (int)*(short *)(iVar1 + 0x22));
        } while ((short)iVar4 < *(short *)(iVar1 + 0x2e));
      }
      goto joined_r0x004f4d21;
    }
    *(undefined1 *)(iVar1 + 0x24) = 0;
  }
  if (*(char *)(DAT_008603d8 + 0x24) != '\0') {
    *(undefined1 *)(DAT_008603d8 + 0x24) = 0;
  }
  if (*(char *)(DAT_008603d4 + 0x24) != '\0') {
    *(undefined1 *)(DAT_008603d4 + 0x24) = 0;
  }
  if (*(char *)(DAT_008603c8 + 0x24) != '\0') {
    *(undefined1 *)(DAT_008603c8 + 0x24) = 0;
  }
  if (*(char *)(DAT_008603c4 + 0x24) != '\0') {
    *(undefined1 *)(DAT_008603c4 + 0x24) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
