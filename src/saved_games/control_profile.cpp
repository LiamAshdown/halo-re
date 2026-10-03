#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>
#include "halo/saved_games/saved_games.hpp"

extern "C" {
extern int32_t selected_saved_item;
extern int16_t control_keyboard_scan_table[k_control_keyboard_key_count];
extern int16_t control_mouse_button_scan_table[k_control_mouse_button_count];
extern int16_t control_mouse_axis_scan_table[k_control_mouse_axis_count][2];
extern int16_t control_gamepad_button_scan_table[k_control_gamepad_count][k_control_gamepad_button_count];
extern int16_t control_gamepad_axis_scan_table[k_control_gamepad_count][k_control_gamepad_axis_count][2];
extern int16_t control_gamepad_pov_scan_table[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count];
extern int32_t input_device_get_axis_count(int32_t device_index);
extern int32_t input_device_get_pov_count(int32_t device_index);
extern int32_t input_device_count;
extern int32_t input_device_to_slot[];
extern int32_t joystick_slot_devices[4];
extern int16_t input_device_find_index_by_guid(controls_gamepad_record *gamepad);
extern void *memcpy(void *dest, const void *src, uint32_t count);
extern uint8_t input_devices[];
extern int32_t input_device_default_profile_tag_find(input_guid guid, uint8_t *out_profile);
extern int16_t control_gamepad_action_scan_buttons[k_control_gamepad_count][2];
extern int16_t input_action_name_to_index(const char *action_name);
extern int32_t input_device_get_button_count(int32_t device_index);
extern saved_player_profile saved_item_working_copy;
extern network_thread_record *variant_write_thread;
extern variant_write_request variant_write_request_state;
}

/**
 * Unbinds whatever device slot the binding descriptor names, by writing the unbound marker into
 * the working profile (keyboard key, mouse button or axis, gamepad button, axis or pov).
 *
 * @address 0x0053ad00
 */
void halo::saved_games::ControlBinding::clear_binding() const
{
    const control_binding_descriptor *binding = self;
    if ((selected_saved_item & 0xf) != 0) {
        return;
    }

    if (binding->device_type == _control_device_keyboard) {
        control_keyboard_scan_table[binding->input_index] = (int16_t)k_control_binding_unbound;
        return;
    }

    if (binding->device_type == _control_device_mouse) {
        if (binding->input_kind != _control_input_axis) {
            control_mouse_button_scan_table[binding->input_index] = (int16_t)k_control_binding_unbound;
            return;
        }
        if (binding->direction != 1) {
            control_mouse_axis_scan_table[binding->input_index][1] = (int16_t)k_control_binding_unbound;
            return;
        }
        control_mouse_axis_scan_table[binding->input_index][0] = (int16_t)k_control_binding_unbound;
        return;
    }

    if (binding->device_type == _control_device_gamepad) {
        if (binding->input_kind == _control_input_axis) {
            int32_t index = binding->input_index;
            int32_t device = binding->device_index;

            if (index < input_device_get_axis_count(device)) {
                if (binding->direction != 1) {
                    control_gamepad_axis_scan_table[device][index][1] = (int16_t)k_control_binding_unbound;
                } else {
                    control_gamepad_axis_scan_table[device][index][0] = (int16_t)k_control_binding_unbound;
                }
            }
        } else if (binding->input_kind == _control_input_pov) {
            int32_t index = binding->input_index;
            int32_t device = binding->device_index;

            if (index < input_device_get_pov_count(device)) {
                control_gamepad_pov_scan_table[device][index][binding->direction] = (int16_t)k_control_binding_unbound;
            }
        } else {
            control_gamepad_button_scan_table[binding->device_index][binding->input_index] = (int16_t)k_control_binding_unbound;
        }
    }
}

/**
 * (dest first, source second)
 * Finds the gamepad slot matching key in both dest and source. If both are found and the
 * source slot has customized bindings, copies that slot's button, action-button, axis and pov
 * bindings plus its two rate bytes from source to dest. Returns 1 if the copy was made, 0
 * otherwise.
 * FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
 *
 * @address 0x0053b700
 */
