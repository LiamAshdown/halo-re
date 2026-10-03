/**
 * Network message handlers and gameplay event notifications of the game engine.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>
#include "networking.h"
#include "items.h"

#include "halo/game/game1_notifications.hpp"
#include "halo/sound/api.hpp"

extern "C" {
extern game_engine_definition *current_game_engine;
extern uint8_t message_delta_decode_compound_field(void *event, void *out_values);
extern void message_delta_decode_compound_field_staged(void *event);
extern void game_engine_reset_respawns_and_cleanup_bipeds(void);
extern void game_engine_cleanup_stray_items(void);
extern void game_engine_cleanup_stray_projectiles(void);
extern data_array *player_data;
extern Globals *global_globals;
extern int32_t game_engine_unknown_aa00;
extern game_variant game_engine_variant;
extern data_array *object_data;
extern void game_engine_spawn_player_starting_loadout(uint32_t starting_equipment_index,
    int32_t *frag_count, int32_t *plasma_count);
extern uint32_t game_engine_pack_object_flags_or_passthrough(uint32_t input);
extern network_id_table *machine_table;
extern network_id_table *object_network_id_table;
extern void *datum_get(datum_index handle, data_array *array);
extern uint8_t player_execute_pending_interaction(uint32_t handle);
extern uint8_t player_swap_to_weapon(uint32_t player_index, datum_index target_weapon);
extern Scenario *global_scenario;
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index, uint8_t initial_targeting_flag);
extern void game_engine_init_player_look_state_from_object(datum_index unit, int16_t local_player_index);
extern void unit_apply_starting_profile(int16_t starting_profile_index, datum_index unit_handle,
    uint8_t reset_stats);
extern void game_engine_apply_player_grenade_counts(uint32_t player_index);
extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index);
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index);
extern uint8_t player_add_kill_streak(int32_t slot, int16_t amount, uint32_t player_handle);
extern uint8_t game_engine_teams_enabled_flag;
extern int16_t network_game_mode;
extern uint8_t *network_client;
extern uint8_t player_customization_slot_set(uint8_t *base, uint8_t new_value, uint32_t key);
extern void player_set_team_by_color(uint8_t new_team, int8_t target_team_index_desired);
extern void game_engine_end_game_sequence_stage1(void);
extern void game_engine_end_game_sequence_stage2(void);
extern void game_engine_end_game_sequence_stage3(void);
extern int32_t hash_table_get(hash_table *table, int32_t key);
extern uint8_t network_object_index_cache[];
extern int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key);
extern uint8_t network_message_scratch[0x7ff8];
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern network_server_globals *network_server;
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern tag_instance *tag_instances;
extern int32_t multiplayer_sound_queue_count;
extern multiplayer_sound_request multiplayer_sound_queue[k_maximum_queued_multiplayer_sounds];
extern void game_engine_play_multiplayer_sound(int32_t sound_index, datum_index recipient_player,
    uint8_t broadcast);
extern datum_index player_index_from_unit_index(datum_index unit_index);
}

namespace halo::game::engine1 {

/**
 * Validates an incoming network message and, if valid, triggers the object-cleanup/reset pass and an engine
 * callback.
 *
 * Original register convention: EAX -> event.
 *
 * @address 0x468320
 */
void Notifications::apply_partial_round_reset_message(void *event)
{
    int32_t scratch;

    if (*(int32_t *)*(void **)event == 0) {
        if (message_delta_decode_compound_field(event, &scratch) != 0) {
            game_engine_reset_respawns_and_cleanup_bipeds();
            game_engine_cleanup_stray_items();
            game_engine_cleanup_stray_projectiles();
            if (current_game_engine->reset_objects != 0) {
                ((void (*)(void))current_game_engine->reset_objects)();
            }
        }
    } else {
        message_delta_decode_compound_field_staged(event);
    }
}

/**
 * Computes and applies a player's starting frag and plasma grenade counts based on scenario defaults and game
 * option overrides.
 *
 * Original register convention: EAX -> player_index.
 *
 * @address 0x4613c0
 */
