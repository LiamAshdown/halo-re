#include "halo/ai/actor_combat.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/core/libm.hpp"

namespace c_actor_evaluate_engagement_reachability {
static auto &global_down3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_down3d_pointer);
}


/**
 * actor_evaluate_engagement_reachability: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_evaluate_engagement_reachability.c.txt.
 *
 * @address 0x42b270
 */
int32_t halo::ai::combat_ops::evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster, real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask, datum_index exclude_object_index, uint8_t flying)
{
    using namespace c_actor_evaluate_engagement_reachability;
    uint8_t result[0x50];
    uint32_t mask;
    real_vector3d delta;
    real_vector3d side;
    real_point3d a;
    real_point3d b;
    uint8_t direct_clear;
    float fraction = 0.0f;

    if (self_cluster != -1 && target_cluster != -1 && !halo::scenario::scenario_query::cluster_visibility_test(self_cluster, target_cluster)) {
        return 4;
    }
    mask = allow_wide_mask ? 0xc2b3 : 0xc2a7;
    if (flying) {
        mask &= 0xfffffdff;
    }
    delta.i = target_position->x - self_position->x;
    delta.j = target_position->y - self_position->y;
    delta.k = target_position->z - self_position->z;
    if (!halo::physics::collision_test_movement_segment(mask, self_position, &delta, exclude_object_index, (collision_result *)result)) {
        direct_clear = 1;
    } else {
        direct_clear = 0;
        fraction = *(float *)(result + 0x14);
    }

    if (movement_mode != 0) {
        side.i = self_position->y - target_position->y;
        side.j = target_position->x - self_position->x;
        side.k = 0.0f;
        if (halo::math::vector3d_normalize_with_length(side) == 0.0f) {
            side = *halo::math::globals().global_forward3d_pointer;
        }
        if (movement_mode == 1) {
            real_vector3d offset;

            offset.i = side.i * 0.25f;
            offset.j = side.j * 0.25f;
            offset.k = side.k * 0.25f;
            a.x = offset.i + self_position->x;
            a.y = offset.j + self_position->y;
            a.z = offset.k + self_position->z;
            b.x = self_position->x - offset.i;
            b.y = self_position->y - offset.j;
            b.z = self_position->z - offset.k;
            if (direct_clear) {
                if (halo::physics::collision_test_movement_segment_between_points(&a, target_position, mask, exclude_object_index, (collision_result *)result) ||
                    halo::physics::collision_test_movement_segment_between_points(&b, target_position, mask, exclude_object_index, (collision_result *)result)) {
                    return 1;
                }
                return 0;
            }
            if (!halo::physics::collision_test_movement_segment_between_points(&a, target_position, mask, exclude_object_index, (collision_result *)result) ||
                !halo::physics::collision_test_movement_segment_between_points(&b, target_position, mask, exclude_object_index, (collision_result *)result)) {
                return 1;
            }
        } else if (direct_clear) {
            real_vector3d offset;
            real_point3d raised;

            offset.i = side.i * 0.1f;
            offset.j = side.j * 0.1f;
            offset.k = side.k * 0.1f;
            a.x = offset.i + target_position->x;
            a.y = offset.j + target_position->y;
            a.z = offset.k + target_position->z;
            b.x = target_position->x - offset.i;
            b.y = target_position->y - offset.j;
            b.z = target_position->z - offset.k;
            halo::math::point3d_add_scaled(raised, *(real_vector3d *)global_down3d_pointer, *target_position, 0.1f);
            if (halo::physics::collision_test_movement_segment_between_points(&a, self_position, mask, exclude_object_index, (collision_result *)result) ||
                halo::physics::collision_test_movement_segment_between_points(&b, self_position, mask, exclude_object_index, (collision_result *)result) ||
                halo::physics::collision_test_movement_segment_between_points(&raised, self_position, mask, exclude_object_index, (collision_result *)result)) {
                return 1;
            }
            return 0;
        }
    } else if (direct_clear) {
        return 0;
    }

    {
        float dx = target_position->x - self_position->x;
        float dy = target_position->y - self_position->y;
        float dz = target_position->z - self_position->z;
        float distance = (float)halo::libm::sqrt((double)(dz * dz + dy * dy + dx * dx));

        if (distance < 1.0f) {
            return 4;
        }
        if (distance * fraction < 1.0f) {
            return 2;
        }
        if ((1.0f - fraction) * distance < 4.0f) {
            return 3;
        }
        return 4;
    }
}