uint8_t halo::saved_games::PlayerProfile::copy_gamepad_bindings_by_key(controls_gamepad_record *key,
    saved_player_profile *source)
{
    saved_player_profile *dest = self;
    int32_t dest_slot;
    int32_t source_slot;

    dest_slot = control_profile_gamepad_slot_find(dest, key);
    source_slot = control_profile_gamepad_slot_find(source, key);
    if (dest_slot != -1 && source_slot != -1) {
        if (control_profile_is_customized(source, source_slot)) {
            memcpy(dest->gamepad_button_bindings[dest_slot], source->gamepad_button_bindings[source_slot],
                   sizeof(dest->gamepad_button_bindings[dest_slot]));
            memcpy(dest->gamepad_action_buttons[dest_slot], source->gamepad_action_buttons[source_slot],
                   sizeof(dest->gamepad_action_buttons[dest_slot]));
            memcpy(dest->gamepad_axis_bindings[dest_slot], source->gamepad_axis_bindings[source_slot],
                   sizeof(dest->gamepad_axis_bindings[dest_slot]));
            memcpy(dest->gamepad_pov_bindings[dest_slot], source->gamepad_pov_bindings[source_slot],
                   sizeof(dest->gamepad_pov_bindings[dest_slot]));
            dest->gamepad_rate_a[dest_slot] = source->gamepad_rate_a[source_slot];
            dest->gamepad_rate_b[dest_slot] = source->gamepad_rate_b[source_slot];
            return 1;
        }
    }
    return 0;
}

/**
 * Looks up the first binding of the named action in the working profile and fills binding with
 * its device, index, kind and direction. Returns 1 when one was found.
 *
 * @address 0x0053aa20
 */
uint8_t halo::saved_games::ControlBinding::find_binding_for_action(const char *action_name)
{
    control_binding_descriptor *binding = self;
    int16_t action_index;

    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }

    action_index = input_action_name_to_index(action_name);
    if (action_index == (int16_t)k_control_binding_unbound) {
        return 0;
    }

    if (binding->device_type == _control_device_keyboard) {
        int32_t i;

        i = binding->input_index;
        if (i > 0x6c) {
            return 0;
        }
        while (control_keyboard_scan_table[i] != action_index) {
            i = i + 1;
            if (i > 0x6c) {
                return 0;
            }
        }
        binding->input_kind = _control_input_button;
        binding->input_index = (int16_t)i;
        return 1;
    }

    if (binding->device_type != _control_device_mouse) {
        int32_t device;
        int32_t button_count, axis_count, pov_count;
        int32_t i;
        uint8_t found;

        if (binding->device_type != _control_device_gamepad) {
            return 0;
        }

        device = binding->device_index;
        if (device < 0 || 3 < device) {
            return 0;
        }

        button_count = input_device_get_button_count(device);
        axis_count = input_device_get_axis_count(device);
        pov_count = input_device_get_pov_count(device);

        if (action_index == 8) {
            if (control_gamepad_action_scan_buttons[device][0] == -1) {
                return 0;
            }
            binding->input_kind = _control_input_button;
            binding->input_index = control_gamepad_action_scan_buttons[device][0];
            return 1;
        }
        if (action_index == 9) {
            if (control_gamepad_action_scan_buttons[device][1] == -1) {
                return 0;
            }
            binding->input_kind = _control_input_button;
            binding->input_index = control_gamepad_action_scan_buttons[device][1];
            return 1;
        }

        for (i = 0; i < button_count; i = i + 1) {
            if (control_gamepad_button_scan_table[device][i] == action_index) {
                binding->input_kind = _control_input_button;
                binding->input_index = (int16_t)i;
                return 1;
            }
        }

        for (i = 0; i < axis_count; i = i + 1) {
            if (control_gamepad_axis_scan_table[device][i][0] == action_index) {
                binding->input_kind = _control_input_axis;
                binding->direction = 1;
                binding->input_index = (int16_t)i;
                return 1;
            }
            if (control_gamepad_axis_scan_table[device][i][1] == action_index) {
                binding->input_kind = _control_input_axis;
                binding->direction = 2;
                binding->input_index = (int16_t)i;
                return 1;
            }
        }

        if (pov_count < 1) {
            return 0;
        }
        found = 0;
        for (i = 0; i < pov_count; i = i + 1) {
            int32_t d;

            for (d = 0; d < k_control_gamepad_pov_direction_count; d = d + 1) {
                if (control_gamepad_pov_scan_table[device][i][d] == action_index) {
                    binding->input_kind = _control_input_pov;
                    binding->direction = d;
                    binding->input_index = (int16_t)i;
                    found = 1;
                    break;
                }
            }
        }
        return found;
    }

    {
        int32_t i;

        for (i = 0; i < k_control_mouse_button_count; i = i + 1) {
            if (control_mouse_button_scan_table[i] == action_index) {
                binding->input_kind = _control_input_button;
                binding->input_index = (int16_t)i;
                return 1;
            }
        }
        for (i = 0; i < k_control_mouse_axis_count; i = i + 1) {
            if (control_mouse_axis_scan_table[i][0] == action_index) {
                binding->input_kind = _control_input_axis;
                binding->direction = 1;
                binding->input_index = (int16_t)i;
                return 1;
            }
            if (control_mouse_axis_scan_table[i][1] == action_index) {
                binding->input_kind = _control_input_axis;
                binding->direction = 2;
                binding->input_index = (int16_t)i;
                return 1;
            }
        }
        return 0;
    }
}