void Notifications::apply_player_grenade_counts(uint32_t player_index)
{
    GlobalsGrenade *grenades;
    int32_t frag_max, plasma_max, frag_count, plasma_count;
    player *p;
    datum_index unit;

    if (current_game_engine == 0) {
        return;
    }
    if (current_game_engine->allow_grenade_counts != 0 &&
        ((char (*)(uint32_t))current_game_engine->allow_grenade_counts)(player_index) == 0) {
        return;
    }

    grenades = (GlobalsGrenade *)global_globals->grenades.pointer;
    plasma_max = grenades[1].maximum_count;
    frag_count = grenades[0].mp_spawn_default;
    plasma_count = grenades[1].mp_spawn_default;
    frag_max = grenades[0].maximum_count;

    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit = p->unit;

    if ((game_engine_unknown_aa00 & 8) == 0) {
        if ((game_engine_unknown_aa00 & 4) != 0) {
            frag_max = 2;
            plasma_max = 2;
        }
    } else {
        frag_max = 1;
        plasma_max = 1;
    }

    if ((game_engine_variant.flags & 0x20) == 0) {
        object *obj = ((object_header *)object_data->data)[unit & 0xffff].data;
        if (obj->network_role == 0 || obj->network_role == 3) {
            game_engine_spawn_player_starting_loadout(unit, &frag_count, &plasma_count);
        }
    }

    {
        int32_t plasma_result = plasma_count;
        int32_t frag_result = frag_count;

        if ((game_engine_unknown_aa00 & 4) == 0 && ((game_engine_variant.flags >> 2) & 1) != 0) {
            plasma_result = plasma_max;
            frag_result = frag_max;
        }

        if (unit == (datum_index)0xffffffff) {
            return;
        }
        {
            object *obj = ((object_header *)object_data->data)[unit & 0xffff].data;

            if (obj->network_role != 0 && obj->network_role != 3) {
                return;
            }

            switch (game_engine_variant.weapon_set) {
            case 3:
            case 10:
                plasma_result = plasma_result + frag_result;
                frag_result = 0;
                break;
            case 9:
                frag_result = frag_result + plasma_result;
                plasma_result = 0;
                goto clamp;
            case 0x0d:

                if ((uint8_t)game_engine_pack_object_flags_or_passthrough(0) == 0) {
                    frag_result = 0;
                    plasma_result = 0;
                }
                break;
            default:
                break;
            }
            goto clamp_after_default;

        clamp:
            plasma_result = 0;
        clamp_after_default:
            if (frag_max < frag_result) {
                frag_result = frag_max;
            }
            if (plasma_max < plasma_result) {
                plasma_result = plasma_max;
            }
            *(uint8_t *)((uint8_t *)obj + 0x31e) = (uint8_t)frag_result;
            *(uint8_t *)((uint8_t *)obj + 0x31f) = (uint8_t)plasma_result;

        }
    }
}

/**
 * Applies a received player interaction message.
 *
 * Original register convention: EAX -> envelope.
 *
 * @address 0x478f10
 */
uint8_t Notifications::apply_player_interaction_message(void **envelope)
{
    struct {
        int32_t machine_id;
        int32_t use_secondary_mode;
        int32_t interaction_pooled_id;
        int16_t interaction_type;
        int16_t interaction_seat;
        int32_t secondary_pooled_id;
    } message;

    if (*(int32_t *)*envelope != 0) {
        message_delta_decode_compound_field_staged(envelope);
        return 0;
    }
    if (!message_delta_decode_compound_field(envelope, &message)) {
        return 0;
    }

    {
        uint32_t primary_handle = 0xffffffff;
        if (message.machine_id != 0) {
            primary_handle = (uint32_t)(*(int32_t **)&machine_table->handles)[message.machine_id];
        }

        {
            player *p = (player *)datum_get((datum_index)primary_handle, player_data);
            if (p == 0) {
                return 0;
            }

            {
                datum_index interaction_object = (datum_index)0xffffffff;
                uint32_t secondary_handle = 0xffffffff;

                if (message.interaction_pooled_id != 0) {
                    interaction_object = (datum_index)((int32_t *)object_network_id_table->handles)[
                        message.interaction_pooled_id];
                }
                if (message.secondary_pooled_id != 0) {
                    secondary_handle = (uint32_t)((int32_t *)object_network_id_table->handles)[
                        message.secondary_pooled_id];
                }

                if (interaction_object == (datum_index)0xffffffff && message.interaction_pooled_id != -1) {
                    return 0;
                }

                p->interaction_type = message.interaction_type;
                p->interaction_object = interaction_object;
                p->interaction_seat = message.interaction_seat;

                if (message.use_secondary_mode == 0) {
                    return player_execute_pending_interaction(primary_handle);
                }
                return player_swap_to_weapon(primary_handle, (datum_index)secondary_handle);
            }
        }
    }
}

