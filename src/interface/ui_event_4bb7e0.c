// ui_event_4bb7e0  (not a Ghidra function; ui_event_function_table[145])
// address 0x4bb7e0, size 393 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a14 (index 145); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4bb7e0.
// WRITTEN 2026-09-28 from objdump 0x4bb7e0..0x4bb968: for a selected profile: builds a display mode from its words
//   +0xa68 / +0xa6a / +0xa6c and (byte +0xa6f != 0), reads the current mode into 0x006b7010, and with the master gain
//   dropped to 0.05, when the mode differs rebuilds the present parameters, resets the device, reads the display mode
//   (device vtable +0x20) into 0x007c11f0, resizes the game window and clears the reset request (changed); restores
//   the gain and stamps the widget's creation time with the performance counter in ms (inlined). 0x007196d2 is
//   cleared. Changed: 0x006894ba = 0 while forcing single ticks, else (+0xa6f == 2); returns 1. Unchanged: sets
//   0x007196d2, closes the root widget (auto close 1 ms, fade 0, state 0), returns 0. Without a profile: closes the
//   root widget and returns 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "rasterizer.h"

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
extern uint8_t rasterizer_display_mode_differs(rasterizer_display_mode *requested); // 0x515d10, blam-cc: EDI
extern void rasterizer_build_present_parameters(d3d_present_parameters *dest, rasterizer_display_mode *source); // 0x515fc0, blam-cc: EAX source
extern uint8_t rasterizer_device_reset(d3d_present_parameters *present_parameters); // 0x515d90
extern void rasterizer_resize_game_window(int32_t height, int32_t width); // 0x515b20, blam-cc: EAX height, ECX width
extern void display_mode_get_current(rasterizer_display_mode *out); // 0x515ca0, blam-cc: EDI out
extern uint32_t time_query_performance_counter_ms(void); // 0x449210
extern uint8_t ui_flag_007196d2; // 0x007196d2, UNSURE (only ever set to 1 here)

uint8_t ui_event_4bb7e0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *root;
    int32_t changed = -1;

    if ((selected_saved_item & 0xf) == 0) {
        uint8_t *profile = saved_item_working_copy;
        float gain = sound_master_gain;
        rasterizer_display_mode mode;

        mode.width = *(int16_t *)(profile + 0xa68);
        mode.height = *(int16_t *)(profile + 0xa6a);
        mode.refresh_rate = *(int16_t *)(profile + 0xa6c);
        mode.vsync = (uint8_t)(profile[0xa6f] != 0);
        display_mode_get_current(&ui_video_requested_display_mode_006b7010);
        sound_set_master_gain(0.05f);
        changed = 0;
        if (rasterizer_display_mode_differs(&mode) != 0) {
            d3d_present_parameters parameters;

            rasterizer_build_present_parameters(&parameters, &mode);
            rasterizer_device_reset(&parameters);
            ((int32_t (__stdcall *)(void *, uint32_t, void *))(*(void ***)rasterizer_device)[0x20 / 4])(rasterizer_device, 0,
                &rasterizer_desktop_display_mode);
            changed = 1;
            rasterizer_resize_game_window(mode.height, mode.width);
            rasterizer_needs_reset = 0;
        }
        sound_set_master_gain(gain);
        widget->creation_time = (int32_t)time_query_performance_counter_ms();
    }
    ui_flag_007196d2 = 0;
    if (changed == 1) {
        if (game_time_force_single_tick != 0) {
            unknown_006894ba = 0;
        } else {
            unknown_006894ba = (uint8_t)(saved_item_working_copy[0xa6f] == 2);
        }
        return 1;
    }
    if (changed == 0) {
        ui_flag_007196d2 = 1;
    }
    root = widget;
    while (root->parent != 0) {
        root = root->parent;
    }
    root->milliseconds_to_auto_close = 1;
    root->milliseconds_auto_close_fade = 0;
    root->state = 0;
    return 0;
}
