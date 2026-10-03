#include "halo/ai/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"

namespace halo::ai {

namespace actor_type_crew_update_local {
}

/**
 * Actor AI behaviour: type crew update.
 *
 * @address 0x423890
 */
void ActorView::type_crew_update()
{
    using namespace actor_type_crew_update_local;
    struct actor *actor = halo::ai::actor_at(actor_index);

    if (actor->mode == 0 && actor->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 1);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_alert_from_disturbance(actor_index);
        halo::ai::actor_alert_from_squad_attack(actor_index);
        halo::ai::actor_alert_from_projectile(actor_index);
        halo::ai::actor_alert_from_flag_1b4(actor_index);
        halo::ai::actor_alert_from_damage(actor_index);
        halo::ai::actor_gate_jump_traversal(actor_index, 1, 0, 9);
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (actor->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (actor->mode_data.flee.engage != 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        halo::ai::actor_flee_look_away(actor_index);
        return;
    case 5:
    case 7:
    case 8:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        halo::ai::actor_update_special_mode(actor_index);
        return;
    case 9:
        if (actor->mode_data.vehicle.unit_replaced != 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        if (actor->mode_data.vehicle.failed != 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, actor->mode_data.obey.allow_initiative, actor->mode_data.obey.finished);
        return;
    case 12: {
        uint8_t forced = (actor->mode_data.converse.finished != 0 || actor->conversation_index == halo::k_dword_none) ? 1 : 0;

        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_command_list_permits_escalation(actor_index), forced);
        return;
    }
    case 13:
        if (actor->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}

namespace actor_type_elite_update_local {
}

/**
 * Actor AI behaviour: type elite update.
 *
 * @address 0x423a90
 */
void ActorView::type_elite_update()
{
    using namespace actor_type_elite_update_local;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[act->actor_definition_tag & halo::k_slot_mask].data;

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 4);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_escalate_check_leader_flag(actor_index);
        halo::ai::actor_escalate_check_shield_damage(actor_index);
        halo::ai::actor_escalate_check_target_close(actor_index);
        halo::ai::actor_escalate_check_weapon_range(actor_index);
        halo::ai::actor_escalate_apply(actor_index, 1);
        if (!act->berserking) {
            halo::ai::actor_alert_from_projectile(actor_index);
            halo::ai::actor_alert_from_flag_1b4(actor_index);
            halo::ai::actor_gate_jump_traversal(actor_index, 9, 0, 11);
        }
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_try_grenade_evasion(actor_index, 0, 0);
        halo::ai::actor_seek_vehicle_to_board(actor_index);
        halo::ai::actor_process_vehicle_seat_exit(actor_index);
        halo::ai::actor_update_grenade_throw_decision(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        if (act->mode_data.guard.ambush_active && !act->mode_data.guard.ambush_triggered && !act->mode_data.guard.ambush_retreat) {
            float limit = act->combat_status >= 4 ? ((Actor *)actor_tag)->attack_shield_fraction : ((Actor *)actor_tag)->pursue_shield_fraction;

            if (limit > act->shield_vitality) {
                act->mode_data.guard.ambush_active = 1;
                act->mode_data.flee.panic = 30;
            } else {
                act->mode_data.guard.ambush_active = 0;
                act->mode_data.flee.panic = 0;
            }
        }
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (act->mode_data.flee.engage) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        halo::ai::actor_flee_look_away(actor_index);
        return;
    case 5:
    case 7:
    case 8:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        halo::ai::actor_update_special_mode(actor_index);
        return;
    case 9:
        if (act->mode_data.vehicle.unit_replaced) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        } else if (act->mode_data.vehicle.failed) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        return;
    case 12:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act->mode_data.converse.finished || act->conversation_index == k_datum_index_none));
        return;
    case 13:
        if (act->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}


namespace actor_type_engineer_update_local {
}

/**
 * Actor AI behaviour: type engineer update.
 *
 * @address 0x423d40
 */
void ActorView::type_engineer_update()
{
    using namespace actor_type_engineer_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 1);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_alert_from_squad_attack(actor_index);
        halo::ai::actor_alert_from_projectile(actor_index);
        halo::ai::actor_alert_from_flag_1b4(actor_index);
        halo::ai::actor_alert_from_damage(actor_index);
        halo::ai::actor_gate_jump_traversal(actor_index, 1, 0, 9);
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_update_grenade_throw_decision(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        break;
    case 4:
        if (act->mode_data.flee.engage == 0) {
            halo::ai::actor_flee_look_away(actor_index);
        } else {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            halo::ai::actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 9:
        if (act->mode_data.vehicle.unit_replaced != 0 || act->mode_data.vehicle.failed != 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        break;
    case 12:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_command_list_permits_escalation(actor_index),
            (uint8_t)(act->mode_data.converse.finished != 0 || act->conversation_index == k_datum_index_none));
        break;
    case 13:
        if (act->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}


namespace actor_type_flood_carrier_update_local {
}

/**
 * Actor AI behaviour: type flood carrier update.
 *
 * @address 0x423740
 */
void ActorView::type_flood_carrier_update()
{
    using namespace actor_type_flood_carrier_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 4);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_escalate_check_leader_flag(actor_index);
        halo::ai::actor_escalate_check_shield_damage(actor_index);
        halo::ai::actor_escalate_check_target_close(actor_index);
        halo::ai::actor_escalate_apply(actor_index, 1);
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        break;
    case 4:
        if (act->mode_data.flee.engage != 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        } else {
            halo::ai::actor_flee_look_away(actor_index);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            halo::ai::actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        break;
    default:
        break;
    }
}


namespace actor_type_flood_update_local {
}

/**
 * Actor AI behaviour: type flood update.
 *
 * @address 0x423f30
 */
void ActorView::type_flood_update()
{
    using namespace actor_type_flood_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 4);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_escalate_check_shield_damage(actor_index);
        halo::ai::actor_escalate_apply(actor_index, 3);
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        break;
    case 4:
        if (act->mode_data.flee.engage == 0) {
            halo::ai::actor_flee_look_away(actor_index);
        } else {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            halo::ai::actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        break;
    case 13:
        if (act->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}


namespace actor_type_grunt_update_local {
}

/**
 * Actor AI behaviour: type grunt update.
 *
 * @address 0x424590
 */
void ActorView::type_grunt_update()
{
    using namespace actor_type_grunt_update_local;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t may_broadcast = (uint8_t)((int8_t)act->tally.group_a_by_actor_type[0] > 0);
    uint8_t panics = (uint8_t)((int8_t)act->tally.group_c_by_actor_type[0] > 0);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 1);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_alert_from_disturbance(actor_index);
        halo::ai::actor_alert_from_squad_attack(actor_index);
        halo::ai::actor_alert_from_projectile(actor_index);
        halo::ai::actor_alert_from_flag_1b4(actor_index);
        halo::ai::actor_alert_from_damage(actor_index);
        halo::ai::actor_gate_jump_traversal(actor_index, 1, (char)may_broadcast, 7);
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_seek_vehicle_to_board(actor_index);
        halo::ai::actor_process_vehicle_seat_exit(actor_index);
        halo::ai::actor_update_grenade_throw_decision(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (panics && act->mode_data.flee.panic > 0 && !halo::ai::actor_order_code_is_grenade_throw(act->mode_data.flee.panic)) {
            act->mode_data.flee.finished = 1;
        }
        if (act->mode_data.flee.engage) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        if (halo::ai::actor_flee_look_away(actor_index)) {
            return;
        }
        if (act->mode_data.flee.panic == 0 && act->combat_status >= 5) {
            halo::ai::actor_consider_grenade_throw(actor_index);
        }
        return;
    case 5:
    case 7:
    case 8:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        halo::ai::actor_update_special_mode(actor_index);
        return;
    case 9:
        if (act->mode_data.vehicle.unit_replaced) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        } else if (act->mode_data.vehicle.failed) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        return;
    case 12:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act->mode_data.converse.finished || act->conversation_index == k_datum_index_none));
        return;
    case 13:
        if (act->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}


namespace actor_type_hunter_update_local {
}

/**
 * Actor AI behaviour: type hunter update.
 *
 * @address 0x424810
 */
void ActorView::type_hunter_update()
{
    using namespace actor_type_hunter_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_escalate_check_shield_damage(actor_index);
        halo::ai::actor_escalate_check_weapon_range(actor_index);
        halo::ai::actor_escalate_apply(actor_index, 3);
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 4:
    case 6:
    case 10:
        if (!halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            halo::ai::actor_conditional_state_transition_check(actor_index);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            halo::ai::actor_update_special_mode(actor_index);
        }
        break;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        break;
    case 12:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_command_list_permits_escalation(actor_index),
            (uint8_t)(act->mode_data.converse.finished != 0 || act->conversation_index == k_datum_index_none));
        break;
    case 13:
        if (act->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}


namespace actor_type_infection_swarm_update_local {
extern "C" {
extern game_time_globals *game_time;
extern const real_point3d *global_origin3d_pointer;
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);
#define OBJECT(h) ((uint8_t *)halo::ai::object_at((h)))
#define COMPONENT(h) ((uint8_t *)halo::ai::globals().swarm_component_data->data + ((h) & halo::k_slot_mask) * k_swarm_component_size)
#define F(p, o) (*(float *)((uint8_t *)(p) + (o)))
#define U16(p, o) (*(uint16_t *)((uint8_t *)(p) + (o)))
#define I16(p, o) (*(int16_t *)((uint8_t *)(p) + (o)))
static uint32_t swarm_random_next(void)
{
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    return halo::math::globals().random_seed_global >> 16;
}
static void copy3(real_vector3d *out, const void *in)
{
    out->i = ((const float *)in)[0];
    out->j = ((const float *)in)[1];
    out->k = ((const float *)in)[2];
}
}
}

/**
 * Actor AI behaviour: type infection swarm update.
 *
 * @address 0x424c20
 */
void ActorView::type_infection_swarm_update()
{
    using namespace actor_type_infection_swarm_update_local;
    struct actor *actor = halo::ai::actor_at(actor_index);
    ActorVariant *definition = halo::ai::tag_data<ActorVariant>(actor->actor_variant_tag);
    struct swarm *swarm = halo::ai::swarm_at(actor->swarm_index);
    int32_t picked = -1;
    int16_t member;

    if (swarm->component_pick_delay > 0) {
        swarm->component_pick_delay--;
    } else if (actor->mode == 7 || actor->mode == 10) {
        float delay = ((float)(int32_t)swarm_random_next() * 1.5259022e-05f);
        int16_t count = swarm->component_count;

        delay = (delay + delay + 6.0f) / (float)(int32_t)count * 30.0f;
        if (!(delay > 6.0f)) {
            delay = 6.0f;
        }
        swarm->component_pick_delay = (int16_t)(int32_t)delay;
        picked = (int32_t)(((uint32_t)(int32_t)count * swarm_random_next()) >> 16);
    }

    for (member = 0; member < swarm->component_count; member++) {
        datum_index unit = swarm->unit_index[member];
        biped_object *object = reinterpret_cast<biped_object *>(halo::ai::object_at(unit));
        swarm_component *component = halo::ai::swarm_component_at(swarm->component_index[member]);
        struct prop *best_prop = 0;
        datum_index best_handle = k_datum_index_none;
        datum_index target = k_datum_index_none;
        int16_t behaviour = 0;
        uint8_t speed = 3;
        uint8_t control_byte_1 = 1;
        uint8_t aligned = 0;
        uint8_t target_close = 0;
        uint8_t moving = 0;
        uint8_t airborne = 0;
        uint8_t leap = 0;
        real_vector3d up;
        real_vector3d desired;
        uint16_t flags;
        unit_control_data control;

        up = object->base.up;
        if (object->base.type == 0) {
            if (static_cast<uint32_t>(object->biped.ground_surface_index) != (uint32_t)k_datum_index_none) {
                up = object->biped.ground_normal;
            }
            airborne = has(static_cast<halo::units::biped_flag>(object->biped.flags), halo::units::biped_flag::airborne);
        }

        if (actor->combat_status >= 3) {
            float radius = definition->desired_combat_range[1];
            float best_score = 0.0f;
            float best_distance = 0.0f;
            datum_index prop_handle = actor->first_prop;

            while (prop_handle != k_datum_index_none) {
                struct prop *prop = halo::ai::prop_at(prop_handle);
                datum_index this_handle = prop_handle;

                prop_handle = prop->next_in_actor;
                if (prop->desirability > 0.0f) {
                    float dx = component->position.x - prop->last_known_position.x;
                    float dy = component->position.y - prop->last_known_position.y;
                    float dz = component->position.z - prop->last_known_position.z;
                    float distance = (float)sqrt((double)(dz * dz + dy * dy + dx * dx));
                    float score = 0.0f;

                    if (distance < radius) {
                        score = (1.0f - distance / radius) * 10.0f;
                    }
                    if (prop->state >= 2 && prop->state <= 3) {
                        score += (this_handle == component->leap_target_index) ? 7.0f : 5.0f;
                        if (prop->child_unit_count == 0) {
                            score += 5.0f;
                        }
                    }
                    if (score > best_score) {
                        best_score = score;
                        best_prop = prop;
                        best_handle = this_handle;
                        best_distance = distance;
                    }
                }
            }
            component->leap_target_index = best_handle;
            if (best_handle != k_datum_index_none && best_distance < definition->melee_range &&
                best_prop->state >= 2 && best_prop->state <= 3) {
                target_close = 1;
            }
        } else {
            component->leap_target_index = best_handle;
        }

        switch (actor->mode) {
        case 1:
            behaviour = 0;
            speed = 0;
            break;
        case 2:
            behaviour = 1;
            speed = 1;
            break;
        case 4:
            speed = (uint8_t)((actor->mode_data.flee.panic > 0) * 2 + 3);
            if (actor->mode_data.flee.reference != (uint32_t)k_datum_index_none) {
                behaviour = 5;
                target = actor->mode_data.flee.reference;
            }
            break;
        case 6:
            behaviour = 2;
            speed = 1;
            break;
        case 7:
            if (actor->mode_data.search.stage == 0 && actor->target_unit_index != (uint32_t)k_datum_index_none) {
                behaviour = 4;
                target = actor->target_unit_index;
            } else {
                behaviour = 3;
            }
            speed = 3;
            break;
        case 10:
        case 11:
            speed = 3;
            behaviour = 3;
            if (actor->mode == 11 && has(static_cast<swarm_component_flag>(component->flags), swarm_component_flag::active)) {
                behaviour = 6;
            } else if (component->leap_target_index != (uint32_t)k_datum_index_none) {
                behaviour = (int16_t)((component->infection.detach_delay != 0) + 4);
                control_byte_1 = 0;
                target = component->leap_target_index;
            }
            break;
        default:
            break;
        }

        if (object->base.parent_object == (uint32_t)k_datum_index_none) {
            component->infection.parent_ticks = 0;
            if (component->infection.detach_delay != 0) {
                component->infection.detach_delay--;
            }
        } else {
            unit_object *parent = reinterpret_cast<unit_object *>(halo::ai::object_at(object->base.parent_object));
            uint8_t parent_dead = has(static_cast<halo::objects::vitality_flag>(parent->base.vitality_flags), halo::objects::vitality_flag::health_frozen);
            uint8_t detach = 0;

            if (component->infection.parent_ticks != 0xff) {
                component->infection.parent_ticks++;
            }
            if (parent_dead) {
                if (static_cast<uint32_t>(parent->unit.death_time) != (uint32_t)k_datum_index_none &&
                    (int32_t)(static_cast<uint32_t>(parent->unit.death_time) + 0x4b) < halo::game::globals().game_time->game_time &&
                    best_prop != 0 && best_prop->object_index != object->base.parent_object &&
                    best_prop->state >= 2 && best_prop->state <= 3) {
                    detach = 1;
                }
            } else {
                Unit *parent_tag = halo::ai::tag_data<Unit>(parent->base.definition_tag);

                if ((parent->base.type != 0 || halo::ai::flag_set(parent_tag->unit_flags, halo::tags::unit_tag_flag::melee_attackers_cannot_attach)) &&
                    component->infection.parent_ticks > 0x2d) {
                    component->infection.detach_delay = 0x2d;
                    detach = 1;
                }
            }
            if (detach) {
                halo::units::unit_detach_reposition_and_nudge(unit);
                component->flags &= ~(uint16_t)(swarm_component_flag::attacking | swarm_component_flag::attached);
            } else {
                component->flags |= (uint16_t)swarm_component_flag::attached;
                if (parent_dead) {
                    component->flags &= ~(uint16_t)swarm_component_flag::attacking;
                } else {
                    component->flags |= (uint16_t)swarm_component_flag::attacking;
                }
            }
        }

        auto steer = [&]() {
            if (airborne) {
                component->flags &= ~(uint16_t)swarm_component_flag::attached;
                component->infection.free_ticks = 0;
                return;
            }
            if (component->infection.free_ticks != 0xff) {
                component->infection.free_ticks++;
            }
            component->flags &= ~(uint16_t)(swarm_component_flag::attacking | swarm_component_flag::attached);
            flags = component->flags;

            switch (behaviour) {
            case 1:
            case 2:
            case 3:
                if (!has(static_cast<swarm_component_flag>(flags), swarm_component_flag::initialized)) {
                    memset(&component->infection.turn_wait_ticks, 0, 0x14);
                    component->flags = (uint16_t)((flags & ~(uint16_t)swarm_component_flag::active) | (uint16_t)swarm_component_flag::initialized);
                }
                if (component->infection.turn_ticks != 0) {
                    component->infection.turn_ticks--;
                    if (component->infection.turn_ticks == 0) {
                        component->infection.turn_wait_ticks = (uint8_t)halo::ai::actor_pick_dialogue_variant_a(behaviour);
                    } else {
                        float damping = component->infection.turn_rate * -0.06666667f;
                        float angle = halo::math::random_real_range(-0.020943951f, 0.020943951f) + component->infection.turn_rate + damping;

                        component->infection.turn_rate = angle;
                        halo::math::vector3d_rotate_about_axis(component->infection.heading, up, (float)sin((double)angle),
                            (float)cos((double)angle));
                    }
                } else {
                    if (component->infection.turn_wait_ticks != 0) {
                        component->infection.turn_wait_ticks--;
                    }
                    if (component->infection.turn_wait_ticks == 0) {
                        real_vector3d to_goal;
                        float distance_squared;
                        float angle;

                        component->infection.turn_ticks = (uint8_t)halo::ai::actor_pick_dialogue_variant_b(behaviour);
                        to_goal.i = swarm->aggregate_position.x - component->position.x;
                        to_goal.j = swarm->aggregate_position.y - component->position.y;
                        to_goal.k = swarm->aggregate_position.z - component->position.z;
                        distance_squared = to_goal.k * to_goal.k + to_goal.j * to_goal.j + to_goal.i * to_goal.i;
                        if (!(distance_squared < 0.25f)) {
                            float spread = 0.5f / (float)sqrt((double)distance_squared) * 3.1415927f;

                            angle = halo::math::random_real_range(-spread, spread);
                            component->infection.heading = to_goal;
                        } else {
                            angle = halo::math::random_real_range(-3.1415927f, 3.1415927f);
                            component->infection.heading = object->base.forward;
                        }
                        halo::math::vector3d_rotate_about_axis(component->infection.heading, up, (float)sin((double)angle),
                            (float)cos((double)angle));
                        component->infection.turn_rate = 0.0f;
                    }
                }
                if (component->infection.turn_ticks == 0) {
                    return;
                }
                desired = component->infection.heading;
                moving = 1;
                break;
            case 4:
            case 5: {
                struct prop *prop = halo::ai::prop_at(target);

                desired.i = prop->last_known_position.x - component->position.x;
                desired.j = prop->last_known_position.y - component->position.y;
                desired.k = prop->last_known_position.z - component->position.z;
                if (behaviour == 5) {
                    desired.i = -desired.i;
                    desired.j = -desired.j;
                    desired.k = -desired.k;
                }
                moving = 1;
                break;
            }
            case 6: {
                uint8_t script_flags = component->action.movement_flags;

                if (script_flags & 1) {
                    int16_t kind = component->action.axis;
                    uint8_t negate;

                    moving = 1;
                    if (kind >= 2 && kind <= 3) {
                        halo::math::vector3d_cross_product(desired, component->action.direction, up);
                        negate = (uint8_t)(kind == 3);
                    } else {
                        desired = component->action.direction;
                        negate = (uint8_t)(kind == 1);
                    }
                    if (negate) {
                        desired.i = -desired.i;
                        desired.j = -desired.j;
                        desired.k = -desired.k;
                    }
                }
                if (script_flags & 4) {
                    if ((script_flags & 8) == 0 && component->action.axis == 0 && !halo::units::unit_is_in_busy_animation_state(unit)) {
                        component->flags = (uint16_t)(flags | (uint16_t)swarm_component_flag::leaping);
                        component->action.movement_flags = (uint8_t)(script_flags | 8);
                    }
                    desired = object->base.forward;
                    moving = 1;
                } else if (!moving) {
                    return;
                }
                break;
            }
            default:
                return;
            }

            {
                float length = (float)sqrt((double)(desired.k * desired.k + desired.j * desired.j + desired.i * desired.i));
                float along_up;

                if (!(fabs((double)length) < 0.0001)) {
                    float inverse = 1.0f / length;

                    desired.i *= inverse;
                    desired.j *= inverse;
                    desired.k *= inverse;
                }
                along_up = up.k * desired.k + up.j * desired.j + up.i * desired.i;
                if (along_up > 0.9f) {
                    aligned = 1;
                }
                if (along_up < -0.9f) {
                    desired = object->base.forward;
                } else {
                    real_vector3d side;

                    side.i = up.j * desired.k - up.k * desired.j;
                    side.j = up.k * desired.i - desired.k * up.i;
                    side.k = desired.j * up.i - up.j * desired.i;
                    desired.i = side.j * up.k - side.k * up.j;
                    desired.j = side.k * up.i - up.k * side.i;
                    desired.k = up.j * side.i - side.j * up.i;
                    if (halo::math::vector3d_normalize_with_length(desired) == 0.0f) {
                        desired = object->base.forward;
                    }
                }
            }

            if (behaviour != 6) {
                float turn = 0.0f;
                real_point3d behind;
                real_vector3d side;
                int16_t other;

                behind.x = component->position.x - desired.i * 0.2f;
                behind.y = component->position.y - desired.j * 0.2f;
                behind.z = component->position.z - desired.k * 0.2f;
                side.i = up.j * desired.k - up.k * desired.j;
                side.j = up.k * desired.i - desired.k * up.i;
                side.k = desired.j * up.i - up.j * desired.i;
                if (swarm->component_count > 0) {
                    for (other = 0; other < swarm->component_count; other++) {
                        swarm_component *other_component;
                        float dx, dy, dz, distance_squared, facing;

                        if (other == member) {
                            continue;
                        }
                        other_component = halo::ai::swarm_component_at(swarm->component_index[other]);
                        dx = other_component->position.x - behind.x;
                        dy = other_component->position.y - behind.y;
                        dz = other_component->position.z - behind.z;
                        distance_squared = dz * dz + dy * dy + dx * dx;
                        if (!(distance_squared < 0.64000005f)) {
                            continue;
                        }
                        facing = (dz * desired.k + dy * desired.j + dx * desired.i) / (float)sqrt((double)distance_squared);
                        if (!(facing > 0.5f)) {
                            continue;
                        }
                        if (side.k * dz + side.j * dy + side.i * dx > 0.0f) {
                            turn = turn - (facing - 0.5f) * 0.5f;
                        } else {
                            turn = turn + (facing - 0.5f) * 0.5f;
                        }
                    }
                    if (turn != 0.0f) {
                        if (!(turn <= 1.0f)) {
                            turn = 1.5707964f;
                        } else if (turn < -1.0f) {
                            turn = -1.5707964f;
                        } else {
                            turn = turn * 1.5707964f;
                        }
                        halo::math::vector3d_rotate_about_axis(desired, up, (float)sin((double)turn), (float)cos((double)turn));
                    }
                }
            }

            {
                real_vector3d side;
                float length;

                side.i = up.j * desired.k - up.k * desired.j;
                side.j = up.k * desired.i - desired.k * up.i;
                side.k = desired.j * up.i - up.j * desired.i;
                length = (float)sqrt((double)(side.k * side.k + side.j * side.j + side.i * side.i));
                if (!(fabs((double)length) < 0.0001) && length != 0.0f) {
                    float inverse = 1.0f / length;
                    float side_k = inverse * side.k;

                    side.i *= inverse;
                    side.j *= inverse;
                    desired.i = side.j * up.k - side_k * up.j;
                    desired.j = side_k * up.i - up.k * side.i;
                    desired.k = up.j * side.i - side.j * up.i;
                } else {
                    desired = object->base.forward;
                }
            }
        };

        if (object->base.parent_object == (uint32_t)k_datum_index_none) {
            steer();
        }

        flags = component->flags;
        if (has(static_cast<swarm_component_flag>(flags), swarm_component_flag::leaping)) {
            leap = 1;
        } else if (moving && component->infection.free_ticks >= 0x2d && (member == (int16_t)picked || target_close || aligned)) {
            leap = 1;
        }
        if (has(static_cast<swarm_component_flag>(flags), swarm_component_flag::attached)) {
            object->unit.melee_state = (int8_t)((flags & 1) << 2);
        } else {
            if (target_close && component->infection.detach_delay == 0) {
                flags |= (uint16_t)swarm_component_flag::attacking;
            } else {
                flags &= ~(uint16_t)swarm_component_flag::attacking;
            }
            component->flags = flags;
            if (flags & (uint16_t)swarm_component_flag::attacking) {
                object->unit.melee_state = 3;
                object->biped.melee_target_index = best_prop != 0 ? best_prop->object_index : (uint32_t)k_datum_index_none;
            } else {
                object->unit.melee_state = 0;
            }
        }

        memset(&control, 0, sizeof control);
        control.animation_state = (int8_t)speed;
        control.aiming_speed = (int8_t)control_byte_1;
        control.control_flags = (uint16_t)(leap ? halo::units::unit_control_flag::jump : halo::units::unit_control_flag::none);
        control.weapon_index = -1;
        control.grenade_index = -1;
        control.zoom_level = -1;
        if (moving) {
            control.throttle.i = 1.0f;
            control.throttle.j = 0.0f;
            control.throttle.k = 0.0f;
        } else {
            copy3(&control.throttle, global_origin3d_pointer);
            desired = object->base.forward;
        }
        control.facing_vector = desired;
        control.aiming_vector = desired;
        control.looking_vector = desired;
        halo::units::unit_apply_control_block(unit, &control, -1);
    }
}

#undef OBJECT
#undef COMPONENT
#undef F
#undef U16
#undef I16

namespace actor_type_infection_update_local {
}

/**
 * Actor AI behaviour: type infection update.
 *
 * @address 0x424980
 */
void ActorView::type_infection_update()
{
    using namespace actor_type_infection_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (!halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            halo::ai::actor_conditional_state_transition_check(actor_index);
        }
        break;
    case 4:
        if (act->mode_data.flee.engage != 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        } else {
            halo::ai::actor_flee_look_away(actor_index);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            halo::ai::actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        break;
    default:
        break;
    }
}


namespace actor_type_jackal_update_local {
}

/**
 * Actor AI behaviour: type jackal update.
 *
 * @address 0x425f70
 */
void ActorView::type_jackal_update()
{
    using namespace actor_type_jackal_update_local;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[act->actor_definition_tag & halo::k_slot_mask].data;

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 1);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_alert_from_disturbance(actor_index);
        halo::ai::actor_alert_from_squad_attack(actor_index);
        halo::ai::actor_alert_from_projectile(actor_index);
        halo::ai::actor_alert_from_flag_1b4(actor_index);
        halo::ai::actor_alert_from_damage(actor_index);
        halo::ai::actor_gate_jump_traversal(actor_index, 1, 0, 4);
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_try_grenade_evasion(actor_index, 1, 1);
        halo::ai::actor_seek_vehicle_to_board(actor_index);
        halo::ai::actor_process_vehicle_seat_exit(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        if (act->mode_data.guard.ambush_active && !act->mode_data.guard.ambush_triggered && !act->mode_data.guard.ambush_retreat) {
            float limit = act->combat_status >= 4 ? ((Actor *)actor_tag)->attack_shield_fraction : ((Actor *)actor_tag)->pursue_shield_fraction;

            if (limit > act->shield_vitality) {
                act->mode_data.guard.ambush_active = 1;
                act->mode_data.flee.panic = 30;
            } else {
                act->mode_data.guard.ambush_active = 0;
                act->mode_data.flee.panic = 0;
            }
        }
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (act->mode_data.flee.engage) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        halo::ai::actor_flee_look_away(actor_index);
        return;
    case 5:
    case 7:
    case 8:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        halo::ai::actor_update_special_mode(actor_index);
        return;
    case 9:
        return;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        return;
    case 12:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act->mode_data.converse.finished || act->conversation_index == k_datum_index_none));
        return;
    case 13:
        if (act->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}


