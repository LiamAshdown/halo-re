#include "halo/game/game2_engine_placement.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/game/variant_flags.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/game/constants.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/text/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

typedef struct netgame_equipment_spawn_message {
    int32_t object_hash;
    datum_index definition_tag;
    int16_t equipment_index;
    int16_t pad_0a;
} netgame_equipment_spawn_message;

extern "C" {
extern data_array *player_data;
extern double sqrt(double x);
extern double pow(double base, double exponent);
extern game_engine_definition *current_game_engine;
extern game_variant game_engine_variant;
extern Globals *global_globals;
extern int32_t game_engine_unknown_aa00;
extern uint8_t game_engine_map_table_value;
extern uint8_t network_object_index_cache[];
extern double fcos(double radians);
extern double fsin(double radians);
extern game_time_globals *game_time;
extern int32_t teleport_message_cooldown;
extern wchar_t empty_string;
extern int16_t teleport_flash_type;
extern uint32_t teleport_flash_maximum_intensity;
extern uint32_t teleport_flash_alpha;
extern uint32_t teleport_flash_red;
extern uint32_t teleport_flash_green;
extern uint32_t teleport_flash_blue;
extern uint32_t teleport_flash_duration;
extern int16_t teleport_flash_fade_function;
extern double atan2(double y, double x);
extern int game_engine_find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results);
extern int32_t game_engine_find_one_valid_starting_location(int16_t type, int16_t team, real_point3d *origin, float max_horizontal_dist, float max_height_delta);
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast);
extern int16_t unit_get_local_player_weapon_index(datum_index unit_index);
extern void chimera__hud_message(int16_t local_player_index, wchar_t *text);
extern void game_engine_compute_look_angles_from_vector(real_vector3d *facing, int16_t local_player_index);
extern void game_engine_scan_netgame_flags_noop(int16_t needle);
extern void game_engine_notify_item_expired(datum_index object_index);
extern int32_t game_engine_round_reset_tick;
}