/**
 * Applies a received player spawn loadout message.
 *
 * Original register convention: EAX -> envelope.
 *
 * @address 0x477c70
 */
void Notifications::apply_player_spawn_loadout_message(void **envelope)
{
    struct {
        int32_t machine_id;
        int32_t unit_pooled_id;
        int32_t team;
        int32_t seat_vehicle_pooled_id;
        int32_t seat_number;
        int32_t weapon_pooled_ids[4];
        int16_t desired_weapon_index;
        int16_t kill_streak_delta[2];
        int16_t pad;
    } message;

    if (*(int32_t *)*envelope != 0) {
        message_delta_decode_compound_field_staged(envelope);
        return;
    }
    if (!message_delta_decode_compound_field(envelope, &message)) {
        return;
    }

    {
        datum_index owner_handle = (datum_index)0xffffffff;
        uint32_t player_handle;
        if (message.machine_id != 0) {
            owner_handle = (datum_index)(*(int32_t **)&machine_table->handles)[message.machine_id];
        }
        player_handle = (uint32_t)owner_handle;

        {
            player *p = (player *)datum_get(player_handle, player_data);
            if (p != 0 && message.unit_pooled_id != 0) {
                datum_index new_unit = (datum_index)((int32_t *)object_network_id_table->handles)[
                    message.unit_pooled_id];
                if (new_unit != (datum_index)0xffffffff) {
                    object *unit_obj = object_try_and_get(new_unit, 3);
                    if (unit_obj != 0) {
                        p->unit = new_unit;
                        p->team = message.team;
                        p->team_index = (int8_t)message.team;
                        unit_obj->owner_linkage = (uint32_t)owner_handle;
                        unit_obj->owner_team = (int16_t)p->team;
                        ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->controlling_player = owner_handle;
                        unit_refresh_targeting_flag_and_weapons(new_unit, 1);

                        if (p->local_player_index == -1) {
                            ((struct player *)p)->position_updates.read_index = 0;
                            ((struct player *)p)->position_updates.write_index = 0;
                            ((struct player *)p)->vehicle_updates.read_index = 0;
                            ((struct player *)p)->vehicle_updates.write_index = 0;
                        } else {
                            unit_obj->network_role = 2;
                            game_engine_init_player_look_state_from_object(new_unit, p->local_player_index);
                        }

                        if (current_game_engine == 0 &&
                            ((global_scenario->player_starting_profile.count > 1 && p->deaths > 0) ||
                             global_scenario->player_starting_profile.count != 0)) {
                            int16_t starting_profile_index =
                                (global_scenario->player_starting_profile.count > 1 && p->deaths > 0) ? 1 : 0;
                            unit_apply_starting_profile(starting_profile_index, new_unit, 1);
                        }

                        p->kill_streak[0] = 0;
                        p->kill_streak[1] = 0;
                        p->interaction_type = 0;
                        p->interaction_object = (datum_index)0xffffffff;
                        game_engine_apply_player_grenade_counts(player_handle);

                        {
                            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
                            int32_t i;
                            for (i = 0; i < 4; i++) {
                                int32_t weapon = message.weapon_pooled_ids[i] != 0
                                    ? ((int32_t *)object_network_id_table->handles)[message.weapon_pooled_ids[i]]
                                    : -1;
                                if (weapon == -1) {
                                    unit->weapons[i] = (datum_index)0xffffffff;
                                } else {
                                    unit_pickup_weapon(0, (uint32_t)weapon, new_unit);
                                }
                            }
                            unit->current_weapon_index = -1;
                            unit->desired_weapon_index = message.desired_weapon_index;
                        }

                        if (message.seat_vehicle_pooled_id != -1 && message.seat_vehicle_pooled_id != 0) {
                            datum_index vehicle = (datum_index)((int32_t *)object_network_id_table->handles)[
                                message.seat_vehicle_pooled_id];
                            if (vehicle != (datum_index)0xffffffff) {
                                unit_enter_vehicle_seat(vehicle, (int16_t)message.seat_number, p->unit);
                            }
                        }

                        if (0 < message.kill_streak_delta[0]) {
                            player_add_kill_streak(0, message.kill_streak_delta[0], player_handle);
                        }
                        if (0 < message.kill_streak_delta[1]) {
                            player_add_kill_streak(1, message.kill_streak_delta[1], player_handle);
                        }
                    }
                }
            }
        }
    }
}

