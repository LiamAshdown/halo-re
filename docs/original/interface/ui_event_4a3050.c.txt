// ui_event_4a3050  (not a Ghidra function; ui_event_function_table[156])
// address 0x4a3050, size 141 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a40 (index 156); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3050.
// WRITTEN 2026-09-28 from objdump 0x4a3050..0x4a30dc: the first child is shown at scale 1.0 when the game engine
//   has teams, else hidden at 0x3eaa7efa and its parent focuses its own second child; the next two children are shown
//   at 1.0 when network_game_mode is 2, else hidden at 0x3eaa7efa. Returns 1.
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
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int16_t network_game_mode; // 0x00719720

uint8_t ui_event_4a3050(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *child = widget->first_child;
    int32_t i;

    if (current_game_engine != 0 && game_engine_teams_enabled_flag != 0) {
        child->hidden = 0;
        child->scale = 1.0f;
    } else {
        child->hidden = 1;
        *(uint32_t *)&child->scale = 0x3eaa7efa;
        child->parent->focused_child = child->parent->first_child->next_sibling;
    }
    for (i = 0; i < 2; i++) {
        child = child->next_sibling;
        if (network_game_mode == 2) {
            child->hidden = 0;
            child->scale = 1.0f;
        } else {
            child->hidden = 1;
            *(uint32_t *)&child->scale = 0x3eaa7efa;
        }
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
