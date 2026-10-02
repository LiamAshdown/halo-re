// config_set_linear_texture_addressing_sun  (not a Ghidra function; the "LinearTextureAddressingSun" entry of the config.txt property table at 0x0069fe40,
//   setter slot 0x0069fe64; no C existed, so the stored pointer trapped as unlisted_57d160)
// address 0x57d160, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x57d160..0x57d16a: mov eax, 1 / mov [0x00722b30], eax / ret -- a bare presence
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

extern int32_t config_linear_texture_addressing_sun; // 0x00722b30

// config.txt "LinearTextureAddressingSun" setter: presence alone sets the flag.
uint8_t config_set_linear_texture_addressing_sun(const char *value)
{
    (void)value;
    config_linear_texture_addressing_sun = 1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
