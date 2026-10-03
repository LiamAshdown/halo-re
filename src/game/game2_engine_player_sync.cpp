#include "halo/game/game2_engine_players.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"

#define k_uninitialized_fill 0xfafafafau

static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &ui_split_screen = halo::link::ref<uint8_t>(halo::ui::vars().ui_split_screen);
static auto &global_00719750 = halo::link::ref<uint8_t>(halo::game::vars().global_00719750);
static auto &global_00719772 = halo::link::ref<int16_t>(halo::game::vars().global_00719772);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &shared_hud_text_draw_state = halo::link::ref<uint8_t>(halo::game::vars().shared_hud_text_draw_state);
static auto &machine_table = halo::link::ref<uint8_t *>(halo::game::vars().machine_table);
static auto &player_control_globals_ptr = halo::link::ref<player_control_globals *>(halo::game::vars().player_control_globals_ptr);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &cinematic_globals_ptr = halo::link::ref<uint8_t *>(halo::game::vars().cinematic_globals_ptr);
static auto &local_player_input_frozen = halo::link::ref<uint8_t []>(halo::game::vars().local_player_input_frozen);
static auto &local_player_look_frozen = halo::link::ref<uint8_t []>(halo::game::vars().local_player_look_frozen);
static auto &update_client_staged = halo::link::ref<uint32_t [8]>(halo::game::vars().update_client_staged);
static auto &update_client_staged_count = halo::link::ref<int32_t>(halo::game::vars().update_client_staged_count);
static auto &update_client_unknown_ec4 = halo::link::ref<int32_t>(halo::game::vars().update_client_unknown_ec4);
static auto &player_profile_cache = halo::link::ref<player_profile [16]>(halo::game::vars().player_profile_cache);
static auto &player_profile_cache_count = halo::link::ref<int32_t>(halo::game::vars().player_profile_cache_count);
static auto &object_type_definitions = halo::link::ref<object_type_definition *[k_maximum_object_types]>(halo::game::vars().object_type_definitions);

