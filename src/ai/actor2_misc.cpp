#include "halo/ai/actor_view.hpp"
#include "halo/models/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"

namespace halo::ai {

namespace actor_new_local {
}

/**
 * Allocates and default-initializes a new actor record for the unit type referenced by the given ActorVariant
 * tag: resolves the Actor tag it points at, allocates a datum, then writes every field whose default the engine
 * cares about (the great majority to -1/none, 0, or a small constant), copies actor.
 *
 * @address 0x426760
 */
datum_index ActorOps::run_new(datum_index actor_variant_tag)
{
    using namespace actor_new_local;
    ActorVariant *variant;
    datum_index actor_definition_tag;
    Actor *actor_tag;
    datum_index actor_index;
    actor *self;
    uint32_t flags;

    if (actor_variant_tag == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    variant = (ActorVariant *)(halo::cache::globals().tag_instances[actor_variant_tag & halo::k_slot_mask].data);
    actor_definition_tag = *(datum_index *)&variant->actor_definition.tag_id;
    if (actor_definition_tag == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    actor_tag = (Actor *)(halo::cache::globals().tag_instances[actor_definition_tag & halo::k_slot_mask].data);

    actor_index = halo::memory::datum_new(halo::ai::globals().actor_data);
    if (actor_index == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    flags = *(uint32_t *)actor_tag;

    self->actor_variant_tag = actor_variant_tag;
    self->swarm = (uint8_t)(flags >> 0x1a) & 1;
    self->actor_definition_tag = actor_definition_tag;
    self->type = actor_tag->type;
    self->unit_index = (datum_index)k_datum_index_none;
    self->counts_toward_encounter = 0;
    self->encounter_index = (datum_index)k_datum_index_none;
    self->squad_index = -1;
    self->platoon_index = -1;
    self->encounterless = 0;
    self->original_encounter_index = (datum_index)k_datum_index_none;
    self->original_squad_index = -1;
    self->cluster_count = 0;
    self->total_cluster_count = 0;
    self->cluster_unit_index = (datum_index)k_datum_index_none;
    self->swarm_index = (datum_index)k_datum_index_none;
    self->unit_control_pending = 1;
    self->active = 0;
    self->deactivation_time = (datum_index)k_datum_index_none;
    self->keep_unit_alive = 1;
    self->can_go_dormant = 1;
    self->idle_counter = 0;
    self->first_prop = (datum_index)k_datum_index_none;
    self->nearest_orphan_prop_index = (datum_index)k_datum_index_none;
    self->firing_position_index = -1;
    self->pending_order_request = -1;
    self->standing_order_request = -1;
    self->last_order_request_time = -1;
    self->command_list_run_immediately = 0;
    self->pending_command_list = -1;
    self->command_list_finished_time = -1;
    self->mode = 0;
    self->awareness_level = 2;
    self->combat_status = 0;
    self->minimum_combat_status = 0;
    self->suspicion_status = 0;
    self->ticks_since_threatened = -1;
    self->search_firing_positions = 0;
    self->flying = (uint8_t)(flags >> 0x15) & 1;
    self->pathfinding_surface_index = -1;
    self->active_unit_index = (datum_index)k_datum_index_none;
    self->platoon_defending = 0;
    self->grenade_ally_phase_flag = 0;
    self->nearby_friend_prop_index = (datum_index)k_datum_index_none;
    self->try_to_fight_type = 0;
    self->conversation_index = (datum_index)k_datum_index_none;

    memset(&self->threat_level, 0, 0x1a * sizeof(uint32_t));

    self->last_cover_attempt_time = (datum_index)k_datum_index_none;
    *(uint32_t *)&self->search_wait_time = halo::k_dword_none;
    *(uint32_t *)&self->last_melee_time = halo::k_dword_none;
    self->last_evasion_time = (datum_index)k_datum_index_none;
    self->last_vehicle_search_time = halo::k_dword_none;
    *(uint32_t *)&self->last_vehicle_charge_time = halo::k_dword_none;
    self->last_flee_abort_time = (datum_index)k_datum_index_none;
    self->found_body_time = (datum_index)k_datum_index_none;
    self->retreat_end_time = (datum_index)k_datum_index_none;
    self->retreat_prop_index = (datum_index)k_datum_index_none;
    self->retreat_start_time = (datum_index)k_datum_index_none;
    self->stood_down_body_vitality = 1.0f;
    self->exited_vehicle_index = (datum_index)k_datum_index_none;
    self->exited_vehicle_reentry_time = (datum_index)k_datum_index_none;
    self->panic_cooldown_time = (datum_index)k_datum_index_none;

    if (actor_tag->glass_ignorance_chance > 0.0f) {
        self->ignores_glass = halo::math::random_real() < actor_tag->glass_ignorance_chance;
    }

    self->flee_reason = 0;
    self->secondary_action = 0;
    self->movement_completed = 0;
    self->destination_surface_index = halo::k_dword_none;
    self->destination_radius = halo::k_dword_none;

    memset(&self->movement_action_complete, 0, 0x17 * sizeof(uint32_t));

    self->moving = 0;
    self->forced_aim = 0;
    self->firing_state = 1;
    self->firing_state_timer = 0;
    self->firing_delay_timer = 0;
    self->refire_timer = 0;
    self->line_of_fire_blocked_ticks = 0;
    self->firing_target_ticks = 0;
    self->firing_target_prop_index = (datum_index)k_datum_index_none;
    self->last_grenade_check_time = halo::k_dword_none;
    self->grenade_target_prop_index = halo::k_dword_none;
    self->avoidance_ray_clear_ticks = (datum_index)k_datum_index_none;
    self->unknown_5cc = (datum_index)k_datum_index_none;
    self->unknown_5d0 = (datum_index)k_datum_index_none;
    self->unknown_5d4 = (datum_index)k_datum_index_none;
    self->avoidance_last_direction = -1;
    self->avoidance_turn_around_ticks = -1;
    self->vocalization_line = 0;
    self->vocalization_state = 0;
    self->desired_aiming_vector = *(const real_point3d *)halo::math::globals().global_forward3d_pointer;
    self->desired_facing_vector = *(const real_point3d *)halo::math::globals().global_forward3d_pointer;
    self->desired_looking_vector = *(const real_point3d *)halo::math::globals().global_forward3d_pointer;
    self->grenade_eligible = 0;
    self->grenade_recheck_ticks = 30;
    self->target_combat_status = 0;
    self->target_unit_index = (datum_index)k_datum_index_none;
    self->target_last_seen_time = (datum_index)k_datum_index_none;
    self->ticks_since_engaged = halo::k_dword_none;

    halo::ai::actor_clear_recognition_history(actor_index, 0);

    self->pursuit_target_prop_index = (datum_index)k_datum_index_none;
    halo::ai::actor_dispatch_type_vtable_0x10(actor_index);

    return actor_index;
}

namespace actor_new_and_attach_to_unit_local {
extern "C" {
extern void *actor_type_procs[16];
#define ACTOR_AT(index) ((uint8_t *)halo::ai::globals().actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: new and attach to unit.
 *
 * @address 0x426ac0
 */
datum_index ActorOps::new_and_attach_to_unit(char reuse_existing, datum_index unit_index, datum_index actor_variant_tag, uint32_t encounter_or_none, int16_t squad_index, char ignore_squad, datum_index exclude_actor, char start_active, uint16_t unknown_60, int16_t unknown_62, uint16_t unknown_90, uint8_t unknown_68)
{
    using namespace actor_new_and_attach_to_unit_local;
    datum_index actor_index = k_datum_index_none;
    uint8_t *self;

    if (unit_index == k_datum_index_none || actor_variant_tag == k_datum_index_none) {
        return k_datum_index_none;
    }

    if (reuse_existing != 0) {
        datum_index cursor[4];
        datum_index candidate;

        halo::ai::ai_reference_actor_iterator_init_cursor((int32_t)encounter_or_none, cursor);
        candidate = cursor[2];
        while (halo::ai::globals().state->actors_valid != 0 && candidate != k_datum_index_none) {
            uint8_t *actor = ACTOR_AT(candidate);

            actor_index = candidate;
            candidate = ((struct actor *)actor)->next_in_encounter;
            if (actor[6] == 0 || actor_index == exclude_actor || ((struct actor *)actor)->cluster_count >= 0x10 ||
                ((struct actor *)actor)->actor_variant_tag != actor_variant_tag ||
                (ignore_squad == 0 && ((struct actor *)actor)->squad_index != squad_index)) {
                continue;
            }
            goto attach;
        }
    } else {
        uint8_t *unit = (uint8_t *)halo::objects::object_try_and_get(unit_index, 1);

        if (unit == 0 || (unit[0x106] & 4) != 0) {
            return k_datum_index_none;
        }
    }

    actor_index = halo::ai::actor_new(actor_variant_tag);
    if (actor_index == k_datum_index_none) {
        return k_datum_index_none;
    }
    self = ACTOR_AT(actor_index);
    if (encounter_or_none == (uint32_t)k_datum_index_none) {
        halo::ai::ai_actor_link_to_unassigned_list(actor_index);
    } else {
        if ((encounter_or_none & 0xffff0000) == 0) {
            uint8_t *enc = (uint8_t *)halo::ai::globals().encounter_data->data + (encounter_or_none & halo::k_slot_mask) * k_encounter_size;

            encounter_or_none = ((uint32_t)(int32_t)*(int16_t *)enc << 0x10) | (encounter_or_none & halo::k_slot_mask);
        }
        halo::ai::encounter_add_actor(squad_index, actor_index, encounter_or_none, 0);
    }
    if (start_active == 0) {
        *(int16_t *)(self + 0x6a) = 2;
    } else {
        *(int16_t *)(self + 0x6a) = 0;
        if (self[8] != 0) {
            halo::ai::actor_set_units_active(actor_index, 0);
        }
    }
    *(uint16_t *)(self + 0x60) = unknown_60;
    *(int16_t *)(self + 0x62) = unknown_62;
    if (unknown_62 == -1 || unknown_62 == 0) {
        *(int16_t *)(self + 0x62) = (int16_t)halo::ai::actor_lookup_small_table_entry((int16_t)unknown_60);
    }
    self[0x68] = unknown_68;
    self[0x8e] = 0;
    *(int16_t *)(self + 0x92) = 2;
    *(uint16_t *)(self + 0x90) = unknown_90;
    if (self[6] != ((uint8_t *)actor_type_procs[*(int16_t *)(self + 4)])[0xd]) {
        halo::ai::actor_delete(actor_index, 0);
        return k_datum_index_none;
    }

attach:
    if (reuse_existing == 0) {
        halo::ai::actor_attach_to_unit(actor_index, unit_index);
        return actor_index;
    }
    if (halo::ai::actor_link_to_unit_cluster(actor_index, unit_index) != 0) {
        return actor_index;
    }
    if (*(int16_t *)(ACTOR_AT(actor_index) + 0x1e) == 0) {
        halo::ai::actor_delete(actor_index, 0);
    }
    return k_datum_index_none;
}

#undef ACTOR_AT

namespace actor_place_new_unit_local {
extern "C" {
extern int16_t network_game_mode;
extern object_type_definition *object_type_definitions[k_maximum_object_types];
extern double cos(double x);
extern double sin(double x);
#define TAG_DATA(h) ((uint8_t *)halo::cache::globals().tag_instances[(h) & halo::k_slot_mask].data)
}
}

/**
 * Actor AI behaviour: place new unit.
 *
 * @address 0x427080
 */
datum_index ActorOps::place_new_unit(datum_index actor_variant_or_palette_tag, datum_index encounter_index, int16_t squad_index, uint8_t use_palette_entry, uint16_t unit_type_index, const actor_placement_request *placement_request)
{
    using namespace actor_place_new_unit_local;
    const uint8_t *request = (const uint8_t *)placement_request;
    datum_index variant_tag = actor_variant_or_palette_tag;
    uint8_t *variant;
    uint8_t *actor_definition;
    object_placement_data placement;
    float yaw;
    uint32_t role;
    datum_index unit_index;
    datum_index result;
    char swarm;
    char start_active = 0;
    uint16_t initial_state = 0;
    uint16_t return_state = 0;

    halo::objects::objects_garbage_collection();
    variant = TAG_DATA(variant_tag);
    if (use_palette_entry) {
        variant_tag = *(datum_index *)&((ActorVariant *)variant)->major_variant.tag_id;
        variant = TAG_DATA(variant_tag);
    }
    actor_definition = TAG_DATA(*(datum_index *)&((ActorVariant *)variant)->actor_definition.tag_id);
    halo::objects::object_placement_data_initialize(&placement, *(datum_index *)&((ActorVariant *)variant)->unit.tag_id, k_datum_index_none);
    yaw = ((struct actor_placement_request *)request)->yaw;
    placement.position = *(const real_point3d *)request;
    placement.permutation_group = (int16_t)unit_type_index;
    placement.forward.i = (float)cos((double)yaw);
    placement.forward.j = (float)sin((double)yaw);
    placement.forward.k = 0.0f;

    role = 3;
    if (network_game_mode == 2) {
        int16_t object_type = *(int16_t *)TAG_DATA(placement.definition_tag);

        if (*(int32_t *)((uint8_t *)object_type_definitions[object_type] + 0x10) != -1) {
            role = 0;
        }
    }
    unit_index = halo::objects::object_new_with_datum_role_control(&placement, role);
    if (unit_index == k_datum_index_none) {
        return k_datum_index_none;
    }
    swarm = (char)((*(uint32_t *)actor_definition >> 0x1a) & 1);
    halo::ai::actor_apply_unit_definition_properties(variant_tag, unit_index);
    if (encounter_index != k_datum_index_none) {
        uint8_t *encounter = *(uint8_t **)((uint8_t *)halo::scenario::globals().scenario + 0x430) + (encounter_index & halo::k_slot_mask) * 0xb0;
        uint8_t *squad = *(uint8_t **)(encounter + 0x84) + squad_index * 0xe8;

        initial_state = *(uint16_t *)(squad + 0x24);
        return_state = *(uint16_t *)(squad + 0x26);
        start_active = (char)((*(uint32_t *)(encounter + 0x20) >> 4) & 1);
    }
    if (((struct actor_placement_request *)request)->initial_state_override > 0) {
        initial_state = *(uint16_t *)&((struct actor_placement_request *)request)->initial_state_override;
    }
    if (*(const int16_t *)(request + 0x14) > 0) {
        return_state = *(const uint16_t *)(request + 0x14);
    }
    result = halo::ai::actor_new_and_attach_to_unit(swarm, unit_index, variant_tag, encounter_index, squad_index, 0,
        k_datum_index_none, start_active, initial_state, (int16_t)return_state, *(const uint16_t *)(request + 0x1a),
        (uint8_t)*(int8_t *)&((struct actor_placement_request *)request)->unknown_12);
    if (result == k_datum_index_none) {
        int32_t kind = *(int32_t *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask].data + 0x4);

        if (kind == 0) {
            halo::objects::object_delete_unparented(unit_index);
        } else if (kind != 3) {
            return result;
        }
        halo::objects::object_delete_recursive(unit_index, 0);
    }
    return result;
}

#undef TAG_DATA

namespace actor_point_in_directional_lane_local {
extern "C" {
extern double sqrt(double x);
extern double fabs(double x);
}
}

/**
 * VERIFIED against disassembly 0x414990..0x414a8e (2026-09-30): the 0.0001 (double) length test, the
 * normalisation, the three thresholds and the cross-sign selector all match the code below. stack ->
 * side_thresholds Five-stage gate over the horizontal (x,y) components of three vectors: to_point must h
 *
 * @address 0x414990
 */
uint8_t ActorOps::point_in_directional_lane(real_point3d *to_point, real_point3d *forward, real_point3d *cone_axis, float min_cos_threshold, float side_thresholds[2])
{
    using namespace actor_point_in_directional_lane_local;
    real_vector2d point;
    real_vector2d facing;
    float length;
    float inverse;
    float cross;

    point.i = to_point->x;
    point.j = to_point->y;
    facing.i = forward->x;
    facing.j = forward->y;
    length = (float)sqrt((double)(point.i * point.i + point.j * point.j));
    if ((float)fabs((double)length) < 0.0001f) {
        return 0;
    }
    inverse = 1.0f / length;
    point.i = point.i * inverse;
    point.j = point.j * inverse;
    if (!(length > 0.0f)) {
        return 0;
    }
    if (!(point.j * cone_axis->y + point.i * cone_axis->x > min_cos_threshold)) {
        return 0;
    }
    if (!(halo::math::vector2d_normalize_with_length(facing) > 0.0f)) {
        return 0;
    }
    cross = point.i * facing.j - facing.i * point.j;
    if (!(facing.i * point.i + facing.j * point.j > side_thresholds[cross > 0.0f ? 1 : 0])) {
        return 0;
    }
    return 1;
}

namespace actor_probe_step_direction_local {
}

/**
 * stack -> step_up, stack -> out_flag, stack -> extra_param
 *
 * @address 0x417e50
 */
uint8_t ActorView::probe_step_direction(float step_distance, real_vector2d *direction, uint16_t *variant, float step_up, uint8_t *out_flag, void *extra_param)
{
    using namespace actor_probe_step_direction_local;
    uint16_t index;
    int16_t attempts;
    real_vector2d probe;
    int16_t tried;

    index = *variant;
    attempts = 1;

    switch (index) {
    case 0:
        probe.i = -direction->j;
        probe.j = direction->i;
        break;
    case 1:
        probe.i = direction->j;
        probe.j = -direction->i;
        break;
    case 2:
        probe.i = direction->i;
        probe.j = direction->j;
        break;
    case 3:
        probe.i = -direction->i;
        probe.j = -direction->j;
        break;
    case 4: {
        uint32_t rng;
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        rng = (uint32_t)halo::math::globals().random_seed_global;
        if ((uint16_t)(rng >> 0x10) <= 0x8000) {
            probe.i = direction->j;
            probe.j = -direction->i;
            index = 1;
        } else {
            probe.i = -direction->j;
            probe.j = direction->i;
            index = 0;
        }
        attempts = 2;
        break;
    }
    default:
        probe.i = 0.0f;
        probe.j = 0.0f;
        break;
    }

    for (tried = 0; tried < attempts; tried++) {
        if (halo::ai::actor_check_step_obstruction(actor_index, &probe, step_distance, step_up, out_flag, extra_param)) {
            *variant = index;
            return 1;
        }
        probe.i = -probe.i;
        probe.j = -probe.j;
        index ^= 1;
    }

    *variant = halo::k_word_none;
    return 0;
}

namespace actor_process_order_request_local {
extern "C" {
extern game_time_globals *game_time;
extern int16_t order_code_mode_data_expect[12];
}
}

/**
 * Actor AI behaviour: process order request.
 *
 * @address 0x409ea0
 */
uint8_t ActorView::process_order_request(uint16_t order_code)
{
    using namespace actor_process_order_request_local;
    uint8_t *act = (uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    int16_t mode = ((actor *)act)->mode;
    uint8_t order[k_actor_mode_data_size];
    int16_t code = (int16_t)order_code;

    if (code == -1 && ((struct actor *)act)->last_order_request_time != -1 && ((struct actor *)act)->last_order_request_time + 0x2d >= game_time->game_time) {
        return 0;
    }
    ((struct actor *)act)->last_order_request_time = game_time->game_time;
    if (code == -1) {
        code = ((struct actor *)act)->pending_order_request;
        if (code != -1) {
            ((struct actor *)act)->pending_order_request = -1;
        } else {
            code = ((struct actor *)act)->standing_order_request;
            if (code == -1) {
                code = 0;
            }
        }
    }

    switch (code) {
    case 1:
        if (((actor *)act)->awareness_level != 1) {
            ((actor *)act)->awareness_level = 1;
            halo::ai::actor_set_mode(actor_index, 1, 0);
            return 1;
        }
        break;

    case 8:
        if (mode == 6 && *(int16_t *)(act + 0xc0) == 1) {
            break;
        }
        if (halo::ai::actor_build_order_return_to_anchor(actor_index, (actor_order *)order)) {
            halo::ai::actor_set_mode(actor_index, 6, order);
            return 1;
        }
        break;

    case 9:
        if (mode == 6) {
            if (*(int16_t *)(act + 0xc0) != 3) {
                act[0xaa] = 1;
            }
            break;
        }
        if (halo::ai::actor_build_order_guard(actor_index, (actor_order *)order, 0)) {
            halo::ai::actor_set_mode(actor_index, 6, order);
            return 1;
        }
        break;

    case 10:
        if (halo::ai::actor_get_current_mode_combat_grade(actor_index) == 3) {
            break;
        }
        ((actor *)act)->awareness_level = 3;
        ((struct actor *)act)->minimum_combat_status = 2;
        ((struct actor *)act)->combat_status = 2;
        if (halo::ai::actor_update_melee_combat_action(actor_index)) {
            break;
        }
        if (halo::ai::actor_build_order_return_to_anchor(actor_index, (actor_order *)order)) {
            halo::ai::actor_set_mode(actor_index, 6, order);
            return 1;
        }
        break;

    case 11:
        if (mode == 4) {
            break;
        }
        if (act[0x160] == 0) {
            memset(order, 0, 0x30);
            ((struct actor_order *)order)->parameter = -1;
            *(int32_t *)(order + 0x1c) = -1;
            *(int16_t *)(order + 0xc) = 0xd;
            ((struct actor_order *)order)->order_code = 0xb4;
            order[0x4] = 0;
            order[0x5] = 0;
            if (act[0x6] == 0) {
                halo::ai::actor_check_melee_target_reachable(actor_index, (int16_t *)order);
                if (((struct actor_order *)order)->parameter != -1) {
                    halo::ai::actor_set_mode(actor_index, 4, order);
                    return 1;
                }
                order[0xe] = 0;
            }
        }
        if (((actor *)act)->mode == 6) {
            break;
        }
        if (halo::ai::actor_build_order_return_to_anchor(actor_index, (actor_order *)order)) {
            halo::ai::actor_set_mode(actor_index, 6, order);
            return 1;
        }
        break;

    case 0: case 2: case 3: case 4: case 5: case 6: case 7:
        if (mode == 2 && *(int16_t *)&((struct actor *)act)->mode_data == order_code_mode_data_expect[code]) {
            break;
        }
        if (halo::ai::actor_build_order_default(actor_index, order_code_mode_data_expect[code], (actor_order *)order, -1)) {
            halo::ai::actor_set_mode(actor_index, 2, order);
            return 1;
        }
        break;

    default:
        break;
    }

    if (((actor *)act)->mode == 0 &&
        halo::ai::actor_build_order_default(actor_index, 0, (actor_order *)order, -1)) {
        halo::ai::actor_set_mode(actor_index, 2, order);
        return 1;
    }
    return 0;
}

namespace actor_process_pending_command_list_local {
#define ACTOR(index) ((uint8_t *)halo::ai::globals().actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))
}

/**
 * Actor AI behaviour: process pending command list.
 *
 * @address 0x40a140
 */
uint8_t ActorView::process_pending_command_list()
{
    using namespace actor_process_pending_command_list_local;
    uint8_t *actor = ACTOR(actor_index);
    uint8_t started = 0;
    int16_t record[0x42];

    if (W(0x90) == -1) {
        return 0;
    }
    if (B(0x8e) == 0 && (W(0x6a) == 0 || halo::ai::actor_wants_reload_or_swap(actor_index))) {
        return 0;
    }
    if (halo::ai::actor_squad_action_status_broadcast(actor_index, (int16_t)W(0x90), record) != 0) {
        halo::ai::actor_set_mode(actor_index, 0xb, record);
        started = 1;
    }
    B(0x8e) = 0;
    W(0x90) = -1;
    return started;
}

#undef ACTOR
#undef B
#undef W
#undef D
#undef F

namespace actor_process_vehicle_seat_exit_local {
extern "C" {
extern data_array *player_data;
extern int16_t network_game_mode;
extern game_time_globals *game_time;
extern network_client_globals *network_client;
extern void player_update_history_free_all(void *history);
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(h) & halo::k_slot_mask].data)
#define OBJECT_HEADER(h) (((object_header *)halo::objects::globals().object_data->data)[(h) & halo::k_slot_mask])
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
static void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = OBJECT_DATA(object_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *nodes = self + ((struct object *)self)->nodes.offset;
    uint8_t *seat = *(uint8_t **)(TAG_DATA(*(datum_index *)vehicle) + 0x2e8) + *(int16_t *)(self + 0x2f0) * 0x11c;
    uint8_t *model_nodes;
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    halo::objects::object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &marker, 1);
    offset.x = *(float *)(nodes + 0x28) - marker.node_transform.position.x;
    offset.y = *(float *)(nodes + 0x2c) - marker.node_transform.position.y;
    offset.z = *(float *)(nodes + 0x30) - marker.node_transform.position.z;
    model_nodes = *(uint8_t **)(TAG_DATA(*(datum_index *)(TAG_DATA(*(datum_index *)self) + 0x34)) + 0xbc);
    default_translation = *(real_point3d *)(model_nodes + 0x28);
    if (((vehicle_object *)vehicle)->unit.driver_unit_index == object_index && vehicle[0x2a3] != 0x25 &&
        ((struct object *)self)->parent_object != k_datum_index_none) {
        halo::units::unit_try_set_animation_state(((struct object *)self)->parent_object, 0x25);
    }
    *(datum_index *)(self + 0x32c) = vehicle_index;
    *(int32_t *)(self + 0x330) = game_time->game_time;
    if (*(datum_index *)(self + 0x324) == object_index) {
        *(datum_index *)(self + 0x324) = k_datum_index_none;
    }
    if (*(datum_index *)(self + 0x328) == object_index) {
        *(datum_index *)(self + 0x328) = k_datum_index_none;
    }
    halo::objects::object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + ((struct object *)self)->position.x;
    position.y = offset.y + ((struct object *)self)->position.y;
    position.z = offset.z + ((struct object *)self)->position.z - default_translation.z;
    halo::objects::object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = OBJECT_DATA(object_index);

        halo::math::matrix4x3_multiply((real_matrix4x3 *)(reloaded + ((struct object *)reloaded)->nodes.offset),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)&((struct object *)self)->forward.i = basis.forward;
    *(real_vector3d *)&((struct object *)self)->up.i = basis.up;
    {
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag = TAG_DATA(*(datum_index *)object);

        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1 && (object[0x10] & 1) != 0) {
            halo::objects::object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
            ((struct object *)object)->flags &= ~1u;
            OBJECT_HEADER(object_index).flags |= 2;
        }
    }
    *(int16_t *)(self + 0x2f0) = -1;
    self[0x2a7] = 2;
    if (((vehicle_object *)vehicle)->unit.driver_unit_index == object_index) {
        ((vehicle_object *)vehicle)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((vehicle_object *)vehicle)->unit.gunner_unit_index == object_index) {
        ((vehicle_object *)vehicle)->unit.gunner_unit_index = k_datum_index_none;
    }
    halo::units::unit_recompute_seat_occupants(vehicle_index);
    halo::units::unit_pick_and_ready_next_weapon(object_index);
    {
        int8_t request[2] = { 0x14, 0 };

        halo::units::unit_update_animation_state_machine(object_index, request);
    }
    *(real_point3d *)(self + ((struct object *)self)->node_function_values.offset + 0x10) = default_translation;
    if (((struct object *)self)->type == 0) {
        halo::units::unit_reset_orientation_and_find_position(object_index, vehicle_index);
    }
    halo::objects::object_recalculate_bounding_radius_recursive(object_index);
    if (halo::units::unit_all_seats_unoccupied(vehicle_index) == 1) {
        uint8_t *empty = (uint8_t *)halo::objects::object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = game_time->game_time;
        }
    }
    if (network_game_mode == 1) {
        uint8_t *player = (uint8_t *)halo::memory::datum_get(*(datum_index *)(self + 0x218), player_data);

        if (player != 0 && ((struct player *)player)->local_player_index == -1) {
            ((struct player *)player)->position_updates.read_index = 0;
            ((struct player *)player)->position_updates.write_index = 0;
            ((struct player *)player)->vehicle_updates.read_index = 0;
            ((struct player *)player)->vehicle_updates.write_index = 0;
        }
    }
}
static void biped_free_local_player_history(uint8_t *self)
{
    datum_index player_index = *(datum_index *)(self + 0x218);
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    uint8_t *player;

    if (network_game_mode != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= *(int16_t *)((uint8_t *)player_data + 0x20)) {
        return;
    }
    player = (uint8_t *)player_data->data + *(int16_t *)((uint8_t *)player_data + 0x22) * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || ((struct player *)player)->local_player_index == -1) {
        return;
    }
    if (network_client != 0) {
        player_update_history_free_all(*(void **)&network_client->update_history);
    }
}
}
}

