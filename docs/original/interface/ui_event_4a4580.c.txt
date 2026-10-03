// ui_event_4a4580  (not a Ghidra function; ui_event_function_table[188])
// address 0x4a4580, size 72 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692ac0 (index 188); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a4580.
// WRITTEN 2026-09-28 from objdump 0x4a4580..0x4a45c7: formats checkpoints\\<name> from the data (+0x48) of the ui
//   list item at the widget's committed selection into the pending delete name (0x00718fd0); returns 1. (Out of
//   range: the binary formats from address 0x48; so does this.)
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <stdio.h>
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t ui_list_current; // 0x00692c04
extern growable_array ui_lists[3]; // 0x006b3830, element size 0x10 (ui_list_item)
extern char pending_delete_saved_game_name_00718fd0[]; // 0x00718fd0, UNSURE name

static void *list_item_data(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].data;
    }
    return 0;
}

uint8_t ui_event_4a4580(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *data = (uint8_t *)list_item_data(*(int16_t *)&((struct widget_instance *)widget)->text);

    sprintf(pending_delete_saved_game_name_00718fd0, "checkpoints\\%s", (char *)(data + 0x48)); // format at 0x0066a4dc
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