namespace halo::game {

/**
 * Client-side per-tick players update: fills a 16-entry player_action array from this machine's own staged
 * local input (update_client_distribute_staged_entry); if there is none staged, does nothing.
 *
 * @address 0x474590
 */
void EnginePlayerSync::players_update_client(void)
{
    player_action actions[16];
    int32_t counter;
    data_iterator player_iter;
    player *plr;
    datum_index player_handle;
    player_action current_action;
    uint32_t carried_weapon_index;
    uint32_t carried_grenade_or_zoom;

    memset(actions, 0, sizeof(actions));
    if (halo::game::update_client_distribute_staged_entry((uint8_t *)actions) == 0) {
        return;
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    counter = 0;
    carried_weapon_index = 0;
    carried_grenade_or_zoom = 0;

    plr = (player *)halo::memory::data_iterator_next(&player_iter);
    while (plr != (player *)0) {
        if (plr->local_player_index == -1) {
            player_update_record peek;
            if (halo::game::player_update_queue_pop_current(&peek, &plr->update_history) == 1) {
                current_action = peek.action;
                if (peek.references_remaining == peek.reference_count - 1) {
                    halo::game::player_apply_first_position_update(peek.field0, plr);
                }
            } else if (*(uint8_t *)&plr->update_history.has_current == 1) {
                current_action = *(player_action *)plr->update_history.current;
            } else {
                current_action.control_flags = 0;
                memset((uint8_t *)&current_action + 4, 0, sizeof(current_action) - 4);
                carried_weapon_index = 0;
                carried_grenade_or_zoom = 0;
            }
        } else {
            current_action = actions[counter];
        }
        counter = counter + 1;
        player_handle = player_iter.index;

        if (plr->unit == (datum_index)-1) {
            if (current_game_engine == 0) {
                if (ui_split_screen == 0) {
                    if (plr->deaths == 0) {
                        halo::game::player_respawn(player_handle);
                    } else if (local_player_globals->no_player_has_a_unit == 0) {
                        global_00719750 = 1;
                        if (local_player_globals->teleported != 0) {
                            global_00719772 = 0x5b;
                        }
                    }
                }
            } else if (halo::game::game_engine_player_ready_to_respawn(player_handle) != 0) {
                halo::game::game_engine_resolve_player_team(player_handle);
                halo::game::player_respawn(player_handle);
                if (plr->unit == (datum_index)-1) {
                    plr->respawn_timer = 1;
                } else {
                    halo::game::game_engine_apply_player_grenade_counts(player_handle);
                }
            }
        }

        if (plr->unit != (datum_index)-1) {
            object *unit_obj = halo::game::object_at(plr->unit);
            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

            if ((unit->flags & 0x40) != 0) {
                if (local_player_globals->input_disabled == 0) {
                    unit_control_data ctrl;

                    if ((current_action.control_flags & 0x80) != 0 &&
                        unit->equipment_object_index != (datum_index)-1) {
                        halo::game::player_apply_pickup_effect(player_handle, unit->equipment_object_index);
                        halo::units::unit_release_selected_equipment(plr->unit);
                    }

                    memset(&ctrl, 0, sizeof(ctrl));
                    ctrl.control_flags = (uint16_t)current_action.control_flags;
                    halo::game::player_compute_view_forward_vector(player_handle, &current_action.desired_yaw,
                                                        &ctrl.aiming_vector);
                    ctrl.facing_vector = ctrl.aiming_vector;
                    ctrl.looking_vector = ctrl.aiming_vector;
                    ctrl.throttle.i = current_action.throttle_x;
                    ctrl.throttle.j = current_action.throttle_y;
                    ctrl.throttle.k = 0.0f;
                    ctrl.primary_trigger = current_action.primary_trigger;
                    ctrl.weapon_index = (int16_t)carried_grenade_or_zoom;
                    ctrl.grenade_index = (int16_t)(carried_grenade_or_zoom >> 16);
                    ctrl.zoom_level = (int16_t)carried_weapon_index;
                    ctrl.unknown_0a = 0;
                    ctrl.animation_state = 3;
                    ctrl.aiming_speed = 0;
                    halo::units::unit_apply_control_block(plr->unit, &ctrl, -1);
                } else if (unit->swarm_actor_index == (datum_index)-1 && unit->actor_index == (datum_index)-1) {
                    unit_control_data ctrl;

                    memset(&ctrl, 0, sizeof(ctrl));
                    ctrl.animation_state = 3;
                    ctrl.aiming_speed = 0;
                    ctrl.control_flags = 0;
                    ctrl.weapon_index = -1;
                    ctrl.grenade_index = -1;
                    ctrl.zoom_level = -1;
                    ctrl.throttle = reinterpret_cast<real_vector3d &>(halo::math::globals().global_origin3d);
                    ctrl.primary_trigger = 0.0f;
                    ctrl.facing_vector = unit->desired_facing_vector;
                    ctrl.aiming_vector = unit->desired_aiming_vector;
                    ctrl.looking_vector = unit->desired_looking_vector;
                    halo::units::unit_apply_control_block(plr->unit, &ctrl, -1);
                }
            }
        }

        plr = (player *)halo::memory::data_iterator_next(&player_iter);
    }

    halo::game::game_engine_build_visible_cluster_bitmask((uint32_t *)((uint8_t *)local_player_globals + 0x58), 1);
    halo::game::game_engine_build_visible_cluster_bitmask((uint32_t *)((uint8_t *)local_player_globals + 0x18), 0);
    local_player_globals->local_player_count = (int16_t)(local_player_globals->local_players[0] != (datum_index)-1);
}

/**
 * Applies this tick's queued client update into a 16-entry player_action array plus a 16-entry carry-record
 * array (update_client_queue_apply_tick); if that fails (no update ready), does nothing else.
 *
 * @address 0x4740a0
 */
void EnginePlayerSync::players_update_server(void)
{
    player_action actions[16];
    client_update_carry carry[16];
    int32_t counter;
    data_iterator player_iter;
    player *plr;
    datum_index player_handle;
    player_action *action;
    client_update_carry *entry;
    int32_t grenade_value;

    memset(actions, 0, sizeof(actions));
    memset(carry, 0, sizeof(carry));
    carry[0].flag_a = (uint8_t)-1;

    if ((uint8_t)halo::game::update_client_queue_apply_tick(actions, carry) == 0) {
        return;
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    counter = 0;

    plr = (player *)halo::memory::data_iterator_next(&player_iter);
    while (plr != (player *)0) {
        action = &actions[counter];
        entry = &carry[counter];
        grenade_value = -1;
        counter = counter + 1;
        player_handle = player_iter.index;

        if (entry->flag_a == 1) {
            if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host && entry->field2 == entry->field3 + 1) {
                halo::networking::build_remote_player_transform_update(player_handle, action, entry->field1);
            }
            if (entry->flag_b == 1) {
                grenade_value = entry->field1;
            }
        }

        if (plr->unit == (datum_index)-1) {
            if (current_game_engine == 0) {
                if (ui_split_screen == 0) {
                    if (plr->deaths == 0) {
                        halo::game::player_respawn(player_handle);
                    } else if (local_player_globals->no_player_has_a_unit == 0) {
                        global_00719750 = 1;
                        if (local_player_globals->teleported != 0) {
                            global_00719772 = 0x5b;
                        }
                    }
                }
            } else if (halo::game::game_engine_player_ready_to_respawn(player_handle) != 0) {
                halo::game::game_engine_resolve_player_team(player_handle);
                halo::game::player_respawn(player_handle);
                if (halo::networking::globals().game_mode == halo::networking::k_game_mode_local) {
                    if (plr->unit == (datum_index)-1) {
                        plr->respawn_timer = 1;
                    } else {
                        halo::game::game_engine_apply_player_grenade_counts(player_handle);
                    }
                }
            }
        }

        if (plr->unit != (datum_index)-1) {
            object *unit_obj = halo::game::object_at(plr->unit);
            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

            if ((unit->flags & 0x40) != 0) {
                if (local_player_globals->input_disabled == 0) {
                    if ((action->control_flags & _unit_control_flag_action) != 0 &&
                        unit_obj->parent_object == (datum_index)-1) {
                        if (halo::game::player_execute_pending_interaction(player_handle) == 0) {
                            action->control_flags = action->control_flags | 0x400;
                        }
                    }
                    if ((action->control_flags & 0x4000) == 0 || unit_obj->parent_object != (datum_index)-1) {
                        *(uint8_t *)&plr->weapon_swap_result = 0;
                    } else if (*(uint8_t *)&plr->weapon_swap_result == 0) {
                        *(uint8_t *)&plr->weapon_swap_result = halo::game::player_execute_weapon_drop_interaction(player_handle);
                    }

                    if ((action->control_flags & 0x80) != 0 && unit->equipment_object_index != (datum_index)-1) {
                        halo::game::player_apply_pickup_effect(player_handle, unit->equipment_object_index);
                        halo::units::unit_release_selected_equipment(plr->unit);
                    }

                    if (unit->current_weapon_index != -1) {
                        datum_index weapon_handle = unit->weapons[unit->current_weapon_index];
                        if (weapon_handle != (datum_index)-1) {
                            object *weapon_obj = halo::game::object_at(weapon_handle);
                            Weapon *weapon_tag = (Weapon *)halo::game::tag_data_at(weapon_obj->definition_tag);
                            if ((weapon_tag->weapon_flags & 0x08) != 0) {
                                if ((action->control_flags & 0x1800) != 0) {
                                    if (unit_obj->network_role == 0) {
                                        halo::units::unit_dispatch_scripted_event_1b(1, plr->unit);
                                    }
                                    halo::units::unit_drop_current_weapon(plr->unit, 1);
                                }
                                action->weapon_index = unit->current_weapon_index;
                            }
                        }
                    }

                    {
                        real_vector3d forward;
                        unit_control_data ctrl;

                        halo::game::player_compute_view_forward_vector(player_handle, &action->desired_yaw, &forward);
                        memset(&ctrl, 0, sizeof(ctrl));
                        ctrl.animation_state = 3;
                        ctrl.aiming_speed = 0;
                        ctrl.control_flags = (uint16_t)action->control_flags;
                        ctrl.weapon_index = action->weapon_index;
                        ctrl.grenade_index = action->grenade_index;
                        ctrl.zoom_level = action->zoom_level;
                        ctrl.throttle.i = action->throttle_x;
                        ctrl.throttle.j = action->throttle_y;
                        ctrl.throttle.k = 0.0f;
                        ctrl.primary_trigger = action->primary_trigger;
                        ctrl.facing_vector = forward;
                        ctrl.aiming_vector = forward;
                        ctrl.looking_vector = forward;
                        halo::units::unit_apply_control_block(plr->unit, &ctrl, grenade_value);
                    }
                } else if (unit->swarm_actor_index == (datum_index)-1 && unit->actor_index == (datum_index)-1) {
                    unit_control_data ctrl;

                    memset(&ctrl, 0, sizeof(ctrl));
                    ctrl.animation_state = 3;
                    ctrl.aiming_speed = 0;
                    ctrl.control_flags = 0;
                    ctrl.weapon_index = -1;
                    ctrl.grenade_index = -1;
                    ctrl.zoom_level = -1;
                    ctrl.throttle = reinterpret_cast<real_vector3d &>(halo::math::globals().global_origin3d);
                    ctrl.primary_trigger = 0.0f;
                    ctrl.facing_vector = unit->desired_facing_vector;
                    ctrl.aiming_vector = unit->desired_aiming_vector;
                    ctrl.looking_vector = unit->desired_looking_vector;
                    halo::units::unit_apply_control_block(plr->unit, &ctrl, grenade_value);
                }
            }
        }

        plr = (player *)halo::memory::data_iterator_next(&player_iter);
    }

    halo::game::game_engine_build_visible_cluster_bitmask((uint32_t *)((uint8_t *)local_player_globals + 0x58), 1);
    halo::game::game_engine_build_visible_cluster_bitmask((uint32_t *)((uint8_t *)local_player_globals + 0x18), 0);
    local_player_globals->local_player_count = (int16_t)(local_player_globals->local_players[0] != (datum_index)-1);
}

/**
 * While hosting: for every player whose unit has its (UNSURE) +0x4b8 flag set, clears the flag, copies +0x4bc
 * into player+0xf4, and copies the unit's parent object's (or its own, if unparented) position into
 * player+0xf8/+0xfc/+0x100. For a non-local player, additionally logs a debug line with the current tick
 * count, game time, the copied value, position and the unit's own velocity/throttle.
 *
 * @address 0x476760
 */
void EnginePlayerSync::server_update_player_positions(void)
{
    data_iterator iter;
    player *plr;

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        return;
    }

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)halo::memory::data_iterator_next(&iter);
    while (plr != (player *)0) {
        if (plr->unit != (datum_index)-1) {
            object *unit_obj = halo::game::object_at(plr->unit);
            if (*((uint8_t *)unit_obj + 0x4b8) == 1) {
                object *position_source;

                *((uint8_t *)unit_obj + 0x4b8) = 0;
                *(int32_t *)&((struct player *)plr)->unknown_f4 = *(int32_t *)((uint8_t *)unit_obj + 0x4bc);

                position_source = unit_obj;
                if (unit_obj->parent_object != (datum_index)-1) {
                    position_source = halo::game::object_at(unit_obj->parent_object);
                }
                *(float *)&((struct player *)plr)->unknown_f8 = position_source->position.x;
                *(float *)((uint8_t *)plr + 0xfc) = position_source->position.y;
                *(float *)((uint8_t *)plr + 0x100) = position_source->position.z;

                if (plr->local_player_index == -1) {
                    int32_t value = *(int32_t *)&((struct player *)plr)->unknown_f4;
                    float pos_x = *(float *)&((struct player *)plr)->unknown_f8;
                    float pos_y = *(float *)((uint8_t *)plr + 0xfc);
                    float pos_z = *(float *)((uint8_t *)plr + 0x100);
                    unsigned long ticks = GetTickCount();
                    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

                    halo::networking::network_player_update_history_log_write(
                        "[%d]: [%d]:\t Completed [%d] ([%f] [%f] [%f]), ([%f] [%f]), ([%f] [%f])\n",
                        ticks, game_time->game_time, value, (double)pos_x, (double)pos_y, (double)pos_z,
                        (double)unit->throttle.i, (double)unit->throttle.j,
                        (double)unit_obj->velocity.i, (double)unit_obj->velocity.j);
                }
            }
        }
        plr = (player *)halo::memory::data_iterator_next(&iter);
    }
}