/**
 * Actor AI behaviour: process vehicle seat exit.
 *
 * @address 0x40b080
 */
uint8_t ActorView::process_vehicle_seat_exit()
{
    using namespace actor_process_vehicle_seat_exit_local;
    uint8_t *act = (uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    datum_index driving = ((actor *)act)->active_unit_index;
    uint8_t wanted = 0;
    uint8_t forced = 0;
    uint8_t result = 0;
    datum_index rider_index;
    uint8_t *rider;

    if (driving == k_datum_index_none) {
        act[0x2ed] = 0;
        return 0;
    }
    {
        datum_index prop_index = ((actor *)act)->first_prop;

        while (prop_index != k_datum_index_none) {
            uint8_t *p = (uint8_t *)halo::ai::globals().prop_data->data + (prop_index & halo::k_slot_mask) * k_prop_size;
            int16_t kind = ((prop *)p)->state;

            prop_index = ((prop *)p)->next_in_actor;
            if (kind >= 2 && kind <= 3 && p[0x12e] && p[0x60] && *(datum_index *)&((prop *)p)->relationship_object_index == driving) {
                wanted = 1;
                forced = 1;
                break;
            }
        }
    }
    if (act[0x2ed]) {
        wanted = 1;
    }
    if (act[0x160] && (*(datum_index *)&((struct actor *)act)->stuck_projectile_index != k_datum_index_none ||
                       (((actor *)act)->danger_type == 2 && act[0x28a]))) {
        forced = 1;
    } else if (!wanted) {
        act[0x2ed] = 0;
        return 0;
    }
    act[0x38c] = forced;
    rider_index = ((actor *)act)->unit_index;
    rider = (uint8_t *)halo::objects::object_try_and_get(rider_index, 3);
    if (rider != 0 && network_game_mode != 1 && *(datum_index *)(rider + 0x11c) != k_datum_index_none &&
        *(int16_t *)(rider + 0x2f0) != -1) {
        datum_index vehicle_index = *(datum_index *)(rider + 0x11c);

        if (*(int16_t *)(rider + 0xb4) == 1) {
            uint8_t *self = OBJECT_DATA(rider_index);

            if (((struct object *)self)->parent_object != k_datum_index_none && *(int16_t *)(self + 0x2f0) != -1) {
                biped_detach_from_seat(rider_index, ((struct object *)self)->parent_object);
            }
            biped_free_local_player_history(self);
        } else if (!halo::units::unit_state_is_scripted_animation((unit_data *)(rider + 0x1f4))) {
            uint8_t *rider_tag = TAG_DATA(*(datum_index *)rider);
            datum_index graph = *(datum_index *)(rider_tag + 0x44);
            uint8_t *block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)rider[0x2a0] * 0x64;

            if (*(int32_t *)(block + 0x40) > 8) {
                int16_t exit_animation = (*(int16_t **)(block + 0x44))[8];

                if (exit_animation != -1) {
                    uint8_t *object;
                    uint8_t *object_tag;

                    if (*(datum_index *)(OBJECT_DATA(vehicle_index) + 0x324) == rider_index) {
                        halo::units::unit_notify_weapon_removed(vehicle_index);
                    }
                    halo::units::unit_set_custom_animation(rider_index, graph,
                                              halo::models::animation_choose_random_permutation(graph, exit_animation, static_cast<animation_random_stream>(1)));
                    object = OBJECT_DATA(rider_index);
                    object_tag = TAG_DATA(*(datum_index *)object);
                    if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                        if (object[0x10] & 1) {
                            halo::objects::object_for_each_light_attachment(rider_index, 0, 1);
                        }
                        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                            ((struct object *)object)->flags &= ~1u;
                            OBJECT_HEADER(rider_index).flags |= 2;
                        }
                    }
                    rider[0x2a3] = 0x1b;
                    halo::ai::actor_notify_weapon_pickup_once(rider_index);
                    if (*(int32_t *)(rider + 0x4) == 0) {
                        halo::units::unit_dispatch_scripted_event_9(0, (int32_t)rider_index);
                    }
                    ((struct actor *)act)->exited_vehicle_index = ((actor *)act)->active_unit_index;
                    *(int32_t *)&((struct actor *)act)->exited_vehicle_reentry_time = game_time->game_time + 180;
                    result = 1;
                }
            }
        }
    }
    act[0x38c] = 0;
    act[0x2ed] = 0;
    return result;
}