namespace actor_type_marine_update_local {
}

/**
 * Actor AI behaviour: type marine update.
 *
 * @address 0x4261d0
 */
void ActorView::type_marine_update()
{
    using namespace actor_type_marine_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 1);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_alert_from_squad_attack(actor_index);
        halo::ai::actor_alert_from_projectile(actor_index);
        halo::ai::actor_alert_from_flag_1b4(actor_index);
        halo::ai::actor_alert_from_damage(actor_index);
        halo::ai::actor_gate_jump_traversal(actor_index, 1, 0, 14);
        halo::ai::actor_escalate_check_shield_damage(actor_index);
        halo::ai::actor_escalate_apply(actor_index, (int16_t)((int8_t)act->tally.group_a_by_actor_type[7] > 2 ? 5 : 3));
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_seek_vehicle_to_board(actor_index);
        halo::ai::actor_process_vehicle_seat_exit(actor_index);
        halo::ai::actor_update_grenade_throw_decision(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        return;
    case 6:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        return;
    case 4:
        if (act->mode_data.flee.engage) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
            return;
        }
        halo::ai::actor_flee_look_away(actor_index);
        return;
    case 5:
    case 7:
    case 8:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            return;
        }
        halo::ai::actor_update_special_mode(actor_index);
        return;
    case 9:
        if (act->mode_data.vehicle.unit_replaced) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        } else if (act->mode_data.vehicle.failed) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        return;
    case 12:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_command_list_permits_escalation(actor_index),
                                     (uint8_t)(act->mode_data.converse.finished || act->conversation_index == k_datum_index_none));
        return;
    case 13:
        if (act->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        return;
    default:
        return;
    }
}