/**
 * Encodes `profile_tail` as network message type 0x15 and either broadcasts it (target == -1) or sends it to
 * one machine, when `broadcast_mode` selects the "commit" wire shape and the encode produces a payload.
 * UNSURE: see header -- `has_payload` and the alternate (non-commit) wire shape are reproduced only at the
 * level Ghidra's own decompile supports.
 *
 * @address 0x467010
 */
void EnginePlayerSync::send_player_profile_update(void *has_payload, void *profile_tail, int32_t target)
{
    int32_t encoded_size;
    void *payload_ptr;

    payload_ptr = (has_payload != (void *)0) ? profile_tail : (void *)0;

    encoded_size = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 1, halo::networking::message_id(halo::networking::delta_message::player_profile_update), (uint32_t)payload_ptr, &payload_ptr,
                                                 (uint32_t)profile_tail, 1, 0);
    if (encoded_size > 0) {
        if (target == -1) {
            halo::networking::network_session_broadcast_to_flagged(encoded_size, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
        } else {
            halo::networking::network_session_send_to_machine(target, halo::networking::globals().server, 1, network_message_scratch, encoded_size, 1, 0, 0, 3);
        }
    }
}

/**
 * Looks a key up in the object network-id hash table by walking its bucket chain, returning the stored value
 * or 0 when the table is uninitialized or the key is absent.
 */