#undef OBJECT_DATA
#undef OBJECT_HEADER
#undef TAG_DATA

namespace actor_prop_iterator_init_local {
}

/**
 * Actor AI behaviour: prop iterator init.
 *
 * @address 0x43ecd0
 */
void ActorView::prop_iterator_init(actor_prop_iterator *out_iterator)
{
    using namespace actor_prop_iterator_init_local;
    actor *self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    out_iterator->next = self->first_prop;
}

namespace actor_prop_iterator_next_local {
}

/**
 * TYPES-GAP: duplicated from actor_prop_iterator_init.c (each rewritten file is compiled independently).
 *
 * @address 0x43ecf0
 */
prop * ActorOps::prop_iterator_next(actor_prop_iterator *iterator)
{
    using namespace actor_prop_iterator_next_local;
    datum_index cur = iterator->next;
    prop *result = 0;

    iterator->current = cur;
    if (cur != (datum_index)halo::k_dword_none) {
        result = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (cur & halo::k_slot_mask) * sizeof(prop));
        iterator->next = result->next_in_actor;
    }
    return result;
}

namespace actor_propagate_unit_field_local {
}

/**
 * Writes `value` into the unnamed int16 field at object+0xb8 (see UNSURE above) of every unit the actor
 * controls: its single unit, every unit in its cluster, or every unit in its swarm's component list.
 *
 * @address 0x4276e0
 */
void ActorView::propagate_unit_field(int16_t value)
{
    using namespace actor_propagate_unit_field_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->swarm == 0) {
        if (self->unit_index != (datum_index)k_datum_index_none) {
            object *unit_object = ((object_header *)halo::objects::globals().object_data->data)[self->unit_index & halo::k_slot_mask].data;
            ((struct object *)unit_object)->owner_team = value;
        }
    } else if (self->swarm_index == (datum_index)k_datum_index_none) {
        datum_index unit_index = self->cluster_unit_index;
        if (unit_index != (datum_index)k_datum_index_none) {
            do {
                object *unit_object = ((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask].data;
                ((struct object *)unit_object)->owner_team = value;
                unit_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc);
            } while (unit_index != (datum_index)k_datum_index_none);
        }
    } else {
        swarm *s = &((swarm *)halo::ai::globals().swarm_data->data)[self->swarm_index & halo::k_slot_mask];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            object *unit_object = ((object_header *)halo::objects::globals().object_data->data)[s->unit_index[i] & halo::k_slot_mask].data;
            ((struct object *)unit_object)->owner_team = value;
        }
    }
}

namespace actor_raise_timer_5f6_local {
#define ACTOR(index) ((uint8_t *)halo::ai::globals().actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))
}

/**
 * Actor AI behaviour: raise timer 5f6.
 *
 * @address 0x40f7a0
 */
void ActorView::raise_timer_5f6(int32_t ticks)
{
    using namespace actor_raise_timer_5f6_local;
    uint8_t *actor = ACTOR(actor_index);

    if ((int32_t)W(0x5f6) > ticks) {
        W(0x5f6) = W(0x5f6);
    } else {
        W(0x5f6) = (int16_t)ticks;
    }
}

#undef ACTOR
#undef B
#undef W
#undef D
#undef F

namespace actor_rate_potential_target_local {
}

/**
 * is a tag data pointer of unresolved type Computes a floating-point desirability score for a candidate prop
 * (target-data record) as the given actor's next combat target, combining visibility, distance, alertness and
 * prior-target continuity bonuses. Returns 0.0 for a disqualified candidate.
 *
 * @address 0x41fd50
 */
