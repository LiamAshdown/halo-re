#pragma once

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Control binding screens: preset application, per-device binding rows and bindable action queries.
 */
class ControlsBindings {
public:
    static uint8_t action_column_is_bindable(int32_t slot, int32_t action_index);
    static uint16_t * action_display_name(int32_t device, const char *action_name);
    static uint8_t apply_preset(widget_instance *widget);
    static uint8_t binding_clear(int32_t action_index, int32_t device);
    static int32_t binding_list_refresh_rows(widget_instance *widget, int32_t page);
    static uint8_t binding_row_handle_input(widget_instance *screen);
    static void binding_row_widget_update(int32_t action_index, widget_instance *row, int32_t device);
    static void binding_rows_toggle_device_mode(widget_instance *widget, uint8_t mode);
    static void build_device_label_table(void);
    static void device_label_add(const uint16_t *name, int32_t device_type);
    static uint8_t enumerate_next_assignable_action(int32_t device, int16_t *record, const char *action_name, uint8_t accept_reserved_on_retry);
    static uint8_t key_is_bindable(int32_t action);
};

}
