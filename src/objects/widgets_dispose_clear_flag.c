// widgets_dispose_clear_flag
// address 0x4ffa50, size 42 bytes
// name confidence: 0.7 (out/phase4/objects_functions.md: "Clears the disposing flag for every
//   widget type and then for the widget subsystem itself")
// rewrite confidence: 0.8
// evidence: same broadcast shape as widgets_initialize.c / widgets_dispose.c, walking
//   widget_type_definition.dispose_clear_flag (0x10); types/memory.h data_array.valid at 0x24.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *widget_data; // 0x00860398
extern widget_type_definition widget_type_definitions[k_maximum_widget_types]; // 0x0069c010

void widgets_dispose_clear_flag(void)
{
    int32_t i;

    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].dispose_clear_flag != 0) {
            ((void (*)(void))widget_type_definitions[i].dispose_clear_flag)();
        }
    }
    widget_data->valid = 0;
}

#if 0
Original Ghidra decompilation (0x4ffa50):

void widgets_dispose_clear_flag(void)

{
  undefined **ppuVar1;
  int iVar2;

  ppuVar1 = &PTR_FUN_0069c020;
  iVar2 = 5;
  do {
    if ((code *)*ppuVar1 != (code *)0x0) {
      (*(code *)*ppuVar1)();
    }
    ppuVar1 = ppuVar1 + 10;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  *(undefined1 *)(DAT_00860398 + 0x24) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