/**
 * Applies a team assignment received by a client.
 *
 * Original register convention: EAX -> envelope.
 *
 * @address 0x470a10
 */
void Notifications::client_apply_team_assignment(void **envelope)
{
    uint8_t out_pair[2] = { 0xff, 0xff };

    if (*(int32_t *)*envelope != 0 || current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        message_delta_decode_compound_field_staged(envelope);
        return;
    }
    if (!message_delta_decode_compound_field(envelope, out_pair) || network_game_mode != 1) {
        return;
    }

    player_customization_slot_set(network_client + 0xb14, out_pair[1], (uint8_t)out_pair[0]);
    player_set_team_by_color(out_pair[1], (uint8_t)out_pair[0]);
}

/**
 * Dispatches an end-of-game notification event.
 *
 * Original register convention: EAX -> event, ECX -> stage.
 *
 * @address 0x467230
 */
void Notifications::dispatch_end_game_notification(void *event)
{
    int32_t stage;

    if (**(int32_t **)event != 0) {
        message_delta_decode_compound_field_staged(event);
        return;
    }

    if (message_delta_decode_compound_field(event, &stage) == 0) {
        return;
    }

    if (stage == 1) {
        game_engine_end_game_sequence_stage1();
    } else if (stage == 2) {
        game_engine_end_game_sequence_stage2();
    } else if (stage == 3) {
        game_engine_end_game_sequence_stage3();
    }
}

/**
 * Dispatches a networked game-event notification (event id 0x2f, apparently item pickup) through the engine's
 * event/message system.
 *
 * Original register convention: ECX -> machine_id, stack -> param_1, param_2.
 *
 * @address 0x45f850
 */
void Notifications::dispatch_item_pickup_event(int32_t machine_id, int32_t picked_tag, int32_t param_2)
{
    struct { int32_t slot; int32_t picked_tag; int16_t param_2_low; } fields;
    void *fields_ptr;

    fields.slot = 0;
    if (machine_id != -1) {
        fields.slot = hash_table_get(&object_network_id_table->id_to_index, (int32_t)machine_id);
    }
    if (fields.slot == -1) {
        fields.slot = network_index_cache_find_or_allocate_slot(network_object_index_cache, machine_id);
    }

    fields.picked_tag = picked_tag;
    fields.param_2_low = (int16_t)param_2;
    fields_ptr = &fields;

    network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x2f, 0, &fields_ptr, 0, 1, '\0'), network_server, 1, network_message_scratch, 1, 0, 0, 3);
}

/**
 * Returns the duration in ticks of an announcer sound of the multiplayer globals.
 *
 * Original register convention: EAX -> sound_index.
 *
 * @address 0x46bde0
 */
