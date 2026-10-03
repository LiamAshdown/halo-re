#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"

namespace halo::ai {

namespace actor_react_to_disturbance_local {
#define ACTOR(index) ((uint8_t *)halo::ai::globals().actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (((uint8_t *)actor)[(o)])
#define W(o) (*(int16_t *)((uint8_t *)actor + (o)))
#define D(o) (*(uint32_t *)((uint8_t *)actor + (o)))
#define F(o) (*(float *)((uint8_t *)actor + (o)))
}

/**
 * Actor AI behaviour: react to disturbance.
 *
 * @address 0x40a1e0
 */
uint8_t ActorView::react_to_disturbance(int16_t threshold)
{
    using namespace actor_react_to_disturbance_local;
    struct actor *actor = halo::ai::actor_at(actor_index);
    ActorVariant *definition = halo::ai::tag_data<ActorVariant>(actor->actor_variant_tag);
    real_vector2d direction;
    int16_t action = 4;
    datum_index object = k_datum_index_none;
    int32_t reason = 0;

    if (actor->order_committed != 0 || actor->look_at_priority < threshold) {
        actor->look_at_priority = 0;
        return 0;
    }
    if (actor->look_at_has_point != 0) {
        direction.i = actor->look_at_point.x;
        direction.j = actor->look_at_point.y;
        halo::math::vector2d_normalize_with_length(direction);
        if (direction.j * actor->desired_facing_vector.y + direction.i * actor->desired_facing_vector.x < 0.0f) {
            direction.i = -direction.i;
            direction.j = -direction.j;
            action = 5;
        }
    } else {
        direction.i = actor->facing.i;
        direction.j = actor->facing.j;
        halo::math::vector2d_normalize_with_length(direction);
    }
    halo::ai::actor_queue_secondary_action(actor_index, action, &direction);
    if (actor->look_at_reference != halo::k_dword_none) {
        struct prop *prop = halo::ai::prop_at(actor->look_at_reference);

        object = prop->object_index;
        reason = (prop->enemy != 0) + 2;
    }
    halo::ai::ai_communication_broadcast(0x29, actor->unit_index, object, reason, halo::k_dword_none, halo::k_dword_none, 0);
    if (definition->surprise_fire_wildly_time > 0.0f) {
        actor->firing_state = 4;
        actor->firing_state_timer = (int16_t)(int32_t)(definition->surprise_fire_wildly_time * 30.0f);
    }
    if (definition->surprise_delay_time > 0.0f) {
        halo::ai::actor_raise_timer_5f6(actor_index, (int32_t)(definition->surprise_delay_time * 30.0f));
    }
    actor->unknown_2f0[0] = 1;
    if (actor->look_at_reference != halo::k_dword_none) {
        halo::ai::actor_consider_target_candidate(actor_index, actor->look_at_reference);
    }
    actor->look_at_priority = 0;
    return 1;
}


namespace actor_react_to_threat_event_local {
}

/**
 * extra_param, suppress_vehicle_relay Resolves other_object_index to a vehicle occupant (its gunner unless
 * event_kind == 9, else its driver, falling back to other_object_index itself) to get a relationship object,
 * then relays a pain/notice tick to self_object_index's controlling actor (via actor_mark_
 *
 * @address 0x42be40
 */
void ActorOps::react_to_threat_event(datum_index self_object_index, datum_index other_object_index, int32_t event_kind, real magnitude, uint32_t extra_param, uint8_t suppress_vehicle_relay)
{
    using namespace actor_react_to_threat_event_local;
    object *self_obj;
    object *relationship_obj;
    datum_index relationship_object_index;
    object *vehicle_obj;
    unit_data *vehicle_unit;
    datum_index actor_object_index;
    int32_t reason;
    int32_t event_code;

    self_obj = halo::ai::object_at(self_object_index);
    relationship_object_index = (datum_index)k_datum_index_none;
    relationship_obj = 0;

    if (other_object_index != (datum_index)k_datum_index_none) {
        vehicle_obj = (object *)halo::objects::object_try_and_get(other_object_index, 3);
        if (vehicle_obj != 0) {
            vehicle_unit = (unit_data *)((uint8_t *)vehicle_obj + k_unit_data_offset);
            relationship_object_index = (datum_index)k_datum_index_none;
            if ((int16_t)event_kind != 9) {
                relationship_object_index = vehicle_unit->gunner_unit_index;
            }
            if (relationship_object_index == (datum_index)k_datum_index_none) {
                relationship_object_index = vehicle_unit->driver_unit_index;
                if (relationship_object_index == (datum_index)k_datum_index_none) {
                    relationship_object_index = other_object_index;
                }
                if (relationship_object_index == (datum_index)k_datum_index_none) {
                    goto no_relationship_object;
                }
            }
            relationship_obj = halo::ai::object_at(relationship_object_index);
        }
    }
no_relationship_object:

    if (suppress_vehicle_relay == 0 && (int16_t)event_kind != 1) {
        actor_object_index = ((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->actor_index;
        if (actor_object_index != (datum_index)k_datum_index_none) {
            halo::ai::actor_mark_prop_seen_with_delta(relationship_object_index, actor_object_index, magnitude,
                (const real_vector3d *)extra_param);
        }
    }

    reason = 0;
    if (self_object_index == relationship_object_index) {
        reason = 1;
    } else if (relationship_obj != 0) {
        reason = (halo::game::teams_are_enemies(relationship_obj->owner_team, self_obj->owner_team) != 0) + 2;
    }

    if (suppress_vehicle_relay == 0 && reason == 2) {
        reason = 2;
        event_code = 3;
    } else {
        if (magnitude < 0.3f) {
            goto skip_broadcast;
        }
        event_code = 2;
    }
    halo::ai::ai_communication_broadcast(event_code, self_object_index, relationship_object_index, reason,
                                event_kind, (datum_index)k_datum_index_none, 0);
skip_broadcast:
    if (relationship_obj != 0) {
        halo::game::team_pair_override_refresh(self_obj->owner_team,
                                   relationship_obj->owner_team);
    }
}

}