namespace halo::game {

/**
 * Computes a scaling bonus that grows with the number of nearby same-team entities within a mid-range distance
 * band.
 *
 * @address 0x461c60
 */
float EnginePlacement::rate_location_ally_bonus(uint32_t self_index, real_point3d *point)
{
    player *self = halo::game::player_at(self_index);
    float bonus = 0.0f;
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = halo::memory::data_iterator_next(&iter);
    if (element != 0) {
        do {
            player *other = (player *)element;
            if (other->team == self->team && other->unit != (datum_index)halo::k_dword_none) {
                real_point3d other_position;
                float dx, dy, dz, distance;

                halo::objects::object_get_position(&other_position, other->unit);
                dx = point->x - other_position.x;
                dy = point->y - other_position.y;
                dz = point->z - other_position.z;
                distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

                if (1.0f <= distance && distance <= 6.0f) {
                    bonus = bonus + (float)pow((double)(1.0f - (distance - 1.0f) * 0.2f), (double)0.6f);
                }
            }
            element = halo::memory::data_iterator_next(&iter);
        } while (element != 0);

        if (3.0f < bonus) {
            bonus = 3.0f;
        }
    }
    return bonus * 3.0f + 1.0f;
}

/**
 * Scales a candidate spawn point by how crowded it is: 0 within 0.25 world units of any player's unit, 0.1 out
 * to 1.0, and for players on another team 0 below 2.0 ramping linearly back to full at 5.0. Only the starting-
 * location scorer calls it; the player itself is not excluded from the walk.
 *
 * @address 0x461ad0
 */
float EnginePlacement::rate_location_crowding(uint32_t self_index, real_point3d *point)
{
    uint8_t no_engine = (current_game_engine == 0);
    player *self = halo::game::player_at(self_index);
    uint8_t teams_enabled = (uint8_t)game_engine_variant.teams;
    float scale = 1.0f;
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = halo::memory::data_iterator_next(&iter);
    while (element != 0) {
        player *other = (player *)element;

        if (other->unit != (datum_index)halo::k_dword_none) {
            real_point3d other_position;
            float dx, dy, dz, distance;

            halo::objects::object_get_position(&other_position, other->unit);
            dx = point->x - other_position.x;
            dy = point->y - other_position.y;
            dz = point->z - other_position.z;
            distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

            if ((((no_engine ? 0u : halo::k_dword_none) & teams_enabled) == 0) ||
                (other->team != self->team) || (distance <= 0.25f)) {
                if (0.25f <= distance) {
                    if (distance < 1.0f) {
                        scale = scale * 0.1f;
                    }
                } else {
                    scale = 0.0f;
                }
                if (other->team != self->team) {
                    if (2.0f <= distance) {
                        if (distance <= 5.0f) {
                            scale = (distance - 2.0f) * scale * 0.33333334f;
                        }
                    } else {
                        scale = 0.0f;
                    }
                }
            }
        }
        element = halo::memory::data_iterator_next(&iter);
    }
    return scale;
}

/**
 * Scores one candidate player starting location for one player. In a team game whose engine says teams are on,
 * a location reserved for a different team scores 0 outright; otherwise the score starts as the crowding
 * penalty for the location's own position, is multiplied by the nearby-ally bonus when teams are on, and
 * finally by the active engine's own optional score override if it installs one.
 *
 * @address 0x461d90
 */
real EnginePlacement::rate_player_starting_location(ScenarioPlayerStartingLocation *location, datum_index player_handle)
{
    player *p = (player *)((uint8_t *)player_data->data
                           + ((uint32_t)player_handle & halo::k_datum_slot_mask) * sizeof(player));
    real score;

    if (current_game_engine == 0 || current_game_engine->unknown_84 == 0 ||
        ((char (*)(int32_t))current_game_engine->unknown_84)(0) == 0 ||
        p->team == (int32_t)(int16_t)location->team_index) {
        score = halo::game::game_engine_rate_location_crowding(player_handle,
                                                  (real_point3d *)&location->position);
    } else {
        score = 0.0f;
    }

    if (current_game_engine != 0) {
        if (0.0f < score && game_engine_variant.teams != 0) {
            score = halo::game::game_engine_rate_location_ally_bonus(player_handle,
                                                        (real_point3d *)&location->position) * score;
        }
        if (current_game_engine->rate_starting_location != 0) {
            real override_scale = ((real (*)(datum_index, ScenarioPlayerStartingLocation *))
                                   current_game_engine->rate_starting_location)(player_handle, location);
            return override_scale * score;
        }
    }
    return score;
}

/**
 * Given a player referenced start/placement object, dispatches to the netgame-flag or netgame-equipment
 * remapping routine based on that placement's stored type.
 *
 * @address 0x4630b0
 */
uint32_t EnginePlacement::remap_placement_by_type(uint32_t handle)
{
    int16_t type;

    if (current_game_engine == 0 || handle == halo::k_dword_none) {
        return handle;
    }
    type = *reinterpret_cast<int16_t *>(halo::game::tag_data_at(handle));
    if (type == 2) {
        return (uint32_t)halo::game::game_engine_resolve_netgame_flag_role(handle);
    }
    if (type == 3) {
        return halo::game::game_engine_resolve_multiplayer_placement(handle);
    }
    return handle;
}

/**
 * Resolves a scenario multiplayer object placement... to the actual object/index that should be used for the
 * active game variant, hiding it (-1) when the relevant vehicle set option/probability check fails.
 *
 * @address 0x462c30
 */
uint32_t EnginePlacement::resolve_multiplayer_placement(uint32_t handle)
{
    uint8_t *tag_data;
    int32_t weapon_list_count;
    TagDependency *weapon_list;
    int32_t index;
    int32_t i;

    tag_data = (handle == halo::k_dword_none) ? 0 : (uint8_t *)halo::game::tag_data_at(handle);

    weapon_list_count = (int32_t)global_globals->weapon_list.count;
    weapon_list = (weapon_list_count == 0) ? 0
        : (TagDependency *)global_globals->weapon_list.pointer;

    index = -1;
    for (i = 0; i < weapon_list_count; i++) {
        index = i;
        if (handle == *(uint32_t *)((uint8_t *)weapon_list + i * 0x10 + 0xc)) {
            break;
        }
        index = -1;
    }

    if (index != 0x0c && index != 0x0d) {
        if (tag_data == 0) {
            return handle;
        }
        if (*(int16_t *)(tag_data + 0x308) == 2) {
            return (!halo::game::variant_flag_set(game_engine_variant.flags, halo::game::game_variant_flags::shields_disabled)) ? handle : halo::k_dword_none;
        }
        if (*(int16_t *)(tag_data + 0x308) != 3) {
            return handle;
        }
        return (!halo::game::variant_flag_set(game_engine_variant.flags, halo::game::game_variant_flags::invisible_players)) ? handle : halo::k_dword_none;
    }

    switch (game_engine_variant.weapon_set) {
    case 3:
    case 10:
        index = 0x0d;
        break;
    case 9:
        index = 0x0c;
        break;
    case 0x0d:
        index = -1;
        break;
    default:
        break;
    }

    if ((game_engine_unknown_aa00 & 4) == 0 && halo::game::variant_flag_set(game_engine_variant.flags, halo::game::game_variant_flags::maximum_grenades)) {
        index = -1;
    }

    if ((game_engine_unknown_aa00 & 8) == 0) {
        if ((game_engine_unknown_aa00 & 4) != 0) {
            float roll;
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            roll = (float)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f;
            if (!(roll < 0.55f) && roll != 0.55f) {
                index = -1;
            }
        }
    } else {
        float roll;
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        roll = (float)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f;
        if (!(roll < 0.3f) && roll != 0.3f) {
            index = -1;
        }
    }

    if (index == -1) {
        return halo::k_dword_none;
    }
    return *(uint32_t *)((uint8_t *)global_globals->weapon_list.pointer + 0xc + index * 0x10);
}

/**
 * Finds `handle` in the globals tag's weapon list, remaps that list index according to the variant's
 * starting_equipment setting, and returns the tag id of the weapon the remapped index names. Returns `handle`
 * unchanged when it is not in the list, when its index is 10 or 11, or when the "no duals" style flag
 * (game_variant::flags bit 7) rules the substitution out; returns -1 when the remap produces -1.
 *
 * @address 0x462df0
 */
int32_t EnginePlacement::resolve_netgame_flag_role(uint32_t handle)
{
    int32_t weapon_list_count = (int32_t)global_globals->weapon_list.count;
    TagDependency *weapon_list = (weapon_list_count == 0) ? 0
        : (TagDependency *)global_globals->weapon_list.pointer;
    int32_t index = -1;
    int32_t i;

    for (i = 0; i < weapon_list_count; i++) {
        if (handle == *(uint32_t *)&weapon_list[i].tag_id) {
            index = i;
            break;
        }
    }

    if (game_engine_map_table_value != 0 && halo::game::variant_flag_set(game_engine_variant.flags, halo::game::game_variant_flags::object_placement_filter) &&
        (index == 1 || index == 0x0e)) {
        return -1;
    }
    if (index == 10 || index == 0x0b || index == -1) {
        return (int32_t)handle;
    }

    switch (game_engine_variant.weapon_set) {
    case 1: /* 0x462e85 */
        switch (index) {
        case 3: case 4: case 6: case 7: case 0x0e: case 0x0f: index = 5; break;
        default:                                   index = 4; break;
        }
        break;

    case 2: /* 0x462e9f */
        switch (index) {
        case 3: case 4: case 6: case 7: case 0x0e: case 0x0f: index = 6; break;
        default:                                   index = 0; break;
        }
        break;

    case 3: /* 0x462ebf */
        if (index < 3 || (5 < index && index != 0x0f)) {
            index = 6;
        } else {
            index = 5;
        }
        break;

    case 4: /* 0x462ed8 */
        if (index != 4 && index != 9) {
            index = 9;
        }
        break;

    case 5: /* 0x462ef4 */
        if (index == 4) {
            index = 0;
        } else if (index == 9) {
            index = 8;
        }
        break;

    case 6: /* 0x462fad */
        index = 7;
        break;

    case 7: /* 0x462f02 */
        index = 8;
        break;

    case 8: /* 0x462f13 */
        switch (index) {
        case 1: case 7: case 0x0e: index = 1; break;
        case 4: case 8:            index = 8; break;
        case 6: case 9:            index = 6; break;
        default:                   break;
        }
        break;

    case 9: /* 0x462f34 */
        switch (index) {
        case 3: case 6: case 0x0f: index = 0; break;
        case 5:                    index = 4; break;
        case 0x0e:                 index = 7; break;
        default:                   break;
        }
        break;

    case 10: /* 0x462f79 */
        switch (index) {
        case 0: case 1: case 9: index = 6; break;
        case 3: case 8:         index = 0x0f; break;
        case 4:                 index = 5; break;
        case 7:                 index = 0x0e; break;
        default:                break;
        }
        break;

    case 0x0b: /* 0x462f51 */
        if (index == 1) {
            index = (game_engine_map_table_value != 0) ? -1 : 8;
        } else if (index == 0x0e) {
            index = (game_engine_map_table_value == 0) ? 7 : -1;
        }
        break;

    case 0x0c: /* 0x462f9a */
        switch (index) {
        case 0: case 4: case 7: case 9:            index = 7; break;
        case 1: case 3: case 8: case 0x0f:         index = 1; break;
        case 5: case 6: case 0x0e:                 index = 0x0e; break;
        default:                                   break;
        }
        break;

    default: /* starting_equipment - 1 > 0xb: `ja 0x462fb2`, index untouched */
        break;
    }

    if (index == -1) {
        return -1;
    }
    return *(int32_t *)((uint8_t *)global_globals->weapon_list.pointer + 0xc + index * 0x10);
}

/**
 * Low-confidence: appears to search a scenario placement list for a matching short identifier, but the
 * decompiled loop body has no observable effect, suggesting lost side effects (for example an element index or
 * match flag this decompilation failed to capture)
 *
 * @address 0x4637c0
 */
void EnginePlacement::scan_netgame_flags_noop(int16_t needle)
{
    int32_t count = (int32_t)halo::scenario::globals().scenario->netgame_flags.count;
    int32_t i;

    if (0 < count) {
        int16_t next = 1;
        for (i = 0; i < count; ) {
            int16_t j = next;
            if (needle == ((ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer)[i].type) {
                for (; j < count; j++) {
                }
            }
            i = next;
            next = next + 1;
        }
    }
}

/**
 * If `*message == 0`, decodes the message and spawns the netgame-equipment item it names locally at its
 * scenario-configured position/facing; otherwise (`*message != 0`) treats this as a networked replay and
 * defers entirely to message_delta_decode_compound_field_staged.
 *
 * @address 0x45f8f0
 */
void EnginePlacement::spawn_or_replay_netgame_equipment(int32_t *message)
{
    netgame_equipment_spawn_message decoded;
    ScenarioNetgameEquipment *equipment;
    object_placement_data placement;
    datum_index new_object;

    if (*(int32_t *)*(int32_t **)message != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)message);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)message, &decoded) == 0) {
        return;
    }

    equipment = &((ScenarioNetgameEquipment *)halo::scenario::globals().scenario->netgame_equipment.pointer)[decoded.equipment_index];
    if (equipment == 0) {
        return;
    }

    halo::objects::object_placement_data_initialize(&placement, decoded.definition_tag, k_datum_index_none);
    placement.position.x = equipment->position.x;
    placement.position.y = equipment->position.y;
    placement.position.z = equipment->position.z;
    placement.forward.i = (float)fcos(equipment->facing);
    placement.forward.j = (float)fsin(equipment->facing);
    placement.forward.k = 0.0f;

    new_object = halo::objects::object_new_with_datum_role_control(&placement, 1);
    if (new_object != (datum_index)halo::k_dword_none) {
        object *obj = halo::game::object_at(new_object);

        halo::networking::network_index_cache_insert_if_free(network_object_index_cache, decoded.object_hash, (int32_t)new_object);
        halo::objects::object_list_membership_set(new_object, 0);
        if ((*(uint8_t *)equipment & 1) != 0) {
            obj->flags = obj->flags | 0x20;
        }
        halo::objects::object_type_override_call_0x68(new_object);
    }
}