/**
 * Searches the profile's four gamepad slots for one whose device_key matches key's device_key
 * (the extra dword at [4] first, then the 16-byte guid at [0..3]). Returns the matching slot
 * index (0..3), or -1 if none match.
 *
 * @address 0x0053b6b0
 */
int32_t halo::saved_games::PlayerProfile::gamepad_slot_find(controls_gamepad_record *key)
{
    saved_player_profile *profile = self;
    int32_t i;
    controls_gamepad_record *slot;

    for (i = 0; i < k_control_gamepad_count; i++) {
        slot = &profile->gamepads[i];
        if (slot->product_instance == key->product_instance &&
            slot->product_guid.words[0] == key->product_guid.words[0] &&
            slot->product_guid.words[1] == key->product_guid.words[1] &&
            slot->product_guid.words[2] == key->product_guid.words[2] &&
            slot->product_guid.words[3] == key->product_guid.words[3]) {
            return i;
        }
    }
    return -1;
}

/**
 * Resets the mouse bindings of the profile: every mouse button and axis direction is unbound,
 * then the three default buttons and three default axis directions are bound.
 *
 * @address 0x0053a0d0
 */
void halo::saved_games::PlayerProfile::reset_analog_bindings()
{
    saved_player_profile *profile = self;
    int32_t i;

    for (i = 0; i < k_control_mouse_button_count; i = i + 1) {
        profile->mouse_button_bindings[i] = k_control_binding_unbound;
    }
    for (i = 0; i < k_control_mouse_axis_count; i = i + 1) {
        profile->mouse_axis_bindings[i][0] = k_control_binding_unbound;
        profile->mouse_axis_bindings[i][1] = k_control_binding_unbound;
    }

    profile->mouse_axis_bindings[0][1] = 0x19;
    profile->mouse_button_bindings[0] = 7;
    profile->mouse_button_bindings[2] = 6;
    profile->mouse_button_bindings[1] = 0xb;
    profile->mouse_axis_bindings[1][0] = 0x17;
    profile->mouse_axis_bindings[1][1] = 0x18;
    profile->mouse_axis_bindings[0][0] = 0x1a;
}

/**
 * Resets the keyboard bindings of the profile: all 109 key slots are unbound, then the 21
 * default keys receive their default action ids.
 *
 * @address 0x00539ff0
 */
void halo::saved_games::PlayerProfile::reset_digital_bindings()
{
    saved_player_profile *profile = self;
    int32_t i;

    for (i = 0; i < k_control_keyboard_key_count; i = i + 1) {
        profile->keyboard_bindings[i] = k_control_binding_unbound;
    }

    profile->keyboard_bindings[0] = 9;
    profile->keyboard_bindings[32] = 0x13;
    profile->keyboard_bindings[46] = 0x14;
    profile->keyboard_bindings[45] = 0x15;
    profile->keyboard_bindings[47] = 0x16;
    profile->keyboard_bindings[49] = 1;
    profile->keyboard_bindings[30] = 3;
    profile->keyboard_bindings[34] = 0xd;
    profile->keyboard_bindings[48] = 4;
    profile->keyboard_bindings[59] = 0xe;
    profile->keyboard_bindings[72] = 0;
    profile->keyboard_bindings[69] = 10;
    profile->keyboard_bindings[31] = 5;
    profile->keyboard_bindings[58] = 0xb;
    profile->keyboard_bindings[33] = 2;
    profile->keyboard_bindings[56] = 8;
    profile->keyboard_bindings[35] = 0xf;
    profile->keyboard_bindings[36] = 0x10;
    profile->keyboard_bindings[50] = 0x11;
    profile->keyboard_bindings[1] = 0xc;
    profile->keyboard_bindings[13] = 0x12;
}

/**
 * VERIFIED against disassembly 0x53ae10..0x53aff8 (2026-09-30); fixed: an unknown device type returns 0, not 1.
 *
 * @address 0x0053ae10
 */
