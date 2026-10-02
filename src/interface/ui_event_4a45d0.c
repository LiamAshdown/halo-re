// ui_event_4a45d0  (not a Ghidra function; ui_event_function_table[189])
// address 0x4a45d0, size 24 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692ac4 (index 189); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a45d0.
// WRITTEN 2026-09-28 from objdump 0x4a45d0..0x4a45e7: with a pending delete name (0x00718fd0 non-empty), deletes
//   those saved game files; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char pending_delete_saved_game_name_00718fd0[]; // 0x00718fd0, UNSURE name
extern uint8_t saved_game_delete_files(char *name); // 0x5388c0, blam-cc: EDI name

uint8_t ui_event_4a45d0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (pending_delete_saved_game_name_00718fd0[0] != 0) {
        saved_game_delete_files(pending_delete_saved_game_name_00718fd0);
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
