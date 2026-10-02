// ui_event_49e210  (not a Ghidra function; ui_event_function_table[39])
// address 0x49e210, size 16 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x0069286c (index 39); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49e210.
// WRITTEN 2026-09-28 from objdump 0x49e210..0x49e21f: forgets the cached profile slot (0x00692ac8) and the selected
//   saved item (both -1); returns 1.
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
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant

uint8_t ui_event_49e210(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    profile_slot_lookup_cache_00692ac8 = -1;
    selected_saved_item = -1;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