float ActorView::rate_potential_target(datum_index target_prop_index)
{
    using namespace actor_rate_potential_target_local;
    actor *self;
    prop *target;
    Actor *actor_def;
    ActorVariant *variant_def;
    uint8_t *override_tag;
    int8_t bonus_a;
    int8_t bonus_b;
    int8_t bonus_c;
    int8_t bonus_d;
    float extra;
    float threshold;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    target = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (target_prop_index & halo::k_slot_mask) * sizeof(prop));

    if (target->disregarded != 0 || target->enemy == 0) {
        return 0.0f;
    }

    bonus_a = 0;
    if ((-1 < target->state && target->state < 2) ||
        ((target->dead != 0 && 0x95 < target->dead_ticks) || target->actor_type == 0xf)) {
        return 0.0f;
    }

    extra = 0.0f;
    actor_def = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & halo::k_slot_mask].data;
    variant_def = (ActorVariant *)halo::cache::globals().tag_instances[self->actor_variant_tag & halo::k_slot_mask].data;
    bonus_b = 0;
    bonus_c = 0;

    if (self->swarm == 0 && target->engaged_ticks < 1) {
        bonus_a = (int8_t)halo::ai::actor_has_unshielded_threat_weapon(actor_index);

        if (bonus_a == 0) {
            threshold = (self->berserking == 0) ? variant_def->berserk_melee_range
                                                 : variant_def->melee_range;

            if (2.0f <= target->distance || (bonus_a = 5, target->state == 5)) {
                if (target->relationship_object_index == -1) {
                    if (target->flying == 0 || ((struct Actor *)actor_def)->melee_leap_velocity != 0.0f) {
                        if (target->in_water == self->in_water) {
                            bonus_a = (target->distance >= threshold) ? 2 : 3;
                        } else {
                            bonus_a = 1;
                        }
                    } else {
                        bonus_a = 0;
                    }
                } else {
                    bonus_a = 0;
                }
            }
        } else {
            override_tag = (uint8_t *)halo::ai::actor_get_threat_weapon_definition(actor_index);
            extra = 0.0f;
            variant_def = (ActorVariant *)halo::ai::actor_get_actor_definition(actor_index);

            if (override_tag != (uint8_t *)0 && target->distance >= *(float *)(override_tag + 0x40c)) {
                bonus_a = 2;
            } else if (target->in_water == self->in_water) {
                if (2.0f <= target->distance || (bonus_a = 5, target->state == 5)) {
                    if (target->distance >= *(const float *)((const uint8_t *)variant_def + 0xa0)) {
                        bonus_a = 2;
                        if (target->distance >= ((struct ActorVariant *)variant_def)->maximum_firing_distance) {
                            bonus_a = 1;
                        }
                    } else {
                        bonus_a = 3;
                    }
                }
            } else {
                bonus_a = 2;
            }
        }
    }

    if (target->dead == 0) {
        if (self->swarm == 0 && target->seen != 0 && target->engaged_ticks == 0) {
            bonus_d = 6;
        } else if (target->state < 2 || 3 < target->state) {
            bonus_d = (target->has_current_information == 0) ? (int8_t)(target->state == 4) + 1 : 3;
        } else if (self->swarm != 0) {
            bonus_d = 4;
        } else if (0 < target->engaged_ticks) {
            bonus_d = 3;
        } else if (target->obstruction != 0 && target->obstruction != 1) {
            bonus_d = 3;
        } else if (target->shooting != 0 && (int8_t)target->aiming_at_actor_class < 2) {
            bonus_d = 5;
        } else {
            bonus_d = 4;
        }
    } else {
        bonus_d = 1;
    }

    if (self->target_unit_index == k_datum_index_none) {
        if (target->is_parented != 0 || target_prop_index == self->nearest_orphan_prop_index) {
            extra = 3.0f;
        }
    } else if (target_prop_index == self->target_unit_index && 2 < self->combat_status) {
        bonus_b = 1;
    }

    if (target->preferred_target != 0) {
        bonus_c = 2;
    }

    return extra + 5.0f / (target->distance * 0.1f + 1.0f) +
           (float)((int32_t)bonus_c + (int32_t)bonus_b + (int32_t)bonus_a + (int32_t)bonus_d) * 10.0f;
}

namespace actor_release_from_cluster_or_delete_local {
}

/**
 * Reduces an actor's cluster by one unit and, once the cluster is fully empty, notifies the actor's associated
 * encounter via the shared release helpers.
 *
 * @address 0x428e50
 */
void ActorView::release_from_cluster_or_delete(datum_index unit_index)
{
    using namespace actor_release_from_cluster_or_delete_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    halo::ai::actor_remove_from_unit_cluster(actor_index, unit_index);
    if (self->cluster_count == 0) {
        datum_index encounter_index = self->encounter_index;
        halo::ai::actor_delete(actor_index, 1);
        if (encounter_index != (datum_index)k_datum_index_none) {
            halo::ai::encounter_recompute_morale(encounter_index);
        }
    }
}

namespace actor_remove_from_unit_cluster_local {
}

/**
 * Removes an actor from a unit's cluster (the doubly-linked list of controlling actors) and, if it had a swarm
 * component for that unit, removes that too (swap-with-last, then deletes the freed swarm_component datum).
 * Undoes actor_link_to_unit_cluster.
 *
 * @address 0x427c90
 */
void ActorView::remove_from_unit_cluster(datum_index unit_index)
{
    using namespace actor_remove_from_unit_cluster_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    object_header *header = &((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask];
    object *unit_object = header->data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

    if (unit->swarm_actor_index != actor_index) {
        return;
    }

    header->flags |= _object_header_in_pvs_pass_bit;
    if (unit_object->parent_object == (datum_index)k_datum_index_none &&
        unit_object->location_cluster_index == -1) {
        if ((header->flags & _object_header_active_bit) != 0) {
            header->flags &= ~_object_header_active_bit;
        }
    }

    halo::units::unit_refresh_targeting_flag_and_weapons(unit_index, 0);

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)halo::ai::globals().swarm_data->data)[self->swarm_index & halo::k_slot_mask];
        if (s->component_count > 0) {
            int16_t i;
            for (i = 0; i < s->component_count; i++) {
                if (s->unit_index[i] == unit_index) {
                    datum_index freed_component = s->component_index[i];
                    int16_t new_count = s->component_count - 1;
                    s->component_count = new_count;
                    if (i < new_count) {
                        s->unit_index[i] = s->unit_index[new_count];
                        s->component_index[i] = s->component_index[new_count];
                    }
                    halo::memory::datum_delete(halo::ai::globals().swarm_component_data, freed_component);
                    break;
                }
            }
        }
    }

    {
        uint32_t prev = *(uint32_t *)((uint8_t *)unit_object + 0x200);
        datum_index next = unit->swarm_next_unit_index;

        if (prev == halo::k_dword_none) {
            self->cluster_unit_index = next;
        } else {
            object *prev_object = ((object_header *)halo::objects::globals().object_data->data)[prev & halo::k_slot_mask].data;
            *(uint32_t *)((uint8_t *)prev_object + 0x1fc) = next;
        }
        if (next != (datum_index)k_datum_index_none) {
            object *next_object = ((object_header *)halo::objects::globals().object_data->data)[next & halo::k_slot_mask].data;
            *(uint32_t *)((uint8_t *)next_object + 0x200) = prev;
        }
    }

    unit->swarm_actor_index = (datum_index)k_datum_index_none;
    self->cluster_count = self->cluster_count - 1;
}

namespace actor_replace_object_reference_local {
extern "C" {
extern actor_mode_definition actor_mode_definitions[16];
}
}

/**
 * FIXED (objdump 0x428479): actor_index is read from [esp+8] after push ebx -- a stack argument (EAX is
 * overwritten with actor_data first); callers push it (0x4383a5 push ecx). Scans an actor's numerous cached
 * object-index fields (and its swarm members') for a stale object reference (old_reference) an
 *
 * @address 0x428470
 */
void ActorView::replace_object_reference(uint32_t new_reference, uint32_t old_reference)
{
    using namespace actor_replace_object_reference_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->target_unit_index == old_reference) {
        self->target_unit_index = new_reference;
        if (new_reference == halo::k_dword_none) {
            self->target_combat_status = 0;
        }
    }

    if (self->firing_target_type == 1 && self->firing_target_prop_index == old_reference) {
        self->firing_target_prop_index = new_reference;
        if (new_reference == halo::k_dword_none) {
            self->firing_target_type = 0;
        }
    }

    if (self->grenade_target_prop_index == old_reference) {
        self->grenade_target_prop_index = new_reference;
    }
    if (self->look_at_reference == old_reference) {
        self->look_at_reference = new_reference;
    }
    if (self->pending_panic_prop_index == old_reference) {
        self->pending_panic_prop_index = new_reference;
    }
    if (self->search_prop_index == old_reference) {
        self->search_prop_index = new_reference;
    }
    if (self->retreat_prop_index == old_reference) {
        if (new_reference == halo::k_dword_none) {
            self->retreat_timer = 0;
        }
        self->retreat_prop_index = new_reference;
    }
    if (self->nearby_friend_prop_index == old_reference) {
        self->nearby_friend_prop_index = new_reference;
    }
    if (self->post_combat_prop_index == old_reference) {
        self->post_combat_prop_index = new_reference;
        if (new_reference == halo::k_dword_none) {
            self->post_combat_action = 0;
        }
    }

    if (self->active_movement.type == 5 && *(uint32_t *)&((struct actor *)self)->active_movement.destination.x == old_reference) {
        if (new_reference == halo::k_dword_none) {
            self->active_movement.type = 0;
            self->active_movement.extra = halo::k_dword_none;
        } else {
            *(uint32_t *)&((struct actor *)self)->active_movement.destination.x = new_reference;
        }
    }

    if (self->vocalization_source.code == 1 && self->vocalization_source.payload.handle == old_reference) {
        self->vocalization_source.payload.handle = new_reference;
    }
    if (self->idle_major_direction_type == 1 && *(uint32_t *)((uint8_t *)self + 0x570) == old_reference) {
        *(uint32_t *)((uint8_t *)self + 0x570) = new_reference;
    }
    if (((struct actor *)self)->idle_look_direction_type == 1 && *(uint32_t *)((uint8_t *)self + 0x580) == old_reference) {
        *(uint32_t *)((uint8_t *)self + 0x580) = new_reference;
    }

    if (self->swarm != 0 && self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)halo::ai::globals().swarm_data->data)[self->swarm_index & halo::k_slot_mask];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            swarm_component *component = &((swarm_component *)halo::ai::globals().swarm_component_data->data)[s->component_index[i] & halo::k_slot_mask];
            if (component->leap_target_index == old_reference) {
                component->leap_target_index = new_reference;
            }
        }
    }

    {
        uint32_t proc = *(uint32_t *)((uint8_t *)&actor_mode_definitions[self->mode] + 0x20);
        if (proc != 0) {
            ((void (*)(datum_index, datum_index, datum_index))proc)(actor_index, old_reference, new_reference);
        }
    }
}

namespace actor_report_command_status_local {
extern "C" {
extern uint8_t team_pair_flag_test(int16_t team_a, int16_t team_b);
}
}

/**
 * If this actor has not already reported for its current scripted command, maps actor.unknown_1e4 (1..10,
 * skipping 6) to a chatter event code and broadcasts it along with the actor's controlled unit and, if it has a
 * tracked target prop, that prop's object and a shield-state code (2 = no shield, 3/4 =
 *
 * @address 0x4048b0
 */
int32_t ActorView::report_command_status()
{
    using namespace actor_report_command_status_local;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint8_t *already_reported = (uint8_t *)a->mode_data.raw + (0xa2 - 0x9c);
    uint32_t event_code = 0;

    if (*already_reported != 0) {
        return 0;
    }

    switch (a->post_combat_action) {
    case 1: event_code = 0x30; break;
    case 2: event_code = 0x31; break;
    case 3: event_code = 0x32; break;
    case 4: event_code = 0x33; break;
    case 5: event_code = 0x34; break;
    case 6:
        *already_reported = 1;
        return 0;
    case 7: event_code = 0x35; break;
    case 8: event_code = 0x36; break;
    case 9: event_code = 0x37; break;
    case 10: event_code = 0x38; break;
    default:
        *already_reported = 1;
        return event_code;
    }

    {
        datum_index target_prop_index = *(datum_index *)((uint8_t *)a->mode_data.raw + (0xd8 - 0x9c));
        datum_index target_object = (datum_index)k_datum_index_none;
        int32_t target_state = -1;

        if (target_prop_index != (datum_index)k_datum_index_none) {
            prop *p = &((prop *)halo::ai::globals().prop_data->data)[target_prop_index & halo::k_slot_mask];

            target_object = p->object_index;
            if (p->enemy == 0) {
                target_state = 2;
            } else {
                target_state = (team_pair_flag_test(((actor *)a)->team, ((struct prop *)p)->team) != 0) + 3;
            }
        }
        halo::ai::ai_communication_broadcast(event_code, a->unit_index, target_object, target_state, halo::k_dword_none, halo::k_dword_none, 0);
    }
    *already_reported = 1;
    return 1;
}

