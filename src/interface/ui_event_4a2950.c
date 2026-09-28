// ui_event_4a2950  (not a Ghidra function; ui_event_function_table[134])
// address 0x4a2950, size 70 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006929e8 (index 134); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a2950.
// WRITTEN 2026-09-28 from objdump 0x4a2950..0x4a2995: fills a local 0x1ffc byte profile with the default audio
//   options; on success fills the grandparent as a controls input row from it and plays sound 2. Returns the fill
//   result. (The binary leaves the rest of the local uninitialized; zeroed here.)
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>

extern uint8_t player_profile_set_default_audio_options(void *profile); // 0x53b240, blam-cc: profile in EAX
extern void ui_controls_populate_input_row(widget_instance *widget, const uint8_t *profile_record); // 0x4a22e0, blam-cc: EAX widget, EDI profile_record
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id

uint8_t ui_event_4a2950(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t profile[0x1ffc];
    uint8_t ok;

    memset(profile, 0, sizeof(profile));
    ok = player_profile_set_default_audio_options(profile);
    if (ok != 0) {
        ui_controls_populate_input_row(widget->parent->parent, profile);
        widget_play_sound_effect(2);
    }
    return ok;
}
