// config_set_disable_driver_management  (Ghidra: config_set_disable_driver_management, already
//   named)
// address 0x57d240, size 11 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Config-key setter
//   callback that sets the 'DisableDriverManagement' flag to true." Writes
//   config_disable_driver_management (0x00722b38), the same global config_reset_system_
//   requirements defaults to 0 and shell_parse_config_txt forces to 1 in safe mode.
// register convention: plain __cdecl config.txt property setter; the value string argument is
//   unused (this setter is a bare boolean flag).
// blam-cc: (value on the stack, ignored)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t config_disable_driver_management; // 0x00722b38

// config.txt "DisableDriverManagement" setter: always succeeds and sets the flag.
uint8_t config_set_disable_driver_management(const char *value)
{
    (void)value;
    config_disable_driver_management = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x57d240):

void __cdecl config_set_disable_driver_management(void)

{
  DAT_00722b38 = 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