/**
 * Per-tick update that respawns scenario netgame-equipment items once their configured respawn timer has
 * elapsed.
 *
 * @address 0x45f9f0
 */
void EnginePlacement::update_netgame_equipment(char force_respawn)
{
    int16_t loop_index;
    int32_t count = halo::scenario::globals().scenario->netgame_equipment.count;

    if (count <= 0) {
        return;
    }

    for (loop_index = 0; loop_index < count; loop_index++) {
        ScenarioNetgameEquipment *equipment =
            &((ScenarioNetgameEquipment *)halo::scenario::globals().scenario->netgame_equipment.pointer)[loop_index];
        datum_index item_collection_tag = *(datum_index *)&equipment->item_collection.tag_id;

        if (!halo::game::netgame_equipment_game_type_matches((int16_t *)&equipment->type_0, 4,
                current_game_engine != 0 ? current_game_engine->index : -1)) {
            continue;
        }

        {
            int32_t respawn_interval = 900;
            int32_t extra = (int32_t)((float)loop_index / (float)count * 300.0f);

            if (equipment->spawn_time != 0) {
                respawn_interval = equipment->spawn_time * 0x1e;
            } else if (item_collection_tag != (datum_index)halo::k_dword_none) {
                int16_t permutation_count =
                    *(int16_t *)((uint8_t *)halo::game::tag_data_at(item_collection_tag) + 0x0c);
                if (permutation_count != 0) {
                    respawn_interval = permutation_count * 0x1e;
                }
            }
            respawn_interval = respawn_interval + extra;

            {
                int32_t now = game_time->game_time;

                if (now % respawn_interval == 0 || force_respawn == 1) {
                    if (equipment->spawned_item != halo::k_dword_none) {
                        object *existing = halo::objects::object_try_and_get((datum_index)equipment->spawned_item, _object_mask_item);
                        if (existing != 0 && (((item_data *)((uint8_t *)existing + sizeof(object)))->flags & 0x40) != 0) {
                            float dx = existing->position.x - equipment->position.x;
                            float dy = existing->position.y - equipment->position.y;
                            float dz = existing->position.z - equipment->position.z;
                            float dist = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

                            if (dist <= 0.5f || (existing->flags & 0x20) == 0) {
                                ((item_data *)((uint8_t *)existing + sizeof(object)))->held_game_time =
                                    respawn_interval - 900 + now;
                                continue;
                            }
                            halo::objects::object_delete((datum_index)equipment->spawned_item);
                        }
                        equipment->spawned_item = halo::k_dword_none;
                    }

                    {
                        object_placement_data placement;
                        datum_index new_object;
                        int32_t picked_tag = halo::game::tag_reflexive_pick_weighted_random_index(item_collection_tag);

                        halo::objects::object_placement_data_initialize(&placement, (datum_index)picked_tag, k_datum_index_none);

                        placement.position.x = equipment->position.x;
                        placement.position.y = equipment->position.y;
                        placement.position.z = equipment->position.z;
                        placement.forward.i = (float)fcos(equipment->facing);
                        placement.forward.j = (float)fsin(equipment->facing);
                        placement.forward.k = 0.0f;

                        new_object = halo::objects::object_new_with_datum_role_control(&placement, 3);
                        if (new_object != (datum_index)halo::k_dword_none) {
                            object *obj = halo::game::object_at(new_object);
                            item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));

                            halo::objects::object_list_membership_set(new_object, 0);
                            if (((uint8_t *)equipment)[0] & 1) {
                                obj->flags = obj->flags | 0x20;
                            }
                            obj->network_role = 0;
                            halo::objects::object_type_override_call_0x68(new_object);
                            halo::game::game_engine_dispatch_item_pickup_event(new_object, obj->definition_tag, loop_index);

                            item->held_game_time = item->held_game_time + respawn_interval - 900;

                            if (*(int32_t *)halo::game::tag_data_at(item_collection_tag) == 1) {
                                item->flags = item->flags | 0x40;
                                equipment->spawned_item = new_object;
                            } else {
                                item->flags = item->flags & ~0x40u;
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Handles teleporting a unit through a level teleporter, finding and validating a destination and reporting
 * failures.
 *
 * @address 0x461630
 */
void EnginePlacement::update_teleporter(uint32_t player_index)
{
    player *p = halo::game::player_at(player_index);
    datum_index unit = p->unit;
    object *unit_object;
    int32_t found_index;

    if (unit == (datum_index)halo::k_dword_none) {
        return;
    }
    unit_object = halo::game::object_at(unit);

    if (p->teleporter_flag_index != (datum_index)halo::k_dword_none) {
        ScenarioNetgameFlags *cached = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer
            + (int32_t)p->teleporter_flag_index;
        float dx = unit_object->position.x - cached->position.x;
        float dy = unit_object->position.y - cached->position.y;
        float dz = unit_object->position.z - cached->position.z;
        if (1.0f < dx * dx + dy * dy + dz * dz) {
            p->teleporter_flag_index = (datum_index)halo::k_dword_none;
        }
    }

    found_index = -1;
    halo::game::game_engine_find_valid_starting_locations(&unit_object->position, 0.5f, 0.0f, 6, -1, 1, &found_index);

    if (found_index != -1 && found_index != (int32_t)p->teleporter_flag_index) {
        ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)halo::scenario::globals().scenario->netgame_flags.pointer;
        ScenarioNetgameFlags *entrance = &flags[found_index];
        int16_t entrance_usage_id = (int16_t)entrance->usage_id;

        found_index = -1;
        halo::game::game_engine_find_valid_starting_locations(0, 0.0f, 0.0f, 7, entrance_usage_id, 1, &found_index);

        if (found_index == -1) {
            halo::main::console_print_error_va(0, "failed to teleport %d", (int32_t)entrance_usage_id);
        } else {
            ScenarioNetgameFlags *exit_flag = &flags[found_index];
            real_vector3d forward;
            real_point3d destination_position;
            float pill_height;
            float pill_radius;
            physics_model candidates;
            physics_model_contact contact;
            uint8_t blocked;

            unit_object = halo::game::object_at(unit);
            forward = unit_object->forward;
            p = halo::game::player_at(player_index);
            halo::units::unit_get_crouch_height_offset(&destination_position, p->unit, &pill_height, &pill_radius);

            destination_position.x = exit_flag->position.x;
            destination_position.y = exit_flag->position.y;
            destination_position.z = exit_flag->position.z;

            blocked = halo::physics::physics_model_build_from_sphere_query(0x200380, &destination_position,
                pill_radius + pill_radius + pill_height, pill_height, pill_radius, halo::k_dword_none, &candidates);

            if (blocked != 0) {
                blocked = halo::physics::physics_shape_test_point(&candidates, &destination_position, &contact);
            }

            if (blocked != 0) {
                datum_index obstruction = contact.object_index;

                if (obstruction != (datum_index)halo::k_dword_none) {
                    object *blocker = halo::game::object_at(obstruction);
                    if (((1 << blocker->type) & _object_mask_unit) != 0) {
                        datum_index controller =
                            ((unit_data *)((uint8_t *)blocker +
                                           k_unit_data_offset))->controlling_player;
                        if (controller != (datum_index)halo::k_dword_none) {
                            player *other = halo::game::player_at(controller);
                            other->telefrag_danger = 1;
                            *(int32_t *)((uint8_t *)other + 0xcc) =
                                *(int32_t *)((uint8_t *)other + 0xcc) + 1;
                        }
                    }
                }

                if (teleport_message_cooldown < 1) {
                    wchar_t *text;
                    datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

                    teleport_message_cooldown = 0x78;
                    text = (tag_id == k_datum_index_none) ? &empty_string
                        : reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, 0x65));
                    halo::interface::chimera__hud_message(halo::game::unit_get_local_player_weapon_index(p->unit), text);
                    return;
                }
                teleport_message_cooldown = teleport_message_cooldown - 1;
                return;
            }

            if (p->local_player_index != -1) {
                halo::game::game_engine_queue_multiplayer_sound(0x1b, halo::k_dword_none, 0);
                if (p->local_player_index != -1) {
                    player_screen_flash flash;
                    uint8_t *flash_bytes = (uint8_t *)&flash;
                    int32_t i;

                    for (i = 0; i < (int32_t)sizeof(flash); i++) {
                        flash_bytes[i] = 0;
                    }
                    flash.type = teleport_flash_type;
                    flash.priority = 2;
                    flash.duration = *(float *)&teleport_flash_duration;
                    flash.fade_function = (uint16_t)teleport_flash_fade_function;
                    flash.maximum_intensity = teleport_flash_maximum_intensity;
                    flash.intensity = 0.0f;
                    flash.color.alpha = *(float *)&teleport_flash_alpha;
                    flash.color.red = *(float *)&teleport_flash_red;
                    flash.color.green = *(float *)&teleport_flash_green;
                    flash.color.blue = *(float *)&teleport_flash_blue;
                    halo::effects::player_effect_set_screen_flash_for_player(player_index, &flash, 1.0f);
                }
            }

            {
                float yaw = (float)atan2(forward.j, forward.i);
                yaw = (yaw + exit_flag->facing) - entrance->facing;
                forward.i = (float)fcos(yaw);
                forward.j = (float)fsin(yaw);
                halo::math::vector3d_normalize_with_length(forward);

                halo::objects::object_set_position_and_orientation(unit, &forward, 0, (real_point3d *)(&exit_flag->position));

                if (p->local_player_index != -1) {
                    halo::game::game_engine_compute_look_angles_from_vector(&forward,
                        p->local_player_index);
                }

                p->teleporter_flag_index = (datum_index)halo::game::game_engine_find_one_valid_starting_location(-1, 6,
                    &unit_object->position, 1.0f, 0.0f);

                if ((unit_object->network_role == 1 || unit_object->network_role == 2) &&
                    p->local_player_index != -1 && halo::networking::globals().client != 0) {
                    halo::networking::player_update_history_free_all((player_update_history *)(*(void **)&halo::networking::globals().client->update_history));
                    return;
                }
            }
        }
    }
}

/**
 * Low-confidence: repeatedly walks scenario placement-list counts with no clear side effect in the
 * decompilation; may be a validation/ assert routine or a decompiler artifact.
 *
 * @address 0x463810
 */
void EnginePlacement::validate_scenario_placements_noop(void)
{
    int32_t netgame_flags_count;
    int32_t netgame_equipment_count;
    int16_t i;
    int32_t pass;

    halo::game::game_engine_scan_netgame_flags_noop(0);

    netgame_flags_count = (int32_t)halo::scenario::globals().scenario->netgame_flags.count;
    i = 0;
    if (0 < netgame_flags_count) {
        do {
            i = i + 1;
        } while (i < netgame_flags_count);
    }

    halo::game::game_engine_scan_netgame_flags_noop(0);

    netgame_equipment_count = (int32_t)halo::scenario::globals().scenario->netgame_equipment.count;
    for (pass = 0; pass < 5; pass++) {
        i = 0;
        if (0 < netgame_equipment_count) {
            do {
                i = i + 1;
            } while (i < netgame_equipment_count);
        }
    }
}

/**
 * Touches the predicted-resource list of a tag when the tag id is valid.
 */
void EnginePlacement::touch_tag_if_valid(int32_t tag_id)
{
    uint8_t *tag_data;
    if (tag_id != -1) {
        tag_data = (uint8_t *)halo::game::tag_data_at(tag_id);
        halo::cache::predicted_resource_list_touch((TagReflexive *)(tag_data + 0x170));
    }
}

/**
 * Pre-touches (loads/pins) the predicted-resource lists of every multiplayer vehicle and weapon tag the
 * current map's globals tag might spawn: one vehicle selected by the active variant's vehicle_set nibble (or
 * all six when that nibble is 0, 1, 5 or 8), the oddball ball weapon when the engine is oddball, the ctf flag
 * weapon when the engine is ctf, and every other weapon-list entry except the three slots already ...
 *
 * @address 0x466890
 */
void EnginePlacement::touch_multiplayer_predicted_resources(void)
{
    GlobalsMultiplayerInformation *mp_info;
    GlobalsVehicle *vehicles;
    GlobalsWeapon *weapons;
    int32_t weapon_tags[16];
    int32_t i;

    mp_info = (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
    vehicles = (GlobalsVehicle *)mp_info->vehicles.pointer;

    switch (game_engine_variant.red_vehicle_set & 0xf) {
    case 2:
        halo::objects::object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[0].vehicle.tag_id);
        break;
    case 3:
        halo::objects::object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[1].vehicle.tag_id);
        break;
    case 4:
        halo::objects::object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[2].vehicle.tag_id);
        break;
    case 6:
        halo::objects::object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[3].vehicle.tag_id);
        break;
    case 7:
        halo::objects::object_notify_predicted_resources_if_valid((datum_index)*(int32_t *)&vehicles[4].vehicle.tag_id);
        break;
    default:
        touch_tag_if_valid(*(int32_t *)&vehicles[0].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[1].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[2].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[3].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[4].vehicle.tag_id);
        touch_tag_if_valid(*(int32_t *)&vehicles[5].vehicle.tag_id);
        break;
    }

    weapons = (GlobalsWeapon *)global_globals->weapon_list.pointer;

    if (game_engine_variant.game_engine_index == _game_engine_oddball) {
        touch_tag_if_valid(*(int32_t *)&weapons[10].weapon.tag_id);
    }
    if (game_engine_variant.game_engine_index == _game_engine_ctf) {
        touch_tag_if_valid(*(int32_t *)&weapons[11].weapon.tag_id);
    }

    weapon_tags[0] = *(int32_t *)&weapons[0].weapon.tag_id;
    weapon_tags[1] = *(int32_t *)&weapons[1].weapon.tag_id;
    weapon_tags[2] = *(int32_t *)&weapons[2].weapon.tag_id;
    weapon_tags[3] = -1;
    weapon_tags[4] = *(int32_t *)&weapons[4].weapon.tag_id;
    weapon_tags[5] = *(int32_t *)&weapons[5].weapon.tag_id;
    weapon_tags[6] = *(int32_t *)&weapons[6].weapon.tag_id;
    weapon_tags[7] = *(int32_t *)&weapons[7].weapon.tag_id;
    weapon_tags[8] = *(int32_t *)&weapons[8].weapon.tag_id;
    weapon_tags[9] = *(int32_t *)&weapons[9].weapon.tag_id;
    weapon_tags[10] = -1;
    weapon_tags[11] = -1;
    weapon_tags[12] = *(int32_t *)&weapons[12].weapon.tag_id;
    weapon_tags[13] = *(int32_t *)&weapons[13].weapon.tag_id;
    weapon_tags[14] = *(int32_t *)&weapons[14].weapon.tag_id;
    weapon_tags[15] = *(int32_t *)&weapons[15].weapon.tag_id;

    for (i = 0; i < 16; i = i + 1) {
        if (weapon_tags[i] != -1) {
            halo::objects::object_notify_predicted_resources_if_valid((datum_index)weapon_tags[i]);
        }
    }
}

