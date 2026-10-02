// config_set_use_anisotropic_filter  (not a Ghidra function; the "UseAnisotropicFilter" entry of the config.txt property table at 0x0069fe40,
//   setter slot 0x0069fee4; no C existed, so the stored pointer trapped as unlisted_57d130)
// address 0x57d130, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x57d130..0x57d13a: mov eax, 1 / mov [0x00722b68], eax / ret -- a bare presence
//   flag like config_set_safe_mode; the value string is not read and EAX = 1 comes back.
// blam-cc: (value on the stack, unused).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t config_use_anisotropic_filter; // 0x00722b68

// config.txt "UseAnisotropicFilter" setter: presence alone sets the flag.
uint8_t config_set_use_anisotropic_filter(const char *value)
{
    (void)value;
    config_use_anisotropic_filter = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
