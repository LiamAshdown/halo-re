// ui_game_data_input_4a7280  (not a Ghidra function; game_data_input_function_table[35])
// address 0x4a7280, size 95 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692ba4 (index 35); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a7280.
// WRITTEN 2026-09-28 from objdump 0x4a7280..0x4a72de: for a selected variant, its game type (0x00714eb0 = working
//   copy +0x30) 1..5 sets the selection (+0x40) to 3..7; anything else 8.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself

void ui_game_data_input_4a7280(widget_instance *widget)
{
    int32_t type;

    if ((selected_saved_item & 0xf) != 1) {
        return;
    }
    type = *(int32_t *)(saved_item_working_copy + 0x30);
    widget->selection_index = (int16_t)(type >= 1 && type <= 5 ? type + 2 : 8);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
