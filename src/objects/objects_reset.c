// objects_reset
// address 0x4f4bb0, size 261 bytes
// name confidence: 0.9 (matches functions.md's summary and the vtable field comment on
//   object_type_definition.reset, "0x1c objects_reset", which this function itself is)
// rewrite confidence: 0.75
// evidence: types/objects.h globals list (object_data, object_name_list,
//   collideable/noncollideable_cluster_first/_object_references/_cluster_partition,
//   object_cluster_stamp, object_globals_pointer) and object_globals's field layout, which this
//   function's tail writes field-for-field (collecting_in_clusters, tracked_object_count,
//   first_tracked_object, cluster_pvs_previous[16], cluster_pvs_current[16], last_garbage_collection_tick,
//   ambient_cluster_mode); data_array.valid at 0x24 (types/memory.h); object_type_definition
//   chain (next at 0xc0, reset hook at 0x1c).
// register convention: none (void), matches Ghidra's __cdecl void(void) signature.
// UNSURE: the two collideable/noncollideable_cluster_partition globals are declared void* in
//   types/objects.h (the exact record cluster_partition_new returns was never resolved); this
//   function treats their first 0x25 bytes like a data_array (valid at +0x24, then
//   data_delete_all on the same pointer), so the cast to data_array* here is inferred from that
//   usage, not from a resolved type. UNSURE: 0x006b8c60 and 0x006b8a00 are not documented
//   anywhere in types/objects.h; kept as opaque externs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_objects.h"
#include "fn_memory.h"

extern uint32_t object_unknown_006b8c60; // 0x006b8c60, UNSURE: foreign/undocumented global
extern int32_t object_sound_event_last_tick; // 0x006b8a00, UNSURE: foreign/undocumented global
extern uint16_t object_visibility_computed_mask; // 0x006b8cb0
extern object_type_definition *object_type_definition_list; // 0x008603dc
extern data_array *object_data; // 0x008603b0
extern datum_index *object_name_list; // 0x006b8cb8
extern datum_index *collideable_cluster_first; // 0x008603d0
extern data_array *collideable_object_references; // 0x008603d4
extern void *collideable_cluster_partition; // 0x008603d8
extern datum_index *noncollideable_cluster_first; // 0x008603c0
extern data_array *noncollideable_object_references; // 0x008603c4
extern void *noncollideable_cluster_partition; // 0x008603c8
extern int32_t object_cluster_stamp; // 0x008603cc
extern object_globals *object_globals_pointer; // 0x006b8cbc


void objects_reset(void)
{
    object_type_definition *def;
    datum_index *slot;
    int32_t i;

    object_unknown_006b8c60 = 0xffffffff;
    object_sound_event_last_tick = 0;
    widgets_dispose();
    object_visibility_computed_mask = 0;

    for (def = object_type_definition_list; def != 0; def = def->next) {
        if (def->reset != 0) {
            ((void (*)(void))def->reset)();
        }
    }

    lights_dispose_all();

    object_data->valid = 1;
    data_delete_all(object_data);

    slot = object_name_list;
    for (i = k_maximum_object_names; i != 0; i--) {
        *slot = k_datum_index_none;
        slot++;
    }

    slot = collideable_cluster_first;
    for (i = k_maximum_clusters; i != 0; i--) {
        *slot = k_datum_index_none;
        slot++;
    }

    ((data_array *)collideable_cluster_partition)->valid = 1;
    data_delete_all((data_array *)collideable_cluster_partition);
    collideable_object_references->valid = 1;
    data_delete_all(collideable_object_references);

    slot = noncollideable_cluster_first;
    for (i = k_maximum_clusters; i != 0; i--) {
        *slot = k_datum_index_none;
        slot++;
    }

    ((data_array *)noncollideable_cluster_partition)->valid = 1;
    data_delete_all((data_array *)noncollideable_cluster_partition);
    noncollideable_object_references->valid = 1;
    data_delete_all(noncollideable_object_references);

    for (i = 0; i < 16; i++) {
        object_globals_pointer->cluster_pvs_previous[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        object_globals_pointer->cluster_pvs_current[i] = 0;
    }
    object_globals_pointer->ambient_cluster_mode = 0;
    object_globals_pointer->collecting_in_clusters = 0;
    object_cluster_stamp = 0;
    object_globals_pointer->tracked_object_count = 0;
    object_globals_pointer->last_garbage_collection_tick = 0;
    object_globals_pointer->first_tracked_object = k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x4f4bb0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl objects_reset(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;

  _DAT_006b8c60 = 0xffffffff;
  DAT_006b8a00 = 0;
  widgets_dispose();
  DAT_006b8cb0 = 0;
  for (iVar1 = DAT_008603dc; iVar1 != 0; iVar1 = *(int *)(iVar1 + 0xc0)) {
    if (*(code **)(iVar1 + 0x1c) != (code *)0x0) {
      (**(code **)(iVar1 + 0x1c))();
    }
  }
  lights_dispose_all();
  *(undefined1 *)(DAT_008603b0 + 0x24) = 1;
  data_delete_all();
  puVar3 = DAT_006b8cb8;
  for (iVar1 = 0x200; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0xffffffff;
    puVar3 = puVar3 + 1;
  }
  puVar3 = DAT_008603d0;
  for (iVar1 = 0x200; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0xffffffff;
    puVar3 = puVar3 + 1;
  }
  *(undefined1 *)(DAT_008603d8 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_008603d4 + 0x24) = 1;
  data_delete_all();
  puVar3 = DAT_008603c0;
  for (iVar1 = 0x200; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0xffffffff;
    puVar3 = puVar3 + 1;
  }
  *(undefined1 *)(DAT_008603c8 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_008603c4 + 0x24) = 1;
  data_delete_all();
  iVar1 = DAT_006b8cbc;
  puVar3 = (undefined4 *)(DAT_006b8cbc + 0xc);
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  puVar3 = (undefined4 *)(iVar1 + 0x4c);
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)(iVar1 + 0x90) = 0;
  *(undefined1 *)(iVar1 + 1) = 0;
  DAT_008603cc = 0;
  *(undefined2 *)(iVar1 + 4) = 0;
  *(undefined4 *)(iVar1 + 0x8c) = 0;
  *(undefined4 *)(iVar1 + 8) = 0xffffffff;
  return;
}
#endif
