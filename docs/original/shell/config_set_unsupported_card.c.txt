// config_set_unsupported_card  (not a Ghidra function; the "UnsupportedCard" entry of the config.txt property table at 0x0069fe40,
//   setter slot 0x0069fe7c; no C existed, so the stored pointer trapped as unlisted_57d1c0)
// address 0x57d1c0, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x57d1c0..0x57d1ca: mov eax, 1 / mov [0x00722b3c], eax / ret -- a bare presence
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
extern int32_t config_unsupported_card; // 0x00722b3c

// config.txt "UnsupportedCard" setter: presence alone sets the flag.
uint8_t config_set_unsupported_card(const char *value)
{
    (void)value;
    config_unsupported_card = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
