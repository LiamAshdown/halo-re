// config_set_invalid_sound_driver  (not a Ghidra function; the "InvalidSoundDriver" entry of the config.txt property table at 0x0069fe40,
//   setter slot 0x0069fecc; no C existed, so the stored pointer trapped as unlisted_57d220)
// address 0x57d220, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x57d220..0x57d22a: mov eax, 1 / mov [0x00722b50], eax / ret -- a bare presence
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

extern int32_t config_invalid_sound_driver; // 0x00722b50

// config.txt "InvalidSoundDriver" setter: presence alone sets the flag.
uint8_t config_set_invalid_sound_driver(const char *value)
{
    (void)value;
    config_invalid_sound_driver = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