int32_t EnginePlayerSync::pooled_node_hash_lookup(int32_t key)
{
    int32_t result = 0;
    if (object_network_id_table->id_to_index.initialized == 1) {
        int32_t magnitude = (key < 0) ? -key : key;
        int32_t bucket_count = object_network_id_table->id_to_index.bucket_count;
        int32_t *bucket_table = (int32_t *)object_network_id_table->id_to_index.buckets;
        int32_t *node = *(int32_t **)((uint8_t *)bucket_table + 4 + (magnitude % bucket_count) * 8);
        result = -1;
        while (node != 0) {
            if (*node == key) {
                result = node[1];
                break;
            }
            node = (int32_t *)node[2];
        }
    }
    if (result == -1) {
        result = 0;
    }
    return result;
}

/**
 * Builds a network message carrying: hashes of `player_handle` and `unit_index` (0 if either is -1 or
 * unmapped), `value` verbatim, a hash of the unit's parent object (if it has one), the unit's vehicle seat
 * index, its current weapon index (falling back to desired_weapon_index when unarmed), a hash of each of its
 * four weapon inventory slots, and the owning player's kill-streak pair -- then sends it to `machine_index`,
 * ...
 *
 * @address 0x477a80
 */
void EnginePlayerSync::send_unit_weapon_loadout(uint32_t unit_index, datum_index player_handle, int32_t value, int32_t machine_index)
{
    struct {
        int32_t player_hash;
        int32_t unit_hash;
        int32_t value;
        int32_t parent_hash;
        int16_t vehicle_seat_index;
        int16_t pad;
        int32_t weapon_hashes[4];
        int16_t current_weapon_index;
        int16_t kill_streak[2];
    } fields;
    void *fields_ptr;
    int32_t encoded_bits;
    object *obj;
    unit_data *unit;
    int i;

    obj = (object *)halo::game::object_at(unit_index);
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    fields.player_hash = 0;
    if (player_handle != (datum_index)halo::k_dword_none) {
        fields.player_hash = halo::objects::hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)player_handle);
        if (fields.player_hash == -1) {
            fields.player_hash = 0;
        }
    }

    fields.unit_hash = 0;
    if (unit_index != halo::k_dword_none) {
        fields.unit_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)unit_index);
        if (fields.unit_hash == -1) {
            fields.unit_hash = 0;
        }
    }

    fields.value = value;

    fields.parent_hash = 0;
    if (obj->parent_object != (datum_index)halo::k_dword_none) {
        fields.parent_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)obj->parent_object);
        if (fields.parent_hash == -1) {
            fields.parent_hash = 0;
        }
    }

    fields.vehicle_seat_index = unit->vehicle_seat_index;
    fields.current_weapon_index = unit->current_weapon_index;
    if (fields.current_weapon_index == -1) {
        fields.current_weapon_index = unit->desired_weapon_index;
    }

    for (i = 0; i < 4; i++) {
        int32_t weapon_handle = (int32_t)unit->weapons[i];
        fields.weapon_hashes[i] = (weapon_handle != -1) ? pooled_node_hash_lookup(weapon_handle) : 0;
    }

    {
        player *owner = halo::game::player_at(player_handle);
        fields.kill_streak[0] = owner->kill_streak[0];
        fields.kill_streak[1] = owner->kill_streak[1];
    }

    fields_ptr = &fields;
    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::unit_weapon_loadout), 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        if (machine_index == -1) {
            halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
            return;
        }
        halo::networking::network_session_send_to_machine(machine_index, halo::networking::globals().server, 1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 1, 3);
    }
}