namespace halo::ai {
int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster, real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask, datum_index exclude_object_index, uint8_t flying)
{
    return halo::ai::combat_ops::evaluate_engagement_reachability(self_cluster, target_cluster, target_position, self_position, movement_mode, allow_wide_mask, exclude_object_index, flying);
}
}

namespace c_actor_get_threat_weapon_object_index {
}


/**
 * actor_get_threat_weapon_object_index: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_threat_weapon_object_index.c.txt.
 *
 * @address 0x4282c0
 */
datum_index halo::ai::combat_ops::get_threat_weapon_object_index()
{
    using namespace c_actor_get_threat_weapon_object_index;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    datum_index result = (datum_index)k_datum_index_none;

    if (self->vehicle_gunner != 0 && self->active_unit_index != (datum_index)k_datum_index_none) {
        object *unit_object = ((object_header *)halo::objects::globals().object_data->data)[self->active_unit_index & halo::k_slot_mask].data;
        int16_t slot = *(int16_t *)((uint8_t *)unit_object + 0x2f2);

        result = (datum_index)k_datum_index_none;
        if (slot != -1) {
            result = *(datum_index *)((uint8_t *)unit_object + 0x2f8 + slot * 4);
        }
        if (result != (datum_index)k_datum_index_none) {
            return result;
        }
    }

    if (self->unit_index != (datum_index)k_datum_index_none) {
        uint8_t *variant_tag = (uint8_t *)halo::cache::globals().tag_instances[self->actor_variant_tag & halo::k_slot_mask].data;
        if ((*variant_tag & 0x40) == 0) {

            object *own_unit = ((object_header *)halo::objects::globals().object_data->data)[self->unit_index & halo::k_slot_mask].data;
            return halo::units::unit_get_weapon_object_index(self->unit_index, *(int16_t *)((uint8_t *)own_unit + 0x2f2));
        }
    }
    return result;
}

namespace halo::ai {
datum_index actor_get_threat_weapon_object_index(datum_index actor_index)
{
    return halo::ai::combat_ops(actor_index).get_threat_weapon_object_index();
}
}

namespace c_actor_has_unshielded_threat_weapon {
}


/**
 * actor_has_unshielded_threat_weapon: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_has_unshielded_threat_weapon.c.txt.
 *
 * @address 0x428370
 */
uint8_t halo::ai::combat_ops::has_unshielded_threat_weapon()
{
    using namespace c_actor_has_unshielded_threat_weapon;
    datum_index actor_index = datum;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    uint8_t has_weapon = halo::ai::actor_get_threat_weapon_object_index(actor_index) != (datum_index)k_datum_index_none;

    if (has_weapon && self->unit_index != (datum_index)k_datum_index_none) {
        object *unit_object = ((object_header *)halo::objects::globals().object_data->data)[self->unit_index & halo::k_slot_mask].data;
        if ((*((uint8_t *)unit_object + 0x107) & 1) != 0) {
            has_weapon = 0;
        }
    }
    return has_weapon;
}

namespace halo::ai {
uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index)
{
    return halo::ai::combat_ops(actor_index).has_unshielded_threat_weapon();
}
}

namespace c_actor_issue_multi_target_vocalization {
}


/**
 * actor_issue_multi_target_vocalization: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_issue_multi_target_vocalization.c.txt.
 *
 * @address 0x4303a0
 */
void halo::ai::combat_ops::issue_multi_target_vocalization(int16_t line, datum_index actor_index, int16_t variant, datum_index vehicle_object_index)
{
    using namespace c_actor_issue_multi_target_vocalization;
    void *obj;
    actor_vocalization_context context;

    if (actor_index != (datum_index)k_datum_index_none && 0 < variant &&
        vehicle_object_index != (datum_index)k_datum_index_none) {
        obj = halo::objects::object_try_and_get(vehicle_object_index, -1);
        if (obj != 0) {
            context.code = 0;
            context.payload.handle = (datum_index)k_datum_index_none;
            context.payload.point.y = 0.0f;
            context.payload.point.z = 0.0f;
            halo::ai::actor_begin_vocalization(actor_index, line, variant, &context);
        }
    }
}

namespace halo::ai {
void actor_issue_multi_target_vocalization(int16_t line, datum_index actor_index, int16_t variant, datum_index vehicle_object_index)
{
    halo::ai::combat_ops::issue_multi_target_vocalization(line, actor_index, variant, vehicle_object_index);
}
}

