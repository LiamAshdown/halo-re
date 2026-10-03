#include "halo/game/gamerest_netgame.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/game/records.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/core/datum.hpp"
#include <stdint.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

#ifdef __cplusplus
#define CTF_CUSTOM_WAYPOINT_ZERO custom_waypoint{}
#else
#define CTF_CUSTOM_WAYPOINT_ZERO (custom_waypoint){0}
#endif

extern "C" {
extern game_variant game_engine_variant;
extern int32_t ctf_flag_auto_return_ticks;
extern uint8_t ctf_single_flag_mode;
extern data_array *player_data;
extern object_type_definition *object_type_definitions[k_maximum_object_types];
extern player_globals *local_player_globals;
extern uint8_t ctf_active_team;
extern game_time_globals *game_time;
extern real_point3d *ctf_team_flag_stand_position[2];
extern uint8_t ctf_team_return_credit_active[2];
extern int32_t ctf_team_return_credit_ticks[2];
extern datum_index ctf_team_flag_object[2];
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints];
extern game_engine_definition *current_game_engine;
extern uint8_t network_message_scratch[halo::k_network_message_scratch_size];
}

namespace halo::game {

/**
 * Implements the original `ctf_engine_flag_tick`.
 *
 * @address 0x468bf0
 */
void CtfEngine::flag_tick(uint32_t flag_handle, object *flag_obj)
{
    item_data *item = (item_data *)((uint8_t *)flag_obj + k_item_data_offset);
    int32_t team;
    int32_t other_team;
    datum_index holder_player_index;
    real_point3d item_position;
    uint8_t position_valid;

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        if (game_engine_variant.engine.ctf.single_flag_time > 0) {
            if (ctf_flag_auto_return_ticks > 0) {
                ctf_flag_auto_return_ticks--;
            }
            if (ctf_flag_auto_return_ticks == 0) {
                if ((item->flags & _item_in_inventory_bit) != 0) {
                    if (ctf_single_flag_mode != 0 && *(int32_t *)&((struct object *)flag_obj)->owner_linkage != -1) {
                        player *carrier = (player *)halo::memory::datum_get(
                            (datum_index)((struct object *)flag_obj)->owner_linkage, player_data);
                        if (carrier != (player *)0) {
                            object *unit_obj = halo::objects::object_try_and_get(carrier->unit, _object_mask_unit);
                            if (unit_obj != (object *)0) {
                                unit_data *unit =
                                    (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
                                datum_index current_weapon =
                                    halo::units::unit_get_weapon_object_index((uint32_t)carrier->unit, unit->current_weapon_index);
                                if (current_weapon != (datum_index)flag_handle) {
                                    int32_t slot;
                                    for (slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
                                        if (unit->weapons[slot] == (datum_index)flag_handle) {
                                            unit->current_weapon_index = (int16_t)slot;
                                            halo::units::unit_ready_desired_weapon((uint32_t)carrier->unit, 1);
                                            break;
                                        }
                                    }
                                }
                                current_weapon = halo::units::unit_get_weapon_object_index(
                                    (uint32_t)carrier->unit, unit->current_weapon_index);
                                if (current_weapon == (datum_index)flag_handle) {
                                    halo::units::unit_dispatch_scripted_event_1b(1, (uint32_t)carrier->unit);
                                    halo::units::unit_drop_current_weapon((uint32_t)carrier->unit, 1);
                                }
                            }
                        }
                    }
                    goto notify_teams;
                }

                {
                    data_iterator iter;
                    void *element;
                    iter.data = halo::objects::globals().object_data;
                    iter.next_index = 0;
                    iter.index = (datum_index)halo::k_dword_none;
                    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
                    element = halo::memory::data_iterator_next(&iter);
                    while (element != 0) {
                        halo::game::chimera__kill_feed((datum_index)halo::k_dword_none, 0x2d, (uint32_t)halo::k_dword_none, 1, 0);
                        element = halo::memory::data_iterator_next(&iter);
                    }
                }

                ctf_team_return_credit_active[0] = 0;
                ctf_team_return_credit_active[1] = 0;
                ctf_team_return_credit_ticks[0] = 0;
                ctf_team_return_credit_ticks[1] = 0;
                custom_waypoints[0] = CTF_CUSTOM_WAYPOINT_ZERO;
                custom_waypoints[1] = CTF_CUSTOM_WAYPOINT_ZERO;
                ctf_team_flag_object[team = ((struct object *)flag_obj)->owner_team] = (datum_index)halo::k_dword_none;

                {
                    uint32_t toggled = (uint32_t)(((struct object *)flag_obj)->owner_team + 1) & 0x80000001;
                    if ((int32_t)toggled < 0) {
                        toggled = (toggled - 1 | 0xfffffffe) + 1;
                    }
                    halo::objects::object_delete((datum_index)flag_handle);
                    halo::game::game_engine_ctf_respawn_team_flag((int32_t)toggled, (real_point3d *)0, 0);
                    ctf_active_team = (uint8_t)toggled;
                    flag_handle = *(uint32_t *)((uint8_t *)&ctf_team_flag_object[0] + (int16_t)toggled * 4);
                    flag_obj = halo::game::object_at(flag_handle);
                    item = (item_data *)((uint8_t *)flag_obj + k_item_data_offset);
                    halo::game::game_engine_queue_multiplayer_sound(0x25 + (((struct object *)flag_obj)->owner_team != 0), halo::k_dword_none, 1);
                    halo::game::game_engine_ctf_reset_team_return_credit(flag_handle);
                    custom_waypoints[2] = CTF_CUSTOM_WAYPOINT_ZERO;
                    custom_waypoints[3] = CTF_CUSTOM_WAYPOINT_ZERO;
                    ctf_flag_auto_return_ticks = game_engine_variant.engine.ctf.single_flag_time;
                    halo::game::game_engine_ctf_notify_both_teams((int32_t)toggled);
                }
            }
notify_teams:
            if (local_player_globals->local_players[0] != (datum_index)halo::k_dword_none) {
                player *lp = (player *)((uint8_t *)player_data->data +
                    ((uint32_t)local_player_globals->local_players[0] & halo::k_datum_slot_mask) * sizeof(player));
                *(int32_t *)&((struct player *)lp)->hud_message_index = (ctf_active_team == (uint8_t)lp->team) ? 0x31 : 0x30;
                *(int32_t *)&((struct player *)lp)->hud_message_player = 0;
            }
        }

    } else {
        if (game_engine_variant.engine.ctf.single_flag_time > 0) {
            if (ctf_flag_auto_return_ticks > 0) {
                ctf_flag_auto_return_ticks--;
            }
            if (ctf_flag_auto_return_ticks == 0 && (item->flags & _item_in_inventory_bit) == 0) {
                ctf_flag_auto_return_ticks = game_engine_variant.engine.ctf.single_flag_time;
            }
            goto notify_teams;
        }
        goto weapon_coordination;
    }

    if (game_time->game_time - item->held_game_time <= 0x1fe) {
        goto weapon_coordination;
    }
    {
        int16_t obj_type = *(int16_t *)halo::game::tag_data_at((uint32_t)flag_obj->definition_tag);
        object_type_definition *type_def = object_type_definitions[obj_type];
        if ((halo::game::weapon_flag_set(type_def, halo::tags::weapon_tag_flag::must_be_readied)) == 0) {
            goto weapon_coordination;
        }
    }
    if ((flag_obj->flags & _object_needs_cluster_update_bit) == 0) {
        goto weapon_coordination;
    }
    if (flag_obj->parent_object != (datum_index)halo::k_dword_none) {
        goto weapon_coordination;
    }
    team = ((struct object *)flag_obj)->owner_team;
    {
        uint32_t toggled = (uint32_t)(team + 1) & 0x80000001;
        if ((int32_t)toggled < 0) {
            toggled = (toggled - 1 | 0xfffffffe) + 1;
        }
        other_team = (int32_t)toggled;
    }
    if ((*(uint8_t *)((uint8_t *)flag_obj + 0x22c) & 0x40) != 0) {

        halo::game::game_engine_queue_multiplayer_sound(team != 0 ? 9 : 0xc, halo::k_dword_none, 1);
        ctf_team_return_credit_active[team] = 0;
        ctf_team_return_credit_ticks[team] = 0;

        halo::game::game_engine_broadcast_kill_feed_to_team(0x2b, team, 1);
        halo::game::game_engine_broadcast_kill_feed_to_team(0x2c, other_team, 1);

        halo::game::game_engine_ctf_reset_team_return_credit(flag_handle);
    }

weapon_coordination:
    holder_player_index = halo::game::game_engine_find_player_holding_object((datum_index)flag_handle);
    team = ((struct object *)flag_obj)->owner_team;
    {
        uint32_t toggled = (uint32_t)(team + 1) & 0x80000001;
        if ((int32_t)toggled < 0) {
            toggled = (toggled - 1 | 0xfffffffe) + 1;
        }
        other_team = (int32_t)toggled;
    }
    position_valid = halo::items::item_get_effective_position((datum_index)flag_handle, &item_position);

    if ((game_engine_variant.engine.ctf.single_flag_time < 1 || ctf_active_team == team) && position_valid == 1) {
        int16_t icon;

        halo::game::custom_waypoint_register(holder_player_index, (int16_t)team, &item_position, "flag_blue", 0.0f,
            (datum_index)halo::k_dword_none, (int16_t)other_team);
        icon = halo::interface::hud_waypoint_arrow_find("flag_blue");

        if (icon != -1) {
            real_point3d other_stand = *ctf_team_flag_stand_position[other_team];
            halo::game::custom_waypoint_register((datum_index)halo::k_dword_none, (int16_t)(team + 2), &other_stand,
                "flag_blue", 0.3f, (datum_index)(uint16_t)icon, (int16_t)0xffffffff);
        } else {
            custom_waypoints[team + 2] = CTF_CUSTOM_WAYPOINT_ZERO;
        }
    }
}

/**
 * blam-cc: EBX -> flag_object_index, EDI -> position
 * Drops a CTF flag object at `position` facing the world identity axes, wakes it and resets its
 * velocity, clears an equipment-runtime flag bit (UNSURE which), and resets its
 * held-by/held-since bookkeeping (item_data::ignore_object_index and held_game_time) as if it
 * had just been returned to the ground.
 *
 * @address 0x4666c0
 */
void CtfEngine::clear_carrier(datum_index flag_object_index, real_point3d *position)
{
    object *flag_obj;
    item_data *item;
    uint32_t *unknown_22c;

    if (flag_object_index == (datum_index)halo::k_dword_none) {
        return;
    }

    flag_obj = halo::game::object_at(flag_object_index);

    halo::objects::object_set_position_and_orientation(flag_object_index, halo::math::globals().global_forward3d_pointer,
                                         halo::math::globals().global_up3d_pointer, position);
    halo::objects::object_reset_velocity_and_wake(flag_object_index);

    unknown_22c = (uint32_t *)((uint8_t *)flag_obj + 0x22c);
    *unknown_22c = *unknown_22c & 0xffffffdf;

    item = (item_data *)((uint8_t *)flag_obj + k_item_data_offset);
    item->held_game_time = game_time->game_time;
    item->ignore_object_index = (datum_index)halo::k_dword_none;
}

/**
 * blam-cc: EDX -> count, ESI -> current_engine_index, stack -> types
 * Outside a running multiplayer game, matches only if every entry in `types[0..count)` is 0
 * (the placement has no per-type restriction at all). While a game engine is loaded, matches if
 * any entry equals `current_engine_index` directly, or is one of the special "combined" values
 * 0x0c (always matches), 0x0d (matches unless the engine is ctf, index 1) or 0x0e (matches
 * unless the engine is ctf or race, indices 1 or 5).
 *
 * @address 0x45f7c0
 */
uint8_t NetgameRules::equipment_game_type_matches(int16_t *types, int32_t count, int32_t current_engine_index)
{
    uint8_t result;
    int32_t i;

    if (current_game_engine == 0) {
        result = 1;
        for (i = 0; i < count; i++) {
            result = result & (types[i] == 0);
        }
        return result;
    }

    result = 0;
    for (i = 0; i < count; i++) {
        int16_t type = types[i];
        uint8_t matches_directly = (type == current_engine_index);
        result = result | matches_directly;

        if (type == 0x0c) {
            result = result | 1;
        } else if (type == 0x0d) {
            result = result | (current_engine_index != _game_engine_ctf);
        } else if (type == 0x0e) {
            result = result | (current_engine_index != _game_engine_ctf && current_engine_index != _game_engine_race);
        }
    }
    return result;
}

/**
 * While hosting (network_game_mode == 2) and network_server+0x04 == 1, walks every object
 * that this machine controls (network_role == 0) whose type opts in
 * (object_type_override_call_0x74) and has a non-default object_type_definition +0x10 field,
 * consumes its "changed" flag, notifies the type's create/reset overrides, and -- if the
 * type's encode override reports data to send -- broadcasts a network message for it.
 *
 * @address 0x45b680
 */
void NetgameRules::broadcast_object_type_changes()
{
    object_iterator iterator;
    object *obj;
    uint8_t changed;
    int encode_result;

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host || halo::networking::globals().server->state != 1) {
        return;
    }