/**
 * The MSVC uninitialized-stack fill the retail build still emits for this record. Every field is overwritten
 * by game_engine_build_local_player_control_input before it is read, but the stores are real instructions
 * (objdump 0x471b08..0x471b36), so they are kept.
 *
 * @address 0x471ae0
 */
void EnginePlayerSync::update_local_player_control(int16_t local_player_index, real delta_time, int32_t ticks_this_frame)
{
    local_player_control *control =
        &player_control_globals_ptr->local_players[local_player_index];
    GlobalsPlayerControl *player_control =
        (GlobalsPlayerControl *)global_globals->player_control.pointer;
    player_control_input input;
    uint32_t *input_dwords = (uint32_t *)&input;
    uint32_t button_flags;
    datum_index current_weapon;
    object *unit_object;
    unit_data *unit;
    int32_t i;

    for (i = 0; i < 8; i += 1) {
        input_dwords[i] = k_uninitialized_fill;
    }
    halo::game::game_engine_build_local_player_control_input(local_player_index, delta_time, &input);

    if (local_player_input_frozen[local_player_index * 0xf8] != 0) {
        for (i = 0; i < 8; i += 1) {
            input_dwords[i] = 0;
        }
    }
    button_flags = input.button_flags;

    do {
        if (halo::networking::globals().game_mode == halo::networking::k_game_mode_local) {
            if ((button_flags & 0x18) != 0) {
                int32_t new_unit;

                if ((button_flags & 0x10) != 0) {
                    new_unit = halo::units::object_find_next_untargeted((int32_t)control->unit);
                } else {
                    new_unit = halo::units::object_find_nearest_biped((int32_t)control->unit);
                }
                if (new_unit != -1) {
                    halo::game::local_player_set_controlled_unit((datum_index)new_unit, local_player_index);
                }
            }
            if ((button_flags & 0x20) != 0) {
                if (control->unit == (datum_index)-1) {
                    break;
                }
                halo::units::unit_sample_camera_shake_from_velocity(control->unit);
                button_flags = input.button_flags;
            }
        }

        if (control->unit == (datum_index)-1) {
            break;
        }

        unit_object = halo::game::object_at(control->unit);
        unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

        current_weapon = (datum_index)-1;
        if (unit->current_weapon_index != -1) {
            current_weapon = unit->weapons[unit->current_weapon_index];
        }

        if (control->desired_weapon_index == -1 ||
            unit->weapons[control->desired_weapon_index] == (datum_index)-1) {
            control->desired_weapon_index = unit->desired_weapon_index;
        }

        if ((button_flags & 1) != 0 || control->desired_weapon_index == -1 ||
            unit->weapons[control->desired_weapon_index] == (datum_index)-1) {
            control->desired_weapon_index = halo::units::unit_find_next_zone_permitted_weapon_slot(control->unit,
                control->desired_weapon_index, (int16_t)(button_flags & 1));
            control->desired_zoom_level = -1;
            button_flags = input.button_flags;
        }

        {
            int16_t forced = (int16_t)halo::units::unit_find_weapon_index_with_fixed_flag(control->unit);
            if (forced != -1 && control->desired_weapon_index != forced) {
                control->desired_weapon_index = forced;
                control->desired_zoom_level = -1;
            }
        }

        if (control->desired_grenade_index == -1 ||
            unit->grenade_counts[control->desired_grenade_index] == 0) {
            control->desired_grenade_index = (int16_t)unit->desired_grenade_index;
        }
        if ((button_flags & 2) != 0 || control->desired_grenade_index == -1 ||
            unit->grenade_counts[control->desired_grenade_index] == 0) {
            int16_t start = control->desired_grenade_index;
            int16_t best = -1;
            int16_t index;

            if (start == -1) {
                start = 0;
            }
            index = start;
            for (;;) {
                if (unit->grenade_counts[index] > 0) {
                    best = index;
                    if (index != start) {
                        break;
                    }
                }
                index = index == 1 ? (int16_t)0 : (int16_t)(index + 1);
                if (index == start) {
                    break;
                }
            }
            control->desired_grenade_index = best;
            button_flags = input.button_flags;
        }

        if ((button_flags & 4) != 0 && (player_control_globals_ptr->flags & 1) == 0 &&
            game_time->paused == 0 && current_weapon != (datum_index)-1 &&
            cinematic_globals_ptr[9] == 0) {
            control->desired_zoom_level =
                (int16_t)halo::items::weapon_get_next_zoom_level(control->desired_zoom_level, current_weapon);
        }

        if (local_player_look_frozen[local_player_index * 0xf8] == 0) {
            halo::game::game_engine_update_local_player_look(local_player_index, input.yaw_delta, input.pitch_delta);
        }

        if (unit_object->parent_object == (datum_index)-1) {
            real absolute_throttle_x = control->input_throttle_x < 0.0f
                                           ? -control->input_throttle_x
                                           : control->input_throttle_x;

            if (halo::interface::player_profile_get_flag_by_id(local_player_index) != 0 && absolute_throttle_x > 0.5 &&
                input.pitch_delta < 0.0001f && control->aim_assist_weight < 0.0001f) {
                int32_t ticks = (int32_t)control->autolevelling_ticks + 1;

                if (ticks < 0) {
                    ticks = 0;
                } else if (ticks > 0x7f) {
                    ticks = 0x7f;
                }
                control->autolevelling_ticks = (int8_t)ticks;
                control->autolevelling_active =
                    (uint8_t)((int16_t)(int8_t)ticks > player_control->minimum_autolevelling_ticks);
                break;
            }
            control->autolevelling_ticks = 0;
        }
        control->autolevelling_active = 0;
    } while (false);

    control->input_primary_trigger = input.primary_trigger;
    control->input_control_flags = input.control_flags;
    control->input_throttle_x = input.throttle_x;
    control->input_throttle_y = input.throttle_y;

    if (local_player_index != -1 && local_player_index < k_maximum_local_players &&
        local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        player_action action;

        action.control_flags = input.control_flags;
        action.desired_yaw = control->yaw;
        action.desired_pitch = control->pitch;
        action.throttle_x = input.throttle_x;
        action.throttle_y = input.throttle_y;
        action.primary_trigger = input.primary_trigger;
        action.weapon_index = control->desired_weapon_index;
        action.grenade_index = control->desired_grenade_index;
        action.zoom_level = control->desired_zoom_level;

        ((player_action *)update_client_staged)[update_client_staged_count] = action;
        update_client_staged_count = update_client_staged_count + 1;
        update_client_unknown_ec4 = ticks_this_frame;
    }
}

