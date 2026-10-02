// hs_inspect_string  (not a Ghidra function; entry type 9 (string) of the hs type inspector table 0x68bb48 that
//   hs_evaluate_inspect formats a result with; no C existed, so inspecting such a value trapped as unlisted_489b30)
// address 0x489b30, size 24 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x489b30..: sprintf(buffer, "%s", (char *)value).
// blam-cc: stack -> type, value, buffer (cdecl)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


void hs_inspect_string(int16_t type, int32_t value, char *buffer)
{
    sprintf(buffer, "%s", (char *)value);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
