// objects_dispose
// address 0x4f4db0, size 217 bytes
// name confidence: 0.9 (matches functions.md's summary; matches types/objects.h's
//   object_type_definition.dispose field comment, "0x18 objects_dispose")
// rewrite confidence: 0.7
// evidence: types/objects.h widget_type_definition (5 rows of 0x28 bytes at 0x0069c010; its
//   "reset" field comment reads "0x14 the five callbacks objects_dispose runs", matching the
//   5-row/0x28-stride/+0x14 loop here); object_type_definition chain (next at 0xc0, dispose
//   hook at 0x18); globals list (light_cluster_first 0x00860b20, light_object_references
//   0x00860b28, light_cluster_references 0x00860b24, object_data, object_memory_pool, and the
//   four collideable/noncollideable cluster/object-reference globals).
// register convention: none (void), matches Ghidra's __cdecl void(void) signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern widget_type_definition widget_type_definitions[k_maximum_widget_types]; // 0x0069c010
extern object_type_definition *object_type_definition_list; // 0x008603dc
extern datum_index *light_cluster_first; // 0x00860b20
extern data_array *light_object_references; // 0x00860b28
extern data_array *light_cluster_references; // 0x00860b24
extern data_array *object_data; // 0x008603b0
extern memory_pool *object_memory_pool; // 0x006b8cb4
extern datum_index *collideable_cluster_first; // 0x008603d0
extern void *collideable_cluster_partition; // 0x008603d8
extern data_array *collideable_object_references; // 0x008603d4
extern datum_index *noncollideable_cluster_first; // 0x008603c0
extern void *noncollideable_cluster_partition; // 0x008603c8
extern data_array *noncollideable_object_references; // 0x008603c4

void objects_dispose(void)
{
    int i;
    object_type_definition *def;

    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].reset != 0) {
            ((void (*)(void))widget_type_definitions[i].reset)();
        }
    }

    for (def = object_type_definition_list; def != 0; def = def->next) {
        if (def->dispose != 0) {
            ((void (*)(void))def->dispose)();
        }
    }

    if (light_cluster_first != 0) {
        light_cluster_first = 0;
    }
    if (light_object_references != 0) {
        light_object_references = 0;
    }
    if (light_cluster_references != 0) {
        light_cluster_references = 0;
    }
    if (object_data != 0) {
        object_data = 0;
    }
    if (object_memory_pool != 0) {
        object_memory_pool = 0;
    }
    if (collideable_cluster_first != 0) {
        collideable_cluster_first = 0;
    }
    if (collideable_cluster_partition != 0) {
        collideable_cluster_partition = 0;
    }
    if (collideable_object_references != 0) {
        collideable_object_references = 0;
    }
    if (noncollideable_cluster_first != 0) {
        noncollideable_cluster_first = 0;
    }
    if (noncollideable_cluster_partition != 0) {
        noncollideable_cluster_partition = 0;
    }
    if (noncollideable_object_references != 0) {
        noncollideable_object_references = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4f4db0):

void __cdecl objects_dispose(void)

{
  int iVar1;
  undefined **ppuVar2;
  int iVar3;

  ppuVar2 = &PTR_FUN_0069c024;
  iVar3 = 5;
  do {
    if ((code *)*ppuVar2 != (code *)0x0) {
      (*(code *)*ppuVar2)();
    }
    ppuVar2 = ppuVar2 + 10;
    iVar3 = iVar3 + -1;
    iVar1 = DAT_008603dc;
  } while (iVar3 != 0);
  for (; iVar1 != 0; iVar1 = *(int *)(iVar1 + 0xc0)) {
    if (*(code **)(iVar1 + 0x18) != (code *)0x0) {
      (**(code **)(iVar1 + 0x18))();
    }
  }
  if (DAT_00860b20 != 0) {
    DAT_00860b20 = 0;
  }
  if (DAT_00860b28 != 0) {
    DAT_00860b28 = 0;
  }
  if (DAT_00860b24 != 0) {
    DAT_00860b24 = 0;
  }
  if (DAT_008603b0 != 0) {
    DAT_008603b0 = 0;
  }
  if (DAT_006b8cb4 != 0) {
    DAT_006b8cb4 = 0;
  }
  if (DAT_008603d0 != 0) {
    DAT_008603d0 = 0;
  }
  if (DAT_008603d8 != 0) {
    DAT_008603d8 = 0;
  }
  if (DAT_008603d4 != 0) {
    DAT_008603d4 = 0;
  }
  if (DAT_008603c0 != 0) {
    DAT_008603c0 = 0;
  }
  if (DAT_008603c8 != 0) {
    DAT_008603c8 = 0;
  }
  if (DAT_008603c4 != 0) {
    DAT_008603c4 = 0;
  }
  return;
}
#endif