/**
 * Updates a local player's look yaw/pitch each tick, clamping against any vehicle seat facing constraints and
 * limiting the per-tick turn rate.
 */
real EnginePlayerSync::look_wrap_angle(real a)
{
    if (!(a < 3.1415927f)) {
        a -= 6.2831855f;
    }
    if (a <= -3.1415927f) {
        a += 6.2831855f;
    }
    return a;
}

/**
 * REWRITTEN from objdump. yaw += yaw_delta. In a seat whose definition (unit tag +0x2e8, stride 0x11c) has a
 * yaw range (+0xf0 / +0xf4), the seat marker (name at +0x24) gives the base heading atan2(forward.j,
 * forward.i); a yaw outside [base + min, base + max] snaps to the nearer edge. The yaw is then wrapped into
 * [0, 2pi].
 *
 * @address 0x472160
 */
void EnginePlayerSync::update_local_player_look(int16_t local_player_index, real yaw_delta, real pitch_delta)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    GlobalsPlayerControl *player_control = (GlobalsPlayerControl *)global_globals->player_control.pointer;
    real pitch_min = -1.4922565f;
    real pitch_max = 1.4922565f;
    camera_basis_out camera;
    uint8_t *unit_camera;
    uint8_t *unit;

    halo::game::chimera__spectate_fp_camera_position(&camera, local_player_index);
    look->yaw = yaw_delta + look->yaw;

    if (camera.seat_index != -1) {
        uint8_t *unit_object = (uint8_t *)halo::game::object_at(camera.unit);
        uint8_t *unit_tag = (uint8_t *)halo::game::tag_data_at(*(datum_index *)unit_object);
        UnitSeat *seat = (UnitSeat *)((Unit *)unit_tag)->seats.pointer + (int32_t)camera.seat_index;
        real yaw_min = seat->yaw_minimum;
        real yaw_max = seat->yaw_maximum;

        if (yaw_min != 0.0f || yaw_max != 0.0f) {
            object_marker marker;
            real base, a, b, span, forward_delta, back_delta;

            halo::objects::object_get_node_local_transform(camera.unit, seat->marker_name.string, &marker, 1);
            base = (real)halo::libm::atan2((double)marker.node_transform.forward.j, (double)marker.node_transform.forward.i);
            a = base + yaw_min;
            b = base + yaw_max;
            span = look_wrap_angle(b - a);
            forward_delta = look_wrap_angle(b - look->yaw);
            back_delta = look_wrap_angle(look->yaw - a);
            if (!(span >= 0.0f)) {
                span += 6.2831855f;
            }
            if (!((forward_delta >= 0.0f && forward_delta < span) || (back_delta >= 0.0f && back_delta < span))) {
                if ((real)halo::libm::fabs((double)forward_delta) <= (real)halo::libm::fabs((double)back_delta)) {
                    look->yaw = b;
                } else {
                    look->yaw = a;
                }
            }
        }
    }

    if (!(look->yaw >= 0.0f)) {
        real yaw = look->yaw;
        do {
            yaw += 6.2831855f;
        } while (!(yaw >= 0.0f));
        look->yaw = yaw;
    }
    if (look->yaw > 6.2831855f) {
        real yaw = look->yaw;
        do {
            yaw -= 6.2831855f;
        } while (yaw > 6.2831855f);
        look->yaw = yaw;
    }

    unit_camera = camera.marker_offset;
    if (unit_camera != 0) {
        real target_pitch = *(real *)(unit_camera + 0x40);

        unit = (uint8_t *)halo::game::object_at(camera.unit);
        if (*(real *)(unit_camera + 0x48) != 0.0f || *(real *)(unit_camera + 0x44) != 0.0f) {
            pitch_min = *(real *)(unit_camera + 0x44);
            pitch_max = *(real *)(unit_camera + 0x48);
            if (camera.seat_index != -1 && ((unit_object *)unit)->base.up.k > 0.2f) {
                real_vector3d heading;
                real adjust;

                heading.i = (real)halo::libm::cos((double)look->yaw) * 1.0f;
                heading.j = (real)halo::libm::sin((double)look->yaw) * 1.0f;
                heading.k = 0.0f;
                adjust = 1.5707964f - halo::math::vector3d_angle_between_4cd4f0(((unit_object *)unit)->base.up, heading);
                pitch_min = pitch_min - adjust;
                pitch_max = pitch_max - adjust;
                target_pitch = target_pitch - adjust;
            }
            if (!(pitch_min >= -1.4922565f)) {
                pitch_min = -1.4922565f;
            } else if (!(pitch_min <= 1.4922565f)) {
                pitch_min = 1.4922565f;
            }
            if (!(pitch_max >= -1.4922565f)) {
                pitch_max = -1.4922565f;
            } else if (!(pitch_max <= 1.4922565f)) {
                pitch_max = 1.4922565f;
            }
        }
        if (target_pitch != 0.0f || look->autolevelling_active) {
            real error = (real)(halo::libm::fabs((double)(look->pitch - target_pitch)) * 0.6366197466850281);
            real_vector3d *velocity = &((unit_object *)unit)->base.velocity;
            real speed = (real)halo::libm::sqrt((double)(velocity->i * velocity->i + velocity->j * velocity->j +
                velocity->k * velocity->k));
            real step;

            if (target_pitch != 0.0f) {
                step = speed * error * 0.08f;
            } else {
                step = speed * player_control->look_autolevelling_scale * error;
            }
            halo::game::value_step_toward_target(&look->pitch, target_pitch, step);
        }
    }

    {
        real d = pitch_min - look->pitch_minimum;
        if (!(d >= -0.012271847f)) {
            d = -0.012271847f;
        } else if (!(d <= 0.012271847f)) {
            d = 0.012271847f;
        }
        look->pitch_minimum = d + look->pitch_minimum;
        d = pitch_max - look->pitch_maximum;
        if (!(d >= -0.012271847f)) {
            d = -0.012271847f;
        } else if (!(d <= 0.012271847f)) {
            d = 0.012271847f;
        }
        look->pitch_maximum = d + look->pitch_maximum;
    }

    pitch_delta = pitch_delta + look->pitch;
    look->pitch = pitch_delta;
    if (!(pitch_delta >= look->pitch_minimum)) {
        look->pitch = look->pitch_minimum;
    } else if (!(pitch_delta <= look->pitch_maximum)) {
        look->pitch = look->pitch_maximum;
    }
}

