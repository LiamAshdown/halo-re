#include "halo/interface/ifr1_hud_frame.hpp"
#include <wchar.h>
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/camera/api.hpp"

extern "C" {
extern data_array *object_data;
extern player_globals *local_player_globals;
extern data_array *player_data;
extern int16_t current_local_player_index;
extern hud_messaging_globals *hud_messaging;
extern void hud_set_player_message(int16_t message_index, int16_t local_player_index);
extern void hud_set_message_string_argument(int16_t local_player_index, int16_t slot, int16_t string_index,
                                            uint8_t from_scenario_names);
extern void hud_set_message_icon_argument(int16_t local_player_index, int16_t slot,
                                          const hud_messaging_information *information);
extern void hud_set_action_text_shown(int16_t local_player_index, uint8_t shown);
extern int16_t object_get_hud_text_message_index(datum_index object_index);
extern uint8_t weapon_hud_ammo_state_is_empty(const weapon_hud_ammo_state *state);
extern uint8_t game_engine_pick_hud_hint(datum_index player_index, int32_t maximum_length, uint16_t *out_text);
extern int16_t unit_count_deployed_weapons(datum_index unit_index);
extern int16_t unit_find_next_zone_permitted_weapon_slot(datum_index unit_index, int32_t start_slot, int16_t direction);
extern datum_index unit_get_weapon_object_index(datum_index unit_index, int16_t slot_index);
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern hud_globals_flags *hud_flags;
extern uint8_t current_game_engine;
extern uint8_t motion_sensor_override_value;
extern uint8_t game_engine_teams_enabled_flag;
extern uint8_t *cinematic_globals_ptr;
extern game_time_globals *game_time;
extern void hud_draw_weapon_interface(player *p);
extern void hud_update_interaction_prompt(datum_index player_index);
extern void hud_unit_sounds_update(player *p, uint8_t hud_enabled);
extern void hud_render_unit_interface(player *p);
extern void hud_waypoints_draw_for_player(int16_t local_player_index);
extern void hud_draw_damage_indicators(int16_t local_player_index);
extern void hud_messaging_update(int16_t local_player_index);
extern void hud_waypoint_draw_all_for_player(void);
extern void chimera__motion_sensor_update(void);
}

static uint8_t *object_get(datum_index object_index)
{
    return (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
}

static uint8_t *object_tag_data(datum_index object_index)
{
    return (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)object_get(object_index) & 0xffff].data;
}

static const int16_t *weapon_hud_messaging(const uint8_t *weapon_object)
{
    datum_index weapon_tag = *(const datum_index *)weapon_object;
    datum_index hud = *(datum_index *)((uint8_t *)halo::cache::globals().tag_instances[weapon_tag & 0xffff].data + 0x48c);
    const int16_t *messaging;

    if (hud == (datum_index)-1) {
        return 0;
    }
    messaging = (const int16_t *)((uint8_t *)halo::cache::globals().tag_instances[hud & 0xffff].data + 0x13c);
    return (*messaging == -1) ? 0 : messaging;
}