namespace actor_request_move_and_face_local {
}

/**
 * Actor AI behaviour: request move and face.
 *
 * @address 0x4049d0
 */
uint8_t ActorView::request_move_and_face()
{
    using namespace actor_request_move_and_face_local;
    uint8_t *actor = (uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((struct actor *)actor)->actor_definition_tag & halo::k_slot_mask].data;

    if (actor[6] != 0) {
        *(int16_t *)(actor + 0xc0) = 1;
        return 0;
    }
    if (actor[0x160] != 0) {
        *(int16_t *)(actor + 0xc0) = 1;
        actor[0xaa] = 1;
        return 0;
    }
    if (*(int16_t *)(actor + 0xc0) == 3 && ((struct actor *)actor)->firing_position_index == -1) {
        *(int16_t *)(actor + 0xc0) = 0;
        actor[0xaa] = 1;
    }
    if (actor[0x4c] == 0 || actor[0xaa] == 0) {
        return 0;
    }
    if (*(int16_t *)(actor + 0xc0) == 3 && ((struct actor *)actor)->firing_position_index != -1) {
        halo::ai::actor_push_recognition_entry(actor_index, ((struct actor *)actor)->firing_position_index, 0);
    }
    {
        static actor_firing_position_query query;
        static path_find_context path_context;
        actor_firing_position_candidate candidate;
        uint32_t previous_owner = halo::k_dword_none;
        uint8_t path_ok = 0;
        int16_t found;
        int16_t claimed;

        memset(&query, 0, sizeof(query));
        memset(&candidate, 0, sizeof(candidate));
        query.goal_kind = 4;
        query.group_mask = halo::ai::actor_get_firing_position_group_mask(actor_index, 4, 0);
        query.allow_random_fallback = 1;
        found = (int16_t)halo::ai::actor_find_best_firing_position(actor_index, &query, &candidate, &previous_owner,
            &path_context, &path_ok);
        claimed = halo::ai::actor_claim_firing_position(actor_index, previous_owner, &path_context, found, path_ok);
        actor = (uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
        actor[0xaa] = 0;
        actor[0xb0] = 0;
        if (claimed == -1) {
            *(int16_t *)(actor + 0xc0) = 1;
        } else {
            *(int16_t *)(actor + 0xc0) = 3;
            *(int16_t *)(actor + 0xc4) = claimed;
        }
    }
    *(int16_t *)&((struct actor *)actor)->mode_data = (int16_t)(int32_t)(halo::math::random_real_range(*(float *)(actor_tag + 0x3b8),
        *(float *)(actor_tag + 0x3bc)) * 30.0f);
    return 0;
}

namespace actor_reseed_movement_pause_timer_local {
extern "C" {
extern float weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index);
}
}

/**
 * FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it);
 * those parameters arrive on the stack (1 stack argument(s) read). REWRITTEN from objdump 0x4104e0..0x4105b4: a
 * uniform random pause between the first stance entry's bounds times 1.7 when actor
 *
 * @address 0x4104e0
 */
void ActorView::reseed_movement_pause_timer()
{
    using namespace actor_reseed_movement_pause_timer_local;
    actor *self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    void *definition = halo::ai::actor_get_actor_definition(actor_index);
    uint8_t *entry_a = 0;
    uint8_t *entry_b = 0;
    float lower, upper, fraction, pause;

    halo::ai::actor_select_stance_offset_pair(actor_index, (uint8_t *)definition, &entry_a, &entry_b);
    upper = *(float *)(entry_a + 0x20);
    lower = *(float *)(entry_a + 0x1c);
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    fraction = (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f;
    pause = fraction * (upper - lower) + lower;
    pause = weapon_get_zoom_fov_resolved(0xe, ((struct actor *)self)->team) * pause;
    if (entry_b != 0 && *(float *)(entry_b + 4) != 0.0f) {
        pause = pause * *(float *)(entry_b + 4);
    }
    if (self->playfight != 0) {
        pause = pause * 1.7f;
    }
    self->firing_state_timer = (int16_t)(int32_t)(pause * 30.0f);
}

namespace actor_reset_perception_scratch_local {
extern "C" {
extern const real_vector3d *global_origin3d_pointer;
}
}

/**
 * REWRITTEN from objdump 0x428f40..0x428ffd. Builds a neutral 0x40-byte unit control block: bytes 0/1 = 1, word
 * 2 = 0, words 4/6/8 = -1, +0x0c = the global origin (zero throttle), +0x1c = the unit's forward (or marker
 * normal), +0x28 = unit +0x23c (aim), +0x34 = unit +0x260 (look). It then applies the
 *
 * @address 0x428f40
 */
void ActorOps::reset_perception_scratch(datum_index unit_index)
{
    using namespace actor_reset_perception_scratch_local;
    uint8_t block[0x40];
    uint8_t *unit_object = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask].data;

    memset(block, 0, sizeof(block));
    block[0] = 1;
    block[1] = 1;
    *(int16_t *)(block + 0x2) = 0;
    *(int16_t *)(block + 0x4) = -1;
    *(int16_t *)(block + 0x6) = -1;
    *(int16_t *)(block + 0x8) = -1;
    *(real_vector3d *)(block + 0xc) = *(const real_vector3d *)global_origin3d_pointer;
    halo::units::unit_get_forward_vector_or_marker_normal(unit_index, (real_vector3d *)(block + 0x1c));
    *(real_vector3d *)(block + 0x28) = *(real_vector3d *)(unit_object + 0x23c);
    *(real_vector3d *)(block + 0x34) = *(real_vector3d *)(unit_object + 0x260);
    halo::units::unit_apply_control_block(unit_index, (const unit_control_data *)block, -1);
    halo::units::unit_refresh_targeting_flag_and_weapons(unit_index, 0);
}

namespace actor_reset_squad_link_for_type_change_local {
}

/**
 * UNSURE: the squad index arrives in DX and Ghidra did not attribute it to this call site, so the actor's
 * current squad_index is passed; encounter_add_actor writes it straight back into the same field. FIXED
 * (verified against 0x4290f0..0x429151): the squad is a stack argument (it becomes encounter_add
 *
 * @address 0x4290f0
 */
void ActorView::reset_squad_link_for_type_change(datum_index encounter_index, int16_t squad_index)
{
    using namespace actor_reset_squad_link_for_type_change_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    halo::ai::actor_movement_action_cancel(actor_index);

    if (self->encounterless == 0) {
        if (self->encounter_index != (datum_index)k_datum_index_none) {
            halo::ai::encounter_remove_actor(actor_index, 0);
        }
    } else {
        halo::ai::ai_actor_unlink_from_unassigned_list(actor_index);
    }

    if (encounter_index == (datum_index)k_datum_index_none) {
        halo::ai::ai_actor_link_to_unassigned_list(actor_index);
        return;
    }
    halo::ai::encounter_add_actor(squad_index, actor_index, encounter_index, 1);
}

namespace actor_resolve_look_target_local {
}

/**
 * stack -> require_trust, stack -> use_aiming_deviation, stack -> force_fallback
 *
 * @address 0x414d00
 */
uint8_t ActorOps::resolve_look_target(real_point3d *preferred_direction, datum_index actor_index, float *deviation_table, uint8_t require_trust, uint8_t use_aiming_deviation, uint8_t force_fallback)
{
    using namespace actor_resolve_look_target_local;
    actor *self;
    Actor *definition;
    uint32_t out_in_front;
    real_vector3d direction;
    float yaw_half;
    float pitch_half;
    float pitch_center;
    int32_t wait_ticks;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    definition = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & halo::k_slot_mask].data;
    out_in_front = 0;
    self->idle_major_active = 0;

    if (force_fallback ||
        halo::ai::actor_select_facing_target_prop(actor_index, require_trust, use_aiming_deviation,
            (actor_recognition_scan_result *)((uint8_t *)self + 0x56c), (uint8_t *)&out_in_front) == 0) {
        direction.i = preferred_direction->x;
        direction.j = preferred_direction->y;
        direction.k = preferred_direction->z;

        if (use_aiming_deviation == 0) {
            yaw_half = (definition->maximum_looking_deviation.yaw <= definition->idle_looking_range.yaw)
                           ? definition->maximum_looking_deviation.yaw
                           : definition->idle_looking_range.yaw;
            pitch_half = (definition->maximum_looking_deviation.pitch <= definition->idle_looking_range.pitch)
                             ? definition->maximum_looking_deviation.pitch
                             : definition->idle_looking_range.pitch;
        } else {
            if (require_trust == 0) {
                yaw_half = (definition->maximum_aiming_deviation.yaw <= definition->idle_aiming_range.yaw)
                               ? definition->maximum_aiming_deviation.yaw
                               : definition->idle_aiming_range.yaw;
            } else {
                yaw_half = 3.1415927f;
            }
            pitch_half = (definition->maximum_aiming_deviation.pitch <= definition->idle_aiming_range.pitch)
                              ? definition->maximum_aiming_deviation.pitch
                              : definition->idle_aiming_range.pitch;

            direction.k = 0.0f;
            if (halo::math::vector3d_normalize_with_length(direction) == 0.0f) {
                direction = *halo::math::globals().global_forward3d_pointer;
            }
        }

        pitch_center = -pitch_half;
        if (self->vehicle_gunner != 0) {
            pitch_center = pitch_center * 0.5f;
        }

        self->idle_major_direction_type = 4;
        if (!halo::ai::actor_look_pick_random_point_in_cone(&self->aim_origin, -yaw_half, yaw_half, pitch_center, pitch_half,
                                                    &direction, 1, &self->idle_major_point)) {
            return (uint8_t)out_in_front;
        }
    }

    wait_ticks = halo::ai::actor_look_get_wait_ticks(actor_index, (use_aiming_deviation == 0) + 1, out_in_front, deviation_table);
    self->idle_major_timer = wait_ticks;
    if (wait_ticks == 0) {
        return (uint8_t)out_in_front;
    }
    self->idle_major_active = 1;
    self->idle_major_is_aiming = (int8_t)use_aiming_deviation;
    return (uint8_t)out_in_front;
}

namespace actor_resolve_wander_or_look_direction_local {
}

/**
 * Computes and normalizes a direction vector for the actor to look toward: for a non-swarm actor, either the
 * delta from its body position to a cached wander destination vector (unknown_518, once unknown_504 is set);
 * returns whether the result was non-degenerate. Always false for a swarm actor.
 *
 * @address 0x4287a0
 */
uint8_t ActorView::resolve_wander_or_look_direction(real_vector3d *out_direction)
{
    using namespace actor_resolve_wander_or_look_direction_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->swarm != 0) {
        return 0;
    }

    if (self->moving == 0) {
        if (self->movement_action_complete == 0) {
            return 0;
        }
        out_direction->i = self->destination.x - self->body_position.x;
        out_direction->j = self->destination.y - self->body_position.y;
        out_direction->k = self->destination.z - self->body_position.z;
    } else {
        out_direction->i = self->desired_movement_vector.x;
        out_direction->j = self->desired_movement_vector.y;
        out_direction->k = self->desired_movement_vector.z;
    }

    return halo::math::vector3d_normalize_with_length(*out_direction) != 0.0f;
}

namespace actor_scale_value_by_ally_exposure_local {
}

/**
 * FIXED (objdump 0x420d44..0x420dd1): returns AL = 1, leaving the value alone, when 2+ allies are already
 * alerted (0x420d4e jg with EAX = 1); the scaling paths return 0 (xor al,al). The draft returned void. Adjusts a
 * caller-supplied probability/weight downward based on how many nearby allies of the sa
 *
 * @address 0x420c90
 */
