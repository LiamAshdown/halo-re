#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

namespace halo::ai {

namespace actor_react_to_disturbance_local {
extern "C" {
extern data_array *actor_data;
#define ACTOR(index) ((uint8_t *)actor_data->data + ((index) & halo::k_slot_mask) * k_actor_size)
#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))
extern data_array *prop_data;
extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, uint32_t payload[2]);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern void actor_raise_timer_5f6(datum_index actor_index, int32_t ticks);
extern uint16_t actor_consider_target_candidate(datum_index actor_index, datum_index candidate_prop_index);
extern int32_t __ftol(void);
}
}

/**
 * Actor AI behaviour: react to disturbance.
 *
 * @address 0x40a1e0
 */
uint8_t ActorView::react_to_disturbance(int16_t threshold)
{
    using namespace actor_react_to_disturbance_local;
    uint8_t *actor = ACTOR(actor_index);
    uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[D(0x5c) & halo::k_slot_mask].data;
    real_vector2d direction;
    int16_t action = 4;
    datum_index object = k_datum_index_none;
    int32_t reason = 0;

    if (B(0x160) != 0 || W(0x2ee) < threshold) {
        W(0x2ee) = 0;
        return 0;
    }
    if (B(0x2f8) != 0) {
        direction.i = F(0x2fc);
        direction.j = F(0x300);
        halo::math::vector2d_normalize_with_length(direction);
        if (direction.j * F(0x5a8) + direction.i * F(0x5a4) < 0.0f) {
            direction.i = -direction.i;
            direction.j = -direction.j;
            action = 5;
        }
    } else {
        direction.i = F(0x174);
        direction.j = F(0x178);
        halo::math::vector2d_normalize_with_length(direction);
    }
    actor_queue_secondary_action(actor_index, action, (uint32_t *)&direction);
    if (D(0x2f4) != halo::k_dword_none) {
        uint8_t *prop = (uint8_t *)prop_data->data + (D(0x2f4) & halo::k_slot_mask) * k_prop_size;

        object = ((struct prop *)prop)->object_index;
        reason = (prop[0x60] != 0) + 2;
    }
    ai_communication_broadcast(0x29, D(0x18), object, reason, halo::k_dword_none, halo::k_dword_none, 0);
    if (*(float *)(definition + 0x90) > 0.0f) {
        W(0x5f2) = 4;
        W(0x5f4) = (int16_t)(int32_t)(*(float *)(definition + 0x90) * 30.0f);
    }
    if (*(float *)(definition + 0x8c) > 0.0f) {
        actor_raise_timer_5f6(actor_index, (int32_t)(*(float *)(definition + 0x8c) * 30.0f));
    }
    B(0x2f0) = 1;
    if (D(0x2f4) != halo::k_dword_none) {
        actor_consider_target_candidate(actor_index, D(0x2f4));
    }
    W(0x2ee) = 0;
    return 1;
}

#undef ACTOR
#undef B
#undef W
#undef D
#undef F

namespace actor_react_to_threat_event_local {
extern "C" {
extern data_array *object_data;
extern void *object_try_and_get(datum_index object_index, int32_t kind);
extern void actor_mark_prop_seen_with_delta(datum_index object_index, datum_index actor_index, float delta,
    const real_vector3d *direction);
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern void team_pair_override_refresh(int16_t index_b, int16_t index_a);
}
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

    self_obj = ((object_header *)object_data->data)[self_object_index & halo::k_slot_mask].data;
    relationship_object_index = (datum_index)k_datum_index_none;
    relationship_obj = 0;

    if (other_object_index != (datum_index)k_datum_index_none) {
        vehicle_obj = (object *)object_try_and_get(other_object_index, 3);
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
            relationship_obj = ((object_header *)object_data->data)[relationship_object_index & halo::k_slot_mask].data;
        }
    }
no_relationship_object:

    if (suppress_vehicle_relay == 0 && (int16_t)event_kind != 1) {
        actor_object_index = ((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->actor_index;
        if (actor_object_index != (datum_index)k_datum_index_none) {
            actor_mark_prop_seen_with_delta(relationship_object_index, actor_object_index, magnitude,
                (const real_vector3d *)extra_param);
        }
    }

    reason = 0;
    if (self_object_index == relationship_object_index) {
        reason = 1;
    } else if (relationship_obj != 0) {
        reason = (teams_are_enemies(((struct object *)relationship_obj)->owner_team, ((struct object *)self_obj)->owner_team) != 0) + 2;
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
    ai_communication_broadcast(event_code, self_object_index, relationship_object_index, reason,
                                event_kind, (datum_index)k_datum_index_none, 0);
skip_broadcast:
    if (relationship_obj != 0) {
        team_pair_override_refresh(((struct object *)self_obj)->owner_team,
                                   ((struct object *)relationship_obj)->owner_team);
    }
}

}
