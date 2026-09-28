// config_set_invalid_driver  (not a Ghidra function; the "InvalidDriver" entry of the config.txt property table at 0x0069fe40,
//   setter slot 0x0069fec4; no C existed, so the stored pointer trapped as unlisted_57d210)
// address 0x57d210, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x57d210..0x57d21a: mov eax, 1 / mov [0x00722b4c], eax / ret -- a bare presence
//   flag like config_set_safe_mode; the value string is not read and EAX = 1 comes back.
// blam-cc: (value on the stack, unused).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t config_invalid_driver; // 0x00722b4c

// config.txt "InvalidDriver" setter: presence alone sets the flag.
uint8_t config_set_invalid_driver(const char *value)
{
    (void)value;
    config_invalid_driver = 1;
    return 1;
}