uint8_t ActorView::scale_value_by_ally_exposure(float *value)
{
    using namespace actor_scale_value_by_ally_exposure_local;
    actor *self;
    datum_index prop_index;
    prop *target;
    actor *ally;
    int16_t exposed_count;
    int16_t alert_count;
    float scale;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    prop_index = self->first_prop;
    exposed_count = 0;
    alert_count = 0;

    while (prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (prop_index & halo::k_slot_mask) * sizeof(prop));
        prop_index = target->next_in_actor;

        if (((1 < target->state && target->state < 4) && target->enemy == 0) &&
            (target->actor_type == self->type && target->owner_actor_index != k_datum_index_none)) {
            ally = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (target->owner_actor_index & halo::k_slot_mask) * sizeof(actor));

            if (ally->pending_panic_type < 1 &&
                (ally->mode != 4 || *(int16_t *)((uint8_t *)ally + 0xa8) < 1)) {
                if (target->owner_stalled != 0) {
                    exposed_count++;
                }
            } else {
                alert_count++;
            }
        }
    }

    if (alert_count > 1) {
        return 1;
    }
    {
        if (exposed_count < 2) {
            scale = (float)(1 - exposed_count) * 0.5f + 1.0f;
        } else {
            scale = 1.0f - (float)(exposed_count - 1) * 0.25f;
        }
        if (scale < 0.0f) {
            *value = *value * 0.0f;
            return 0;
        }
        if (2.0f < scale) {
            scale = 2.0f;
        }
        *value = scale * *value;
    }
    return 0;
}

namespace actor_score_blast_area_clear_local {
}

/**
 * Actor AI behaviour: score blast area clear.
 *
 * @address 0x410da0
 */
uint8_t ActorView::score_blast_area_clear(float blast_radius, float safety_radius, real_point3d *point, int16_t *out_count)
{
    using namespace actor_score_blast_area_clear_local;
    actor *self;
    datum_index prop_cursor;
    datum_index counted[32];
    int16_t counted_count;
    uint8_t clear;
    int16_t score;
    prop *p;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    prop_cursor = self->first_prop;
    counted_count = 0;
    clear = 1;
    score = 0;
    p = (prop *)0;

    for (;;) {
        int done_with_hostiles = 0;

        for (;;) {
            for (;;) {
                if (prop_cursor == (datum_index)k_datum_index_none) {
                    goto after_prop_walk;
                }
                p = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (prop_cursor & halo::k_slot_mask) * sizeof(prop));
                prop_cursor = p->next_in_actor;
                if (2 <= p->state && p->state <= 3 && p->dead == 0) break;
            }
            if (p->enemy == 0) {
                done_with_hostiles = 1;
                break;
            }
            {
                float dx = point->x - p->last_known_position.x;
                float dy = point->y - p->last_known_position.y;
                float dz = point->z - p->last_known_position.z;
                if (dx * dx + dy * dy + dz * dz < blast_radius * blast_radius) {
                    if (p->is_parented == 0) {
                        if (p->relationship_object_index == -1) {
                            datum_index owner = p->owner_actor_index;
                            if (owner != (datum_index)k_datum_index_none) {
                                if (counted_count < 0x20) {
                                    counted[counted_count] = owner;
                                    counted_count++;
                                }
                                if (p->swarm_owned == 0) {
                                    score++;
                                } else {
                                    actor *owner_actor = (actor *)((uint8_t *)halo::ai::globals().actor_data->data +
                                                                    (owner & halo::k_slot_mask) * sizeof(actor));
                                    score += owner_actor->cluster_count;
                                }
                            }
                        } else {
                            score += 5;
                        }
                    } else {
                        score += 10;
                    }
                }
            }
        }

        if (!done_with_hostiles) {
            break;
        }

        if (safety_radius <= 0.0f) {
            continue;
        }
        {
            float dx = point->x - p->last_known_position.x;
            float dy = point->y - p->last_known_position.y;
            float dz = point->z - p->last_known_position.z;
            if (safety_radius * safety_radius <= dx * dx + dy * dy + dz * dz) {
                continue;
            }
        }
        clear = 0;
        break;
    }

after_prop_walk:
    if (0.0f < blast_radius && self->target_unit_index != (datum_index)k_datum_index_none) {
        prop *target_prop = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (self->target_unit_index & halo::k_slot_mask) * sizeof(prop));
        datum_index owner = target_prop->owner_actor_index;
        if (owner != (datum_index)k_datum_index_none) {
            actor *owner_actor = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (owner & halo::k_slot_mask) * sizeof(actor));
            if (owner_actor->encounter_index != (datum_index)k_datum_index_none) {
                datum_index iterator[3];
                datum_index cursor;
                iterator[2] = k_datum_index_none;
                halo::ai::ai_reference_actor_iterator_init_cursor((int32_t)owner_actor->encounter_index, iterator);
                cursor = iterator[2];
                while (halo::ai::globals().state->actors_valid != 0 && cursor != (datum_index)k_datum_index_none) {
                    actor *candidate = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (cursor & halo::k_slot_mask) * sizeof(actor));
                    datum_index next = candidate->next_in_encounter;
                    int16_t i;
                    uint8_t already_counted = 0;

                    for (i = 0; i < counted_count; i++) {
                        if (counted[i] == cursor) { already_counted = 1; break; }
                    }
                    if (!already_counted) {
                        float dx = point->x - candidate->body_position.x;
                        float dy = point->y - candidate->body_position.y;
                        float dz = point->z - candidate->body_position.z;
                        if (dx * dx + dy * dy + dz * dz < blast_radius * blast_radius) {
                            if (candidate->swarm == 0) {
                                score++;
                            } else {
                                score += candidate->cluster_count;
                            }
                        }
                    }
                    cursor = next;
                }
            }
        }
    }

    if (clear != 0 && self->encounter_index != (datum_index)k_datum_index_none && 0.0f < safety_radius) {
        datum_index iterator[3];
        datum_index cursor;
        iterator[2] = k_datum_index_none;
        halo::ai::ai_reference_actor_iterator_init_cursor((int32_t)self->encounter_index, iterator);
        cursor = iterator[2];
        while (halo::ai::globals().state->actors_valid != 0 && cursor != (datum_index)k_datum_index_none) {
            actor *candidate = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (cursor & halo::k_slot_mask) * sizeof(actor));
            float dx = point->x - candidate->body_position.x;
            float dy = point->y - candidate->body_position.y;
            float dz = point->z - candidate->body_position.z;
            cursor = candidate->next_in_encounter;
            if (dx * dx + dy * dy + dz * dz < safety_radius * safety_radius) {
                clear = 0;
                break;
            }
        }
    }

    if (out_count != (int16_t *)0) {
        *out_count = score;
    }
    return clear;
}

namespace actor_select_facing_target_prop_local {
extern "C" {
extern game_time_globals *game_time;
extern double cos(double x);
}
}

/**
 * stack -> out_result, stack -> out_in_front Walks actor.first_prop's chain looking for the highest-weighted
 * prop of kind 2 or 3 (with prop.unknown_32 set) that also passes a directional test against the actor's look
 * cones, and reports it through out_result / out_in_front. Every prop of the wrong kind
 *
 * @address 0x414a90
 */
uint8_t ActorView::select_facing_target_prop(uint8_t require_trust, uint8_t skip_lane_test, actor_recognition_scan_result *out_result, uint8_t *out_in_front)
{
    using namespace actor_select_facing_target_prop_local;
    actor *self;
    Actor *definition;
    float side_thresholds[2];
    float delta_r;
    float aiming_cos_threshold;
    float looking_cos_threshold;
    int32_t now;
    datum_index next_handle;
    datum_index current_handle;
    prop *cur;
    float score;
    uint8_t still_valid;
    uint8_t best_found;
    datum_index best_handle;
    prop *best_prop;
    uint8_t best_in_front;
    float best_score;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    definition = (Actor *)halo::cache::globals().tag_instances[self->actor_definition_tag & halo::k_slot_mask].data;
    now = game_time->game_time;

    if (self->awareness_level == 3) {
        side_thresholds[0] = (float)cos((double)definition->combat_look_delta_l);
        delta_r = definition->combat_look_delta_r;
    } else {
        side_thresholds[0] = (float)cos((double)definition->noncombat_look_delta_l);
        delta_r = definition->noncombat_look_delta_r;
    }
    side_thresholds[1] = (float)cos((double)delta_r);

    aiming_cos_threshold = definition->cosine_maximum_aiming_deviation.yaw;
    looking_cos_threshold = definition->cosine_maximum_looking_deviation.yaw;

    best_handle = (datum_index)k_datum_index_none;
    best_score = 0.0f;
    best_prop = 0;
    best_in_front = 0;
    best_found = 0;

    next_handle = self->first_prop;
    for (;;) {
        current_handle = next_handle;
        if (next_handle == (datum_index)k_datum_index_none) {
            break;
        }
        cur = &((prop *)halo::ai::globals().prop_data->data)[next_handle & halo::k_slot_mask];
        next_handle = cur->next_in_actor;

        if (!(cur->state > 1 && cur->state < 4 && cur->visual_perception != 0)) {
            cur->interest_satisfied = 0.0f;
            continue;
        }
        if (cur->interest <= 0.0f) {
            continue;
        }

        if (cur->last_attention_time == -1) {
            score = 1.0f;
        } else {
            score = ((float)now - (float)cur->last_attention_time) * 0.0016666667f - 1.0f;
        }
        score = (cur->interest - cur->interest_satisfied) / cur->interest + score;
        if (1.0f < score) {
            score = 1.0f;
        }
        score = score * cur->interest;
        still_valid = (cur->interest_satisfied < cur->interest) ? 1 : 0;

        if (score <= 0.0f) {
            continue;
        }

        if (skip_lane_test) {
            uint8_t accepted;
            if (require_trust == 0 || !still_valid) {
                accepted = halo::math::point3d_within_horizontal_cone(*((real_point3d *)&cur->direction), self->desired_facing_vector, aiming_cos_threshold);
            } else {
                accepted = 1;
            }
            if (accepted && best_score < score) {
                best_handle = current_handle;
                best_prop = cur;
                best_in_front = still_valid;
                best_score = score;
                best_found = 1;
            }
        } else {
            uint8_t accepted = halo::ai::actor_point_in_directional_lane((real_point3d *)&cur->direction, &self->desired_aiming_vector,
                                                                 &self->desired_facing_vector, looking_cos_threshold, side_thresholds);
            if (accepted && best_score < score) {
                best_handle = current_handle;
                best_prop = cur;
                best_in_front = still_valid;
                best_score = score;
                best_found = 1;
            }
        }
    }

    if (best_found && best_handle != (datum_index)k_datum_index_none) {
        best_prop->interest_satisfied = best_prop->interest;
        best_prop->last_attention_time = now;
        out_result->candidate = best_handle;
        out_result->flag = 1;
        *out_in_front = best_in_front;
        return 1;
    }
    return 0;
}

namespace actor_select_stance_offset_pair_local {
}

/**
 * Actor AI behaviour: select stance offset pair.
 *
 * @address 0x4106b0
 */
void ActorView::select_stance_offset_pair(uint8_t *base, uint8_t **out_a, uint8_t **out_b)
{
    using namespace actor_select_stance_offset_pair_local;
    actor *self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    *out_a = base + 0xcc;
    if (self->berserking != 0) {
        *out_b = base + 0x130;
        return;
    }
    if (self->new_target_firing_pattern != 0) {
        *out_b = base + 0x100;
        return;
    }
    if (self->moving_firing_pattern != 0) {
        *out_b = base + 0x118;
        return;
    }
    *out_b = (uint8_t *)0;
}

namespace actor_set_combat_alert_flag_local {
}

/**
 * Toggles the actor's (and, for grouped actors, its whole cluster's) combat-alert object flag when the alert
 * state changes.
 *
 * @address 0x421a40
 */
