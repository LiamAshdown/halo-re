// ui_event_4a3000  (not a Ghidra function; ui_event_function_table[155])
// address 0x4a3000, size 68 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a3c (index 155); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3000.
// WRITTEN 2026-09-28 from objdump 0x4a3000..0x4a3043: the second child is shown at scale 1.0 when network_game_mode
//   is 2 or the game engine has teams, else hidden at scale 0x3eaa7efa (about 1/3); returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t network_game_mode; // 0x00719720
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc

uint8_t ui_event_4a3000(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *child = widget->first_child->next_sibling;

    if (network_game_mode == 2 || (current_game_engine != 0 && game_engine_teams_enabled_flag != 0)) {
        child->hidden = 0;
        child->scale = 1.0f;
    } else {
        child->hidden = 1;
        *(uint32_t *)&child->scale = 0x3eaa7efa;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
