// ui_event_4a12f0  (not a Ghidra function; ui_event_function_table[78])
// address 0x4a12f0, size 20 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692908 (index 78); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a12f0.
// WRITTEN 2026-09-28 from objdump 0x4a12f0..0x4a1303: forgets the cached profile slot (-1) and the pending delete
//   name (first byte of 0x00718fd0); returns 1.
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
extern char pending_delete_saved_game_name_00718fd0[]; // 0x00718fd0, UNSURE name

uint8_t ui_event_4a12f0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    profile_slot_lookup_cache_00692ac8 = -1;
    pending_delete_saved_game_name_00718fd0[0] = 0;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
