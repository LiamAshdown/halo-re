#include "halo/objects/flags.hpp"
#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"

namespace halo::ai {

namespace actor_resolve_flee_source_point_local {
}

/**
 * Fills *out with a direction (or, for a handful of reason codes, an absolute point copied verbatim) built from
 * one of seven sources selected by reason->code, and returns whether the result is usable: reason codes 2 (only
 * when it reads the direct facing/lean vectors) and 4 return true unconditionally
 *
 * @address 0x4146c0
 */
uint8_t ActorOps::resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out, datum_index actor_index)
{
    using namespace actor_resolve_flee_source_point_local;
    actor *self;
    prop *target_prop;
    real_point3d *target_point;
    object *target_object;
    float length;

    self = halo::ai::actor_at(actor_index);

    switch (reason->code) {
    case 0:
        if (self->moving == 0) {
            return 0;
        }
        *out = *(real_vector3d *)&self->desired_movement_vector;
        break;

    case 1: {
        target_prop = (prop *)halo::memory::datum_get(reason->payload.handle, halo::ai::globals().prop_data);
        if (target_prop == 0) {
            return 0;
        }
        target_point = &target_prop->head_position;
        out->i = target_point->x - self->aim_origin.x;
        out->j = target_point->y - self->aim_origin.y;
        out->k = target_point->z - self->aim_origin.z;
        break;
    }

    case 2:
        if (self->firing_state == 2) {
            *out = self->firing_vector;
            return 1;
        }
        if (self->target_in_firing_range != 0) {
            *out = *(real_vector3d *)&self->target_aim_vector;
            return 1;
        }
        if (self->target_unit_index == (datum_index)k_datum_index_none) {
            return 0;
        }
        target_prop = &((prop *)halo::ai::globals().prop_data->data)[self->target_unit_index & halo::k_slot_mask];
        out->i = target_prop->center_of_mass.x - self->aim_origin.x;
        out->j = target_prop->center_of_mass.y - self->aim_origin.y;
        out->k = target_prop->center_of_mass.z - self->aim_origin.z;
        break;

    case 3:
        out->i = reason->payload.point.x - self->aim_origin.x;
        out->j = reason->payload.point.y - self->aim_origin.y;
        out->k = reason->payload.point.z - self->aim_origin.z;
        break;

    case 4:
        *out = *(real_vector3d *)&reason->payload.point;
        return 1;

    case 5:
        if (self->danger_type < 1) {
            return 0;
        }
        out->i = self->flee_from_point.x - self->aim_origin.x;
        out->j = self->flee_from_point.y - self->aim_origin.y;
        out->k = self->flee_from_point.z - self->aim_origin.z;
        break;

    case 6: {
        real_point3d source_position;

        target_object = (object *)halo::objects::object_try_and_get(reason->payload.handle, halo::k_dword_none);
        if (target_object == 0) {
            return 0;
        }
        if ((halo::objects::object_type_mask_of(target_object->type) & _object_mask_unit) != 0) {
            halo::units::unit_get_primary_eye_marker_position(reason->payload.handle, &source_position);
        } else {
            halo::objects::object_get_position(&source_position, reason->payload.handle);
        }
        out->i = source_position.x - self->aim_origin.x;
        out->j = source_position.y - self->aim_origin.y;
        out->k = source_position.z - self->aim_origin.z;
        break;
    }

    default:
        return 0;
    }

    length = halo::math::vector3d_normalize_with_length(*out);
    return (0.0f < length) ? 1 : 0;
}

}
