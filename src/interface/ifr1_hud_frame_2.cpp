#include "halo/interface/ifr1_hud_frame.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include <wchar.h>
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &current_local_player_index = halo::link::ref<int16_t>(halo::ui::vars().current_local_player_index);
static auto &hud_messaging = halo::link::ref<hud_messaging_globals *>(halo::ui::vars().hud_messaging);
static auto &hud_flags = halo::link::ref<hud_globals_flags *>(halo::ui::vars().hud_flags);
static auto &current_game_engine = halo::link::ref<uint8_t>(halo::game::vars().current_game_engine);
static auto &motion_sensor_override_value = halo::link::ref<uint8_t>(halo::ui::vars().motion_sensor_override_value);
static auto &cinematic_globals_ptr = halo::link::ref<uint8_t *>(halo::game::vars().cinematic_globals_ptr);
#include "halo/interface/constants.hpp"
#include "halo/interface/flags.hpp"

static unit_object *unit_get(datum_index object_index)
{
    return halo::interface::object_record<unit_object>(object_index);
}

static Unit *object_tag_data(datum_index object_index)
{
    return halo::interface::tag_data<Unit>(halo::interface::object_record<object>(object_index)->definition_tag);
}

static const int16_t *weapon_hud_messaging(const object *weapon_object)
{
    datum_index hud = halo::interface::tag_handle(halo::interface::tag_data<Weapon>(weapon_object->definition_tag)->hud_interface.tag_id);
    const int16_t *messaging;

    if (hud == (datum_index)-1) {
        return 0;
    }
    static_assert(offsetof(WeaponHUDInterface, messaging_information_sequence_index) == 0x13c, "WeaponHUDInterface messaging block");
    messaging = (const int16_t *)&halo::interface::tag_data<WeaponHUDInterface>(hud)->messaging_information_sequence_index;
    return (*messaging == -1) ? 0 : messaging;
}

