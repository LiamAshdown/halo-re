/**
 * Control binding table, last-used-binding cache, bind/unbind commands, rebind capture and device default profiles.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "crt.h"
#include "cache.h"
#include <wchar.h>
#include <string.h>

#include "halo/input/bindings.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" { extern uint8_t g_control_binding_state; }
extern "C" { extern uint8_t g_control_binding_secondary_active; }
extern "C" { extern Globals *global_globals; }
extern "C" { extern uint32_t current_game_engine; }
extern "C" { extern uint8_t g_control_binding_region_e4[0xa0]; }
namespace halo::input {

/**
 * Implements control binding table initialize.
 *
 * @address 0x4f3700
 */
void Bindings::control_binding_table_initialize(void)
{
    uint8_t *row;
    int32_t offset;
    int32_t outer_row = 0;
    int32_t field_index;

    g_control_binding_secondary_active = (halo::game::globals().teams_enabled != 0);
    g_control_binding_state = 0;

    row = g_control_binding_region_e4;
    do {
        uint8_t *cell = row;
        offset = 0;
        field_index = 0;
        do {
            ((control_binding_half *)(cell - 4))->entry_count = 0;
            ((control_binding_half *)(cell - 4))->selected_count = 0;
            ((control_binding_half *)(cell - 4))->limit = (int32_t)halo::input::control_word_extract_field(outer_row, field_index);
            field_index++;

            {
                uint8_t *sub = cell + 0x10;
                int32_t count = 8;
                do {
                    ((control_binding_entry *)(sub - 4))->id = -1;
                    ((control_binding_entry *)(sub - 4))->selected = 0;
                    sub += 8;
                    count--;
                } while (count != 0);
            }

            if (current_game_engine == 0) {
                ((control_binding_half *)(cell - 4))->profile_default = -1;
            } else {
                ((control_binding_half *)(cell - 4))->profile_default = *(int32_t *)(*(int32_t *)((uint8_t *)global_globals->multiplayer_information.pointer + 0x24) + 0xc + offset);
            }

            offset += 0x10;
            cell += 0xa0;
        } while (offset < 0x60);

        row += 0x50;
        outer_row++;
    } while (row < g_control_binding_region_e4 + 0xa0);
}

}

extern "C" { extern uint8_t g_control_binding_region_ec[0x3c0]; }
extern "C" { extern uint8_t g_control_binding_region_e0[0x3c0]; }
namespace halo::input {

/**
 * Implements control binding table query.
 *
 * @address 0x4f3ad0
 */
uint8_t Bindings::control_binding_table_query(int32_t target, int32_t raw_id)
{
    uint8_t result = 1;
    uint8_t *cursor;
    uint8_t *region_end;
    int32_t row;
    int32_t row_index;

    if (current_game_engine == 0) {
        return result;
    }

    cursor = g_control_binding_region_ec;
    region_end = g_control_binding_region_ec + sizeof(g_control_binding_region_ec);
    row_index = -1;
    row = 0;
    while (cursor < region_end) {
        if (*(int32_t *)cursor == target) {
            row_index = row;
            break;
        }
        cursor += 0xa0;
        row++;
    }

    result = 0;
    if (row_index < 0 || row_index > 5) {
        return 0;
    }

    {
        int32_t pair;
        for (pair = 0; pair <= 2; pair++) {
            int32_t idx = pair + row_index * 2;
            int32_t count = *(int32_t *)(g_control_binding_region_e0 + idx * 0x50);
            if (count != 0) {
                uint8_t *id_cursor = g_control_binding_region_e0 + 0x10 + idx * 0x50;
                uint32_t slot;
                for (slot = 0; slot < (uint32_t)count; slot++) {
                    if (*(int32_t *)id_cursor == raw_id) {
                        result = *(uint8_t *)(g_control_binding_region_e0 + 0x14 + (slot + idx * 10) * 8);
                        return result;
                    }
                    id_cursor += 8;
                }
            }
        }
    }

    return result;
}

}

