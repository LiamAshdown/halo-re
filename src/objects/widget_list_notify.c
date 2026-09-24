// widget_list_notify
// address 0x4ffca0, size 106 bytes
// name confidence: 0.8 (types/objects.h's own widget_type_definition.render comment names this
//   function directly: "widget_list_notify")
// rewrite confidence: 0.55
// evidence: types/objects.h object (first_widget 0x16c), widget (type 0x02, next_widget 0x08),
//   widget_type_definition (render 0x24); global 0x008603b0 object_data, 0x00860398 widget_data.
// register convention: Ghidra shows a single unresolved `unaff_EDI`; by the object_data lookup
//   shape shared with widget_new.c / widget_delete_all.c, EDI is the object index.
// blam-cc: EDI -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern data_array *widget_data; // 0x00860398
extern widget_type_definition widget_type_definitions[k_maximum_widget_types]; // 0x0069c010

void widget_list_notify(uint32_t object_index /*EDI*/) // blam-cc: EDI -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    datum_index handle = obj->first_widget;

    while (handle != (datum_index)0xffffffff) {
        widget *entry = &((widget *)widget_data->data)[handle & 0xffff];
        if (widget_type_definitions[entry->type].render != 0) {
            ((void (*)(void))widget_type_definitions[entry->type].render)();
        }
        handle = entry->next_widget;
    }
}

#if 0
Original Ghidra decompilation (0x4ffca0):

void FUN_004ffca0(void)

{
  int iVar1;
  uint uVar2;
  uint unaff_EDI;

  uVar2 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc) + 0x16c
                   );
  while (uVar2 != 0xffffffff) {
    iVar1 = *(int *)(DAT_00860398 + 0x34) + (uVar2 & 0xffff) * 0xc;
    if ((code *)(&PTR_LAB_0069c034)[*(short *)(iVar1 + 2) * 10] != (code *)0x0) {
      (*(code *)(&PTR_LAB_0069c034)[*(short *)(iVar1 + 2) * 10])();
    }
    uVar2 = *(uint *)(iVar1 + 8);
  }
  return;
}
#endif
