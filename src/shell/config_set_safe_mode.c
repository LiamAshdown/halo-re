// config_set_safe_mode  (Ghidra: no function created; the phase-4 types agent carved a
//   placeholder "missed_57d230" from the config-property-table evidence)
// address 0x57d230, size 11 bytes
// name confidence 0.7, rewrite confidence 0.9
// evidence: out/phase4/shell_types_notes.md: "The remaining 22 setters (0x57d0c0,
//   0x57d110..0x57d230, 0x57d250 UMA, 0x57d2a0) are not Ghidra functions"; types/shell.h names
//   0x00722b60 config_safe_mode ("SafeMode"). Same bare-flag shape as
//   config_set_disable_driver_management.c.
// register convention: config.txt property setter; the value string parameter is present for
//   table-shape consistency but unread (this setter is a bare presence flag).
// blam-cc: (value on the stack, unused).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t config_safe_mode; // 0x00722b60

// config.txt "SafeMode" setter: presence alone sets the flag.
uint8_t config_set_safe_mode(const char *value)
{
    (void)value;
    config_safe_mode = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x57d230):

void missed_57d230(void)

{
  DAT_00722b60 = 1;
  return;
}
#endif
