#include "halo/items/items.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"

extern "C" {
extern data_array *object_data;
extern tag_instance *tag_instances;
extern game_engine_definition *current_game_engine;
extern uint8_t *global_structure_collision_bsp;
extern void item_detonation_timer_start(uint32_t object_index);
extern void item_compute_rotation(uint32_t object_index);
extern void object_list_membership_set(uint32_t object_index, char add);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index, bsp_leaf_reference *location);
extern void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index);
extern void random_get_table_point(real_vector3d *out);
extern double sqrt(double x);
extern void object_recompute_basis_from_marker_delta(object *obj, object_marker *marker, real_matrix4x3 *output_matrix);
extern double fsin(double x);
extern double fcos(double x);
extern data_array *player_data;
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer);
void item_align_to_normal_and_point(real_point3d *out_position, uint32_t item_index, real_vector3d *normal, real_point3d *point);
uint8_t item_get_effective_position(datum_index object_index, real_point3d *out_position);
}

namespace halo::items {

/**
 * Applies a translational impulse to an item, adding it into velocity, waking it from a resting
 * surface (snapping it back to a legal clearance above that surface first) when the impulse is
 * non-trivial, and inducing a matching angular jitter or wobble.
 *
 * @address 0x4bd080
 */
void item_ref::accelerate(real_vector3d *delta, uint8_t apply_detonation_timer)
{
    uint32_t item_index = datum;
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if ((item->flags & _item_does_not_accelerate_bit) != 0) {
        return;
    }
    if (obj->parent_object != (datum_index)0xffffffff) {
        return;
    }

    if (apply_detonation_timer != 0 && current_game_engine == 0) {
        Item *tag = (Item *)tag_instances[obj->definition_tag & 0xffff].data;
        if ((tag->item_flags & 0x02) != 0) {
            item_detonation_timer_start(item_index);
        }
    }

    if ((item->flags & _item_at_rest_on_structure_bit) == 0) {
        obj->flags &= ~(uint32_t)_object_at_rest_bit;
    } else if (0.0001f <= delta->i * delta->i + delta->j * delta->j + delta->k * delta->k) {
        object_marker marker;
        if (object_get_node_local_transform(item_index, (char *)"ground point", &marker, 1) != 0) {
            real_plane3d plane;
            int32_t surface_plane_ref = *(int32_t *)((uint8_t *)((ModelCollisionGeometryBSP *)global_structure_collision_bsp)->surfaces.pointer
                + (int32_t)(int16_t)item->resting_surface_index * 0x0c);
            real_point3d marker_position = marker.node_transform.position;
            real correction;
            real_point3d corrected_position;

            structure_bsp_plane_fetch_signed(&plane, global_structure_collision_bsp, surface_plane_ref);
            correction = 0.05f - ((plane.normal.i * marker_position.x +
                plane.normal.j * marker_position.y + plane.normal.k * marker_position.z) - plane.d);
            corrected_position.x = plane.normal.i * correction + marker_position.x;
            corrected_position.y = plane.normal.j * correction + marker_position.y;
            corrected_position.z = plane.normal.k * correction + marker_position.z;

            object_set_position_and_relink(&corrected_position, item_index, 0);
        }
        obj->flags &= ~(uint32_t)_object_at_rest_bit;
        item->flags &= ~(uint32_t)_item_at_rest_on_structure_bit;
    }

    obj->velocity.i += delta->i;
    obj->velocity.j += delta->j;
    obj->velocity.k += delta->k;

    if (item->ignore_object_index != (datum_index)0xffffffff ||
        (item->flags & _item_at_rest_on_structure_bit) == 0 ||
        0.0001f <= delta->i * delta->i + delta->j * delta->j + delta->k * delta->k) {
        real_vector3d cross_axis;
        real magnitude = (real)sqrt((double)(delta->i * delta->i + delta->j * delta->j + delta->k * delta->k));
        uint32_t seed_snapshot;
        real length;
        real angle;

        if (magnitude < 0.0001f) {
            halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
            magnitude = (real)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f;
        }
        seed_snapshot = halo::math::globals().random_seed_global;

        halo::math::vector3d_cross_product(cross_axis, *delta, *halo::math::globals().global_up3d_pointer);
        length = halo::math::vector3d_normalize_with_length(cross_axis);
        if (length <= 0.0f) {
            random_get_table_point(&cross_axis);
            seed_snapshot = halo::math::globals().random_seed_global;
        }

        halo::math::globals().random_seed_global = seed_snapshot * 0x19660d + 0x3c6ef35f;
        angle = (real)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f * magnitude * 1.5707964f;
        obj->angular_velocity.i += cross_axis.i * angle;
        obj->angular_velocity.j += cross_axis.j * angle;
        obj->angular_velocity.k += cross_axis.k * angle;
    } else {
        object_marker marker;
        real_vector3d axis;
        real angle;

        if (object_get_node_local_transform(item_index, (char *)"ground point", &marker, 1) != 0) {
            axis = marker.node_transform.up;
        } else {
            axis = *halo::math::globals().global_up3d_pointer;
        }
        angle = halo::math::random_real_range(-1.5707964f, 1.5707964f);
        obj->angular_velocity.i += axis.i * angle;
        obj->angular_velocity.j += axis.j * angle;
        obj->angular_velocity.k += axis.k * angle;
    }

    item_compute_rotation(item_index);
    object_list_membership_set(item_index, 0);
}

/**
 * Builds a rotation that tilts an item's "ground point" marker basis so its up vector matches
 * `normal`, positions the result at `point` (defaulting to the marker's own node-space
 * position), applies it to the item's basis, and returns the item's resulting world position.
 *
 * @address 0x4bd5d0
 */
void item_ref::align_to_normal_and_point(real_point3d *out_position, real_vector3d *normal, real_point3d *point)
{
    uint32_t item_index = datum;
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    object_marker marker;
    real_vector3d rotated_forward;
    real_matrix4x3 basis;
    real_vector3d *up;
    real_vector3d *forward;
    real dot;
    real s;
    real_point3d discard;

    if (object_get_node_local_transform(item_index, (char *)"ground point", &marker, 1) == 0) {
        return;
    }

    if (point == 0) {
        point = &marker.node_transform.position;
    }
    if (out_position == 0) {
        out_position = &discard;
    }

    up = &marker.node_transform.up;
    forward = &marker.node_transform.forward;
    dot = up->i * normal->i + up->j * normal->j + up->k * normal->k;
    s = (real)sqrt((double)(2.0f * (dot + 1.0f)));

    if (s <= 0.01f) {
        real_vector3d cross1;
        halo::math::vector3d_cross_product(cross1, *forward, *normal);
        halo::math::vector3d_cross_product(rotated_forward, *normal, cross1);
        halo::math::vector3d_normalize_with_length(rotated_forward);
    } else {
        real_quaternion q;
        real inverse_s = 1.0f / s;
        real_vector3d axis;
        halo::math::vector3d_cross_product(axis, *normal, *up);
        q.i = axis.i * inverse_s;
        q.j = axis.j * inverse_s;
        q.k = axis.k * inverse_s;
        q.w = s * 0.5f;
        halo::math::quaternion_rotate_vector(q, *forward, rotated_forward);
    }

    halo::math::matrix4x3_from_forward_up(*normal, rotated_forward, basis);
    basis.position = *point;

    object_recompute_basis_from_marker_delta(obj, &marker, &basis);

    *out_position = obj->position;
}

/**
 * Recomputes an item's rotation_axis / rotation_sine / rotation_cosine from its current
 * angular velocity, unless the object is already at rest (object flags bit 0x20), in which
 * case the axis is left alone (item_update owns it in that state) but the trig pair is still
 * refreshed to match whichever branch was taken.
 *
 * @address 0x4bd500
 */
void item_ref::compute_rotation()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
    real magnitude = (real)sqrt((double)obj->angular_velocity.k * (double)obj->angular_velocity.k +
                                 (double)obj->angular_velocity.j * (double)obj->angular_velocity.j +
                                 (double)obj->angular_velocity.i * (double)obj->angular_velocity.i);