uint8_t halo::saved_games::ControlBinding::set_binding(int16_t value) const
{
    const control_binding_descriptor *binding = self;
    saved_player_profile *profile;

    if ((selected_saved_item & 0xf) != 0) {
        return 1;
    }
    profile = &saved_item_working_copy;

    if (binding->device_type == _control_device_keyboard) {
        profile->keyboard_bindings[binding->input_index] = value;
        return 1;
    }

    if (binding->device_type == _control_device_mouse) {
        if (binding->input_kind != _control_input_axis) {
            profile->mouse_button_bindings[binding->input_index] = value;
            return 1;
        }
        if (binding->direction != 1) {
            profile->mouse_axis_bindings[binding->input_index][1] = value;
            return 1;
        }
        profile->mouse_axis_bindings[binding->input_index][0] = value;
        return 1;
    }

    if (binding->device_type == _control_device_gamepad) {
        int32_t device = binding->device_index;

        if (binding->input_kind == _control_input_axis) {
            int32_t index = binding->input_index;

            if (index < input_device_get_axis_count(device)) {
                if (binding->direction != 1) {
                    profile->gamepad_axis_bindings[device][index][1] = value;
                } else {
                    profile->gamepad_axis_bindings[device][index][0] = value;
                }
                return 1;
            }
        } else if (binding->input_kind == _control_input_pov) {
            int32_t index = binding->input_index;

            if (index < input_device_get_pov_count(device)) {
                profile->gamepad_pov_bindings[device][index][binding->direction] = value;
                return 1;
            }
        } else {
            int32_t index = binding->input_index;

            if (index < input_device_get_button_count(device)) {
                if (value == 8) {
                    profile->gamepad_action_buttons[device][0] = (int16_t)index;
                    if (profile->gamepad_action_buttons[device][1] == (int16_t)index) {
                        profile->gamepad_action_buttons[device][1] = (int16_t)k_control_binding_unbound;
                    }
                    return 1;
                }
                if (value == 9) {
                    profile->gamepad_action_buttons[device][1] = (int16_t)index;
                    if (profile->gamepad_action_buttons[device][0] == (int16_t)index) {
                        profile->gamepad_action_buttons[device][0] = (int16_t)k_control_binding_unbound;
                    }
                    return 1;
                }
                profile->gamepad_button_bindings[device][index] = value;
                return 1;
            }
        }
        return 0;
    }

    return 0;
}

