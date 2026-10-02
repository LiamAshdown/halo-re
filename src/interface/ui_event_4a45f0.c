// ui_event_4a45f0  (not a Ghidra function; ui_event_function_table[178])
// address 0x4a45f0, size 83 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a98 (index 178); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a45f0.
// WRITTEN 2026-09-28 from objdump 0x4a45f0..0x4a4642: formats checkpoints\\<name> from the data (+0x48) of the ui
//   list item at the widget's committed selection into a local and returns the checkpoint load result (0x5391a0).
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
extern uint8_t saved_game_load_checkpoint_by_name(char *name); // 0x5391a0

static void *list_item_data(int16_t index)
{
    if (index >= 0 && index < ui_lists[ui_list_current].count) {
        return ((ui_list_item *)ui_lists[ui_list_current].data)[index].data;
    }
    return 0;
}

uint8_t ui_event_4a45f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *data = (uint8_t *)list_item_data(*(int16_t *)&((struct widget_instance *)widget)->text);
    char name[0x40];

    sprintf(name, "checkpoints\\%s", (char *)(data + 0x48)); // format at 0x0066a4dc
    return saved_game_load_checkpoint_by_name(name);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
