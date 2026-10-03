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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *antenna_data; // 0x008603ac

extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count

void antennas_initialize(void)
{
    antenna_data = game_state_new((char *)"antenna", k_maximum_antennas, 0x2bc /* EBX at the original call */);
}

#if 0
Original Ghidra decompilation (0x4faa20):

void antennas_initialize(void)

{
  DAT_008603ac = game_state_new("antenna",0xc);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