void ActorView::set_combat_alert_flag(uint8_t new_flag)
{
    using namespace actor_set_combat_alert_flag_local;
    actor *self;
    object *unit_obj;
    datum_index cluster_unit;
    object *cluster_obj;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    if ((char)new_flag == self->berserking) {
        return;
    }
    self->berserking = new_flag;
    self->berserk_announced = 0;

    if (self->swarm == 0) {
        unit_obj = ((object_header *)halo::objects::globals().object_data->data)[self->unit_index & halo::k_slot_mask].data;
        if (new_flag == 0) {
            ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->flags &= 0xffffff7f;
        } else {
            ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->flags |= 0x80;
        }
    } else {
        cluster_unit = self->cluster_unit_index;
        while (cluster_unit != k_datum_index_none) {
            cluster_obj = ((object_header *)halo::objects::globals().object_data->data)[cluster_unit & halo::k_slot_mask].data;
            cluster_obj->vitality_flags |= 0x80;
            cluster_unit = ((unit_data *)((uint8_t *)cluster_obj + k_unit_data_offset))->swarm_next_unit_index;
        }
    }

    if (new_flag != 0) {
        self->always_charge = 1;
    }
}

namespace actor_set_flag_bit1_local {
}

/**
 * Actor AI behaviour: set flag bit1.
 *
 * @address 0x42a5b0
 */
void ActorView::set_flag_bit1()
{
    using namespace actor_set_flag_bit1_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    self->control_flags |= _actor_flag_unknown_bit1;
}

namespace actor_set_override_target_local {
}

/**
 * Enables or disables the override-target flag (bit 0x800 of actor.flags) and records the supplied value into
 * actor.override_target either way.
 *
 * @address 0x42a5e0
 */
void ActorView::set_override_target(uint8_t enable, datum_index override_target)
{
    using namespace actor_set_override_target_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (enable != 0) {
        self->control_flags |= _actor_flag_override_target;
    } else {
        self->control_flags &= ~_actor_flag_override_target;
    }
    self->override_target = override_target;
}

namespace actor_set_target_alert_stage1_local {
}

/**
 * Marks a per-prop alert/notice flag (noticed_a) and, if that prop is the caller actor's current target,
 * refreshes the actor's target combat status.
 *
 * @address 0x41fb00
 */
void TargetView::set_target_alert_stage1(datum_index actor_index)
{
    using namespace actor_set_target_alert_stage1_local;
    prop *target;
    actor *self;

    if (target_prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (target_prop_index & halo::k_slot_mask) * sizeof(prop));
        target->noticed_a = 1;

        self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
        if (target_prop_index == self->target_unit_index) {
            halo::ai::actor_update_target_combat_status(actor_index);
            halo::ai::actor_update_awareness_level(actor_index);
        }
    }
}

namespace actor_set_target_alert_stage2_local {
}

/**
 * Marks a second per-prop alert/notice flag (noticed_b) and refreshes the actor's target combat status if that
 * prop is the actor's current target.
 *
 * @address 0x41fb60
 */
void TargetView::set_target_alert_stage2(datum_index actor_index)
{
    using namespace actor_set_target_alert_stage2_local;
    prop *target;
    actor *self;

    if (target_prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (target_prop_index & halo::k_slot_mask) * sizeof(prop));
        target->noticed_b = 1;

        self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
        if (target_prop_index == self->target_unit_index) {
            halo::ai::actor_update_target_combat_status(actor_index);
            halo::ai::actor_update_awareness_level(actor_index);
        }
    }
}

namespace actor_set_target_alert_stage3_local {
}

/**
 * Marks the third per-prop alert flag and promotes the prop's kind, or, when no target prop is supplied, clears
 * the actor's own perception/awareness accumulator fields.
 *
 * @address 0x41fbc0
 */
void TargetView::set_target_alert_stage3(datum_index actor_index)
{
    using namespace actor_set_target_alert_stage3_local;
    actor *self;
    prop *target;

    if (target_prop_index == k_datum_index_none) {
        self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
        *(int16_t *)&self->pursuit_position_count = 0;
        self->target_lost = 0;
        self->unknown_3bd[0] = 0;
        self->minimum_combat_status = 0;
        self->suspicion_status = 0;
        halo::ai::actor_update_awareness_level(actor_index);
        return;
    }

    target = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (target_prop_index & halo::k_slot_mask) * sizeof(prop));
    if (target->state == 4) {
        target->state = 5;
    }
    target->noticed_c = 1;

    self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    if (target_prop_index == self->target_unit_index) {
        halo::ai::actor_update_target_combat_status(actor_index);
        halo::ai::actor_update_awareness_level(actor_index);
    }
}

namespace actor_set_units_active_local {
}

/**
 * Puts the actor's units to sleep (dormant = 1) or wakes them (dormant = 0), whether it has a lone unit, a
 * cluster object's active bit: waking sets it (0x4f50f0, unless the object is parented or flagged 0x100000),
 * sleeping clears it (0x4f5130). Actor +0x13 records the state (a sleeping actor's +0x14 c
 *
 * @address 0x427860
 */
void ActorView::set_units_active(uint8_t dormant)
{
    using namespace actor_set_units_active_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->active != 0 && self->keep_unit_alive != dormant) {
        if (self->swarm == 0) {
            if (self->unit_index != (datum_index)k_datum_index_none) {
                if (dormant == 0) {
                    halo::objects::object_mark_pending_delete(self->unit_index);
                } else {
                    halo::objects::object_clear_pending_delete_flag(self->unit_index);
                }
            }
        } else if (self->swarm_index == (datum_index)k_datum_index_none) {
            datum_index unit_index = self->cluster_unit_index;
            while (unit_index != (datum_index)k_datum_index_none) {
                object_header *header = &((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask];
                object *unit_object = header->data;

                if (dormant == 0) {
                    halo::objects::object_mark_pending_delete(unit_index);
                } else if ((header->flags & _object_header_active_bit) != 0) {
                    header->flags &= ~_object_header_active_bit;
                }
                unit_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc);
            }
        } else {
            swarm *s = &((swarm *)halo::ai::globals().swarm_data->data)[self->swarm_index & halo::k_slot_mask];
            int16_t i;

            for (i = 0; i < s->component_count; i++) {
                if (dormant == 0) {
                    halo::objects::object_mark_pending_delete(s->unit_index[i]);
                } else {
                    object_header *header = &((object_header *)halo::objects::globals().object_data->data)[s->unit_index[i] & halo::k_slot_mask];
                    if ((header->flags & _object_header_active_bit) != 0) {
                        header->flags &= ~_object_header_active_bit;
                    }
                }
            }
        }

        self->keep_unit_alive = dormant;
        if (dormant == 0) {
            *(int16_t *)((uint8_t *)self + 0x14) = 0;
        }
    }
}

namespace actor_should_hold_position_local {
}

/**
 * FIXED (objdump 0x41064d..0x41069b): EDX is the actor definition; the hold timer +0x5f4 is trunc(((def +0x84 -
 * def +0x80) * random + def +0x80) * 30). The draft stored 0.
 *
 * @address 0x4105c0
 */
uint8_t ActorView::should_hold_position(uint8_t *definition)
{
    using namespace actor_should_hold_position_local;
    actor *self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));

    if (self->firing_target_type == 1) {
        prop *p = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (self->firing_target_prop_index & halo::k_slot_mask) * sizeof(prop));
        if (3 < p->state && p->state < 6) {
            self->target_lost = 1;
            self->firing_state_timer = 0;
            return 0;
        }
    }

    if (self->force_fire != 0) {
        self->firing_state_timer = 0;
        return self->force_fire == 0;
    }

    {
        float lo = *(float *)(definition + 0x80);
        float hi = *(float *)(definition + 0x84);
        float r;

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        r = (float)(int32_t)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f;
        self->firing_state_timer = (int16_t)(int32_t)(((hi - lo) * r + lo) * 30.0f);
    }
    return 1;
}

namespace actor_snapshot_orientation_local {
extern "C" {
extern const real_vector3d *global_origin3d_pointer;
}
}

/**
 * Snapshots the actor's current orientation basis (facing and its two companion vectors) into the secondary
 * snapshot block, resets flags and override_target to zero, seeds queued_look_vector from the shared origin
 * vector, and marks the snapshot's status word unset.
 *
 * @address 0x4294d0
 */
void ActorView::snapshot_orientation()
{
    using namespace actor_snapshot_orientation_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    self->snapshot_facing = self->facing;
    self->aiming_vector_snapshot = self->unit_aiming_vector;
    self->looking_vector_snapshot = self->unit_looking_vector;

    self->control_flags = 0;
    self->override_target = 0;
    self->throttle = *global_origin3d_pointer;
    self->control_animation_impulse = -1;
}

namespace actor_spawn_additional_units_local {
extern "C" {
extern double cos(double x);
extern double sin(double x);
}
}

/**
 * health_scale Spawns up to spawn_count additional units around source_actor's position and attaches new actors
 * to them (reusing the source's squad/platoon identity when it has no live unit of its own, otherwise its
 * controlling actor's), optionally randomizing scale/health when health_scale is positiv
 *
 * @address 0x427280
 */
int16_t ActorOps::spawn_additional_units(datum_index actor_variant_tag, int16_t spawn_count, datum_index source_actor_index, float health_scale)
{
    using namespace actor_spawn_additional_units_local;
    int16_t spawned = 0;

    if (actor_variant_tag == (datum_index)k_datum_index_none || spawn_count <= 0) {
        return 0;
    }

    {
        object *source_object = ((object_header *)halo::objects::globals().object_data->data)[source_actor_index & halo::k_slot_mask].data;
        unit_data *source_unit = (unit_data *)((uint8_t *)source_object + k_unit_data_offset);
        int16_t encounter_index, squad_index;

        if (source_unit->swarm_actor_index == (datum_index)k_datum_index_none &&
            source_unit->actor_index == (datum_index)k_datum_index_none) {
            encounter_index = *(int16_t *)((uint8_t *)source_object + 0x334);
            squad_index = *(int16_t *)((uint8_t *)source_object + 0x336);
        } else {
            actor *owner_actor = &((actor *)halo::ai::globals().actor_data->data)[source_unit->actor_index & halo::k_slot_mask];
            encounter_index = owner_actor->encounter_index;
            squad_index = owner_actor->squad_index;
        }

        if (encounter_index == -1 || squad_index == -1) {
            return 0;
        }

        {
            const uint8_t *variant_tag_data = (const uint8_t *)(halo::cache::globals().tag_instances[actor_variant_tag & halo::k_slot_mask].data);
            const uint32_t *variant = (const uint32_t *)variant_tag_data;
            const uint8_t *actor_tag_data = (const uint8_t *)(halo::cache::globals().tag_instances[variant[4] & halo::k_slot_mask].data);
            int16_t i;

            for (i = 0; i < spawn_count; i++) {
                object_placement_data placement;
                datum_index new_object;
                float angle;
                uint32_t random_bits;

                halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
                random_bits = halo::math::globals().random_seed_global >> 16;
                angle = (float)(int32_t)random_bits * 1.5259022e-05f * 6.2831855f;
                halo::objects::object_placement_data_initialize(&placement, (datum_index)variant[8], (datum_index)k_datum_index_none);
                placement.forward.i = (float)cos(angle);
                placement.forward.j = (float)sin(angle);
                placement.forward.k = 0.0f;
                halo::objects::object_get_position(&placement.position, source_actor_index);
                placement.position.x = placement.forward.i * 0.3f + placement.position.x;
                placement.position.y = placement.forward.j * 0.3f + placement.position.y;
                placement.position.z = placement.forward.k * 0.3f + (placement.position.z + 0.3f);

                new_object = halo::objects::object_new(&placement);
                if (new_object == (datum_index)k_datum_index_none) {
                    continue;
                }
                {
                    uint8_t *new_obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[new_object & halo::k_slot_mask].data;
                    char reuse_existing = (char)((*(const uint32_t *)actor_tag_data >> 0x1a) & 1);
                    datum_index new_actor;

                    if (((object *)new_obj)->type == 0) {
                        halo::units::unit_find_placement_position(new_object, halo::k_dword_none, 0, 1.0f, 1, 0, 0, 0,
                            (real_vector3d *)&placement.position);
                    }
                    halo::ai::actor_apply_unit_definition_properties(actor_variant_tag, new_object);
                    new_actor = halo::ai::actor_new_and_attach_to_unit(reuse_existing, new_object, actor_variant_tag,
                        (uint32_t)(int32_t)encounter_index, squad_index, 0, (datum_index)k_datum_index_none, 0, 2, 0,
                        halo::k_word_none, 0);
                    if (new_actor == (datum_index)k_datum_index_none) {
                        halo::objects::object_delete(new_object);
                        continue;
                    }
                    if (health_scale > 0.0f) {
                        real_vector3d impulse;
                        float r1 = halo::math::random_real_range(0.5f, 1.0f);
                        float r2;

                        impulse.i = placement.forward.i * r1;
                        impulse.j = placement.forward.j * r1;
                        r2 = halo::math::random_real_range(0.8f, 1.5f);
                        impulse.i = impulse.i * health_scale;
                        impulse.j = impulse.j * health_scale;
                        impulse.k = r2 * health_scale;
                        if (((object *)new_obj)->type == 0) {
                            halo::units::unit_apply_impulse(new_object, &impulse);
                        }
                    }
                    spawned = spawned + 1;
                }
            }
        }
    }

    return spawned;
}