int32_t Notifications::get_multiplayer_sound_duration_ticks(int32_t sound_index)
{
    GlobalsMultiplayerInformation *mp_info =
        (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
    uint8_t *sound;
    uint32_t tag_id;

    if (mp_info == (GlobalsMultiplayerInformation *)0 || sound_index >= (int32_t)mp_info->sounds.count) {
        return 0;
    }
    sound = (uint8_t *)mp_info->sounds.pointer + sound_index * 0x10;
    if (sound == (uint8_t *)0) {
        return 0;
    }
    tag_id = *(uint32_t *)(sound + 0xc);
    if (tag_id == 0xffffffff) {
        return 0;
    }
    return (*(int32_t *)((uint8_t *)tag_instances[tag_id & 0xffff].data + 0x84) * 30) / 1000;
}

/**
 * Checks whether a pending flag is unset and a valid multiplayer-sound table entry exists for the given index,
 * and if so triggers local sound playback; otherwise clears/defers.
 *
 * Original register convention: EAX -> event, ECX -> sound_index.
 *
 * @address 0x46bca0
 */
void Notifications::handle_sound_status_event(void *event)
{
    if (*(int32_t *)*(void **)event == 0) {
        int32_t sound_index;
        if (message_delta_decode_compound_field(event, &sound_index) != 0) {
            GlobalsMultiplayerInformation *mp_info =
                (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
            if (mp_info != (GlobalsMultiplayerInformation *)0 &&
                sound_index < (int32_t)mp_info->sounds.count) {
                uint8_t *sound = (uint8_t *)mp_info->sounds.pointer + sound_index * 0x10;
                if (sound != (uint8_t *)0 && *(int32_t *)(sound + 0xc) != -1) {
                    halo::sound::sound_start_unspatialized(*(datum_index *)(sound + 0xc), 1.0f);
                }
            }
        }
    } else {
        message_delta_decode_compound_field_staged(event);
    }
}

/**
 * Per-tick service of the multiplayer announcer sound queue.
 *
 * @address 0x46bd80
 */
void Notifications::multiplayer_sound_queue_tick(void)
{
    if (multiplayer_sound_queue_count != 0) {
        multiplayer_sound_queue[0].remaining_ticks--;
        if (multiplayer_sound_queue[0].remaining_ticks == 0) {
            if (multiplayer_sound_queue_count > 1) {
                int32_t i;
                for (i = 1; i < multiplayer_sound_queue_count; i++) {
                    multiplayer_sound_queue[i - 1] = multiplayer_sound_queue[i];
                }
            }
            multiplayer_sound_queue_count--;
            if (multiplayer_sound_queue_count != 0) {
                game_engine_play_multiplayer_sound(multiplayer_sound_queue[0].sound_index,
                    (datum_index)0xffffffff, multiplayer_sound_queue[0].broadcast);
            }
        }
    }
}

/**
 * Notifies the active game variant callback when an unclaimed item object is about to be despawned.
 *
 * Original register convention: EDX -> object_index.
 *
 * @address 0x45f510
 */
void Notifications::notify_item_expired(datum_index object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));
    uint32_t *extension_flags = (uint32_t *)((uint8_t *)obj + 0x22c);

    if (obj->parent_object == (datum_index)0xffffffff &&
        (item->flags & _item_in_inventory_bit) == 0 &&
        (*extension_flags & 0x20) != 0) {
        *extension_flags = *extension_flags & 0xffffffdf;
        if (current_game_engine != 0 && current_game_engine->object_expired != 0) {
            ((void (*)(datum_index))current_game_engine->object_expired)(object_index);
        }
    }
}

/**
 * Notifies the game variant about a pickup-flag state change and asks it whether the item pickup is permitted.
 *
 * Original register convention: stack -> unit_index, weapon_index.
 *
 * @address 0x462000
 */
uint8_t Notifications::notify_weapon_ready_state_change(datum_index unit_index, datum_index weapon_index)
{
    object *weapon;
    Object *weapon_definition;

    if (current_game_engine == 0) {
        return 1;
    }
    weapon = object_try_and_get(weapon_index, _object_mask_weapon);
    if (weapon == 0) {
        return 1;
    }
    weapon_definition = (Object *)tag_instances[
        (((object_header *)object_data->data)[weapon_index & 0xffff].data->definition_tag) & 0xffff
    ].data;
    if (((*(uint32_t *)((uint8_t *)weapon_definition + 0x308) >> 3) & 1) == 0) {
        return 1;
    }
    if ((((struct weapon_object *)weapon)->weapon.flags & 0x20) != 0) {
        ((struct weapon_object *)weapon)->weapon.flags &= 0xffffffdf;
        if (current_game_engine->object_expired != 0) {
            ((void (*)(datum_index))current_game_engine->object_expired)(weapon_index);
        }
    }
    ((struct weapon_object *)weapon)->weapon.flags |= 0x20;
    if (current_game_engine->weapon_ready_state_change != 0) {
        return ((uint8_t (*)(datum_index, datum_index))current_game_engine->weapon_ready_state_change)
            (weapon_index, player_index_from_unit_index(unit_index));
    }
    return 1;
}

}