extern "C" { extern uint32_t control_word_primary; }
extern "C" { extern uint32_t control_binding_device_type; }
namespace halo::input {

/**
 * Implements control binding table update a.
 *
 * @address 0x4f3890
 */
void Bindings::control_binding_table_update_a(void)
{
    uint32_t low_nibble = control_word_primary & 0xf;
    int32_t row_offset = 0;
    int32_t pair_index = 0;

    do {
        if (low_nibble == 0) {
            int32_t sub;
            for (sub = 0; sub < 2; sub++) {
                uint8_t *cell = g_control_binding_region_e0 + row_offset + sub * 0x50;
                int32_t count = ((control_binding_half *)cell)->entry_count;
                int32_t i;

                for (i = 0; i < count; i++) {
                    uint8_t value = ((control_binding_half *)cell)->entries[i].device_mask;
                    uint8_t bit;

                    switch (control_binding_device_type) {
                    case 1: bit = (uint8_t)((value >> 1) & 1); break;
                    case 2: bit = (uint8_t)(value & 1); break;
                    case 3: bit = (uint8_t)((value >> 3) & 1); break;
                    case 4: bit = (uint8_t)((value >> 2) & 1); break;
                    default: bit = 0; break;
                    }
                    if (bit) {
                        ((control_binding_half *)cell)->entries[i].selected = 1;
                    }
                }
            }
        } else {
            uint8_t *base = g_control_binding_region_e0 + row_offset;
            int32_t remaining = ((control_binding_half *)base)->limit;
            int32_t settled = 0;

            while (remaining > 0 && !settled) {
                uint32_t a = (uint32_t)((control_binding_half *)base)->selected_count;
                uint32_t b = (uint32_t)((control_binding_half *)(g_control_binding_region_e0 + 0x50 + row_offset))->selected_count;
                int32_t pick;
                int32_t has_pick = 0;

                settled = 1;
                if (b < a) {
                    if (b < (uint32_t)((control_binding_half *)(g_control_binding_region_e0 + 0x50 + row_offset))->entry_count) {
                        pick = 1; has_pick = 1;
                    } else if (a < (uint32_t)((control_binding_half *)base)->entry_count) {
                        pick = 0; has_pick = 1;
                    }
                } else {
                    if (a < (uint32_t)((control_binding_half *)base)->entry_count) {
                        pick = 0; has_pick = 1;
                    } else if (b < (uint32_t)((control_binding_half *)(g_control_binding_region_e0 + 0x50 + row_offset))->entry_count) {
                        pick = 1; has_pick = 1;
                    }
                }

                if (has_pick) {
                    int32_t idx = pick + pair_index;

                    int32_t *count_cell = (int32_t *)(g_control_binding_region_e0 + 4 + idx * 0x50);
                    int32_t slot = *count_cell + idx * 10;
                    *count_cell = *count_cell + 1;
                    settled = 0;
                    *(uint8_t *)(g_control_binding_region_e0 + 0x14 + slot * 8) = 1;
                    remaining--;
                }
            }
        }

        row_offset += 0xa0;
        pair_index += 2;
    } while (row_offset <= 0x3bf);
}

}

extern "C" { extern uint32_t control_word_secondary; }
namespace halo::input {

/**
 * Implements control binding table update b.
 *
 * @address 0x4f39d0
 */
void Bindings::control_binding_table_update_b(void)
{
    int32_t word_selector = 0;
    do {
        int32_t count_offset = word_selector * 10;
        int32_t row = 0;
        int32_t row_offset = word_selector * 0x50;

        do {
            int32_t count = *(int32_t *)(g_control_binding_region_e0 + row_offset);
            uint32_t word = (word_selector != 1) ? control_word_primary : control_word_secondary;

            if ((word & 0xf) == 0) {
                uint8_t *entries = g_control_binding_region_e0 + row_offset + 0x14;
                int32_t i;

                for (i = 0; i < count; i++) {
                    uint8_t value = entries[i * 8 + 2];
                    uint8_t bit;

                    switch (control_binding_device_type) {
                    case 1: bit = (uint8_t)((value >> 1) & 1); break;
                    case 2: bit = (uint8_t)(value & 1); break;
                    case 3: bit = (uint8_t)((value >> 3) & 1); break;
                    case 4: bit = (uint8_t)((value >> 2) & 1); break;
                    default: bit = 0; break;
                    }
                    if (bit) {
                        entries[i * 8] = 1;
                    }
                }
            } else {
                int32_t remaining = *(int32_t *)(g_control_binding_region_e0 + row_offset + 8);
                int32_t i = 0;
                if (count > 0) {
                    do {
                        if (remaining < 1) break;
                        *(int32_t *)(g_control_binding_region_e0 + row_offset + 4) =
                            *(int32_t *)(g_control_binding_region_e0 + row_offset + 4) + 1;
                        {
                            int32_t slot = i + count_offset;
                            remaining--;
                            i++;
                            *(uint8_t *)(g_control_binding_region_e0 + 0x14 + slot * 8) = 1;
                        }
                    } while (i < count);
                }
            }

            row++;
            row_offset += 0xa0;
            count_offset += 0x14;
        } while (row < 6);

        word_selector++;
    } while (word_selector <= 1);
}

}

namespace halo::input {

/**
 * Implements control word extract field.
 *
 * @address 0x4f3680
 */
uint32_t Bindings::control_word_extract_field(uint32_t which_word, uint32_t field_index)
{
    uint32_t word = (which_word == 1) ? control_word_secondary : control_word_primary;

    if ((word & 0xf) != 8) {
        word = halo::game::game_variant_option_default_by_index(word & 0xf);
    }

    switch (field_index) {
    case 0: return (word >> 4) & 7;
    case 1: return (word >> 7) & 7;
    case 2: return (word >> 10) & 7;
    case 3: return (word >> 0x10) & 7;
    case 4: return (word >> 0x13) & 7;
    case 5: return (word >> 0xd) & 7;
    default: return 0;
    }
}

}

