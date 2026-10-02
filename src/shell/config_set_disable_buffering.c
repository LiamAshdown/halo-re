// config_set_disable_buffering  (Ghidra: no function created; the phase-4 types agent carved the stub name "missed_57d110" from the config-property-table evidence)
// address 0x57d110, size 11 bytes
// name confidence 0.7, rewrite confidence 0.9
// evidence: out/phase4/shell_types_notes.md: "The remaining 22 setters (0x57d0c0,
//   0x57d110..0x57d230, 0x57d250 UMA, 0x57d2a0) are not Ghidra functions"; types/shell.h names
//   0x00722b54 config_disable_buffering ("DisableBuffering (forced in safe mode)"). A flag setter
//   of the same one-instruction shape as every other config_set_*.c in this module (see
//   config_set_disable_driver_management.c).
// register convention: config.txt property setter; the value string parameter is present for
//   table-shape consistency but unread (this setter is a bare presence flag).
// blam-cc: (value on the stack, unused).

// VERIFIED against disassembly 0x57d110..0x57d11b (2026-09-30): single flag store, returns 1
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t config_disable_buffering; // 0x00722b54

// config.txt "DisableBuffering" setter: presence alone sets the flag.
uint8_t config_set_disable_buffering(const char *value)
{
    (void)value;
    config_disable_buffering = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x57d110):

void missed_57d110(void)

{
  DAT_00722b54 = 1;
  return;
}
#endif
