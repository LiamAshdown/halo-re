#include "halo/ai/actor_props.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"
#include "projectiles.h"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"

namespace c_actor_danger_update_reaction {




static uint8_t actor_danger_prop_seen_twice(datum_index actor_index, datum_index object_index)
{
    datum_index prop_index = halo::ai::actor_find_prop_for_object(object_index, actor_index);

    if (prop_index == k_datum_index_none) {
        return 0xff;
    }
    return halo::ai::prop_at(prop_index)->perception_level >= 2;
}

static uint8_t actor_danger_asleep(struct actor *actor)
{
    struct encounter *encounter = 0;

    if (actor->encounter_index != halo::k_dword_none) {
        encounter = halo::ai::encounter_at(actor->encounter_index);
    }
    return actor->awareness_level == 1 || (encounter != 0 && encounter->blind != 0);
}

static uint8_t actor_danger_stance(datum_index actor_index)
{
    struct actor *actor = halo::ai::actor_at(actor_index);

    return actor->combat_status >= 2 ? 2 : (actor->awareness_level >= 3);
}
}


/**
 * actor_danger_update_reaction: behaviour unchanged from the original routine.
 *
 * @address 0x41eda0
 */
void halo::ai::prop_ops::danger_update_reaction()
{
    using namespace c_actor_danger_update_reaction;
    datum_index actor_index = datum;
    struct actor *actor = halo::ai::actor_at(actor_index);
    object *object;
    actor_firing_positions block;
    real_point3d *position = &actor->flee_from_point;
    real_point3d *block_point = &block.body_position;
    uint8_t noticed = 0;
    uint8_t own = 0;

    if (actor->danger_type <= 0) {
        return;
    }
    object = (struct object *)halo::objects::object_try_and_get(actor->danger_object_index, halo::k_dword_none);
    if (object == 0) {
        actor->danger_type = 0;
        return;
    }
    halo::objects::object_get_position(position, actor->danger_object_index);
    halo::ai::actor_get_firing_positions(actor_index, &block, position);
    actor->danger_velocity = *(real_vector3d *)&object->velocity.i;
    {
        float dx = position->x - block_point->x;
        float dy = position->y - block_point->y;
        float dz = position->z - block_point->z;

        actor->danger_distance = (float)halo::libm::sqrt((double)(dz * dz + dy * dy + dx * dx));
    }
    actor->danger_segment_end.x = actor->danger_velocity.i * 45.0f + position->x;
    actor->danger_segment_end.y = actor->danger_velocity.j * 45.0f + position->y;
    actor->danger_segment_end.z = actor->danger_velocity.k * 45.0f + position->z;
    actor->danger_center.x = (actor->danger_segment_end.x + position->x) * 0.5f;
    actor->danger_center.y = (position->y + actor->danger_segment_end.y) * 0.5f;
    actor->danger_center.z = (position->z + actor->danger_segment_end.z) * 0.5f;
    {
        float dx = position->x - actor->danger_center.x;
        float dy = position->y - actor->danger_center.y;
        float dz = position->z - actor->danger_center.z;

        actor->danger_radius = (float)halo::libm::sqrt((double)(dz * dz + dy * dy + dx * dx)) + actor->danger_object_radius;
    }

    switch (actor->danger_type) {
    case 1: {
        int16_t state = 0;
        int32_t frames;

        noticed = actor->danger_reaction_delayed;
        if (!noticed) {
            uint8_t seen = actor_danger_prop_seen_twice(actor_index, actor->danger_object_index);

            if (seen != 0xff) {
                noticed = seen;
            }
        }
        frames = halo::units::unit_get_animation_frames_remaining(actor->danger_object_index, &state);
        actor->danger_countdown = state == 0x19 ? (int16_t)frames : -1;
        break;
    }
    case 2: {
        Projectile *tag;
        int16_t cluster;
        int16_t status;

        if (actor->unit_index != halo::k_dword_none && object->parent_object == actor->unit_index) {
            own = 1;
        }
        if (((projectile_object *)object)->projectile.detonation_timer > 0.0f && ((projectile_object *)object)->projectile.detonation_timer_rate > 0.0f) {
            actor->danger_countdown = (int16_t)halo::x87::fistp_round((1.0f - ((projectile_object *)object)->projectile.detonation_timer) / ((projectile_object *)object)->projectile.detonation_timer_rate);
        } else {
            actor->danger_countdown = -1;
        }
        if (actor->danger_reaction_delayed != 0 || own) {
            noticed = 1;
            break;
        }
        if (actor_danger_asleep(actor)) {
            break;
        }
        tag = halo::ai::tag_data<Projectile>(object->definition_tag);
        if (!(actor->danger_distance < tag->ai_perception_radius)) {
            break;
        }
        cluster = object->location_cluster_index;
        if (object->parent_object != halo::k_dword_none) {
            unit_object *root = (unit_object *)halo::ai::object_at(halo::objects::object_get_root_object_index(actor->danger_object_index));

            cluster = ((struct object *)root)->location_cluster_index;
        }
        status = (int16_t)halo::ai::actor_evaluate_engagement_reachability(block.location.cluster_index, cluster,
            position, &block.aim_origin, 0, 0, actor->danger_object_index, actor->active_unit_index != halo::k_dword_none);
        actor = halo::ai::actor_at(actor_index);
        if (halo::ai::actor_dispatch_look_handler_by_posture(status, actor_index, &block, position, 0, 1,
                actor_danger_stance(actor_index)) >= 2) {
            noticed = 1;
        }
        break;
    }
    case 3: {
        Unit *tag = halo::ai::tag_data<Unit>(object->definition_tag);
        float vi = object->velocity.i;
        float vj = object->velocity.j;
        float vk = object->velocity.k;
        uint8_t asleep;
        bsp_leaf_reference *location;
        int16_t status;

        if (vi * vi + vj * vj + vk * vk < 4.4444445e-05f || tag->base.bounding_radius + 10.0f < actor->danger_distance) {
            actor->danger_type = 0;
            break;
        }
        noticed = actor->danger_reaction_delayed;
        if (noticed) {
            break;
        }
        if (halo::units::unit_data_of(object)->driver_unit_index != halo::k_dword_none) {
            uint8_t seen = actor_danger_prop_seen_twice(actor_index, halo::units::unit_data_of(object)->driver_unit_index);

            if (seen != 0xff) {
                noticed = seen;
                break;
            }
        }
        asleep = actor_danger_asleep(actor);
        location = halo::ai::object_location((struct object *)object);
        if (object->parent_object != halo::k_dword_none) {
            location = halo::ai::object_location(halo::ai::object_at(halo::objects::object_get_root_object_index(actor->danger_object_index)));
        }
        status = (int16_t)halo::ai::actor_evaluate_engagement_reachability(block.location.cluster_index,
            location->cluster_index, position, &block.aim_origin, 0, 0, actor->danger_object_index, actor->active_unit_index != halo::k_dword_none);
        if (!asleep &&
            halo::ai::actor_dispatch_look_handler_by_posture(status, actor_index, &block, position, 0, 1,
                actor_danger_stance(actor_index)) >= 2) {
            noticed = 1;
            break;
        }
        if ((int16_t)halo::ai::actor_target_hearing_check(location, status, actor_index, &block, (int16_t)tag->constant_sound_volume,
                position) >= 2) {
            noticed = 1;
        }
        break;
    }
    default:
        break;
    }

    actor = halo::ai::actor_at(actor_index);
    if (noticed && actor->danger_reaction_delayed == 0) {
        uint8_t payload[0x10];

        memset(payload, 0, sizeof(payload));
        *(int16_t *)payload = 5;
        halo::ai::actor_begin_vocalization(actor_index, 0xc, 1, (actor_vocalization_context *)payload);
    }
    actor->danger_reaction_delayed = noticed;
    actor->danger_is_own = own;
}

namespace halo::ai {
void actor_danger_update_reaction(datum_index actor_index)
{
    halo::ai::prop_ops(actor_index).danger_update_reaction();
}
}


