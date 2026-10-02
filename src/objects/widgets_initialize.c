// widgets_initialize
// address 0x4ff9d0, size 56 bytes
// name confidence: 0.9 (out/phase4/objects_functions.md, corroborated by the "widget" string
//   and by types/objects.h's own widget_type_definition table documentation)
// rewrite confidence: 0.85
// evidence: types/objects.h globals list (widget_data 0x00860398, widget_type_definitions
//   0x0069c010, k_maximum_widgets 0x40), widget_type_definition (initialize field at 0x08).
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

extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count

void widgets_initialize(void)
{
    int32_t i;

    widget_data = game_state_new((char *)"widget", k_maximum_widgets, 0xc /* EBX at the original call */);
    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].initialize != 0) {
            ((void (*)(void))widget_type_definitions[i].initialize)();
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ff9d0):

void widgets_initialize(void)

{
  undefined **ppuVar1;
  int iVar2;

  DAT_00860398 = game_state_new("widget",0x40);
  ppuVar1 = &PTR_flags_initialize_0069c018;
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
