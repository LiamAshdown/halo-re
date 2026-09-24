// antennas_reset_data_pointer  (Ghidra: FUN_004faa70; renamed, Blam-style, not previously
// named)
// address 0x4faa70, size 20 bytes
// name confidence: 0.5 (matches functions.md's summary: "Resets the antenna data-array pointer/
//   handle if one is currently set")
// rewrite confidence: 0.7
// evidence: global 0x008603ac antenna_data.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *antenna_data; // 0x008603ac

void antennas_reset_data_pointer(void)
{
    if (antenna_data != 0) {
        antenna_data = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4faa70):

void FUN_004faa70(void)

{
  if (DAT_008603ac != 0) {
    DAT_008603ac = 0;
  }
  return;
}
#endif
