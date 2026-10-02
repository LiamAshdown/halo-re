// antennas_clear_disposing_flag  (Ghidra: FUN_004faa60; renamed, Blam-style, not previously
// named)
// address 0x4faa60, size 10 bytes
// name confidence: 0.6 (matches functions.md's summary: "Clears the antenna subsystem's
//   disposing flag")
// rewrite confidence: 0.85
// evidence: types/memory.h data_array (valid 0x24); global 0x008603ac antenna_data.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *antenna_data; // 0x008603ac

void antennas_clear_disposing_flag(void)
{
    antenna_data->valid = 0;
}

#if 0
Original Ghidra decompilation (0x4faa60):

void FUN_004faa60(void)

{
  *(undefined1 *)(DAT_008603ac + 0x24) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