/**
 * Adds `player_handle` to the first free (in_use == 0) player-profile-cache slot, unless it is already
 * present. Silently does nothing if the table is full or already has 16 live entries scanned without finding a
 * free one (the scan itself is bounds-checked against the table's own end address, matching the original).
 *
 * @address 0x466c60
 */
void EnginePlayerSync::player_profile_cache_add(datum_index player_handle)
{
    int32_t slot;
    uint8_t *scan;

    if (halo::game::game_engine_player_profile_cache_find(player_handle) != -1) {
        return;
    }

    slot = 0;
    scan = (uint8_t *)player_profile_cache;
    while (*scan != 0) {
        scan = scan + sizeof(player_profile);
        slot = slot + 1;
        if (scan >= (uint8_t *)player_profile_cache + sizeof(player_profile_cache)) {
            return;
        }
    }

    player_profile_cache[slot].in_use = 1;
    player_profile_cache[slot].player = player_handle;
    player_profile_cache_count = player_profile_cache_count + 1;
}

/**
 * Returns the slot index of the active player-profile-cache entry whose stored player handle equals
 * `player_handle`, or -1 if none matches.
 *
 * @address 0x466e80
 */
int32_t EnginePlayerSync::player_profile_cache_find(datum_index player_handle)
{
    int32_t i;
    int32_t result;

    result = -1;
    for (i = 0; i < 16; i = i + 1) {
        if (player_profile_cache[i].in_use != 0 && player_profile_cache[i].player == player_handle) {
            result = i;
        }
    }
    return result;
}

