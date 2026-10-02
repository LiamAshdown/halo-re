// config_set_old_driver  (not a Ghidra function; the "OldDriver" entry of the config.txt property table at 0x0069fe40,
//   setter slot 0x0069fea4; no C existed, so the stored pointer trapped as unlisted_57d1d0)
// address 0x57d1d0, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x57d1d0..0x57d1da: mov eax, 1 / mov [0x00722b44], eax / ret -- a bare presence
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

extern int32_t config_old_driver; // 0x00722b44

// config.txt "OldDriver" setter: presence alone sets the flag.
uint8_t config_set_old_driver(const char *value)
{
    (void)value;
    config_old_driver = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
