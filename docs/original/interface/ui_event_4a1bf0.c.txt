// ui_event_4a1bf0  (not a Ghidra function; ui_event_function_table[100])
// address 0x4a1bf0, size 53 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692960 (index 100); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1bf0.
// WRITTEN 2026-09-28 from objdump 0x4a1bf0..0x4a1c24: when the main menu music is pending (exactly 1), stops the
//   looping sound sound\\music\\title1\\title1 (tag_lookup lsnd, path at 0x0066a044) if it is loaded and clears the
//   flag; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t main_menu_music_pending; // 0x00718fc6
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern void sound_looping_stop(datum_index looping_definition); // 0x544120, blam-cc: EAX

uint8_t ui_event_4a1bf0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    if (main_menu_music_pending == 1) {
        datum_index music = tag_lookup(0x6c736e64, (char *)"sound\\music\\title1\\title1"); // 'lsnd', string at 0x0066a044

        if (music != 0xffffffff) {
            sound_looping_stop(music);
        }
        main_menu_music_pending = 0;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