/**
 * Re-captures every active player-profile-cache entry with the given `commit` flag, then invokes the active
 * game engine's optional profiles_updated callback (commit, callback_extra_ arg), if one is registered.
 * UNSURE: see header for the callback's real signature.
 *
 * @address 0x466cb0
 */
void EnginePlayerSync::player_profile_cache_sync_all(int32_t commit, void *callback_extra_arg)
{
    int32_t i;
    void (*callback)(int32_t, void *);

    for (i = 0; i < 16; i = i + 1) {
        if (player_profile_cache[i].in_use == 1) {
            halo::game::game_engine_capture_player_profile(i, commit);
        }
    }

    callback = (void (*)(int32_t, void *))current_game_engine->profiles_updated;
    if (callback != (void (*)(int32_t, void *))0) {
        callback(commit, callback_extra_arg);
    }
}

/**
 * Finds the first Scenario::starting_equipment entry whose game-type list matches the active game engine, then
 * spawns up to five items -- one per item_collection slot -- each a weighted random pick from that
 * collection's own reflexive, alternating role 3/0 for object_new_with_datum_role_control the same way the
 * sibling netgame-equipment spawner does.
 *
 * @address 0x4611f0
 */
void EnginePlayerSync::spawn_player_starting_loadout(uint32_t starting_equipment_index, int32_t *frag_count, int32_t *plasma_count)
{
    int32_t count = (int32_t)halo::scenario::globals().scenario->starting_equipment.count;
    int32_t i;
    ScenarioStartingEquipment *equipment;

    if (count <= 0) {
        return;
    }

    equipment = (ScenarioStartingEquipment *)halo::scenario::globals().scenario->starting_equipment.pointer;
    i = 0;
    for (;;) {
        if (halo::game::netgame_equipment_game_type_matches(&equipment->type_0, 4,
                current_game_engine != 0 ? current_game_engine->index : 0)) {
            break;
        }
        i = i + 1;
        equipment = equipment + 1;
        if (count <= i) {
            return;
        }
    }

    {
        TagDependency *item_collections = &equipment->item_collection_1;
        uint8_t first_spawn = 1;
        int32_t slot;

        for (slot = 0; slot < 5; slot++) {
            if (*(int32_t *)&item_collections[slot].tag_id != -1) {
                object_placement_data placement;
                datum_index picked_tag = (datum_index)halo::game::tag_reflexive_pick_weighted_random_index(
                    *(datum_index *)&item_collections[slot].tag_id);
                uint32_t role = 3;
                datum_index new_object;

                halo::objects::object_placement_data_initialize(&placement, picked_tag, (datum_index)halo::k_dword_none);

                if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
                    tag_instance *tag_inst = &halo::cache::globals().tag_instances[picked_tag & halo::k_datum_slot_mask];
                    Object *object_tag = (Object *)tag_inst->data;
                    if (object_type_definitions[object_tag->object_type]->network_delta_message_type != -1) {
                        role = 0;
                    }
                }

                new_object = halo::objects::object_new_with_datum_role_control(&placement, role);
                if (new_object != (datum_index)halo::k_dword_none) {
                    if (!first_spawn) {
                        datum_index previous = (datum_index)halo::game::object_at(new_object);
                        if (halo::units::unit_has_weapon_of_type(new_object, previous) != 0) {
                            int32_t category = *(int32_t *)((uint8_t *)previous + 4);
                            if (category == 0) {
                                halo::objects::object_delete_unparented(new_object);
                            }
                            if (category == 0 || category == 3) {
                                halo::objects::object_delete_recursive(new_object, 0);
                            }
                            continue;
                        }
                    }
                    halo::units::unit_pickup_weapon((int16_t)(0 - first_spawn & 2), new_object, starting_equipment_index);
                    first_spawn = 0;
                }
            }
        }
    }

    if ((equipment->flags & 1) != 0) {
        *frag_count = 0;
        *plasma_count = 0;
    }
    if ((equipment->flags & 2) != 0) {
        *plasma_count = *plasma_count + *frag_count;
        *frag_count = 0;
    }
}

}
