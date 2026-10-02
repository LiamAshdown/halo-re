// ui_game_data_input_4a7340  (not a Ghidra function; game_data_input_function_table[39])
// address 0x4a7340, size 15 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692bb4 (index 39); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a7340.
// WRITTEN 2026-09-28 from objdump 0x4a7340..0x4a734e: zeroes the state (+0x10) of the widget's second child.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

void ui_game_data_input_4a7340(widget_instance *widget)
{
    widget->first_child->next_sibling->state = 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
