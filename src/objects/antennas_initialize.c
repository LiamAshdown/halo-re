// antennas_initialize  (Ghidra: antennas_initialize, already named)
// address 0x4faa20, size 28 bytes
// name confidence: 0.65 (already carries this name; matches functions.md's summary:
//   "Creates the global antenna instance data array (data_new(\"antenna\", 12))")
// rewrite confidence: 0.85
// evidence: types/objects.h k_maximum_antennas (12), global 0x008603ac antenna_data.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *antenna_data; // 0x008603ac

extern data_array *game_state_new(char *name, int32_t maximum_count); // memory module, 0x5380d0

void antennas_initialize(void)
{
    antenna_data = game_state_new("antenna", k_maximum_antennas);
}

#if 0
Original Ghidra decompilation (0x4faa20):

void antennas_initialize(void)

{
  DAT_008603ac = game_state_new("antenna",0xc);
  return;
}
#endif
