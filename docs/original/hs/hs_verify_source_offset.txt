// hs_verify_source_offset  (Ghidra: hs_verify_source_offset, already named)
// address 0x4858a0, size 27 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: bounds-checks against hs_compiled_source_length, matching the CEA string hint and
// its sole use in hs_compile_postprocess.
// register convention: the offset argument is unrecognized by Ghidra (in_ECX); by the blam-cc
// convention this is the second register slot, ECX.

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t hs_compiled_source_length; // 0x006b14bc
extern char *hs_compile_error;            // 0x006b14d4

// blam-cc: offset in ECX
// Validates that `offset` still falls within [0, hs_compiled_source_length); flags a
// recompile-needed compile error otherwise.
char hs_verify_source_offset(int32_t offset)
{
    char valid;

    valid = 1;
    if ((offset < 0) || (hs_compiled_source_length <= offset)) {
        hs_compile_error = (char *)"bad source offset (you need to recompile.)";
        valid = 0;
    }
    return valid;
}

#if 0
Original Ghidra decompilation (0x4858a0):

undefined1 hs_verify_source_offset(void)

{
  undefined1 uVar1;
  int in_ECX;

  uVar1 = 1;
  if ((in_ECX < 0) || (DAT_006b14bc <= in_ECX)) {
    DAT_006b14d4 = "bad source offset (you need to recompile.)";
    uVar1 = 0;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