namespace actor_type_mounted_weapon_update_local {
}

/**
 * Actor AI behaviour: type mounted weapon update.
 *
 * @address 0x4263f0
 */
void ActorView::type_mounted_weapon_update()
{
    using namespace actor_type_mounted_weapon_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    if (halo::ai::actor_wants_reload_or_swap(actor_index) == 0) {
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 4:
    case 6:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) == 0) {
            halo::ai::actor_conditional_state_transition_check(actor_index);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) == 0) {
            halo::ai::actor_update_special_mode(actor_index);
        }
        break;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        break;
    default:
        break;
    }
}


namespace actor_type_sentinel_update_local {
}

/**
 * Actor AI behaviour: type sentinel update.
 *
 * @address 0x4264d0
 */
void ActorView::type_sentinel_update()
{
    using namespace actor_type_sentinel_update_local;
    actor *act = halo::ai::actor_at(actor_index);

    if (act->mode == 0 && act->awareness_level != 0) {
        halo::ai::actor_process_order_request(actor_index, halo::k_word_none);
    }
    halo::ai::actor_process_pending_command_list(actor_index);
    halo::ai::actor_react_to_disturbance(actor_index, 1);
    if (!halo::ai::actor_wants_reload_or_swap(actor_index)) {
        halo::ai::actor_escalate_to_guard_or_combat(actor_index);
        halo::ai::actor_update_danger_avoidance(actor_index);
    }

    switch (act->mode) {
    case 3:
    case 10:
        if (halo::ai::actor_update_combat_behavior(actor_index, 1, 0) || halo::ai::actor_conditional_state_transition_check(actor_index)) {
            return;
        }
        halo::ai::actor_update_grenade_and_morale_reactions(actor_index);
        break;
    case 4:
        if (act->mode_data.flee.engage == 0) {
            halo::ai::actor_flee_look_away(actor_index);
        } else {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    case 5:
    case 7:
    case 8:
        if (!halo::ai::actor_update_combat_behavior(actor_index, 1, 0)) {
            halo::ai::actor_update_special_mode(actor_index);
        }
        break;
    case 6:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_combat_status_should_hold(actor_index, 3, 6), 0);
        break;
    case 11:
        halo::ai::actor_update_combat_behavior(actor_index, act->mode_data.obey.allow_initiative, act->mode_data.obey.finished);
        break;
    case 12:
        halo::ai::actor_update_combat_behavior(actor_index, halo::ai::actor_command_list_permits_escalation(actor_index),
            (uint8_t)(act->mode_data.converse.finished != 0 || act->conversation_index == k_datum_index_none));
        break;
    case 13:
        if (act->danger_type == 0) {
            halo::ai::actor_update_combat_behavior(actor_index, 1, 1);
        }
        break;
    default:
        break;
    }
}


}
