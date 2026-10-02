// ui_event_4a2f10  (not a Ghidra function; ui_event_function_table[144])
// address 0x4a2f10, size 225 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a10 (index 144); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a2f10.
// WRITTEN 2026-09-28 from objdump 0x4a2f10..0x4a2ff0: with a current profile handle (0x00714dd4 != -1): selects it,
//   copies the host name (0x00719170) and subname (0x007191f0) into the profile (+0xd8c / +0xeac; the binary also
//   calls wcslen on each, unused), when not saving copies 0x00719204 into +0xebf, when 0x006894a2 is set stores the
//   quality selection clamped to 0..4 in +0xfc0, then saves when changed or else drops the selection. While saving
//   (0x00719010) returns 0; otherwise starts a new server with that name and password and returns the high byte of
//   the entry ECX, which the dispatchers (0x497c9a, 0x49a4be) load with the table index below 0xbe: always 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t saved_player_profile_slots_handle; // 0x00714dd4
extern void saved_item_select(int32_t item); // 0x495be0, blam-cc: EBX -> item
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern uint16_t network_host_name_00719170[0x40]; // 0x00719170
extern uint16_t network_host_subname_007191f0[9]; // 0x007191f0
extern uint8_t save_in_progress_00719010; // 0x00719010
extern int32_t resolution_selection_00719204; // 0x00719204
extern uint8_t network_game_info_packet_flag; // 0x006894a2
extern int32_t quality_selection_00692b04; // 0x00692b04
extern uint8_t saved_item_has_unsaved_changes(void); // 0x495ea0
extern uint8_t player_profile_save(void); // 0x495d40
extern uint8_t network_game_start_new_server_with_name_and_password(uint32_t unused, uint16_t *name, uint16_t *password); // 0x4e4150

uint8_t ui_event_4a2f10(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int32_t handle = saved_player_profile_slots_handle;

    if (handle != -1) {
        uint8_t *profile;

        saved_item_select(handle);
        profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
        wcscpy((wchar_t *)(profile + 0xd8c), (const wchar_t *)network_host_name_00719170);
        wcscpy((wchar_t *)(profile + 0xeac), (const wchar_t *)network_host_subname_007191f0);
        if (save_in_progress_00719010 == 0) {
            profile[0xebf] = (uint8_t)resolution_selection_00719204;
        }
        if (network_game_info_packet_flag != 0) {
            int32_t quality = quality_selection_00692b04;

            profile[0xfc0] = (uint8_t)(quality < 0 ? 0 : quality > 4 ? 4 : quality);
        }
        if (saved_item_has_unsaved_changes() != 0) {
            player_profile_save();
        } else {
            selected_saved_item = -1;
        }
    }
    if (save_in_progress_00719010 != 0) {
        return 0;
    }
    network_game_start_new_server_with_name_and_password(0, network_host_name_00719170, network_host_subname_007191f0);
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
