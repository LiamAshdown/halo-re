#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Per-frame game-data input functions run for each game_data_inputs entry of a widget
 * (game_data_input_function_table).
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiGameDataInputs {
    static void input_4a3b70(widget_instance *widget);
    static void input_4a4c70(widget_instance *widget);
    static void input_4a5740(widget_instance *widget);
    static void input_4a6880(widget_instance *widget);
    static void input_4a6a60(widget_instance *widget);
    static void input_4a6ab0(widget_instance *widget);
    static void input_4a6b70(widget_instance *widget);
    static void input_4a6d50(widget_instance *widget);
    static void input_4a6e50(widget_instance *widget);
    static void input_4a6e90(widget_instance *widget);
    static void input_4a6f00(widget_instance *widget);
    static void input_4a6fa0(widget_instance *widget);
    static void input_4a7180(widget_instance *widget);
    static void input_4a7210(widget_instance *widget);
    static void input_4a7280(widget_instance *widget);
    static void input_4a7300(widget_instance *widget);
    static void input_4a7340(widget_instance *widget);
    static void input_4a7350(widget_instance *widget);
    static void input_4a73d0(widget_instance *widget);
    static void input_4a7660(widget_instance *widget);
    static void input_4a7880(widget_instance *widget);
    static void input_4b5ce0(widget_instance *widget);
};

}
