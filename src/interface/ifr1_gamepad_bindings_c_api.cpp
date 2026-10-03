#include "halo/interface/ifr1_gamepad_bindings.hpp"

/**
 * C ABI entry point; forwards to halo::interface::GamepadBindings::bindings_restore.
 *
 * @address 0x4b5a70
 */
extern "C" uint8_t controls_gamepad_bindings_restore(void)
{
    return halo::interface::GamepadBindings::bindings_restore();
}

/**
 * C ABI entry point; forwards to halo::interface::GamepadBindings::list_add.
 * blam-cc: EAX -> list
 *
 * @address 0x4b5800
 */
extern "C" uint8_t controls_gamepad_list_add(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    return halo::interface::GamepadBindings::list_add(entry, list);
}

/**
 * C ABI entry point; forwards to halo::interface::GamepadBindings::list_find.
 * blam-cc: EDX -> list, stack -> entry
 *
 * @address 0x4b5760
 */
extern "C" int32_t controls_gamepad_list_find(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    return halo::interface::GamepadBindings::list_find(entry, list);
}

/**
 * C ABI entry point; forwards to halo::interface::GamepadBindings::list_remove.
 * blam-cc: EDI -> list
 *
 * @address 0x4b5850
 */
extern "C" uint8_t controls_gamepad_list_remove(const controls_gamepad_record *entry, controls_gamepad_record *list)
{
    return halo::interface::GamepadBindings::list_remove(entry, list);
}

/**
 * C ABI entry point; forwards to halo::interface::GamepadBindings::lists_load.
 *
 * @address 0x4b58d0
 */
extern "C" uint8_t controls_gamepad_lists_load(widget_instance *screen)
{
    return halo::interface::GamepadBindings::lists_load(screen);
}

/**
 * C ABI entry point; forwards to halo::interface::GamepadBindings::lists_refresh.
 * blam-cc: screen -> ECX
 *
 * @address 0x4b55d0
 */
extern "C" void controls_gamepad_lists_refresh(widget_instance *screen)
{
    halo::interface::GamepadBindings::lists_refresh(screen);
}

/**
 * C ABI entry point; forwards to halo::interface::GamepadBindings::toggle_assignment.
 *
 * @address 0x4b5b20
 */
extern "C" uint8_t controls_gamepad_toggle_assignment(widget_instance *row)
{
    return halo::interface::GamepadBindings::toggle_assignment(row);
}

/**
 * C ABI entry point; forwards to halo::interface::GamepadBindings::widget_nodes_collect.
 * blam-cc: out -> EAX, screen -> ECX
 *
 * @address 0x4b5560
 */
extern "C" void controls_gamepad_widget_nodes_collect(widget_instance **out, widget_instance *screen)
{
    halo::interface::GamepadBindings::widget_nodes_collect(out, screen);
}
