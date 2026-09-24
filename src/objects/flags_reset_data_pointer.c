// flags_reset_data_pointer
// address 0x4fb520, size 20 bytes
// name confidence: 0.45 (still FUN_004fb520 in Ghidra; functions.md: "Resets the flag
//   data-array pointer/handle if one is currently set")
// rewrite confidence: 0.75
// evidence: types/objects.h globals list (flag_data 0x008603a8); this is the one widget-type
//   callback (widget_type_definition.reset) that does not simply forward to objects_reset,
//   consistent with functions.md's summary.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *flag_data; // 0x008603a8

void flags_reset_data_pointer(void)
{
    if (flag_data != 0) {
        flag_data = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4fb520):

void FUN_004fb520(void)

{
  if (DAT_008603a8 != 0) {
    DAT_008603a8 = 0;
  }
  return;
}
#endif
