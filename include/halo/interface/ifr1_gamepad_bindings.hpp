#pragma once

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
 * Behaviour of the original GamepadBindings functions.
 */
class GamepadBindings {
public:
    static uint8_t bindings_restore(void);
    static uint8_t list_add(const controls_gamepad_record *entry, controls_gamepad_record *list);
    static int32_t list_find(const controls_gamepad_record *entry, controls_gamepad_record *list);
    static uint8_t list_remove(const controls_gamepad_record *entry, controls_gamepad_record *list);
    static uint8_t lists_load(widget_instance *screen);
    static void lists_refresh(widget_instance *screen);
    static uint8_t toggle_assignment(widget_instance *row);
    static void widget_nodes_collect(widget_instance **out, widget_instance *screen);
};

}
