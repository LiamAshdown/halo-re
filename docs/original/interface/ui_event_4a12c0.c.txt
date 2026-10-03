// ui_event_4a12c0  (not a Ghidra function; ui_event_function_table[77])
// address 0x4a12c0, size 35 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692904 (index 77); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a12c0.
// WRITTEN 2026-09-28 from objdump 0x4a12c0..0x4a12e2: when the cached profile slot (0x00692ac8) is a profile entry
//   (low nibble 1), deletes that saved game (unless -1) and returns 1; otherwise 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern uint8_t saved_game_delete_by_handle(int32_t handle); // 0x53c960, blam-cc: EDI handle

uint8_t ui_event_4a12c0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t handle = profile_slot_lookup_cache_00692ac8;

    if ((handle & 0xf) != 1) {
        return 0;
    }
    if (handle != -1) {
        saved_game_delete_by_handle(handle);
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
