#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Widget text refresh and status-flag synchronisation helpers.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiWidgets {
    static void widget_sync_profile_status_flag(widget_instance *widget);
    static void widget_text_ensure_and_refresh(widget_instance *widget);
    static void widget_text_from_hud_objective(widget_instance *widget);
};

}
