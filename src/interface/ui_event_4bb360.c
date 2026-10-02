// ui_event_4bb360  (not a Ghidra function; ui_event_function_table[125])
// address 0x4bb360, size 622 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x006929c4 (index 125); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4bb360.
// WRITTEN 2026-09-28 from objdump 0x4bb360..0x4bb5cd: for a selected profile (else returns 1), from the
//   grandparent's children's spinner lists: resolution (out of range -> 0) width / height words into +0xa68 / +0xa6a,
//   its refresh rate (out of range -> 0) into +0xa6c, vsync clamped 0..2 into +0xa6f, three flags (+0xa70..+0xa72 =
//   selection != 0), two more clamped 0..2 (+0xa73 / +0xa74), the gamma byte (0x00695464) into +0xa76. Returns
//   whether that mode differs from the current one; when it does not, closes the grandparent restoring the previous
//   widget. Either way plays sound 3 and sets 0x007196d1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern int32_t video_resolution_count; // 0x007196cc
extern video_resolution video_resolutions[0x20]; // 0x006b6690
extern int32_t video_gamma_setting; // 0x00695464, UNSURE name (its low byte is stored)
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
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0, blam-cc: EAX
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern uint8_t ui_flag_007196d1; // 0x007196d1, UNSURE (cleared by 0x4bb290, gates the gamma apply in 0x4bb300)

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

static uint8_t clamp_selection(widget_instance *group, int16_t maximum)
{
    int16_t selection = first_list_child(group)->selection_index;

    return (uint8_t)(selection < 0 ? 0 : selection > maximum ? maximum : selection);
}

uint8_t ui_event_4bb360(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    widget_instance *screen = widget->parent->parent;
    uint8_t result = 1;

    if (profile != 0) {
        widget_instance *group = screen->first_child;
        rasterizer_display_mode mode;
        int32_t resolution;
        int32_t refresh;

        resolution = first_list_child(group)->selection_index;
        if (resolution < 0 || resolution >= video_resolution_count) {
            resolution = 0;
        }
        *(uint16_t *)(profile + 0xa68) = (uint16_t)video_resolutions[resolution].width;
        *(uint16_t *)(profile + 0xa6a) = (uint16_t)video_resolutions[resolution].height;
        group = group->next_sibling;
        refresh = first_list_child(group)->selection_index;
        if (refresh < 0 || (uint32_t)refresh >= video_resolutions[resolution].refresh_rate_count) {
            refresh = 0;
        }
        *(uint16_t *)(profile + 0xa6c) = (uint16_t)video_resolutions[resolution].refresh_rates[refresh];
        group = group->next_sibling;
        profile[0xa6f] = clamp_selection(group, 2);
        group = group->next_sibling;
        profile[0xa70] = (uint8_t)(first_list_child(group)->selection_index != 0);
        group = group->next_sibling;
        profile[0xa71] = (uint8_t)(first_list_child(group)->selection_index != 0);
        group = group->next_sibling;
        profile[0xa72] = (uint8_t)(first_list_child(group)->selection_index != 0);
        group = group->next_sibling;
        profile[0xa73] = clamp_selection(group, 2);
        group = group->next_sibling;
        profile[0xa74] = clamp_selection(group, 2);
        profile[0xa76] = (uint8_t)video_gamma_setting;
        mode.width = *(int16_t *)(profile + 0xa68);
        mode.height = *(int16_t *)(profile + 0xa6a);
        mode.refresh_rate = *(int16_t *)(profile + 0xa6c);
        mode.vsync = (uint8_t)(profile[0xa6f] != 0);
        result = rasterizer_display_mode_differs(&mode);
        if (result == 0) {
            widget_instance_close_and_restore_previous(screen);
        }
    }
    widget_play_sound_effect(3);
    ui_flag_007196d1 = 1;
    return result;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
