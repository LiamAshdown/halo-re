/**
 * @file include/halo/objects/widgets.hpp
 * Object system API: widgets.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * The widget datum list that hangs off objects, and its fan-out to the antenna, flag and glow widget types.
 */
class WidgetSystem {
public:
    /**
     * Creates the widget data array and initialises the widget types.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004ff9d0
     */
    static void initialize();

    /**
     * Disposes the widget system.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004ffa10
     */
    static void dispose();

    /**
     * Clears the widget disposing flag.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004ffa50
     */
    static void dispose_clear_flag();

    /**
     * Creates the widgets defined by an object's tag.
     *
     * Original register convention: Ghidra shows a single unresolved `in_EAX`; by the object_data lookup shape shared
     * with every function in this module that resolves an object from a bare index, EAX is the object index.
     *
     * @address 0x004ffa80
     */
    static void create(uint32_t object_index);

    /**
     * Deletes every widget of an object.
     *
     * Original register convention: Ghidra shows a single unresolved `in_EAX`; by the same reasoning as widget_new.c,
     * EAX is the object index.
     *
     * @address 0x004ffbe0
     */
    static void delete_all(uint32_t object_index);

    /**
     * Returns whether any widget in a list carries the flag.
     *
     * @address 0x004ffc60
     */
    static int8_t list_has_flag(datum_index first_widget);

    /**
     * Calls the render hook of every widget of an object.
     *
     * Original register convention: Ghidra shows a single unresolved `unaff_EDI`; by the object_data lookup shape
     * shared with widget_new.c / widget_delete_all.c, EDI is the object index.
     *
     * @address 0x004ffca0
     */
    static void list_notify(uint32_t object_index, uint32_t render_arg, void *render_context);

    /**
     * Runs the update of every widget type for dt seconds.
     *
     * Original register convention: one stack parameter (dt), forwarded unchanged to each callback.
     *
     * @address 0x004ffd10
     */
    static void update_all(float dt);
};

}  // namespace halo::objects
