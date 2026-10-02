// ui_event_4a1280  (not a Ghidra function; ui_event_function_table[76])
// address 0x4a1280, size 64 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692900 (index 76); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1280.
// WRITTEN 2026-09-28 from objdump 0x4a1280..0x4a12bf: for a cached profile slot without bit 30 and with low nibble
//   0: deletes that saved game (unless -1); when the cached slot is the current profile handle (0x00714dd4, read
//   before the delete) picks a profile automatically; returns 1. Otherwise 0.
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
extern int32_t saved_player_profile_slots_handle; // 0x00714dd4
extern uint8_t saved_game_delete_by_handle(int32_t handle); // 0x53c960, blam-cc: EDI handle
extern void player_profile_auto_select(void); // 0x4952c0

uint8_t ui_event_4a1280(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t handle = profile_slot_lookup_cache_00692ac8;
    int32_t current;

    if ((handle & 0x40000000) != 0 || (handle & 0xf) != 0) {
        return 0;
    }
    current = saved_player_profile_slots_handle;
    if (handle != -1) {
        saved_game_delete_by_handle(handle);
        handle = profile_slot_lookup_cache_00692ac8;
    }
    if (handle == current) {
        player_profile_auto_select();
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