    if (magnitude != 0.0f) {
        item->flags |= _item_rotation_valid_bit;
        if ((obj->flags & 0x20) == 0) {
            real inverse = 1.0f / magnitude;
            item->rotation_axis.i = inverse * obj->angular_velocity.i;
            item->rotation_axis.j = inverse * obj->angular_velocity.j;
            item->rotation_axis.k = inverse * obj->angular_velocity.k;
        }
        item->rotation_sine = (real)fsin((double)magnitude);
        item->rotation_cosine = (real)fcos((double)magnitude);
        return;
    }
    item->flags &= ~_item_rotation_valid_bit;
    item->rotation_sine = 0.0f;
    item->rotation_cosine = 1.0f;
}

/**
 * Returns an item's world position, following the attachment chain to the controlling
 * player's current object when the item itself is held (item_flags bit 0x01) rather than
 * resting in the world. Returns false and zeroes *out_position when no position could be
 * resolved.
 *
 * @address 0x4bd740
 */
uint8_t item_ref::get_effective_position(real_point3d *out_position)
{
    datum_index object_index = datum;
    object *obj = object_try_and_get(object_index, _object_mask_item);

    out_position->x = 0.0f;
    out_position->y = 0.0f;
    out_position->z = 0.0f;

    if (obj == 0) {
        return 0;
    }

    if ((((item_data *)((uint8_t *)obj + k_item_data_offset))->flags & _item_in_inventory_bit) != 0) {
        uint32_t owner_linkage = obj->owner_linkage;
        if (owner_linkage == (uint32_t)k_datum_index_none) {
            return 0;
        }
        {
            uint8_t *player = (uint8_t *)halo::memory::datum_get(owner_linkage, player_data);
            if (player == 0) {
                return 0;
            }
            {
                uint32_t controlled_object_index = (uint32_t)((struct player *)player)->unit;
                if (controlled_object_index == (uint32_t)k_datum_index_none) {
                    return 0;
                }
                obj = ((object_header *)object_data->data)[controlled_object_index & 0xffff].data;
            }
        }
    }

    *out_position = obj->bounding_center;
    return 1;
}

}

extern "C" {

void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer)
{
    halo::items::item_ref(item_index).accelerate(delta, apply_detonation_timer);
}

void item_align_to_normal_and_point(real_point3d *out_position, uint32_t item_index, real_vector3d *normal, real_point3d *point)
{
    halo::items::item_ref(item_index).align_to_normal_and_point(out_position, normal, point);
}

void item_compute_rotation(uint32_t object_index)
{
    halo::items::item_ref(object_index).compute_rotation();
}

uint8_t item_get_effective_position(datum_index object_index, real_point3d *out_position)
{
    return halo::items::item_ref(object_index).get_effective_position(out_position);
}

}
