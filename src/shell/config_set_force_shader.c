// config_set_force_shader  (Ghidra: no function created; the phase-4 types agent carved the stub name "missed_57d0c0" from the config-property-table evidence)
// address 0x57d0c0, size 77 bytes
// name confidence 0.7, rewrite confidence 0.85
// evidence: out/phase4/shell_types_notes.md: "The remaining 22 setters (0x57d0c0,
//   0x57d110..0x57d230, 0x57d250 UMA, 0x57d2a0) are not Ghidra functions"; types/shell.h names
//   0x00722b64 config_force_shader ("ForceShader \"%d\"; 9999 in safe mode"). Matches
//   config_set_decal_z_bias.c's established shape: a config.txt property-table setter, `uint8_t
//   (*)(const char *value)`.
// register convention: config.txt property setter; value string is a plain stack parameter
//   (matches every other entry in this module's config_properties table).
// blam-cc: (value on the stack, matching Ghidra's recognized parameter).
// VERIFIED against disassembly 0x57d0c0..0x57d10c (2026-09-30); fixed: parsed starts as the value pointer (the original
//   scans into its own argument slot).
// UNSURE: the "2a"/"2A" special case (config_force_shader = 0x270e = 9998) has no attested
//   meaning beyond "not the numeric ForceShader ID path"; kept exactly as decompiled.

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t sscanf(const char *buffer, const char *format, ...); // 0x626572 CRT
extern int32_t config_force_shader; // 0x00722b64

// config.txt "ForceShader" setter: "2a"/"2A" forces 9998 (0x270e); otherwise parses a decimal
// shader id, falling back to 9999 (0x270f) when sscanf produces zero (parse failure).
uint8_t config_set_force_shader(const char *value)
{
    // 0x57d0d3: sscanf's output pointer is `lea eax,[esp+4]`, i.e. the function's own `value` argument slot, so when
    // nothing is parsed the "result" is the low 32 bits of the value pointer (nonzero), not 0.
    int32_t parsed = (int32_t)(uintptr_t)value;

    if (*(const uint16_t *)value == 0x6132 || *(const uint16_t *)value == 0x4132) {
        config_force_shader = 0x270e;
    } else {
        sscanf(value, "%d", &parsed);
        config_force_shader = parsed;
        if (parsed == 0) {
            config_force_shader = 0x270f;
            return 1;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x57d0c0):

undefined4 missed_57d0c0(short *param_1)

{
  if ((*param_1 == 0x6132) || (*param_1 == 0x4132)) {
    DAT_00722b64 = (short *)0x270e;
  }
  else {
    _sscanf((char *)param_1,"%d",&param_1);
    DAT_00722b64 = param_1;
    if (param_1 == (short *)0x0) {
      DAT_00722b64 = (short *)0x270f;
      return 1;
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