namespace actor_start_search_timer_local {
extern "C" {
extern game_time_globals *game_time;
}
}

/**
 * Marks the start of an investigation timer: stamps the current tick on the actor (a scratch field also read
 * elsewhere), resolves the prop's relationship/obstruction object, and queues a priority-2, 90-tick search
 * position at the prop's ground position (with its path surface index and a 1.25 radius/we
 *
 * @address 0x422130
 */
void ActorView::start_search_timer(datum_index prop_index)
{
    using namespace actor_start_search_timer_local;
    prop *target = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    self->found_body_time = (uint32_t)game_time->game_time;

    halo::ai::actor_target_get_relationship_object(prop_index);

    halo::ai::actor_queue_search_position(actor_index, &target->pathfinding_point, 2, 0,
                                target->pathfinding_surface_index, 0x3fc00000 , 90,
                                prop_index, 90, 1);
}

namespace actor_swarm_for_each_component_local {
}

/**
 * If the actor is not a swarm, invokes callback once for the actor's own unit. Otherwise, for every live
 * component of the actor's swarm: optionally (reset_first) zeroes the component's scratch region
 * (swarm_component+0x1c..0x3f) and marks it bit 3 "active" in its flags (+2), then, if that active bit i
 *
 * @address 0x407040
 */
void ActorView::swarm_for_each_component(char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, uint16_t *caller_record)
{
    using namespace actor_swarm_for_each_component_local;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (a->swarm == 0) {
        callback(actor_index, a->unit_index, *caller_record, caller_record + 4, (int32_t)(intptr_t)(caller_record + 0x16), callback_extra);
        return;
    }

    {
        swarm *sw = &((swarm *)halo::ai::globals().swarm_data->data)[a->swarm_index & halo::k_slot_mask];
        int16_t i;

        for (i = 0; i < sw->component_count; i++) {
            swarm_component *comp = &((swarm_component *)halo::ai::globals().swarm_component_data->data)[sw->component_index[i] & halo::k_slot_mask];
            uint8_t *comp_base = (uint8_t *)comp;

            if (reset_first != 0) {
                memset(comp_base + 0x1c, 0, 0x24);
                *(uint16_t *)&((struct swarm_component *)comp_base)->flags = (*(uint16_t *)&((struct swarm_component *)comp_base)->flags & 0xfffb) | 8;
            }
            if ((comp_base[2] & 8) != 0) {
                callback(actor_index, sw->unit_index[i], *caller_record, comp_base + 0x1c, 0, callback_extra);
            }
        }
    }
}

namespace actor_swarm_for_each_component_thunk_local {
}

/**
 * fixed callback used by ai_reference_invoke_squad_callback_406f80.c (this module)
 *
 * @address 0x407240
 */
void ActorView::swarm_for_each_component_thunk()
{
    using namespace actor_swarm_for_each_component_thunk_local;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint16_t *caller_record = (uint16_t *)((uint8_t *)a + 0x9c);

    halo::ai::actor_swarm_for_each_component(actor_index, 0, (actor_swarm_member_callback)halo::ai::actor_obey_member_advance, 0, caller_record);
}

namespace actor_take_danger_escape_local {
extern "C" {
typedef struct actor_dodge_entry {
    int16_t action;
    int16_t direction;
    float bias;
} actor_dodge_entry;
extern const actor_dodge_entry actor_dodge_table[];
#define ACTOR(h) ((uint8_t *)halo::ai::globals().actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

/**
 * Actor AI behaviour: take danger escape.
 *
 * @address 0x40e060
 */
uint8_t ActorOps::take_danger_escape(real_vector3d *path_delta, datum_index actor_index, uint32_t escape, uint32_t extra, float distance)
{
    using namespace actor_take_danger_escape_local;
    uint8_t *act = ACTOR(actor_index);
    uint16_t direction_kind = (uint16_t)escape;
    float step_distance = *(float *)&extra;
    uint8_t probe_flag;
    float probe_extra[4];
    float a = 0.0f;
    float b = 0.0f;
    float scores[4];
    float fx;
    float fy;
    float best = -0.5f;
    int16_t best_action = -1;
    int16_t best_direction = -1;
    int32_t i;
    float payload[2];
    uint8_t queued;

    if (((actor *)act)->active_unit_index != k_datum_index_none) {
        return 0;
    }
    if (!halo::ai::actor_probe_step_direction(actor_index, step_distance, (real_vector2d *)path_delta, &direction_kind, distance,
                                    &probe_flag, probe_extra)) {
        return 0;
    }
    switch ((int16_t)direction_kind) {
    case 0: a = -path_delta->j; b = path_delta->i; break;
    case 1: a = path_delta->j; b = -path_delta->i; break;
    case 2:
    case 3: a = path_delta->i; b = path_delta->j; break;
    default: b = probe_extra[1]; break;
    }
    fx = ((actor *)act)->facing.i;
    fy = ((actor *)act)->facing.j;
    scores[2] = b * fy + a * fx;
    scores[0] = fx * b - fy * a;
    scores[3] = -scores[2];
    scores[1] = -scores[0];
    for (i = 0; actor_dodge_table[i].action != -1; i++) {
        float value = scores[actor_dodge_table[i].direction] + actor_dodge_table[i].bias;

        if (value > best && halo::units::unit_scripted_action_animation_exists(((actor *)act)->unit_index, actor_dodge_table[i].action)) {
            best = value;
            best_direction = actor_dodge_table[i].direction;
            best_action = actor_dodge_table[i].action;
        }
    }
    if (best_action == -1) {
        return 0;
    }
    switch (best_direction) {
    case 0: payload[0] = b; payload[1] = -a; break;
    case 1: payload[0] = -b; payload[1] = a; break;
    case 2:
    case 3: payload[0] = a; payload[1] = b; break;
    default: payload[0] = 0.0f; payload[1] = 0.0f; break;
    }
    queued = halo::ai::actor_queue_secondary_action(actor_index, best_action, (uint32_t *)payload);
    if (queued) {
        halo::ai::ai_communication_broadcast(0x2c, ((actor *)act)->unit_index, -1, -1, -1, -1, 0);
    }
    return queued;
}

#undef ACTOR

namespace actor_toggle_active_state_local {
extern "C" {
extern game_time_globals *game_time;
}
}

/**
 * Toggles the actor's active/dormant flag. Deactivating clears its perceived-prop list and swarm, deactivates
 * its units, and stamps the current tick into a scratch field. Reactivating a swarm actor first (re)creates its
 * swarm, bailing out if that fails; either way marks the actor active and, if its aw
 *
 * @address 0x4277c0
 */
uint8_t ActorOps::toggle_active_state(uint8_t activate, datum_index actor_index)
{
    using namespace actor_toggle_active_state_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->active == activate) {
        return 1;
    }

    if (activate == 0) {
        halo::ai::actor_clear_perceived_props(actor_index);
        halo::ai::actor_delete_swarm(actor_index);
        halo::ai::actor_set_units_active(actor_index, 1);
        self->active = 0;
        self->deactivation_time = (int32_t)game_time->game_time;
        return 1;
    }

    if (self->swarm != 0) {
        halo::ai::actor_create_swarm(actor_index);
        if (self->swarm_index == (datum_index)k_datum_index_none) {
            self->swarm_pending = 1;
            return 0;
        }
    }

    self->active = 1;
    if (self->awareness_level == 0) {
        halo::ai::actor_set_units_active(actor_index, 0);
        return 1;
    }
    return 1;
}

namespace actor_unlink_prop_local {
}

/**
 * Actor AI behaviour: unlink prop.
 *
 * @address 0x43ea20
 */
void ActorView::unlink_prop(datum_index prop_to_remove)
{
    using namespace actor_unlink_prop_local;
    actor *self = (actor *)((uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * sizeof(actor));
    datum_index cur = self->first_prop;

    if (cur == prop_to_remove) {
        prop *removed = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (cur & halo::k_slot_mask) * sizeof(prop));
        self->first_prop = removed->next_in_actor;
        return;
    }

    for (;;) {
        prop *p = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (cur & halo::k_slot_mask) * sizeof(prop));
        if (p->next_in_actor == prop_to_remove) {
            prop *removed = (prop *)((uint8_t *)halo::ai::globals().prop_data->data + (prop_to_remove & halo::k_slot_mask) * sizeof(prop));
            p->next_in_actor = removed->next_in_actor;
            return;
        }
        cur = p->next_in_actor;
    }
}

namespace actor_unlink_unit_local {
}

/**
 * Detaches the actor from its single bound unit, marking the object header's "in PVS pass" bit and, if the unit
 * was never actually placed (no parent, no cluster), clearing its active bit too. Clears the unit's
 * back-reference, decrements the owning encounter's live count if this actor was counted towar
 *
 * @address 0x427bc0
 */
void ActorView::unlink_unit()
{
    using namespace actor_unlink_unit_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    datum_index unit_index = self->unit_index;

    if (unit_index != (datum_index)k_datum_index_none) {
        object_header *header = &((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask];
        object *unit_object = header->data;

        header->flags |= _object_header_in_pvs_pass_bit;
        if (unit_object->parent_object == (datum_index)k_datum_index_none &&
            unit_object->location_cluster_index == -1) {
            if ((header->flags & _object_header_active_bit) != 0) {
                header->flags &= ~_object_header_active_bit;
            }
        }

        halo::units::unit_refresh_targeting_flag_and_weapons(unit_index, 0);

        *(datum_index *)((uint8_t *)unit_object + 500) = (datum_index)k_datum_index_none;

        if (self->counts_toward_encounter != 0 && self->encounter_index != (datum_index)k_datum_index_none) {
            encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[self->encounter_index & halo::k_slot_mask];
            enc->live_count = enc->live_count - 1;
        }

        self->unit_index = (datum_index)k_datum_index_none;
        self->counts_toward_encounter = 0;
    }
}

namespace actor_vehicle_not_recently_left_local {
extern "C" {
extern game_time_globals *game_time;
#define ACTOR(h) ((uint8_t *)halo::ai::globals().actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)halo::ai::globals().prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
}
}

/**
 * Actor AI behaviour: vehicle not recently left.
 *
 * @address 0x40ac30
 */
uint8_t ActorView::vehicle_not_recently_left(datum_index vehicle_index)
{
    using namespace actor_vehicle_not_recently_left_local;
    uint8_t *act = ACTOR(actor_index);

    if (vehicle_index != ((struct actor *)act)->exited_vehicle_index) {
        return 1;
    }
    return (uint8_t)(game_time->game_time >= *(int32_t *)&((struct actor *)act)->exited_vehicle_reentry_time);
}

#undef ACTOR
#undef TAG_DATA
#undef PROP

namespace actor_wants_reload_or_swap_local {
}

/**
 * Actor AI behaviour: wants reload or swap.
 *
 * @address 0x40ab80
 */
uint8_t ActorView::wants_reload_or_swap()
{
    using namespace actor_wants_reload_or_swap_local;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint8_t result = 0;

    if (a->pending_command_list != -1 && a->command_list_delay > 0) {
        result = 1;
    }

    if (a->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[a->encounter_index & halo::k_slot_mask];
        encounter_squad_state *squad = &halo::ai::globals().squad_states[enc->first_squad + a->squad_index];

        if (squad->squad_delay_ticks > 0) {
            if (a->combat_status < 5) {
                result = 1;
            } else {
                halo::ai::encounter_squad_clear_spawn_delay(a->encounter_index, a->squad_index);
            }
        }
    }

    if (a->mode == _actor_mode_flee) {
        if (a->mode_data.raw[0x9e - 0x9c] == 0 && a->mode_data.raw[0xa1 - 0x9c] == 0) {
            return 1;
        }
    }
    return result;
}

}
