/**
 * C linkage shims for the input module: one extern "C" function per original symbol, forwarding to the
 * C++ implementation in namespace halo::input or to the member function of the record it operates on.
 */

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <wchar.h>
#include "ai.h"
#include "crt.h"
#include <string.h>
#include "win32.h"
#include <stdarg.h>
#include "objects.h"
#include "units.h"

#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/input/system.hpp"

extern "C" {

void chimera__axis_text(int16_t axis_index, uint8_t direction, uint16_t *out_text)
{
    halo::input::BindingNames::chimera__axis_text(axis_index, direction, out_text);
}

void chimera__button_text(int16_t button_index, uint16_t *out_text)
{
    halo::input::BindingNames::chimera__button_text(button_index, out_text);
}

void chimera__pov_text(int16_t pov_index, int16_t direction_index, uint16_t *out_text)
{
    halo::input::BindingNames::chimera__pov_text(pov_index, direction_index, out_text);
}

int16_t input_action_name_to_index(char *name)
{
    return halo::input::BindingNames::action_name_to_index(name);
}

int16_t input_axis_direction_name_to_index(char *name)
{
    return halo::input::BindingNames::axis_direction_name_to_index(name);
}

void input_get_axis_direction_name(int16_t direction_index, uint16_t *out_name)
{
    halo::input::BindingNames::get_axis_direction_name(direction_index, out_name);
}

void input_get_binding_display_name(control_binding_descriptor *binding, uint16_t *out_text)
{
    halo::input::BindingNames::get_binding_display_name(binding, out_text);
}

void input_get_keyboard_key_name(int16_t key_index, uint16_t *out_name)
{
    halo::input::BindingNames::get_keyboard_key_name(key_index, out_name);
}

void input_get_mouse_axis_name(int16_t axis_index, uint8_t direction, uint16_t *out_name)
{
    halo::input::BindingNames::get_mouse_axis_name(axis_index, direction, out_name);
}

void input_get_mouse_button_name(int16_t button_index, uint16_t *out_name)
{
    halo::input::BindingNames::get_mouse_button_name(button_index, out_name);
}

int16_t input_joystick_pov_direction_name_to_index(char *name)
{
    return halo::input::BindingNames::joystick_pov_direction_name_to_index(name);
}

uint32_t input_keyboard_key_name_to_index(char *name)
{
    return halo::input::BindingNames::keyboard_key_name_to_index(name);
}

uint32_t input_mouse_axis_name_to_index(char *name, uint8_t *out_direction)
{
    return halo::input::BindingNames::mouse_axis_name_to_index(name, out_direction);
}

uint32_t input_mouse_button_name_to_index(char *name)
{
    return halo::input::BindingNames::mouse_button_name_to_index(name);
}

void control_binding_table_initialize()
{
    halo::input::Bindings::control_binding_table_initialize();
}

uint8_t control_binding_table_query(int32_t target, int32_t raw_id)
{
    return halo::input::Bindings::control_binding_table_query(target, raw_id);
}

void control_binding_table_update_a()
{
    halo::input::Bindings::control_binding_table_update_a();
}

void control_binding_table_update_b()
{
    halo::input::Bindings::control_binding_table_update_b();
}

uint32_t control_word_extract_field(uint32_t which_word, uint32_t field_index)
{
    return halo::input::Bindings::control_word_extract_field(which_word, field_index);
}

uint8_t input_apply_control_binding(control_binding_descriptor *binding, int32_t action_index)
{
    return halo::input::Bindings::apply_control_binding(binding, action_index);
}

void input_apply_named_device_default_profile(uint16_t *device_name)
{
    halo::input::Bindings::apply_named_device_default_profile(device_name);
}

void input_bind_capture_reset()
{
    halo::input::Bindings::bind_capture_reset();
}

void input_bind_scan_set_active(uint8_t enable_scan)
{
    halo::input::Bindings::bind_scan_set_active(enable_scan);
}

void input_clear_control_binding(control_binding_descriptor *binding)
{
    halo::input::Bindings::clear_control_binding(binding);
}

uint32_t input_device_default_profile_tag_find(input_guid device_guid, void *out_profile)
{
    return halo::input::Bindings::device_default_profile_tag_find(device_guid, out_profile);
}

uint8_t input_get_last_used_binding(int16_t action, control_binding_descriptor *out)
{
    return halo::input::Bindings::get_last_used_binding(action, out);
}

void input_last_used_binding_copy(int16_t action, control_binding_descriptor *source)
{
    halo::input::Bindings::last_used_binding_copy(action, source);
}

void input_last_used_binding_set(int16_t action, int16_t device_type, int16_t device_index, int16_t input_kind, int16_t input_index, int32_t direction)
{
    halo::input::Bindings::last_used_binding_set(action, device_type, device_index, input_kind, input_index, direction);
}

uint8_t input_parse_device_binding_string(char *device_class_name, char *name, control_binding_descriptor *out_binding)
{
    return halo::input::Bindings::parse_device_binding_string(device_class_name, name, out_binding);
}

uint8_t input_profile_copy_bindings_by_device(int32_t category, saved_player_profile *dst, saved_player_profile *src)
{
    return halo::input::Bindings::profile_copy_bindings_by_device(category, dst, src);
}

uint8_t input_refresh_last_used_binding(int32_t device_class, int16_t action)
{
    return halo::input::Bindings::refresh_last_used_binding(device_class, action);
}

void input_scan_any_bound_input()
{
    halo::input::Bindings::scan_any_bound_input();
}

void test_input_device_defaults_find(char *device_id_ansi)
{
    halo::input::Bindings::test_input_device_defaults_find(device_id_ansi);
}

void control_binding_table_register_single(int32_t target, int32_t selector, int32_t raw_id, uint32_t raw_value)
{
    halo::input::Bindings::control_binding_table_register_single(target, selector, raw_id, raw_value);
}

void hs_bind_control(const char *device_class_name, const char *input_name, const char *action_name)
{
    halo::input::Bindings::hs_bind_control(device_class_name, input_name, action_name);
}

void hs_unbind_control(const char *device_class_name, const char *input_name)
{
    halo::input::Bindings::hs_unbind_control(device_class_name, input_name);
}

void input_accumulate_axis_value(player_control_settings *settings, local_player_input_state *state, int16_t action)
{
    halo::input::GameActions::accumulate_axis_value(settings, state, action);
}

uint8_t input_accumulator_is_idle(local_player_input_state *current, local_player_input_state *previous)
{
    return halo::input::GameActions::accumulator_is_idle(current, previous);
}

float input_clamp_unit_float(float value)
{
    return halo::input::GameActions::clamp_unit_float(value);
}

void input_game_action_update()
{
    halo::input::GameActions::game_action_update();
}

void input_joystick_set_axis_scale_x(int16_t slot, float value)
{
    halo::input::GameActions::joystick_set_axis_scale_x(slot, value);
}

void input_joystick_set_axis_scale_y(int16_t slot, float value)
{
    halo::input::GameActions::joystick_set_axis_scale_y(slot, value);
}

float input_mouse_acceleration_evaluate(float sensitivity, int32_t magnitude)
{
    return halo::input::GameActions::mouse_acceleration_evaluate(sensitivity, magnitude);
}

void input_reset_state_and_axis_configs()
{
    halo::input::GameActions::reset_state_and_axis_configs();
}

float input_sensitivity_to_turn_rate(float sensitivity)
{
    return halo::input::GameActions::sensitivity_to_turn_rate(sensitivity);
}

uint8_t input_should_invert_look(int16_t local_player_index)
{
    return halo::input::GameActions::should_invert_look(local_player_index);
}

int32_t input_device_count_by_guid(const uint32_t *guid)
{
    return halo::input::DirectInput::device_count_by_guid(guid);
}

uint32_t input_device_find_index_by_guid(controls_gamepad_record *record)
{
    return halo::input::DirectInput::device_find_index_by_guid(record);
}

int32_t input_device_get_axis_count(int16_t slot_index)
{
    return halo::input::DirectInput::device_get_axis_count(slot_index);
}

int32_t input_device_get_button_count(int16_t slot_index)
{
    return halo::input::DirectInput::device_get_button_count(slot_index);
}

int32_t input_device_get_pov_count(int16_t slot_index)
{
    return halo::input::DirectInput::device_get_pov_count(slot_index);
}

void input_device_list_print()
{
    halo::input::DirectInput::device_list_print();
}

void input_device_release(int16_t slot_index)
{
    halo::input::DirectInput::device_release(slot_index);
}

void input_directinput_acquire_devices()
{
    halo::input::DirectInput::directinput_acquire_devices();
}

uint8_t input_directinput_initialize()
{
    return halo::input::DirectInput::directinput_initialize();
}

void input_directinput_poll_devices()
{
    halo::input::DirectInput::directinput_poll_devices();
}

void input_directinput_release_devices()
{
    halo::input::DirectInput::directinput_release_devices();
}

void input_directinput_unacquire_devices()
{
    halo::input::DirectInput::directinput_unacquire_devices();
}

int32_t __stdcall input_enumerate_gamepad_callback(const di_device_instance *instance, void *reference)
{
    return halo::input::DirectInput::enumerate_gamepad_callback(instance, reference);
}

int32_t __stdcall input_enumerate_gamepad_object_callback(const di_device_object_instance *object, void *reference)
{
    return halo::input::DirectInput::enumerate_gamepad_object_callback(object, reference);
}

uint8_t input_get_key_state(int16_t key_index)
{
    return halo::input::DirectInput::get_key_state(key_index);
}

uint8_t input_get_mouse_button_state(int16_t button_index)
{
    return halo::input::DirectInput::get_mouse_button_state(button_index);
}

uint8_t input_guid_parse_ansi(input_guid *out_guid, char *ansi)
{
    return halo::input::DirectInput::guid_parse_ansi(out_guid, ansi);
}

void input_joystick_state_process(joystick_raw_state *raw, joystick_state *dest, input_device *device)
{
    raw->joystick_state_process(dest, device);
}

void input_key_block_timer_set(int16_t key, int32_t duration_ms)
{
    halo::input::DirectInput::key_block_timer_set(key, duration_ms);
}

void input_key_block_timers_expire()
{
    halo::input::DirectInput::key_block_timers_expire();
}

uint8_t input_keyboard_device_create()
{
    return halo::input::DirectInput::keyboard_device_create();
}

void input_keyboard_set_capture_mode(uint8_t enable_capture)
{
    halo::input::DirectInput::keyboard_set_capture_mode(enable_capture);
}

uint8_t input_mouse_device_create()
{
    return halo::input::DirectInput::mouse_device_create();
}

void input_mouse_state_process(mouse_state *dest, di_mouse_state2 *raw)
{
    dest->mouse_state_process(raw);
}

void input_record_windows_key_message(uint32_t wparam, int32_t message)
{
    halo::input::DirectInput::record_windows_key_message(wparam, message);
}

int16_t input_joystick_axis_name_to_index(char *name, uint8_t *out_direction)
{
    return halo::input::BindingNames::joystick_axis_name_to_index(name, out_direction);
}

int16_t input_joystick_button_name_to_index(char *name)
{
    return halo::input::BindingNames::joystick_button_name_to_index(name);
}

int16_t input_joystick_pov_name_to_index(char *name, int16_t *out_direction)
{
    return halo::input::BindingNames::joystick_pov_name_to_index(name, out_direction);
}

void input_menu_generate_events()
{
    halo::input::UiEvents::menu_generate_events();
}

void input_queue_initialize()
{
    halo::input::UiEvents::queue_initialize();
}

uint8_t input_queue_pop_event(ui_input_event *out_event, int16_t queue_index)
{
    return halo::input::UiEvents::queue_pop_event(out_event, queue_index);
}

void input_queue_push_event(int16_t queue_index, ui_input_event *record)
{
    halo::input::UiEvents::queue_push_event(queue_index, record);
}

void input_queue_sample_time_update()
{
    halo::input::UiEvents::queue_sample_time_update();
}

void input_print_bound_controls()
{
    halo::input::BindingNames::print_bound_controls();
}

void input_state_initialize()
{
    halo::input::InputSystem::state_initialize();
}

uint32_t input_system_initialize()
{
    return halo::input::InputSystem::system_initialize();
}

void input_time_base_resync()
{
    halo::input::InputSystem::time_base_resync();
}

void input_update_tick()
{
    halo::input::InputSystem::update_tick();
}

}