namespace halo::interface {

/**
 *
 * blam-cc: player_index -> EDX
 *
 * @address 0x4a9b80
 */
void HudFrame::update_interaction_prompt(datum_index player_index)
{
    player *p = halo::interface::player_record(player_index);
    int16_t local = current_local_player_index;
    int16_t target_message;

    if (halo::game::globals().local_player_globals->mode != 0 && p->unit == (datum_index)-1) {
        static const int16_t mode_message[4] = { 0xb, 0xa, 0x9, 0xc };
        halo::interface::hud_set_player_message(mode_message[halo::game::globals().local_player_globals->mode - 1], (uint16_t)local);
        return;
    }

    if (p->interaction_object == (datum_index)-1) {
        target_message = -1;
    } else {
        target_message = object_tag_data(p->interaction_object)->base.hud_text_message_index;
    }

    switch (p->interaction_type) {
    case 1:
    case 2:
        halo::interface::hud_set_player_message(0, (uint16_t)local);
        halo::interface::hud_set_message_string_argument(local, 0, target_message, 0);
        return;

    case 3: {
        unit_object *unit = unit_get(p->unit);
        halo::interface::hud_set_player_message(7, (uint16_t)local);
        halo::interface::hud_set_message_string_argument(local, 0,
            halo::interface::object_get_hud_text_message_index(unit->base.parent_object), 0);
        return;
    }

    case 5: {
        unit_object *unit = unit_get(p->unit);
        halo::interface::hud_set_player_message(1, (uint16_t)local);
        halo::interface::hud_set_message_string_argument(local, 0,
            halo::interface::object_get_hud_text_message_index(unit->unit.equipment_object_index), 0);
        halo::interface::hud_set_message_string_argument(local, 1, target_message, 0);
        return;
    }

    case 6:
    case 7: {
        object *weapon = (object *)halo::objects::object_try_and_get(p->interaction_object, 4);
        const int16_t *messaging;
        if (weapon == 0) {
            return;
        }
        messaging = weapon_hud_messaging(weapon);
        halo::interface::hud_set_player_message((p->interaction_type == 6) ? 4 : 0, (uint16_t)local);
        if (messaging == 0) {
            halo::interface::hud_set_message_string_argument(local, 0, target_message, 0);
        } else {
            halo::interface::hud_set_message_icon_argument(local, 0, (const hud_messaging_information *)messaging);
        }
        return;
    }

    case 8:
    case 9: {
        UnitSeat *seats;
        int16_t seat_message;
        halo::interface::hud_set_player_message(6, (uint16_t)local);
        seats = halo::interface::reflexive_elements<UnitSeat>(object_tag_data(p->interaction_object)->seats);
        seat_message = (int16_t)seats[p->interaction_seat].hud_text_message_index;
        halo::interface::hud_set_message_string_argument(local, 0, seat_message, 0);
        halo::interface::hud_set_message_string_argument(local, 1, target_message, 0);
        return;
    }

    case 10: {
        uint16_t vehicle_string = (uint16_t)unit_get(p->interaction_object)->unit.controlling_player;
        if (vehicle_string != halo::k_word_none) {
            halo::interface::hud_set_player_message(3, (uint16_t)local);
            halo::interface::hud_set_message_string_argument(local, 0, (int16_t)vehicle_string, 1);
        } else {
            halo::interface::hud_set_player_message(2, (uint16_t)local);
            halo::interface::hud_set_message_string_argument(local, 0, target_message, 0);
        }
        return;
    }

    case 11:
        halo::interface::hud_set_player_message(8, (uint16_t)local);
        halo::interface::hud_set_message_string_argument(local, 0, halo::interface::object_get_hud_text_message_index(p->interaction_object), 0);
        return;

    default:
        break;
    }

    {
        uint16_t hint_text[halo::interface::k_hint_text_chars];
        if (halo::game::game_engine_pick_hud_hint(player_index, halo::interface::k_hint_text_chars, hint_text)) {
            hud_player_messaging_state *msg = &hud_messaging->players[0] + local;
            halo::interface::hud_set_action_text_shown(local, 1);
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
        unit_object *unit_record = unit_get(unit_index);
        unit_data *unit = &unit_record->unit;
        datum_index current_weapon = halo::units::unit_get_weapon_object_index(unit_index, unit->current_weapon_index);
        datum_index parent = unit_record->base.parent_object;
        uint8_t can_switch = 1;
        weapon_hud_ammo_state ammo;

        if (parent != (datum_index)-1 && unit->vehicle_seat_index != -1) {
            UnitSeat *seats = halo::interface::reflexive_elements<UnitSeat>(object_tag_data(parent)->seats);
            can_switch = ((seats[unit->vehicle_seat_index].flags & 0xc) == 0);
        }

        if (current_weapon != (datum_index)-1 && can_switch) {
            halo::items::weapon_build_hud_ammo_state(current_weapon, &ammo);
            if (halo::interface::weapon_hud_ammo_state_is_empty(&ammo)) {
                int16_t slot = unit->current_weapon_index;
                int16_t remaining = halo::units::unit_count_deployed_weapons(unit_index);
                datum_index candidate;

                for (;;) {
                    slot = halo::units::unit_find_next_zone_permitted_weapon_slot(unit_index, slot, 1);
                    unit_index = p->unit;
                    candidate = halo::units::unit_get_weapon_object_index(unit_index, slot);
                    halo::items::weapon_build_hud_ammo_state(candidate, &ammo);
                    if (!halo::interface::weapon_hud_ammo_state_is_empty(&ammo)) {
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

                if (!halo::interface::weapon_hud_ammo_state_is_empty(&ammo) && candidate != current_weapon) {
                    object *weapon;
                    const int16_t *messaging;
                    halo::interface::hud_set_player_message(5, (uint16_t)local);
                    weapon = (object *)halo::objects::object_try_and_get(candidate, 4);
                    if (weapon != 0) {
                        messaging = weapon_hud_messaging(weapon);
                        if (messaging != 0) {
                            halo::interface::hud_set_message_icon_argument(local, 0, (const hud_messaging_information *)messaging);
                        } else {
                            halo::interface::hud_set_message_string_argument(local, 0, target_message, 0);
                        }
                        return;
                    }
                }
            }
        }
    }

    halo::interface::hud_set_action_text_shown(local, 0);
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
        player_index = halo::game::globals().local_player_globals->local_players[local_player_index];
    }
    camera_type = halo::camera::camera_get_type_for_player(local_player_index);

    if (player_index == (datum_index)-1) {
        return;
    }

    {
        player *local_player = halo::interface::player_record(player_index);

        if ((current_game_engine == 0 || ((motion_sensor_override_value & 2) != 0 && halo::game::globals().teams_enabled != 0)) &&
            cinematic_globals_ptr[9] == 0) {
            halo::interface::hud_waypoint_draw_all_for_player();
        }

        if (halo::game::globals().game_time->paused == 0) {
            int16_t expected = (halo::game::globals().local_player_globals->local_players[0] != (datum_index)-1) ? 0 : -1;
            if (current_local_player_index == expected) {
                halo::interface::chimera__motion_sensor_update();
            }
        }

        if (hud_flags->hud_enabled == 0) {
            halo::interface::hud_unit_sounds_update(local_player, 0);
            halo::interface::hud_messaging_update(current_local_player_index);
            return;
        }

        if (camera_type != 3 && camera_type != 2 && local_player->unit != (datum_index)-1) {
            halo::interface::hud_draw_weapon_interface(local_player);
            halo::interface::hud_update_interaction_prompt(player_index);
            halo::interface::hud_unit_sounds_update(local_player, hud_flags->hud_enabled);
            halo::interface::hud_render_unit_interface(local_player);
            halo::interface::hud_waypoints_draw_for_player(current_local_player_index);
            halo::interface::hud_draw_damage_indicators(current_local_player_index);
            halo::interface::hud_messaging_update(current_local_player_index);
            return;
        }

        halo::interface::hud_update_interaction_prompt(player_index);
        halo::interface::hud_unit_sounds_update(local_player, hud_flags->hud_enabled);
        halo::interface::hud_messaging_update(current_local_player_index);
    }
}

}
