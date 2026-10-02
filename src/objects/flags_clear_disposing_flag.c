// flags_clear_disposing_flag
// address 0x4fb510, size 10 bytes
// name confidence: 0.5 (still FUN_004fb510 in Ghidra; functions.md: "Clears the flag
//   subsystem's disposing flag", the paired setter to flags_dispose's `flag_data->valid = 1`)
// rewrite confidence: 0.8
// evidence: types/memory.h data_array.valid at 0x24; types/objects.h globals list (flag_data
//   0x008603a8).
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *flag_data; // 0x008603a8

void flags_clear_disposing_flag(void)
{
    flag_data->valid = 0;
}

#if 0
Original Ghidra decompilation (0x4fb510):

void FUN_004fb510(void)

{
  *(undefined1 *)(DAT_008603a8 + 0x24) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
