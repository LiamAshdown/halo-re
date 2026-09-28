// config_set_use_alternate_convolve_mask  (not a Ghidra function; the "UseAlternateConvolveMask" entry of the config.txt property table at 0x0069fe40,
//   setter slot 0x0069fe9c; no C existed, so the stored pointer trapped as unlisted_57d1a0)
// address 0x57d1a0, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x57d1a0..0x57d1aa: mov eax, 1 / mov [0x00722b78], eax / ret -- a bare presence
//   flag like config_set_safe_mode; the value string is not read and EAX = 1 comes back.
// blam-cc: (value on the stack, unused).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t config_use_alternate_convolve_mask; // 0x00722b78

// config.txt "UseAlternateConvolveMask" setter: presence alone sets the flag.
uint8_t config_set_use_alternate_convolve_mask(const char *value)
{
    (void)value;
    config_use_alternate_convolve_mask = 1;
    return 1;
}
