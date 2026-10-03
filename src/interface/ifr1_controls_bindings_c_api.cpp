#include "halo/interface/ifr1_controls_bindings.hpp"

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::action_column_is_bindable.
 * blam-cc: ECX -> slot, EDX -> action_index
 *
 * @address 0x4b4df0
 */
extern "C" uint8_t controls_action_column_is_bindable(int32_t slot, int32_t action_index)
{
    return halo::interface::ControlsBindings::action_column_is_bindable(slot, action_index);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::action_display_name.
 * blam-cc: device -> EAX, action_name -> EDI
 *
 * @address 0x4b44c0
 */
extern "C" uint16_t * controls_action_display_name(int32_t device, const char *action_name)
{
    return halo::interface::ControlsBindings::action_display_name(device, action_name);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::apply_preset.
 *
 * @address 0x4b4c50
 */
extern "C" uint8_t controls_apply_preset(widget_instance *widget)
{
    return halo::interface::ControlsBindings::apply_preset(widget);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::binding_clear.
 * blam-cc: action_index -> EAX
 *
 * @address 0x4b4e20
 */
extern "C" uint8_t controls_binding_clear(int32_t action_index, int32_t device)
{
    return halo::interface::ControlsBindings::binding_clear(action_index, device);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::binding_list_refresh_rows.
 * blam-cc: widget -> EAX
 *
 * @address 0x4b4790
 */
extern "C" int32_t controls_binding_list_refresh_rows(widget_instance *widget, int32_t page)
{
    return halo::interface::ControlsBindings::binding_list_refresh_rows(widget, page);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::binding_row_handle_input.
 *
 * @address 0x4b4f30
 */
extern "C" uint8_t controls_binding_row_handle_input(widget_instance *screen)
{
    return halo::interface::ControlsBindings::binding_row_handle_input(screen);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::binding_row_widget_update.
 * blam-cc: action_index -> EAX
 *
 * @address 0x4b4520
 */
extern "C" void controls_binding_row_widget_update(int32_t action_index, widget_instance *row, int32_t device)
{
    halo::interface::ControlsBindings::binding_row_widget_update(action_index, row, device);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::binding_rows_toggle_device_mode.
 * blam-cc: widget -> ESI
 *
 * @address 0x4b53a0
 */
extern "C" void controls_binding_rows_toggle_device_mode(widget_instance *widget, uint8_t mode)
{
    halo::interface::ControlsBindings::binding_rows_toggle_device_mode(widget, mode);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::build_device_label_table.
 *
 * @address 0x4b4890
 */
extern "C" void __cdecl controls_build_device_label_table(void)
{
    halo::interface::ControlsBindings::build_device_label_table();
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::device_label_add.
 *
 * @address 0x4b4830
 */
extern "C" void controls_device_label_add(const uint16_t *name, int32_t device_type)
{
    halo::interface::ControlsBindings::device_label_add(name, device_type);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::enumerate_next_assignable_action.
 * blam-cc: device -> EAX, record -> ECX, action_name -> EDI
 *
 * @address 0x4b43e0
 */
extern "C" uint8_t controls_enumerate_next_assignable_action(int32_t device, int16_t *record, const char *action_name, uint8_t accept_reserved_on_retry)
{
    return halo::interface::ControlsBindings::enumerate_next_assignable_action(device, record, action_name, accept_reserved_on_retry);
}

/**
 * C ABI entry point; forwards to halo::interface::ControlsBindings::key_is_bindable.
 * blam-cc: action -> EDX
 *
 * @address 0x4b43c0
 */
extern "C" uint8_t controls_key_is_bindable(int32_t action)
{
    return halo::interface::ControlsBindings::key_is_bindable(action);
}
