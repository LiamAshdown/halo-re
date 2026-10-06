/**
 * @file include/halo/interface/coop_option.hpp
 * The CO-OP row added to Choose Difficulty (src/interface/coop_option.cpp).
 */
#pragma once

#include <stdint.h>

struct widget_instance;
typedef uint32_t datum_index;

namespace halo::interface::coop_option {

/** After a root widget is built: adds the row when it is the Choose Difficulty screen. */
void screen_loaded(widget_instance *root, datum_index tag_index);
/** A widget is being closed. */
void closed(widget_instance *widget);
/** True when the event toggled co-op and must not reach the widget's own handlers. */
bool handle_event(widget_instance *widget, const int16_t *event);
/** The text to draw in place of the widget's string list entry, or null. */
const uint16_t *text(widget_instance *widget);

}  // namespace halo::interface::coop_option
