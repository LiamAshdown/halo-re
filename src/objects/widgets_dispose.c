// widgets_dispose
// address 0x4ffa10, size 46 bytes
// name confidence: 0.85 (out/phase4/objects_functions.md)
// rewrite confidence: 0.85
// evidence: same shape as widgets_initialize.c, walking widget_type_definition.dispose (0x0c)
//   instead of .initialize; types/memory.h data_array.valid at 0x24.
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

extern void data_delete_all(data_array *array); // UNSURE: zero visible args at the call site,
    // as in lights_dispose_all.c; memory module, 0x4d0580

void widgets_dispose(void)
{
    int32_t i;

    widget_data->valid = 1;
    data_delete_all(widget_data); // UNSURE: Ghidra shows no visible arguments

    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].dispose != 0) {
            ((void (*)(void))widget_type_definitions[i].dispose)();
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ffa10):

void widgets_dispose(void)

{
  undefined **ppuVar1;
  int iVar2;

  *(undefined1 *)(DAT_00860398 + 0x24) = 1;
  data_delete_all();
  ppuVar1 = &PTR_flags_dispose_0069c01c;
  iVar2 = 5;
  do {
    if ((code *)*ppuVar1 != (code *)0x0) {
      (*(code *)*ppuVar1)();
    }
    ppuVar1 = ppuVar1 + 10;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