/**
 * Every tick, sets each item's render/collision scale from its Item tag (or 1.0 while the item is held), then,
 * for items whose game engine implements object_in_play_update, notifies both the expiry hook and the engine's
 * own per-tick pickup-eligibility callback for weapons that are still fully attached (root parent, tag flag
 * bit 3 set).
 *
 * @address 0x45f560
 */
void EnginePlacement::update_item_scale_and_pickup(void)
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = _object_mask_item;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = halo::objects::object_iterator_next(&iterator);
    while (obj != 0) {
        item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));

        if ((item->flags & _item_in_inventory_bit) == 0) {
            Item *tag = (Item *)halo::game::tag_data_at(obj->definition_tag);
            obj->scale = (tag->scale == 0.0f) ? 1.0f : tag->scale;
        } else {
            obj->scale = 1.0f;
        }

        if (current_game_engine != 0 && current_game_engine->object_in_play_update != 0) {
            object_header *hdr = (object_header *)halo::memory::datum_get(iterator.handle, halo::objects::globals().object_data);

            if (hdr != 0 && (1u << hdr->type) == _object_mask_weapon && hdr->data != 0 &&
                ((*(uint32_t *)((uint8_t *)halo::game::tag_data_at(obj->definition_tag) + 0x308) >> 3) & 1) != 0) {
                halo::game::game_engine_notify_item_expired(iterator.handle);
                ((void (*)(datum_index, object *))current_game_engine->object_in_play_update)(
                    iterator.handle, hdr->data);
            }
        }

        obj = halo::objects::object_iterator_next(&iterator);
    }
}

