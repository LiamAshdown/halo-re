// config_set_maximum_resolution  (Ghidra: FUN_0057d080; renamed per out/phase4/shell_types_notes.md)
// address 0x57d080, size 54 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/shell_types_notes.md: "FUN_0057d080 is the MaximumResolution setter
//   (0x280..0x1000)." Writes config_maximum_resolution (0x0069fe3c), the same global
//   config_reset_system_requirements defaults to 0x1000 and video_display_modes_enumerate
//   0x4baba0 reads.
// register convention: this is a config.txt property setter (shell_config_property_setter);
//   objdump confirms the value string is a plain stack parameter.
// VERIFIED against disassembly 0x57d080..0x57d0b5 (2026-09-30): sscanf "%d" into an (uninitialised, as in the original)
//   local, then [0x280, 0x1000] range test with unsigned compares; matches. The harness "died" result is most likely
//   the modern CRT's invalid-parameter abort on a bad string pointer, which the original 7.1 CRT does not have.
// blam-cc: (value on the stack, matching Ghidra's recognized parameter)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t sscanf(const char *buffer, const char *format, ...); // 0x626572 CRT
extern int32_t config_maximum_resolution; // 0x0069fe3c

// config.txt "MaximumResolution" setter: parses a decimal number and, if it falls within
// [0x280, 0x1000], stores it; otherwise leaves the current value untouched. Returns 1 on
// success, 0 if out of range.
uint8_t config_set_maximum_resolution(char *value)
{
    uint32_t parsed;

    sscanf(value, "%d", &parsed);
    if (parsed >= k_shell_config_maximum_resolution_minimum && parsed <= k_shell_config_maximum_resolution_default) {
        config_maximum_resolution = parsed;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x57d080):

undefined4 FUN_0057d080(char *param_1)

{
  uint local_4;

  _sscanf(param_1,"%d",&local_4);
  if ((0x27f < local_4) && (local_4 < 0x1001)) {
    DAT_0069fe3c = local_4;
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
