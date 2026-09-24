// datum_index_invalidate  (Ghidra: FUN_004d02c0)
// address 0x4d02c0, size 11 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: out/phase4/memory_functions.md summary "Sets a 32-bit handle/index field to the
// invalid sentinel value 0xFFFFFFFF."; matches types/memory.h k_datum_index_none.
// register convention: destination pointer as the recognized parameter (param_1); no other
// registers used.

#include "tags.h"
#include "memory.h"

// blam-cc: destination as the recognized parameter
// Sets *out_index to the invalid datum_index sentinel (k_datum_index_none).
void datum_index_invalidate(datum_index *out_index)
{
    *out_index = k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x4d02c0):

void FUN_004d02c0(undefined4 *param_1)

{
  *param_1 = 0xffffffff;
  return;
}
#endif