extern "C" { extern input_device input_devices[8]; }
extern "C" { extern int32_t joystick_slot_devices[4]; }
extern "C" { extern int16_t keyboard_bindings[k_control_keyboard_key_count]; }
extern "C" { extern int16_t mouse_button_bindings[k_control_mouse_button_count]; }
extern "C" { extern int16_t mouse_axis_bindings[k_control_mouse_axis_count][2]; }
extern "C" { extern int16_t gamepad_button_bindings[k_control_gamepad_count][k_control_gamepad_button_count]; }
extern "C" { extern int16_t gamepad_axis_bindings[k_control_gamepad_count][k_control_gamepad_axis_count][2]; }
extern "C" { extern int16_t gamepad_pov_bindings[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; }
namespace halo::input {

/**
 * Writes a parsed device-input descriptor's game-control assignment into the matching
 * keyboard/mouse/joystick binding table of the active binding profile (settings[0]). A joystick
 * binding is range-checked against the target device's reported axis/button/POV count and
 * silently dropped if the descriptor's input_index is out of range or the slot has no device.
 * Returns 1 on a successful write, 0 otherwise (unrecognized device_type, or out-of-range
 * joystick input).
 *
 * @address 0x48b7b0
 */
uint8_t Bindings::apply_control_binding(control_binding_descriptor *binding, int32_t action_index)
{
    int32_t device;
    int32_t count;

    switch (binding->device_type) {
    case _control_device_keyboard:
        keyboard_bindings[binding->input_index] = (int16_t)action_index;
        return 1;

    case _control_device_mouse:
        if (binding->input_kind != _control_input_axis) {
            mouse_button_bindings[binding->input_index] = (int16_t)action_index;
        } else if (binding->direction == 1) {
            mouse_axis_bindings[binding->input_index][0] = (int16_t)action_index;
        } else {
            mouse_axis_bindings[binding->input_index][1] = (int16_t)action_index;
        }
        return 1;

    case _control_device_gamepad:
        if (binding->input_kind == _control_input_axis) {
            count = 0;
            device = joystick_slot_devices[binding->device_index];
            if (device != -1) {
                count = input_devices[device].axis_count;
            }
            if (binding->input_index < count) {
                if (binding->direction == 1) {
                    gamepad_axis_bindings[binding->device_index][binding->input_index][0] = (int16_t)action_index;
                } else {
                    gamepad_axis_bindings[binding->device_index][binding->input_index][1] = (int16_t)action_index;
                }
                return 1;
            }
        } else if (binding->input_kind == _control_input_pov) {
            device = joystick_slot_devices[binding->device_index];
            count = 0;
            if (device != -1) {
                count = input_devices[device].pov_count;
            }
            if (binding->input_index < count) {
                gamepad_pov_bindings[binding->device_index][binding->input_index][binding->direction] = (int16_t)action_index;
                return 1;
            }
        } else {
            device = joystick_slot_devices[binding->device_index];
            count = 0;
            if (device != -1) {
                count = input_devices[device].button_count;
            }
            if (binding->input_index < count) {
                gamepad_button_bindings[binding->device_index][binding->input_index] = (int16_t)action_index;
                return 1;
            }
        }
        return 0;

    default:
        return 0;
    }
}

}

namespace halo::input {

/**
 * Looks up the named full-profile-definition InputDeviceDefaults tag ("devc") and, if found,
 * creates and saves a new player profile from it.
 *
 * Original register convention: device_name in EDI.
 *
 * @address 0x4901b0
 */
void Bindings::apply_named_device_default_profile(uint16_t *device_name)
{
    tag_iterator iterator;
    datum_index tag_id;
    InputDeviceDefaults *defaults;
    uint16_t *tag_profile_name;
    uint32_t profile_handle;
    saved_player_profile profile;

    iterator.next_index = -1;
    iterator.group_tag = (tag_group)0x64657663;

    tag_id = halo::cache::tag_iterator_next(&iterator);
    if (tag_id == (datum_index)0xffffffff) {
        return;
    }

    for (;;) {
        defaults = (InputDeviceDefaults *)halo::cache::globals().tag_instances[(uint16_t)tag_id].data;
        tag_profile_name = (uint16_t *)(defaults->profile.pointer + 2);
        if (defaults->device_type == inputdevicedefaultsdevicetype_full_profile_definition &&
            _wcsicmp((const wchar_t *)device_name, (const wchar_t *)tag_profile_name) == 0) {
            break;
        }
        tag_id = halo::cache::tag_iterator_next(&iterator);
        if (tag_id == (datum_index)0xffffffff) {
            return;
        }
    }

    profile_handle = halo::saved_games::saved_game_create_default_profile(tag_profile_name);
    if (profile_handle != 0xffffffff) {
        if (halo::saved_games::player_profile_get((int32_t)profile_handle, &profile) != 0) {
            if (halo::input::input_profile_copy_bindings_by_device(2, &profile,
                    (saved_player_profile *)defaults->profile.pointer) != 0) {
                profile.flags |= 0x0006;
                halo::saved_games::player_profile_save_539bf0((int32_t)profile_handle, &profile);
            }
        }
    }
}

}

extern "C" { extern input_abstraction_globals input_globals; }
namespace halo::input {

/**
 * Clears the local player's current-frame input accumulator (action state 0) and the cached
 * system-key hold states, and marks the frame idle so the bind-capture UI starts from a clean
 * state.
 *
 * @address 0x48b5f0
 */
void Bindings::bind_capture_reset(void)
{
    memset(&input_globals.states[0], 0, sizeof(input_globals.states[0]));

    input_globals.system_key_states[0] = 0;
    input_globals.system_key_states[1] = 0;
    input_globals.system_key_states[2] = 0;
    input_globals.pad_24ab = 0;

    input_globals.idle = 1;
}

}

extern "C" { extern uint8_t input_suppressed; }
extern "C" { extern joystick_state joystick_states[4]; }
extern "C" { extern joystick_state joystick_neutral_state; }
namespace halo::input {

/**
 * Begins or ends the bind-capture axis scan. Starting the scan sets the bind-scan mode bit and
 * snapshots each joystick slot's current axis/button/POV state (or the neutral state while
 * input is suppressed, or zero for an unmapped slot) into scan_baselines, which
 * input_scan_any_bound_input later compares against. Ending the scan clears the mode bit and
 * zeroes every baseline.
 *
 * @address 0x48b6b0
 */
void Bindings::bind_scan_set_active(uint8_t enable_scan)
{
    uint8_t suppressed;
    int32_t slot;

    suppressed = input_suppressed;

    if (enable_scan == 0) {
        input_globals.mode_flags = input_globals.mode_flags & ~_input_mode_bind_scan_bit;
        memset(input_globals.scan_baselines, 0, sizeof(input_globals.scan_baselines));
        return;
    }

    input_globals.mode_flags = input_globals.mode_flags | _input_mode_bind_scan_bit;

    for (slot = 0; slot < 4; slot++) {
        if (joystick_slot_devices[slot] == -1) {
            memset(&input_globals.scan_baselines[slot], 0, sizeof(joystick_state));
        } else if (suppressed == 0) {
            input_globals.scan_baselines[slot] = joystick_states[slot];
        } else {
            input_globals.scan_baselines[slot] = joystick_neutral_state;
        }
    }
}

}

namespace halo::input {

/**
 * Clears (unbinds) a device input's game-control assignment in the matching keyboard/mouse/
 * joystick binding table of the active binding profile (settings[0]), writing k_input_unbound.
 * The gamepad axis and POV branches range-check input_index against the mapped device's
 * reported count first (as input_apply_control_binding does); the gamepad button branch does
 * not, matching the binary.
 *
 * @address 0x48b9b0
 */
void Bindings::clear_control_binding(control_binding_descriptor *binding)
{
    int32_t device;
    int32_t count;

    switch (binding->device_type) {
    case _control_device_keyboard:
        keyboard_bindings[binding->input_index] = k_input_unbound;
        return;

    case _control_device_mouse:
        if (binding->input_kind != _control_input_axis) {
            mouse_button_bindings[binding->input_index] = k_input_unbound;
        } else if (binding->direction == 1) {
            mouse_axis_bindings[binding->input_index][0] = k_input_unbound;
        } else {
            mouse_axis_bindings[binding->input_index][1] = k_input_unbound;
        }
        return;

    case _control_device_gamepad:
        if (binding->input_kind == _control_input_axis) {
            count = 0;
            device = joystick_slot_devices[binding->device_index];
            if (device != -1) {
                count = input_devices[device].axis_count;
            }
            if (binding->input_index < count) {
                if (binding->direction == 1) {
                    gamepad_axis_bindings[binding->device_index][binding->input_index][0] = k_input_unbound;
                } else {
                    gamepad_axis_bindings[binding->device_index][binding->input_index][1] = k_input_unbound;
                }
            }
        } else if (binding->input_kind == _control_input_pov) {
            device = joystick_slot_devices[binding->device_index];
            count = 0;
            if (device != -1) {
                count = input_devices[device].pov_count;
            }
            if (binding->input_index < count) {
                gamepad_pov_bindings[binding->device_index][binding->input_index][binding->direction] = k_input_unbound;
            }
        } else {
            gamepad_button_bindings[binding->device_index][binding->input_index] = k_input_unbound;
        }
        return;

    default:
        return;
    }
}

}

namespace halo::input {

/**
 * Scans every InputDeviceDefaults ("devc") tag for a mouse/keyboard or joystick/gamepad entry
 * whose device_id matches device_guid, and copies its saved_player_profile-sized profile block
 * out to out_profile. Returns the matching tag's datum index, or 0xffffffff if none match.
 *
 * @address 0x490110
 */
uint32_t Bindings::device_default_profile_tag_find(input_guid device_guid, void *out_profile)
{
    tag_iterator iterator;
    datum_index tag_id;
    InputDeviceDefaults *defaults;

    iterator.next_index = -1;
    iterator.group_tag = (tag_group)0x64657663;

    tag_id = halo::cache::tag_iterator_next(&iterator);
    while (tag_id != (datum_index)0xffffffff) {
        defaults = (InputDeviceDefaults *)halo::cache::globals().tag_instances[(uint16_t)tag_id].data;
        if (defaults->device_type == inputdevicedefaultsdevicetype_mouse_and_keyboard ||
            defaults->device_type == inputdevicedefaultsdevicetype_joysticks_gamepads_etc) {
            if (memcmp(&device_guid, (void *)defaults->device_id.pointer, sizeof(input_guid)) == 0) {
                memcpy(out_profile, (void *)defaults->profile.pointer, k_saved_player_profile_size);
                return (uint32_t)tag_id;
            }
        }
        tag_id = halo::cache::tag_iterator_next(&iterator);
    }
    return 0xffffffff;
}

}

extern "C" { extern int32_t last_input_device; }
namespace halo::input {

/**
 * Returns the input device/type most recently used to activate game control `action` by copying
 * input_globals.last_used_bindings[action] into *out. If that record is not yet known
 * (device_type == 0), first tries to resolve it on the last device class that produced any
 * input (last_input_device), then falls back to trying every device class 0..4 in order.
 * Leaves *out untouched if the control turns out not to be bound anywhere.
 *
 * @address 0x48bde0
 */
uint8_t Bindings::get_last_used_binding(int16_t action, control_binding_descriptor *out)
{
    control_binding_descriptor *cached;
    uint8_t found;
    int32_t device_class;

    cached = &input_globals.last_used_bindings[action];
    found = (cached->device_type != 0);

    if (!found) {
        found = halo::input::input_refresh_last_used_binding(last_input_device, action);
        if (!found) {
            for (device_class = 0; device_class < 5; device_class++) {
                found = halo::input::input_refresh_last_used_binding(device_class, action);
                if (found) {
                    break;
                }
            }
        }
    }

    if (found) {
        *out = *cached;
    }
    return found;
}

}

namespace halo::input {

/**
 * Directly sets (source != 0) or clears (source == 0) the cached last-used-binding record for
 * game control action, when action is a valid input action index. No range checks against the
 * source device's reported capabilities are performed here.
 *
 * @address 0x48be50
 */
void Bindings::last_used_binding_copy(int16_t action, control_binding_descriptor *source)
{
    if (action >= 0 && action < k_input_action_count) {
        if (source == 0) {
            input_globals.last_used_bindings[action].device_type = 0;
            input_globals.last_used_bindings[action].device_index = 0;
            input_globals.last_used_bindings[action].input_kind = 0;
            input_globals.last_used_bindings[action].input_index = 0;
            input_globals.last_used_bindings[action].direction = 0;
        } else {
            input_globals.last_used_bindings[action] = *source;
        }
    }
}

}

namespace halo::input {

/**
 * direction on the stack
 * Writes every field of last_used_bindings[action] directly, when action is a valid input
 * action index (0 .. k_input_action_count - 1).
 *
 * Original register convention: action index in EAX, device_type in ECX, device_index in EDX, input_kind/input_index/.
 *
 * @address 0x490050
 */
void Bindings::last_used_binding_set(int16_t action, int16_t device_type, int16_t device_index, int16_t input_kind, int16_t input_index, int32_t direction)
{
    control_binding_descriptor *binding;

    if (action >= 0 && action < k_input_action_count) {
        binding = &input_globals.last_used_bindings[action];
        binding->device_type = device_type;
        binding->device_index = device_index;
        binding->input_kind = input_kind;
        binding->input_index = input_index;
        binding->direction = direction;
    }
}

}

extern "C" { extern int32_t _stricmp(const char *a, const char *b); }
namespace halo::input {

/**
 * Parses device_class_name (keyboard/key, mouse, mouseaxis, joystick, joystickaxis, joystickpov)
 * and name into *out_binding. Returns 1 on success, 0 if device_class_name is unrecognized or
 * name doesn't resolve within that class.
 *
 * Original register convention: device_class_name in EDI, name on the stack, out_binding in ESI.
 *
 * @address 0x48fea0
 */
uint8_t Bindings::parse_device_binding_string(char *device_class_name, char *name, control_binding_descriptor *out_binding)
{
    uint32_t index;
    int16_t joystick_index;
    uint8_t byte_direction;
    int16_t pov_direction;

    if (_stricmp(device_class_name, "keyboard") == 0 || _stricmp(device_class_name, "key") == 0) {
        index = halo::input::input_keyboard_key_name_to_index(name);
        if (index == 0xffff) {
            return 0;
        }
        out_binding->device_type = _control_device_keyboard;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_button;
        out_binding->input_index = (int16_t)index;
        out_binding->direction = 0;
        return 1;
    }

    if (_stricmp(device_class_name, "mouse") == 0) {
        index = halo::input::input_mouse_button_name_to_index(name);
        if (index == 0xffff) {
            return 0;
        }
        out_binding->device_type = _control_device_mouse;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_button;
        out_binding->input_index = (int16_t)index;
        out_binding->direction = 0;
        return 1;
    }

    if (_stricmp(device_class_name, "mouseaxis") == 0) {
        index = halo::input::input_mouse_axis_name_to_index(name, &byte_direction);
        if (index == 0xffff) {
            return 0;
        }
        out_binding->device_type = _control_device_mouse;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_axis;
        out_binding->input_index = (int16_t)index;
        out_binding->direction = (byte_direction == 0) ? 2 : 1;
        return 1;
    }

    if (_stricmp(device_class_name, "joystick") == 0) {
        joystick_index = halo::input::input_joystick_button_name_to_index(name);
        if (joystick_index == -1) {
            return 0;
        }
        out_binding->device_type = _control_device_gamepad;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_button;
        out_binding->input_index = joystick_index;
        out_binding->direction = 0;
        return 1;
    }

    if (_stricmp(device_class_name, "joystickaxis") == 0) {
        joystick_index = halo::input::input_joystick_axis_name_to_index(name, &byte_direction);
        if (joystick_index == -1) {
            return 0;
        }
        out_binding->device_type = _control_device_gamepad;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_axis;
        out_binding->input_index = joystick_index;
        out_binding->direction = (byte_direction == 0) ? 2 : 1;
        return 1;
    }

    if (_stricmp(device_class_name, "joystickpov") == 0) {
        joystick_index = halo::input::input_joystick_pov_name_to_index(name, &pov_direction);
        if (joystick_index == -1) {
            return 0;
        }
        out_binding->device_type = _control_device_gamepad;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_pov;
        out_binding->input_index = joystick_index;
        out_binding->direction = pov_direction;
        return 1;
    }

    return 0;
}

}

namespace halo::input {

/**
 * Copies a category of binding/settings fields from src to dst:
 * 2 -- identity (name, player_color, campaign_progress OR-mask) plus keyboard/mouse bindings
 * and the mouse-scale rates
 * 0 -- keyboard/mouse bindings and the mouse-scale rates only
 * 1 -- gamepad slot 0's bindings, action buttons, device record and per-pad rates only
 * any other value -- nothing category-specific
 * Every category except -1 additionally copies the four digital movement/look rates and the
 * look-inversion bytes. Returns 0 immediately for category -1, 1 otherwise.
 *
 * Original register convention: category in EAX, destination in EDX, source in EBX.
 *
 * @address 0x490280
 */
uint8_t Bindings::profile_copy_bindings_by_device(int32_t category, saved_player_profile *dst, saved_player_profile *src)
{
    int32_t i;

    if (category == 2) {
        memcpy(dst->name, src->name, sizeof(dst->name));
        dst->name[k_player_profile_name_length - 1] = 0;
        dst->player_color = src->player_color;

        for (i = 0; i < k_campaign_level_count; i++) {
            dst->campaign_progress[i] |= 0x0f;
        }

        memcpy(dst->keyboard_bindings, src->keyboard_bindings, sizeof(dst->keyboard_bindings));
        memcpy(dst->mouse_button_bindings, src->mouse_button_bindings, sizeof(dst->mouse_button_bindings));
        memcpy(dst->mouse_axis_bindings, src->mouse_axis_bindings, sizeof(dst->mouse_axis_bindings));

        dst->mouse_forward_scale = src->mouse_forward_scale;
        dst->mouse_strafe_scale = src->mouse_strafe_scale;
        dst->mouse_look_x_sensitivity = src->mouse_look_x_sensitivity;
        dst->mouse_look_y_sensitivity = src->mouse_look_y_sensitivity;
        dst->master_volume = src->master_volume;
        dst->effects_volume = src->effects_volume;
        dst->music_volume = src->music_volume;
    } else if (category == 0) {
        memcpy(dst->keyboard_bindings, src->keyboard_bindings, sizeof(dst->keyboard_bindings));
        memcpy(dst->mouse_button_bindings, src->mouse_button_bindings, sizeof(dst->mouse_button_bindings));
        memcpy(dst->mouse_axis_bindings, src->mouse_axis_bindings, sizeof(dst->mouse_axis_bindings));

        dst->mouse_forward_scale = src->mouse_forward_scale;
        dst->mouse_strafe_scale = src->mouse_strafe_scale;
        dst->mouse_look_x_sensitivity = src->mouse_look_x_sensitivity;
        dst->mouse_look_y_sensitivity = src->mouse_look_y_sensitivity;
    } else if (category == 1) {
        dst->gamepad_action_buttons[0][0] = src->gamepad_action_buttons[0][0];
        dst->gamepad_action_buttons[0][1] = src->gamepad_action_buttons[0][1];
        memcpy(dst->gamepad_button_bindings[0], src->gamepad_button_bindings[0],
               sizeof(dst->gamepad_button_bindings[0]));
        memcpy(dst->gamepad_axis_bindings[0], src->gamepad_axis_bindings[0],
               sizeof(dst->gamepad_axis_bindings[0]));
        memcpy(dst->gamepad_pov_bindings[0], src->gamepad_pov_bindings[0],
               sizeof(dst->gamepad_pov_bindings[0]));
        dst->gamepads[0] = src->gamepads[0];
        dst->gamepad_axis_scale_x = src->gamepad_axis_scale_x;
        dst->gamepad_axis_scale_y = src->gamepad_axis_scale_y;
        dst->gamepad_rate_a[0] = src->gamepad_rate_a[0];
        dst->gamepad_rate_b[0] = src->gamepad_rate_b[0];
    } else if (category == -1) {
        return 0;
    }

    dst->forward_rate = src->forward_rate;
    dst->strafe_rate = src->strafe_rate;
    dst->look_x_rate = src->look_x_rate;
    dst->look_y_rate = src->look_y_rate;
    dst->look_sensitivity = src->look_sensitivity;
    dst->look_inverted = src->look_inverted;
    dst->look_inverted_driving = src->look_inverted_driving;
    dst->auto_center_look = src->auto_center_look;
    return 1;
}

}

extern "C" { extern int16_t gamepad_action_buttons[k_control_gamepad_count][2]; }
namespace halo::input {

/**
 * Scans one device class's binding tables for an entry that maps to game control `action`, and
 * if one is found, caches it as that control's last-used binding (input_globals.last_used_
 * bindings[action]) -- unless action is out of the last-used-binding table's range, in which
 * case the scan still reports success but performs no write.
 *
 * device_class == 0 scans keyboard, then mouse buttons, then mouse axes (direction 2 checked
 * before direction 1, matching the binary). device_class 1..4 scans gamepad slot
 * (device_class - 1): first the accept/back (action 8/9) button shortcut, then buttons, then
 *
 * @address 0x48bae0
 */
uint8_t Bindings::refresh_last_used_binding(int32_t device_class, int16_t action)
{
    int16_t i;
    int32_t slot;
    int32_t device;
    int32_t count;
    int32_t octant;
    uint8_t found;
    int16_t special_button;

    if (device_class < 0 || device_class > 4) {
        return 0;
    }

    if (device_class == 0) {
        for (i = 0; i < (int16_t)k_control_keyboard_key_count; i++) {
            if (keyboard_bindings[i] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_keyboard;
                    input_globals.last_used_bindings[action].device_index = 0;
                    input_globals.last_used_bindings[action].input_kind = _control_input_button;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 0;
                }
                return 1;
            }
        }

        for (i = 0; i < k_control_mouse_button_count; i++) {
            if (mouse_button_bindings[i] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_mouse;
                    input_globals.last_used_bindings[action].device_index = 0;
                    input_globals.last_used_bindings[action].input_kind = _control_input_button;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 0;
                }
                return 1;
            }
        }

        for (i = 0; i < k_control_mouse_axis_count; i++) {
            if (mouse_axis_bindings[i][1] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_mouse;
                    input_globals.last_used_bindings[action].device_index = 0;
                    input_globals.last_used_bindings[action].input_kind = _control_input_axis;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 2;
                }
                return 1;
            }
            if (mouse_axis_bindings[i][0] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_mouse;
                    input_globals.last_used_bindings[action].device_index = 0;
                    input_globals.last_used_bindings[action].input_kind = _control_input_axis;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 1;
                }
                return 1;
            }
        }
        return 0;
    }

    slot = device_class - 1;
    count = 0;
    device = joystick_slot_devices[slot];
    if (device != -1) {
        count = input_devices[device].button_count;
    }

    found = 0;
    special_button = -1;
    if (action == _input_action_accept) {
        special_button = gamepad_action_buttons[slot][0];
    } else if (action == _input_action_back) {
        special_button = gamepad_action_buttons[slot][1];
    }
    if ((action == _input_action_accept || action == _input_action_back) && special_button != -1) {
        halo::input::input_last_used_binding_set(action, _control_device_gamepad, (int16_t)slot,
                                     _control_input_button, special_button, 0);
        found = 1;
    }

    if (!found) {
        for (i = 0; i < count; i++) {
            if (gamepad_button_bindings[slot][i] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_gamepad;
                    input_globals.last_used_bindings[action].device_index = (int16_t)slot;
                    input_globals.last_used_bindings[action].input_kind = _control_input_button;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 0;
                }
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        for (i = 0; i < count; i++) {
            if (gamepad_axis_bindings[slot][i][1] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_gamepad;
                    input_globals.last_used_bindings[action].device_index = (int16_t)slot;
                    input_globals.last_used_bindings[action].input_kind = _control_input_axis;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 2;
                }
                found = 1;
                break;
            }
            if (gamepad_axis_bindings[slot][i][0] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_gamepad;
                    input_globals.last_used_bindings[action].device_index = (int16_t)slot;
                    input_globals.last_used_bindings[action].input_kind = _control_input_axis;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 1;
                }
                found = 1;
                break;
            }
        }

        if (!found) {
            for (i = 0; i < count; i++) {
                for (octant = 0; octant < k_control_gamepad_pov_direction_count; octant++) {
                    if (gamepad_pov_bindings[slot][i][octant] == action) {
                        if (action >= 0 && action < k_input_action_count) {
                            input_globals.last_used_bindings[action].device_type = _control_device_gamepad;
                            input_globals.last_used_bindings[action].device_index = (int16_t)slot;
                            input_globals.last_used_bindings[action].input_kind = _control_input_pov;
                            input_globals.last_used_bindings[action].input_index = i;
                            input_globals.last_used_bindings[action].direction = octant;
                        }
                        return 1;
                    }
                }
            }
        }
    }

    return found;
}

}

