#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Assorted UI screen-level behaviour: cursor, error modal, pause check, colours and option
 * application.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiScreens {
    static void audio_options_apply_volume_sliders(widget_instance *widget);
    static void chat_window_reset_position(void);
    static uint32_t check_for_pause_game(void);
    static void cursor_update(void);
    static void error_modal_update(void);
    static ColorRGB * get_saved_color(ColorRGB *out);
    static ColorARGB * get_saved_pulse_color(ColorARGB *out);
    static void handler_4a68f0(uint8_t *widget);
};

}
