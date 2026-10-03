#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Tab-group synchronisation for the 5, 7, 9-wide and grouped tab layouts of the UI.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiTabGroups {
    static void tab_group_sync_5wide(widget_instance *widget);
    static void tab_group_sync_7wide(widget_instance *widget);
    static void tab_group_sync_9wide(widget_instance *widget);
    static void tab_group_sync_grouped(widget_instance *widget);
};

}