extern "C" { extern void *mouse_device; }
extern "C" { extern mouse_state live_mouse_state; }
extern "C" { extern mouse_state mouse_neutral_state; }
namespace halo::input {

/**
 * Scans, in priority order, for the next "fresh" raw input activation: a mouse button just
 * pressed (hold count exactly 1), a keyboard key just pressed, a joystick button currently held
 * (any hold count), the mouse X/Y axes or wheel past k_input_scan_mouse_threshold, a joystick
 * axis whose delta from its scan_baselines snapshot exceeds k_input_scan_axis_threshold, or a
 * non-centered joystick POV. Writes the result into input_globals.scan_result (all zero if
 * nothing is found).
 *
 * @address 0x48f8c0
 */
void Bindings::scan_any_bound_input(void)
{
    control_binding_descriptor *result;
    int32_t slot;
    int32_t i;
    joystick_state *source;
    mouse_state *mouse;
    int32_t delta;

    result = &input_globals.scan_result;

    for (i = 0; i < k_input_mouse_button_count; i++) {
        if (mouse_device != 0 && input_suppressed == 0 && live_mouse_state.button_frames[i] == 1) {
            result->device_type = _control_device_mouse;
            result->device_index = 0;
            result->input_kind = _control_input_button;
            result->input_index = (int16_t)i;
            result->direction = 0;
            return;
        }
    }

    for (i = 0; i < k_control_keyboard_key_count; i++) {
        if (halo::input::input_get_key_state((int16_t)i) == 1) {
            result->device_type = _control_device_keyboard;
            result->device_index = 0;
            result->input_kind = _control_input_button;
            result->input_index = (int16_t)i;
            result->direction = 0;
            return;
        }
    }

    for (slot = 0; slot < 4; slot++) {
        if (joystick_slot_devices[slot] != -1) {
            source = (input_suppressed == 0) ? &joystick_states[slot] : &joystick_neutral_state;
            for (i = 0; i < input_devices[joystick_slot_devices[slot]].button_count; i++) {
                if (source->button_frames[i] != 0) {
                    result->device_type = _control_device_gamepad;
                    result->device_index = (int16_t)slot;
                    result->input_kind = _control_input_button;
                    result->input_index = (int16_t)i;
                    result->direction = 0;
                    return;
                }
            }
        }
    }

    mouse = (mouse_device == 0) ? (mouse_state *)0
            : (input_suppressed != 0) ? &mouse_neutral_state : &live_mouse_state;

    if (mouse->x > k_input_scan_mouse_threshold) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 0;
        result->direction = 1;
        return;
    }
    if (mouse->x < -k_input_scan_mouse_threshold) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 0;
        result->direction = 2;
        return;
    }

    if (mouse->y > k_input_scan_mouse_threshold) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 1;
        result->direction = 1;
        return;
    }

    if (mouse->y < -k_input_scan_mouse_threshold) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 1;
        result->direction = 2;
        return;
    }
    if (mouse->wheel > 0) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 2;
        result->direction = 1;
        return;
    }
    if (mouse->wheel < 0) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 2;
        result->direction = 2;
        return;
    }

    for (slot = 0; slot < 4; slot++) {
        if (joystick_slot_devices[slot] != -1) {
            source = (input_suppressed == 0) ? &joystick_states[slot] : &joystick_neutral_state;
            for (i = 0; i < input_devices[joystick_slot_devices[slot]].axis_count; i++) {
                delta = (int32_t)source->axes[i] - (int32_t)input_globals.scan_baselines[slot].axes[i];
                if (delta > k_input_scan_axis_threshold) {
                    result->device_type = _control_device_gamepad;
                    result->device_index = (int16_t)slot;
                    result->input_kind = _control_input_axis;
                    result->input_index = (int16_t)i;
                    result->direction = 1;
                    return;
                }
                if (delta < -k_input_scan_axis_threshold) {
                    result->device_type = _control_device_gamepad;
                    result->device_index = (int16_t)slot;
                    result->input_kind = _control_input_axis;
                    result->input_index = (int16_t)i;
                    result->direction = 2;
                    return;
                }
            }
        }
    }

    for (slot = 0; slot < 4; slot++) {
        if (joystick_slot_devices[slot] != -1) {
            source = (input_suppressed == 0) ? &joystick_states[slot] : &joystick_neutral_state;
            for (i = 0; i < input_devices[joystick_slot_devices[slot]].pov_count; i++) {
                if (source->povs[i] != -1) {
                    result->device_type = _control_device_gamepad;
                    result->device_index = (int16_t)slot;
                    result->input_kind = _control_input_pov;
                    result->input_index = (int16_t)i;
                    result->direction = source->povs[i];
                    return;
                }
            }
        }
    }

    result->device_type = 0;
    result->device_index = 0;
    result->input_kind = 0;
    result->input_index = 0;
    result->direction = 0;
}

}

namespace halo::input {

/**
 * Debug harness for input_device_default_profile_tag_find: parses device_id_ansi as a GUID
 * string and reports (via the verbose console) whether a matching InputDeviceDefaults tag was
 * found. Dead code in this build: nothing calls it.
 *
 * Original register convention: device id ANSI string in ECX.
 *
 * @address 0x490090
 */
void Bindings::test_input_device_defaults_find(char *device_id_ansi)
{
    input_guid guid;
    uint8_t saved_profile[k_saved_player_profile_size];
    int32_t tag_id;

    halo::input::input_guid_parse_ansi(&guid, device_id_ansi);
    tag_id = (int32_t)halo::input::input_device_default_profile_tag_find(guid, saved_profile);
    if (tag_id == -1) {
        halo::interface::console_printf_verbose((ColorARGB *)0, (char *)"deviceid %s has no default", device_id_ansi);
        return;
    }
    halo::interface::console_printf_verbose((ColorARGB *)0, (char *)"Default profile in tag %d", tag_id);
}

}
