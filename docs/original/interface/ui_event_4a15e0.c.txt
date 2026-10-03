// ui_event_4a15e0  (not a Ghidra function; ui_event_function_table[82])
// address 0x4a15e0, size 100 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692918 (index 82); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a15e0.
// WRITTEN 2026-09-28 from objdump 0x4a15e0..0x4a1643: the same search as 0x4a1570, committing the staged message
//   with AX = 0; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_client_globals *network_client;
extern char network_player_entry_validate(void *entry); // 0x4de9f0, blam-cc: EAX -> entry
extern int32_t network_staged_message_commit(void *client, int16_t value); // 0x4da250, blam-cc: ECX client,
    // AX value (its C still takes only the client and ignores AX: OPEN, networking phase)

uint8_t ui_event_4a15e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *client = (uint8_t *)network_client; // still addressed by byte offset below
    int32_t i;

    if (client == 0) {
        return 1;
    }
    for (i = 0; i < 0x10; i++) {
        uint8_t *entry = client + 0xcb6 + i * 0x20;

        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == *(int16_t *)client &&
            (int16_t)(int8_t)entry[0x1d] == event[1]) {
            network_staged_message_commit(client, 0);
            return 1;
        }
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