namespace halo::saved_games::control_profile {

/**
 * Clears every device-to-profile-slot mapping that refers to gamepad slots of profile. Does
 * nothing for a null profile.
 *
 * @address 0x0053b5a0
 */
void clear_device_slot_mappings(saved_player_profile *profile)
{
    int32_t count;
    uint8_t *entry;
    int16_t device_index;
    int32_t slot;

    if (profile == 0) {
        return;
    }
    count = (int16_t)input_device_count;
    entry = (uint8_t *)profile + 0x1108;
    while (0 < count) {
        device_index = input_device_find_index_by_guid((controls_gamepad_record *)entry);
        if (device_index != -1 && device_index < input_device_count ) {
            slot = input_device_to_slot[device_index * 0x90];
            if (slot != -1) {
                input_device_to_slot[device_index * 0x90] = -1;
                joystick_slot_devices[slot] = -1;
            }
        }
        entry = entry + 0x220;
        count = count - 1;
    }
}

/**
 * Counts the profile's already-used gamepad slots (0..4, by the first word of each of the four
 * controls_gamepad_record entries at +0x1108). While fewer than 4 are used, walks the connected
 * device table once trying to add each device whose GUID resolves to a device_defaults tag
 * (input_device_default_profile_tag_find), then, if slots are still free, walks the table again
 * unconditionally. Both passes hand the copied device record to control_profile_find_or_create_gamepad_slot, which finds or
 * reuses a free profile slot and stores the device's binding data into it.
 *
 * @address 0x0053b7f0
 */
void fill_default_gamepad_slots(saved_player_profile *profile)
{
    uint32_t used_count;
    int32_t device_count;
    int32_t i;
    controls_gamepad_record entry;
    uint8_t have_entry;
    int32_t tag_index;
    uint8_t added;
    input_guid key;
    uint8_t tag_scratch[0x1ffc];

    used_count = 0;
    if (0 < (int16_t)input_device_count) {
        if (profile != 0) {
            used_count = (profile->gamepads[0].name[0] != 0);
            if (profile->gamepads[1].name[0] != 0) {
                used_count = used_count + 1;
            }
            if (profile->gamepads[2].name[0] != 0) {
                used_count = used_count + 1;
            }
            if (profile->gamepads[3].name[0] != 0) {
                used_count = used_count + 1;
            }
        }
        device_count = (int32_t)(int16_t)input_device_count;

        i = 0;
        have_entry = 0;
        if (0 < device_count) {
            do {
                if (3 < (int32_t)used_count) {
                    break;
                }
                if ((int16_t)i < input_device_count) {
                    memcpy(&entry, input_devices + (int16_t)i * 0x240, sizeof(entry));
                    have_entry = 1;
pass1_try_add:
                    key = entry.product_guid;
                    tag_index = input_device_default_profile_tag_find(key, tag_scratch);
                    if (tag_index != -1) {
                        added = control_profile_find_or_create_gamepad_slot(&entry, profile);
                        if (added != 0) {
                            used_count = used_count + 1;
                        }
                    }
                } else if (have_entry != 0) {
                    goto pass1_try_add;
                }
                i = i + 1;
            } while (i < device_count);
        }

        i = 0;
        have_entry = 0;
        if (0 < device_count) {
            do {
                if (3 < (int32_t)used_count) {
                    return;
                }
                if ((int16_t)i < input_device_count) {
                    memcpy(&entry, input_devices + (int16_t)i * 0x240, sizeof(entry));
                    have_entry = 1;
pass2_try_add:
                    added = control_profile_find_or_create_gamepad_slot(&entry, profile);
                    if (added != 0) {
                        used_count = used_count + 1;
                    }
                } else if (have_entry != 0) {
                    goto pass2_try_add;
                }
                i = i + 1;
            } while (i < device_count);
        }
    }
    return;
}

/**
 * Finalizes a newly written gamepad slot of profile by validating its device identity (the four
 * device key dwords). Returns 1 on success, 0 for a null profile or a bad slot index.
 *
 * @address 0x0053b500
 */
uint8_t finalize_slot(saved_player_profile *profile, int32_t gamepad_index)
{
    controls_gamepad_record *slot;
    saved_player_profile template_profile;
    input_guid guid;

    if (profile == 0 || gamepad_index < 0 || 4 <= gamepad_index) {
        return 0;
    }
    slot = &profile->gamepads[gamepad_index];
    if (slot->name[0] == 0) {
        return 0;
    }

    guid.words[0] = slot->product_guid.words[0];
    guid.words[1] = slot->product_guid.words[1];
    guid.words[2] = slot->product_guid.words[2];
    guid.words[3] = slot->product_guid.words[3];
    if (input_device_default_profile_tag_find(guid, (uint8_t *)&template_profile) != -1) {
        return (uint8_t)control_profile_copy_gamepad_bindings_by_key(slot, profile, &template_profile);
    }
    return 0;
}

/**
 * VERIFIED against disassembly 0x53b470..0x53b4f4 (2026-09-30): the NULL/duplicate early exits (return 0), the first free slot
 *   test (first 16-bit unit of the 0x220 byte record == 0), reset / 0x88-dword copy / finalize order and the return values match;
 *   the callee register conventions (find: EDX profile, EBX key; reset: ESI, EDX; finalize: EDI + stack) are as the callees declare.
 *   A difftest crash here would come from one of those three callees on random data.
 *
 * @address 0x0053b470
 */
uint8_t find_or_create_gamepad_slot(controls_gamepad_record *source, saved_player_profile *profile)
{
    int32_t i;

    if (profile == 0 || control_profile_gamepad_slot_find(profile, source) != -1) {
        return 0;
    }

    for (i = 0; i < k_control_gamepad_count; i = i + 1) {
        if (profile->gamepads[i].name[0] == 0) {
            control_profile_reset_slot(profile, i);
            profile->gamepads[i] = *source;
            control_profile_finalize_slot(profile, i);
            return 1;
        }
    }
    return 0;
}

/**
 * True if the gamepad slot is in use (a nonzero first name word) and any of its button, action,
 * axis or pov bindings differs from the unbound default.
 *
 * @address 0x0053b370
 */
uint8_t is_customized(saved_player_profile *profile, int32_t gamepad_index)
{
    int32_t i, j;

    if (profile == 0 || gamepad_index < 0 || 4 <= gamepad_index) {
        return 0;
    }
    if (profile->gamepads[gamepad_index].name[0] == 0) {
        return 0;
    }

    for (i = 0; i < k_control_gamepad_button_count; i = i + 1) {
        if (profile->gamepad_button_bindings[gamepad_index][i] != k_control_binding_unbound) {
            return 1;
        }
    }
    for (i = 0; i < 2; i = i + 1) {
        if (profile->gamepad_action_buttons[gamepad_index][i] != -1) {
            return 1;
        }
    }
    for (i = 0; i < k_control_gamepad_axis_count; i = i + 1) {
        if (profile->gamepad_axis_bindings[gamepad_index][i][0] != k_control_binding_unbound ||
            profile->gamepad_axis_bindings[gamepad_index][i][1] != k_control_binding_unbound) {
            return 1;
        }
    }
    for (i = 0; i < k_control_gamepad_pov_count; i = i + 1) {
        for (j = 0; j < k_control_gamepad_pov_direction_count; j = j + 1) {
            if (profile->gamepad_pov_bindings[gamepad_index][i][j] != k_control_binding_unbound) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Rebuilds the device-to-profile-slot mappings of profile. Existing mappings are cleared first,
 * then each used gamepad slot is matched to a connected device.
 *
 * @address 0x0053b620
 */
void reestablish_device_slot_mappings(saved_player_profile *profile)
{
    int32_t slot;
    int16_t device_index;

    if (profile == 0) {
        return;
    }
    control_profile_clear_device_slot_mappings(profile);

    for (slot = 0; slot < k_control_gamepad_count; slot = slot + 1) {
        if (profile->gamepads[slot].name[0] != 0) {
            device_index = input_device_find_index_by_guid(&profile->gamepads[slot]);
            if (device_index != -1 && device_index < input_device_count  &&
                input_device_to_slot[device_index * 0x90] == -1 &&
                joystick_slot_devices[slot] == -1) {
                input_device_to_slot[device_index * 0x90] = slot;
                joystick_slot_devices[slot] = device_index;
            }
        }
    }
}

/**
 * Zeroes the gamepad record of one slot and unbinds every button, action, axis and pov binding
 * for it. The same reset player_profile_initialize applies to all four slots.
 *
 * @address 0x0053b2b0
 */
void reset_slot(saved_player_profile *profile, int32_t gamepad_index)
{
    uint32_t *zero;
    int32_t i, j;

    if (profile == 0 || gamepad_index < 0 || 4 <= gamepad_index) {
        return;
    }

    zero = (uint32_t *)&profile->gamepads[gamepad_index];
    for (i = 0x88; i != 0; i = i - 1) {
        *zero = 0;
        zero = zero + 1;
    }

    for (i = 0; i < k_control_gamepad_button_count; i = i + 1) {
        profile->gamepad_button_bindings[gamepad_index][i] = k_control_binding_unbound;
    }
    profile->gamepad_action_buttons[gamepad_index][0] = -1;
    profile->gamepad_action_buttons[gamepad_index][1] = -1;
    for (i = 0; i < k_control_gamepad_axis_count; i = i + 1) {
        profile->gamepad_axis_bindings[gamepad_index][i][0] = k_control_binding_unbound;
        profile->gamepad_axis_bindings[gamepad_index][i][1] = k_control_binding_unbound;
    }
    for (i = 0; i < k_control_gamepad_pov_count; i = i + 1) {
        for (j = 0; j < k_control_gamepad_pov_direction_count; j = j + 1) {
            profile->gamepad_pov_bindings[gamepad_index][i][j] = k_control_binding_unbound;
        }
    }
}

/**
 * reaches through variant_write_thread
 * and the low word of default_game_variant_count)
 * Blocks until the asynchronous game-variant writer thread (if any) has exited, closes its
 * handle and clears its thread-table record, then zeroes the shared variant-write parameter
 * block (and the thread pointer / default-count word immediately after it).
 *
 * @address 0x0053bae0
 */
void variant_write_wait_and_clear(void)
{
    uint32_t exit_code;
    uint8_t *zero_cursor;
    int32_t i;
    int32_t got_code;

    if (variant_write_thread != 0) {
        do {
            do {
                got_code = GetExitCodeThread(variant_write_thread->handle, (LPDWORD)&exit_code);
            } while (got_code == 0);
        } while (exit_code == 0x103);
        CloseHandle(variant_write_thread->handle);
        variant_write_thread->handle = 0;
        variant_write_thread->in_use = 0;
    }
    zero_cursor = (uint8_t *)&variant_write_request_state;
    for (i = 0x29; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }
    return;
}

}  // namespace halo::saved_games::control_profile