/**
 * The engine's per-round reset: clears every unit's grenade counts, respawns/cleans up bipeds, deletes stray
 * items, deletes stray projectiles, runs the loaded gametype's own reset_objects hook (if any), stamps the
 * current game_time, force-reloads netgame equipment, resets/cleans vehicles (or does Race's stray-vehicle
 * sweep), and clears the cached player profile stats.
 *
 * @address 0x468260
 */
void EnginePlacement::reset_round_objects(void)
{
    halo::game::game_engine_reset_all_unit_grenade_counts();
    halo::game::game_engine_reset_respawns_and_cleanup_bipeds();
    halo::game::game_engine_cleanup_stray_items();
    halo::game::game_engine_cleanup_stray_projectiles();
    if (current_game_engine->reset_objects != 0) {
        ((void (*)(void))current_game_engine->reset_objects)();
    }
    game_engine_round_reset_tick = game_time->game_time;
    halo::game::game_engine_update_netgame_equipment(1);
    halo::game::game_engine_reset_vehicles_or_race_cleanup();
    halo::game::game_engine_reset_player_profile_stats();
}

/**
 * In Race, deletes any unparented/stray vehicle exactly like the item and projectile sweeps (network_role 0 ->
 * both delete calls, network_role 3 -> recursive delete only). In every other gametype, instead resets every
 * live vehicle's facing via unit_set_facing_from_index_table.
 *
 * @address 0x4681a0
 */
