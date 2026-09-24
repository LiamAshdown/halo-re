// widgets_update_all
// address 0x4ffd10, size 39 bytes
// name confidence: 0.85 (out/phase4/objects_functions.md: "Advances every concrete widget
//   subsystem (antennas, flags, and others) by one timestep")
// rewrite confidence: 0.8
// evidence: same broadcast shape as widgets_initialize.c, walking widget_type_definition.update
//   (0x20) and forwarding the single dt argument to each.
// register convention: one stack parameter (dt), forwarded unchanged to each callback.
// blam-cc: stack -> dt

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern widget_type_definition widget_type_definitions[k_maximum_widget_types]; // 0x0069c010

void widgets_update_all(float dt) // blam-cc: stack -> dt
{
    int32_t i;

    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].update != 0) {
            ((void (*)(float))widget_type_definitions[i].update)(dt);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ffd10):

void widgets_update_all(undefined4 param_1)

{
  undefined **ppuVar1;
  int iVar2;

  ppuVar1 = &PTR_flags_update_0069c030;
  iVar2 = 5;
  do {
    if ((code *)*ppuVar1 != (code *)0x0) {
      (*(code *)*ppuVar1)(param_1);
    }
    ppuVar1 = ppuVar1 + 10;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
