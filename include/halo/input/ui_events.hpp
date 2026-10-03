#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::input {

/**
 * Menu navigation event generation and the four UI input event queues.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiEvents {
    static void menu_generate_events(void);
    static void queue_initialize(void);
    static uint8_t queue_pop_event(ui_input_event *out_event, int16_t queue_index);
    static void queue_push_event(int16_t queue_index, ui_input_event *record);
    static void queue_sample_time_update(void);
};

}