namespace halo::interface {

/**
 * Original engine function hud_update_interaction_prompt; the author notes are in
 * docs/original/interface/hud_update_interaction_prompt.txt.
 * blam-cc: player_index -> EDX
 *
 * @address 0x4a9b80
 */
void HudFrame::update_interaction_prompt(datum_index player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    int16_t local = current_local_player_index;
    int16_t target_message;

    if (local_player_globals->mode != 0 && p->unit == (datum_index)-1) {
        static const int16_t mode_message[4] = { 0xb, 0xa, 0x9, 0xc };
        hud_set_player_message(mode_message[local_player_globals->mode - 1], (uint16_t)local);
        return;
    }

    if (p->interaction_object == (datum_index)-1) {
        target_message = -1;
    } else {
        target_message = *(int16_t *)(object_tag_data(p->interaction_object) + 0x13c);
    }

    switch (p->interaction_type) {
    case 1:
    case 2:
        hud_set_player_message(0, (uint16_t)local);
        hud_set_message_string_argument(local, 0, target_message, 0);
        return;

    case 3: {
        uint8_t *unit_object = object_get(p->unit);
        hud_set_player_message(7, (uint16_t)local);
        hud_set_message_string_argument(local, 0,
            object_get_hud_text_message_index(*(datum_index *)(unit_object + 0x11c)), 0);
        return;
    }

    case 5: {
        uint8_t *unit_object = object_get(p->unit);
        hud_set_player_message(1, (uint16_t)local);
        hud_set_message_string_argument(local, 0,
            object_get_hud_text_message_index(*(datum_index *)(unit_object + 0x318)), 0);
        hud_set_message_string_argument(local, 1, target_message, 0);
        return;
    }

    case 6:
    case 7: {
        uint8_t *weapon = (uint8_t *)object_try_and_get(p->interaction_object, 4);
        const int16_t *messaging;
        if (weapon == 0) {
            return;
        }
        messaging = weapon_hud_messaging(weapon);
        hud_set_player_message((p->interaction_type == 6) ? 4 : 0, (uint16_t)local);
        if (messaging == 0) {
            hud_set_message_string_argument(local, 0, target_message, 0);
        } else {
            hud_set_message_icon_argument(local, 0, (const hud_messaging_information *)messaging);
        }
        return;
    }

    case 8:
    case 9: {
        uint8_t *seats;
        int16_t seat_message;
        hud_set_player_message(6, (uint16_t)local);
        seats = *(uint8_t **)(object_tag_data(p->interaction_object) + 0x2e8);
        seat_message = *(int16_t *)(seats + p->interaction_seat * 0x11c + 0xec);
        hud_set_message_string_argument(local, 0, seat_message, 0);
        hud_set_message_string_argument(local, 1, target_message, 0);
        return;
    }

    case 10: {
        uint16_t vehicle_string = *(uint16_t *)(object_get(p->interaction_object) + 0x218);
        if (vehicle_string != 0xffff) {
            hud_set_player_message(3, (uint16_t)local);
            hud_set_message_string_argument(local, 0, (int16_t)vehicle_string, 1);
        } else {
            hud_set_player_message(2, (uint16_t)local);
            hud_set_message_string_argument(local, 0, target_message, 0);
        }
        return;
    }

    case 11:
        hud_set_player_message(8, (uint16_t)local);
        hud_set_message_string_argument(local, 0, object_get_hud_text_message_index(p->interaction_object), 0);
        return;

    default:
        break;
    }

    {
        uint16_t hint_text[0x400];
        if (game_engine_pick_hud_hint(player_index, 0x400, hint_text)) {
            hud_player_messaging_state *msg = &hud_messaging->players[0] + local;
            hud_set_action_text_shown(local, 1);
            wcsncpy((wchar_t *)msg->action_text, (const wchar_t *)hint_text, 0xff);
            msg->action_text[0xff] = 0;
            return;
        }
    }

    if (p->unit == (datum_index)-1) {
        hud_player_messaging_state *msg = &hud_messaging->players[0] + local;
        msg->prompt_changed = msg->prompt_changed | (msg->message_shown != 0);
        msg->message_shown = 0;
        msg->message = 0;
        msg->message_shown_copy = 0;
        return;
    }

    {
        datum_index unit_index = p->unit;
        uint8_t *unit_object = object_get(unit_index);
        unit_data *unit = (unit_data *)(unit_object + k_unit_data_offset);
        datum_index current_weapon = unit_get_weapon_object_index(unit_index, unit->current_weapon_index);
        datum_index parent = *(datum_index *)(unit_object + 0x11c);
        uint8_t can_switch = 1;
        weapon_hud_ammo_state ammo;

        if (parent != (datum_index)-1 && unit->vehicle_seat_index != -1) {
            uint8_t *seats = *(uint8_t **)(object_tag_data(parent) + 0x2e8);
            can_switch = ((seats[unit->vehicle_seat_index * 0x11c] & 0xc) == 0);
        }

        if (current_weapon != (datum_index)-1 && can_switch) {
            halo::items::weapon_build_hud_ammo_state(current_weapon, &ammo);
            if (weapon_hud_ammo_state_is_empty(&ammo)) {
                int16_t slot = unit->current_weapon_index;
                int16_t remaining = unit_count_deployed_weapons(unit_index);
                datum_index candidate;

                for (;;) {
                    slot = unit_find_next_zone_permitted_weapon_slot(unit_index, slot, 1);
                    unit_index = p->unit;
                    candidate = unit_get_weapon_object_index(unit_index, slot);
                    halo::items::weapon_build_hud_ammo_state(candidate, &ammo);
                    if (!weapon_hud_ammo_state_is_empty(&ammo)) {
                        break;
                    }
                    if (candidate == current_weapon) {
                        break;
                    }
                    remaining = remaining - 1;
                    if (remaining != 0) {
                        break;
                    }
                }

                if (!weapon_hud_ammo_state_is_empty(&ammo) && candidate != current_weapon) {
                    uint8_t *weapon;
                    const int16_t *messaging;
                    hud_set_player_message(5, (uint16_t)local);
                    weapon = (uint8_t *)object_try_and_get(candidate, 4);
                    if (weapon != 0) {
                        messaging = weapon_hud_messaging(weapon);
                        if (messaging != 0) {
                            hud_set_message_icon_argument(local, 0, (const hud_messaging_information *)messaging);
                        } else {
                            hud_set_message_string_argument(local, 0, target_message, 0);
                        }
                        return;
                    }
                }
            }
        }
    }

    hud_set_action_text_shown(local, 0);
}

/**
 * Updates and renders the full player HUD for the currently displayed local player: waypoints and motion
 * sensor first (subject to several unrelated gates), then either the full unit-driving HUD (spectate overlay,
 * weapon message state, unit interface, ammo/name overlay) or, if not currently driving a unit or the camera
 * is in a cutscene/first-person-only mode, just the weapon message state and a bare messaging update.
 *
 * @address 0x4a99f0
 */
void HudFrame::update_player(void)
{
    int16_t local_player_index = current_local_player_index;
    datum_index player_index;
    int16_t camera_type;

    if (local_player_index == -1 || local_player_index > 0) {
        player_index = (datum_index)-1;
    } else {
        player_index = local_player_globals->local_players[local_player_index];
    }
    camera_type = halo::camera::camera_get_type_for_player(local_player_index);

    if (player_index == (datum_index)-1) {
        return;
    }

    {
        player *local_player = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

        if ((current_game_engine == 0 || ((motion_sensor_override_value & 2) != 0 && game_engine_teams_enabled_flag != 0)) &&
            cinematic_globals_ptr[9] == 0) {
            hud_waypoint_draw_all_for_player();
        }

        if (game_time->paused == 0) {
            int16_t expected = (local_player_globals->local_players[0] != (datum_index)-1) ? 0 : -1;
            if (current_local_player_index == expected) {
                chimera__motion_sensor_update();
            }
        }

        if (hud_flags->hud_enabled == 0) {
            hud_unit_sounds_update(local_player, 0);
            hud_messaging_update(current_local_player_index);
            return;
        }

        if (camera_type != 3 && camera_type != 2 && local_player->unit != (datum_index)-1) {
            hud_draw_weapon_interface(local_player);
            hud_update_interaction_prompt(player_index);
            hud_unit_sounds_update(local_player, hud_flags->hud_enabled);
            hud_render_unit_interface(local_player);
            hud_waypoints_draw_for_player(current_local_player_index);
            hud_draw_damage_indicators(current_local_player_index);
            hud_messaging_update(current_local_player_index);
            return;
        }

        hud_update_interaction_prompt(player_index);
        hud_unit_sounds_update(local_player, hud_flags->hud_enabled);
        hud_messaging_update(current_local_player_index);
    }
}

}