void EnginePlacement::reset_vehicles_or_race_cleanup(void)
{
    object_iterator iter;
    object *obj;

    iter.type_mask = _object_mask_vehicle;
    iter.flags_mask = 0;
    iter.unknown_05 = 0;
    iter.index = 0;
    iter.handle = (datum_index)halo::k_dword_none;

    if (game_engine_variant.game_engine_index == _game_engine_race) {
        obj = halo::objects::object_iterator_next(&iter);
        while (obj != (object *)0) {
            if (obj->network_role == 0) {
                halo::objects::object_delete_unparented(iter.handle);
                halo::objects::object_delete_recursive(iter.handle, 0);
            } else if (obj->network_role == 3) {
                halo::objects::object_delete_recursive(iter.handle, 0);
            }
            obj = halo::objects::object_iterator_next(&iter);
        }
    } else {
        obj = halo::objects::object_iterator_next(&iter);
        while (obj != (object *)0) {
            halo::units::unit_set_facing_from_index_table((uint32_t)iter.handle);
            obj = halo::objects::object_iterator_next(&iter);
        }
    }
}

/**
 * Low-confidence: builds a packed flags/color byte for an object from the game engine option bitfield when not
 * in the DAT_0087aa00-bit2 mode, otherwise passes the input through unchanged in its low byte.
 *
 * @address 0x462bd0
 */
uint32_t EnginePlacement::pack_object_flags_or_passthrough(uint32_t input)
{
    uint32_t result = input & 0xffffff00u;

    if ((game_engine_unknown_aa00 & 4) == 0) {
        result = (((game_engine_variant.flags >> 10) << 8) |
                   (uint8_t)(game_engine_variant.flags >> 2)) & 0xffffff01u;
    }
    return result;
}

}
