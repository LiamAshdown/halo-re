// ui_event_4bb970  (not a Ghidra function; ui_event_function_table[146])
// address 0x4bb970, size 263 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a18 (index 146); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4bb970.
// WRITTEN 2026-09-28 from objdump 0x4bb970..0x4bba76: unless 0x007196d2 is set: stores the requested display mode's
//   width, height and refresh rate words in the selected profile (+0xa68 / +0xa6a / +0xa6c; the binary writes through
//   null without a profile), with vsync sets +0xa6f to 1 + (0x006894ba != 0); 0x006894ba becomes 0 while forcing
//   single ticks, else (+0xa6f == 2). With the master gain dropped to 0.05, when the mode differs from the current
//   one rebuilds the present parameters, resets the device, reads the display mode (device vtable +0x20) into
//   0x007c11f0, resizes the game window and clears the reset request; then restores the gain. Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern uint8_t ui_flag_007196d2; // 0x007196d2, UNSURE (only ever set to 1 here)
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern float sound_master_gain; // 0x007252ac
extern void sound_set_master_gain(float gain); // 0x548590
extern rasterizer_display_mode ui_video_requested_display_mode_006b7010; // 0x006b7010, UNSURE name (the video options screen's pick)
extern uint8_t unknown_006894ba; // 0x006894ba
extern int32_t game_time_force_single_tick; // 0x007196d8
extern d3d_display_mode rasterizer_desktop_display_mode; // 0x007c11f0
extern uint8_t rasterizer_needs_reset; // 0x0071d16d
extern void *rasterizer_device; // 0x0071d174

extern void rasterizer_build_present_parameters(d3d_present_parameters *dest, rasterizer_display_mode *source); // 0x515fc0, blam-cc: EAX source
extern uint8_t rasterizer_device_reset(d3d_present_parameters *present_parameters); // 0x515d90


uint8_t ui_event_4bb970(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    float gain;
    uint8_t *profile;

    if (ui_flag_007196d2 != 0) {
        return 1;
    }
    gain = sound_master_gain;
    profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    *(uint16_t *)(profile + 0xa68) = (uint16_t)ui_video_requested_display_mode_006b7010.width;
    *(uint16_t *)(profile + 0xa6c) = (uint16_t)ui_video_requested_display_mode_006b7010.refresh_rate;
    *(uint16_t *)(profile + 0xa6a) = (uint16_t)ui_video_requested_display_mode_006b7010.height;
    if (ui_video_requested_display_mode_006b7010.vsync != 0) {
        profile[0xa6f] = (uint8_t)((unknown_006894ba != 0) + 1);
    }
    if (game_time_force_single_tick != 0) {
        unknown_006894ba = 0;
    } else {
        unknown_006894ba = (uint8_t)(profile[0xa6f] == 2);
    }
    sound_set_master_gain(0.05f);
    if (rasterizer_display_mode_differs(&ui_video_requested_display_mode_006b7010) != 0) {
        d3d_present_parameters parameters;

        rasterizer_build_present_parameters(&parameters, &ui_video_requested_display_mode_006b7010);
        rasterizer_device_reset(&parameters);
        ((int32_t (__stdcall *)(void *, uint32_t, void *))(*(void ***)rasterizer_device)[0x20 / 4])(rasterizer_device, 0,
            &rasterizer_desktop_display_mode);
        rasterizer_resize_game_window(ui_video_requested_display_mode_006b7010.height, ui_video_requested_display_mode_006b7010.width);
        rasterizer_needs_reset = 0;
    }
    sound_set_master_gain(gain);
    return 1;
}
