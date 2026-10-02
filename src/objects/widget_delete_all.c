// widget_delete_all
// address 0x4ffbe0, size 122 bytes
// name confidence: 0.85 (out/phase4/objects_functions.md: "Deletes every widget attached to an
//   object, dispatching to each widget's type-specific dispose routine")
// rewrite confidence: 0.55
// evidence: types/objects.h object (first_widget 0x16c), widget (type 0x02, instance 0x04,
//   next_widget 0x08), widget_type_definition (delete_instance 0x1c).
// register convention: Ghidra shows a single unresolved `in_EAX`; by the same reasoning as
//   widget_new.c, EAX is the object index.
// blam-cc: EAX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern data_array *widget_data; // 0x00860398
extern widget_type_definition widget_type_definitions[k_maximum_widget_types]; // 0x0069c010

extern void datum_delete(data_array *array, datum_index handle); // memory module, 0x4d0510

void widget_delete_all(uint32_t object_index /*EAX*/) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    datum_index handle = obj->first_widget;

    while (handle != (datum_index)0xffffffff) {
        uint16_t index = (uint16_t)handle;
        widget *entry = &((widget *)widget_data->data)[index];
        datum_index next = entry->next_widget;
        datum_index instance = entry->instance;

        if (instance != (datum_index)0xffffffff) {
            void (*delete_instance)(datum_index) =
                (void (*)(datum_index))widget_type_definitions[entry->type].delete_instance;
            delete_instance(instance);
        }
        datum_delete(widget_data, handle);
        handle = next;
    }

    obj->first_widget = (datum_index)0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4ffbe0):

void widget_delete_all(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  uint uVar5;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar2 = *(uint *)(iVar1 + 0x16c);
  iVar4 = DAT_00860398;
  while (uVar2 != 0xffffffff) {
    uVar5 = uVar2 & 0xffff;
    uVar2 = *(uint *)(*(int *)(iVar4 + 0x34) + 8 + uVar5 * 0xc);
    iVar4 = *(int *)(iVar4 + 0x34) + uVar5 * 0xc;
    iVar3 = *(int *)(iVar4 + 4);
    if (iVar3 != -1) {
      (**(code **)(&DAT_0069c02c + *(short *)(iVar4 + 2) * 0x28))(iVar3);
    }
    iVar4 = datum_delete();
  }
  *(undefined4 *)(iVar1 + 0x16c) = 0xffffffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
