// widget_list_has_flag
// address 0x4ffc60, size 64 bytes
// name confidence: 0.8 (types/objects.h's own widget_type_definition.flag comment names this
//   function directly: "the per-type value widget_list_has_flag reports")
// rewrite confidence: 0.7
// evidence: types/objects.h widget (type 0x02, next_widget 0x08), widget_type_definition (flag
//   0x04); global 0x00860398 widget_data.
// register convention: Ghidra shows a single unresolved `in_ECX`; by functions.md's summary
//   ("Returns whether any widget attached to an object belongs to a type marked in a per-type
//   flag table") this is the object's first_widget handle, the natural entry point for a widget
//   chain walk.
// blam-cc: ECX -> first_widget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *widget_data; // 0x00860398
extern widget_type_definition widget_type_definitions[k_maximum_widget_types]; // 0x0069c010

int8_t widget_list_has_flag(datum_index first_widget /*ECX*/) // blam-cc: ECX -> first_widget
{
    datum_index handle = first_widget;

    if (handle == (datum_index)0xffffffff) {
        return 0;
    }

    for (;;) {
        widget *entry = &((widget *)widget_data->data)[handle & 0xffff];
        if (widget_type_definitions[entry->type].flag != 0) {
            return 1;
        }
        handle = entry->next_widget;
        if (handle == (datum_index)0xffffffff) {
            return 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ffc60):

undefined1 FUN_004ffc60(void)

{
  int iVar1;
  undefined1 uVar2;
  uint in_ECX;

  uVar2 = 0;
  if (in_ECX != 0xffffffff) {
    while (iVar1 = *(int *)(DAT_00860398 + 0x34) + (in_ECX & 0xffff) * 0xc,
          (&DAT_0069c014)[*(short *)(iVar1 + 2) * 0x28] == '\0') {
      in_ECX = *(uint *)(iVar1 + 8);
      if (in_ECX == 0xffffffff) {
        return uVar2;
      }
    }
    uVar2 = 1;
  }
  return uVar2;
}
#endif
