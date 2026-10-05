/**
 * The input module's public API (include/halo/input/api.hpp): forwards the calls other modules make to the C++ implementation in
 * namespace halo::input or to the member function of the record it operates on, and exposes the module's engine globals.
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
#include "halo/input/devices.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/input/system.hpp"
#include "halo/input/api.hpp"


namespace halo::input {


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

uint32_t input_device_default_profile_tag_find(input_guid device_guid, void *out_profile)
{
    return halo::input::Bindings::device_default_profile_tag_find(device_guid, out_profile);
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

uint8_t input_get_key_state(int16_t key_index)
{
    return halo::input::InputDevices::get_key_state(key_index);
}

void keyboard_flush(void)
{
    halo::input::InputDevices::keyboard_flush();
}

}
