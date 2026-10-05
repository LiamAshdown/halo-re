/**
 * @file src/input/globals.cpp
 * Binds halo::input::Globals to the engine variables the data image defines under their original link names.
 */

#include <cstring>
#include "tags.h"
#include "halo/text/api.hpp"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <wchar.h>
#include "crt.h"
#include <string.h>
#include "halo/input/binding_names.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/input/state.hpp"
#include "ai.h"
#include "halo/input/bindings.hpp"
#include "halo/saved_games/api.hpp"
#include <stdarg.h>
#include "halo/input/devices.hpp"
#include "halo/cseries/api.hpp"
#include "halo/shell/api.hpp"
#include "objects.h"
#include "units.h"
#include "halo/input/game_actions.hpp"
#include "halo/memory/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/core/crt.hpp"
#include "halo/input/api.hpp"
#include "halo/input/state.hpp"
#include "link/input.hpp"

namespace halo::input {

Globals &Service::instance()
{
    static Globals globals{
        ::joystick_slot_devices,
        ::input_suppressed,
        ::g_control_binding_state,
        ::g_control_binding_secondary_active,
    };
    return globals;
}

State &StateService::instance()
{
    static State state{
        ::missing_string_text,
        ::input_action_names,
        ::pov_direction_names,
        ::joystick_button_prefix,
        ::decimal_suffixes,
        ::joystick_pov_prefix,
        ::joystick_axis_prefix,
        ::input_devices,
        ::keyboard_bindings,
        ::mouse_button_bindings,
        ::mouse_axis_bindings,
        ::gamepad_button_bindings,
        ::gamepad_axis_bindings,
        ::gamepad_pov_bindings,
        ::game_engine_teams_enabled_flag,
        ::global_globals,
        ::current_game_engine,
        ::g_control_binding_region_e4,
        ::g_control_binding_region_ec,
        ::g_control_binding_region_e0,
        ::control_word_primary,
        ::control_binding_device_type,
        ::control_word_secondary,
        ::input_globals,
        ::joystick_states,
        ::joystick_neutral_state,
        ::last_input_device,
        ::gamepad_action_buttons,
        ::mouse_device,
        ::live_mouse_state,
        ::mouse_neutral_state,
        ::g_control_binding_id,
        ::g_control_binding_value,
        ::input_device_count,
        ::joystick_devices,
        ::input_acquired,
        ::keyboard_device,
        ::mouse_wheel_granularity,
        ::direct_input8_create,
        ::iid_directinput8a,
        ::direct_input,
        ::key_event_read_index,
        ::key_event_count,
        ::key_frames,
        ::key_release_pending,
        ::scan_code_to_key,
        ::game_time_force_single_tick,
        ::joystick_data_format,
        ::input_last_error,
        ::key_block_timers,
        ::system_keys,
        ::key_events,
        ::guid_sys_keyboard,
        ::mouse_button_map,
        ::guid_sys_mouse,
        ::virtual_key_to_key,
        ::character_to_key,
        ::mouse_axis_frames,
        ::joystick_axis_frames,
        ::joystick_pov_frames,
        ::look_yaw_rate_setting,
        ::look_pitch_rate_setting,
        ::mouse_acceleration,
        ::mouse_acceleration_cached,
        ::mouse_acceleration_defaults,
        ::mouse_acceleration_points,
        ::local_player_globals,
        ::player_data,
        ::joystick_objects,
        ::nojoystick,
        ::input_menu_exit_deadline,
        ::input_event_queue_active,
        ::menu_repeat_states,
        ::mouse_double_click_time,
        ::input_queue_sample_time,
    };
    return state;
}

}  // namespace halo::input
