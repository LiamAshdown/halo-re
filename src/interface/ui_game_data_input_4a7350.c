// ui_game_data_input_4a7350  (not a Ghidra function; game_data_input_function_table[40])
// address 0x4a7350, size 12 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692bb8 (index 40); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a7350.
// WRITTEN 2026-09-28 from objdump 0x4a7350..0x4a735b: scale (+0x24) = 1.0.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

void ui_game_data_input_4a7350(widget_instance *widget)
{
    widget->scale = 1.0f;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