    iterator.type_mask = halo::k_dword_none;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = halo::objects::object_iterator_next(&iterator);
    while (obj != (object *)0) {
        if (obj->network_role == 0 && halo::objects::object_type_override_call_0x74(iterator.handle) == 1 &&
            object_type_definitions[obj->type]->network_delta_message_type != -1) {
            changed = halo::objects::object_datum_consume_pending_flag(iterator.handle);
            if (changed != 0) {
                halo::objects::object_type_override_call_0x68(iterator.handle);
            }

            encode_result = halo::objects::object_type_override_call_0x6c(iterator.handle, network_message_scratch, halo::k_network_message_scratch_size, changed == 0);
            if (0 < encode_result) {
                halo::networking::network_session_broadcast_to_flagged(encode_result, halo::networking::globals().server, 1, network_message_scratch, changed != 0, 0, 0, 3);
            }
            halo::objects::object_type_override_call_0x7c(iterator.handle);
        }
        obj = halo::objects::object_iterator_next(&iterator);
    }
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::CtfEngine::flag_tick; forwards to the C++ implementation.
 * register convention: flag handle and flag object pointer are both ordinary stack parameters.
 * blam-cc: EAX team
 * blam-cc: ESI sound, EDI player, stack broadcast
 * blam-cc: ESI message_type, BL broadcast, stack team
 *
 * @address 0x468bf0
 */
void ctf_engine_flag_tick(uint32_t flag_handle, object *flag_obj)
{
    halo::game::CtfEngine::flag_tick(flag_handle, flag_obj);
}

/**
 * C entry point for halo::game::CtfEngine::clear_carrier; forwards to the C++ implementation.
 * register convention: object handle in EBX, target position in EDI.
 * // blam-cc: EBX -> flag_object_index, EDI -> position
 * blam-cc: EBX -> flag_object_index, EDI -> position
 *
 * @address 0x4666c0
 */
void ctf_flag_object_clear_carrier(datum_index flag_object_index, real_point3d *position)
{
    halo::game::CtfEngine::clear_carrier(flag_object_index, position);
}

/**
 * C entry point for halo::game::NetgameRules::equipment_game_type_matches; forwards to the C++ implementation.
 * register convention: entry count in EDX (in_EDX), current game_engine_index in ESI
 * blam-cc: EDX -> count, ESI -> current_engine_index, stack -> types
 * blam-cc: EDX -> count, ESI -> current_engine_index, stack -> types
 *
 * @address 0x45f7c0
 */
uint8_t netgame_equipment_game_type_matches(int16_t *types, int32_t count, int32_t current_engine_index)
{
    return halo::game::NetgameRules::equipment_game_type_matches(types, count, current_engine_index);
}

/**
 * C entry point for halo::game::NetgameRules::broadcast_object_type_changes; forwards to the C++ implementation.
 * register convention: __cdecl, no arguments.
 * blam-cc: EDI -> object_index
 *
 * @address 0x45b680
 */
void network_server_broadcast_object_type_changes(void)
{
    halo::game::NetgameRules::broadcast_object_type_changes();
}

}
