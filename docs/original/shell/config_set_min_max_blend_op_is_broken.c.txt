// config_set_min_max_blend_op_is_broken  (Ghidra: no function created; the phase-4 types agent
//   carved the stub name "missed_57d2a0" from the config-property-table evidence)
// address 0x57d2a0, size 11 bytes
// name confidence 0.7, rewrite confidence 0.9
// evidence: out/phase4/shell_types_notes.md: "The remaining 22 setters (0x57d0c0,
//   0x57d110..0x57d230, 0x57d250 UMA, 0x57d2a0) are not Ghidra functions"; types/shell.h names
//   0x00722b7c config_min_max_blend_op_is_broken ("MinMaxBlendOpIsBroken (forced in safe mode)").
//   Same bare-flag shape as config_set_disable_driver_management.c.
// register convention: config.txt property setter; the value string parameter is present for
//   table-shape consistency but unread (this setter is a bare presence flag).
// blam-cc: (value on the stack, unused).

// VERIFIED against disassembly 0x57d2a0..0x57d2ab (2026-09-30): single flag store, returns 1
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t config_min_max_blend_op_is_broken; // 0x00722b7c

// config.txt "MinMaxBlendOpIsBroken" setter: presence alone sets the flag.
uint8_t config_set_min_max_blend_op_is_broken(const char *value)
{
    (void)value;
    config_min_max_blend_op_is_broken = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x57d2a0):

void missed_57d2a0(void)

{
  DAT_00722b7c = 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
