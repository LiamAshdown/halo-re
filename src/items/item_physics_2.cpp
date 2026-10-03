#include "halo/items/items.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/scenario/api.hpp"

extern "C" {
extern data_array *object_data;
extern game_time_globals *game_time;
extern game_engine_definition *current_game_engine;
extern int16_t network_game_mode;
extern uint8_t *global_structure_collision_bsp;
extern real_vector3d *global_origin3d_pointer;
extern real_vector3d *global_down3d_pointer;
extern char s_ground_point_marker[];
extern uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t flags, uint32_t exclude_object_index, collision_result *result);
extern uint8_t any_local_player_within_10_units(const real_point3d *query_point);
extern void object_list_membership_set(uint32_t object_index, char add);
extern real_matrix4x3 *object_get_node_marker_address(uint32_t object_index, int16_t node_index);
extern void item_compute_rotation(uint32_t object_index);
extern uint8_t object_collision_test_cluster_group(uint32_t flags, real_point3d *position, uint32_t exclude_object_index);
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index, bsp_leaf_reference *location);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t maximum_markers);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern int8_t breakable_surface_is_intact(int16_t bit_index);
extern void object_recompute_basis_from_marker_delta(object *obj, object_marker *marker, real_matrix4x3 *output_matrix);
extern void object_delete(uint32_t object_index);
extern double fabs(double x);
extern double sqrt(double x);
uint8_t halo::items::item_update(uint32_t item_index);
}

namespace halo::items {

#define F(p, o) (*(float *)((p) + (o)))

/**
 * the item falls: accelerate by one tick of gravity along global down
 */
static void item_start_falling(uint32_t item_index)
{
    real_vector3d fall;

    fall.i = halo::physics::globals().gravity * global_down3d_pointer->i;
    fall.j = halo::physics::globals().gravity * global_down3d_pointer->j;
    fall.k = halo::physics::globals().gravity * global_down3d_pointer->k;
    halo::items::item_accelerate(item_index, &fall, 0);
}

/**
 * Member form of the original item_update: update.
 *
 * @address 0x4bc5c0
 */
uint8_t item_ref::update()
{
    uint32_t item_index = datum;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[item_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)obj & 0xffff].data;
    real_vector3d *forward = &((item_object *)obj)->base.forward;
    real_vector3d *up = &((item_object *)obj)->base.up;

    if ((((item_object *)obj)->base.flags & 0x800) && ((item_object *)obj)->base.parent_object == k_datum_index_none) {
        if ((((Item *)tag)->item_flags & 1) && !(fabs(((item_object *)obj)->base.up.k - 1.0f) < 9.999999747378752e-05)) {
            real_vector3d side;

            *up = *halo::math::globals().global_up3d_pointer;
            halo::math::vector3d_cross_product(side, *forward, *up);
            halo::math::vector3d_cross_product(*forward, *up, side);
            if (halo::math::vector3d_normalize_with_length(*forward) == 0.0f) {
                *forward = *halo::math::globals().global_forward3d_pointer;
            }
        }

        if (!(((item_object *)obj)->base.flags & 0x20)) {
            real_vector3d velocity = ((item_object *)obj)->base.velocity;
            real_point3d target;
            collision_result hit;

            if (!(((Item *)tag)->item_flags & 4)) {
                velocity.k -= halo::physics::globals().gravity;
            }
            target.x = ((item_object *)obj)->base.position.x + velocity.i;
            target.y = ((item_object *)obj)->base.position.y + velocity.j;
            target.z = ((item_object *)obj)->base.position.z + velocity.k;
            if (halo::physics::collision_test_movement_segment_between_points(&((item_object *)obj)->base.position, &target, 0x1ff3e9,
                                                               ((item_object *)obj)->item.ignore_object_index, &hit)) {
                real speed_factor;
                int16_t hit_type = *(int16_t *)&hit;

                target.x += hit.plane.normal.i * 0.05f;
                target.y += hit.plane.normal.j * 0.05f;
                target.z += hit.plane.normal.k * 0.05f;
                speed_factor = (real)sqrt(velocity.j * velocity.j + velocity.i * velocity.i + velocity.k * velocity.k) * 10.0f;
                if (!(speed_factor >= 0.0f)) {
                    speed_factor = 0.0f;
                } else if (!(speed_factor <= 1.0f)) {
                    speed_factor = 1.0f;
                }
                if (*(datum_index *)&((Item *)tag)->material_effects.tag_id != k_datum_index_none && any_local_player_within_10_units(&hit.point)) {
                    halo::effects::material_effects_play_at_marker(*(datum_index *)&((Item *)tag)->material_effects.tag_id, 8, *(int16_t *)&hit.material_type,
                                                    (uint32_t *)&hit.leaf, *(uint32_t *)&speed_factor, &hit.point,
                                                    &hit.plane.normal);
                }
                if (*(datum_index *)&((Item *)tag)->collision_sound.tag_id != k_datum_index_none) {
                    sound_placement placement;

                    *(real_point3d *)&placement.position = target;
                    *(real_vector3d *)&placement.forward = hit.plane.normal;
                    *(real_vector3d *)&placement.velocity = *global_origin3d_pointer;
                    placement.leaf_index = ((item_object *)obj)->base.location_leaf_index;
                    *(int32_t *)&placement.cluster_index = *(int32_t *)&((item_object *)obj)->base.location_cluster_index;
                    halo::sound::sound_start_at_location(*(datum_index *)&((Item *)tag)->collision_sound.tag_id, &placement, speed_factor);
                }
                if ((hit_type == 2 ||
                     (hit_type == 3 &&
                      ((1u << (((uint8_t *)object_data->data)[(hit.object_index & 0xffff) * 0xc + 3] & 0x1f)) & 0x3c0))) &&
                    hit.plane.normal.k > 0.7071f &&
                    -(hit.plane.normal.j * velocity.j + hit.plane.normal.i * velocity.i +
                      hit.plane.normal.k * velocity.k) < 0.05f) {
                    real spin;

                    target = hit.point;
                    halo::items::item_align_to_normal_and_point(&target, item_index, &hit.plane.normal, &hit.point);
                    spin = ((item_object *)obj)->base.angular_velocity.j * hit.plane.normal.j + ((item_object *)obj)->base.angular_velocity.k * hit.plane.normal.k +
                           ((item_object *)obj)->base.angular_velocity.i * hit.plane.normal.i;
                    velocity.i = 0.0f;
                    velocity.j = 0.0f;
                    velocity.k = 0.0f;
                    ((item_object *)obj)->base.angular_velocity.i = hit.plane.normal.i * spin;
                    ((item_object *)obj)->base.angular_velocity.j = hit.plane.normal.j * spin;
                    ((item_object *)obj)->base.angular_velocity.k = spin * hit.plane.normal.k;
                    if (current_game_engine == 0 && (datum_index)((item_object *)obj)->base.owner_linkage == k_datum_index_none) {
                        object_list_membership_set(item_index, 1);
                    }
                    ((item_object *)obj)->base.flags |= 0x20;
                    if (hit_type != 2) {
                        ((item_object *)obj)->item.flags |= 0x10;
                        ((item_object *)obj)->item.resting_object_index = hit.object_index;
                        halo::math::matrix4x3_inverse_transform_point(*object_get_node_marker_address(hit.object_index, 0),
                                                          *(&((item_object *)obj)->item.contact_point), hit.point);
                    } else {
                        ((item_object *)obj)->item.flags |= 8;
                        ((item_object *)obj)->item.resting_surface_index = *(int16_t *)((uint8_t *)&hit + 0x44);
                        ((item_object *)obj)->item.resting_bsp_index = halo::scenario::globals().structure_bsp_index;
                    }
                    ((item_object *)obj)->item.rotation_axis = hit.plane.normal;
                    halo::items::item_compute_rotation(item_index);
                    ((item_object *)obj)->item.ignore_object_index = k_datum_index_none;
                } else {
                    real impulse = hit.plane.normal.i * velocity.i * -1.4f - hit.plane.normal.j * velocity.j * 1.4f -
                                   hit.plane.normal.k * velocity.k * 1.4f;

                    if (hit_type != 2 && !(1.5f > impulse)) {
                        impulse = 1.5f;
                    }
                    velocity.i += hit.plane.normal.i * impulse;
                    velocity.j += hit.plane.normal.j * impulse;
                    velocity.k += hit.plane.normal.k * impulse;
                    target = hit.point;
                    if (halo::physics::object_collision_test_cluster_group(0x1ff3e9, &target, item_index)) {
                        target.x = hit.plane.normal.i * 0.05f + hit.point.x;
                        target.y = hit.plane.normal.j * 0.05f + hit.point.y;
                        target.z = hit.plane.normal.k * 0.05f + hit.point.z;
                    }
                    halo::physics::object_collision_test_cluster_group(0x1ff3e9, &target, item_index);
                }
            }
            ((item_object *)obj)->base.velocity = velocity;
            object_set_position_and_relink(&target, item_index, &hit.leaf);
        } else if (!(((Item *)tag)->item_flags & 4)) {
            object_marker marker;
            uint32_t flags = ((item_object *)obj)->item.flags;

            object_get_node_local_transform(item_index, s_ground_point_marker, &marker, 1);
            if ((flags & 8) && ((item_object *)obj)->item.resting_surface_index != -1 && ((item_object *)obj)->item.resting_bsp_index == halo::scenario::globals().structure_bsp_index) {
                uint8_t *surface = *(uint8_t **)(global_structure_collision_bsp + 0x40) + ((item_object *)obj)->item.resting_surface_index * 0xc;

                if ((surface[8] & 8) && !breakable_surface_is_intact((int16_t)surface[9])) {
                    ((item_object *)obj)->item.flags = flags & ~8u;
                    ((item_object *)obj)->item.resting_surface_index = -1;
                    item_start_falling(item_index);
                }
            } else if (flags & 0x10) {
                datum_index support = ((item_object *)obj)->item.resting_object_index;

                if (object_try_and_get(support, 0xffffffff) != 0) {
                    real_point3d contact;

                    halo::math::matrix4x3_transform_point(contact, *(&((item_object *)obj)->item.contact_point),
                                              *object_get_node_marker_address(support, 0));
                    halo::items::item_align_to_normal_and_point(0, item_index, &((item_object *)obj)->item.rotation_axis, &contact);
                } else {
                    ((item_object *)obj)->item.flags = flags & ~0x10u;
                    item_start_falling(item_index);
                }
            }
            ((item_object *)obj)->base.angular_velocity.i *= 0.9f;
            ((item_object *)obj)->base.angular_velocity.j *= 0.9f;
            ((item_object *)obj)->base.angular_velocity.k *= 0.9f;
            halo::items::item_compute_rotation(item_index);
        }

        if (((item_object *)obj)->item.flags & 4) {
            real_vector3d *axis = &((item_object *)obj)->item.rotation_axis;
            real sin_angle = ((item_object *)obj)->item.rotation_sine;
            real cos_angle = ((item_object *)obj)->item.rotation_cosine;
            object_marker marker;
            real_vector3d side;

            if (network_game_mode == 0 && (((item_object *)obj)->base.flags & 0x20) &&
                (int16_t)object_get_node_local_transform(item_index, s_ground_point_marker, &marker, 1)) {
                real_matrix4x3 frame = marker.node_transform;

                halo::math::vector3d_rotate_about_axis(frame.forward, *axis, sin_angle, cos_angle);
                halo::math::vector3d_rotate_about_axis(frame.up, *axis, sin_angle, cos_angle);
                halo::math::vector3d_cross_product(frame.left, frame.forward, frame.up);
                halo::math::vector3d_cross_product(frame.forward, frame.up, frame.left);
                halo::math::vector3d_normalize_with_length(frame.forward);
                halo::math::vector3d_normalize_with_length(frame.left);
                halo::math::vector3d_normalize_with_length(frame.up);
                object_recompute_basis_from_marker_delta((object *)obj, &marker, &frame);
            } else {
                halo::math::vector3d_rotate_about_axis(*forward, *axis, sin_angle, cos_angle);
                halo::math::vector3d_rotate_about_axis(*up, *axis, sin_angle, cos_angle);
            }
            halo::math::vector3d_normalize_with_length(*up);
            halo::math::vector3d_cross_product(side, *forward, *up);
            halo::math::vector3d_cross_product(*forward, *up, side);
            halo::math::vector3d_normalize_with_length(*forward);
        }
    }

    if (((item_object *)obj)->item.detonation_countdown > 0) {
        ((item_object *)obj)->item.detonation_countdown -= 1;
        if (((item_object *)obj)->item.detonation_countdown == 0) {
            halo::effects::effect_new_on_object(item_index, *(datum_index *)&((Item *)tag)->detonation_effect.tag_id, item_index, -1, 0.0f, 0.0f, 0, 0);
            object_delete(item_index);
        }
    }
    if (((item_object *)obj)->item.flags & 1) {
        ((item_object *)obj)->item.held_game_time = game_time->game_time;
    }
    return 1;
}

#undef F

}

namespace halo::items {

uint8_t item_update(uint32_t item_index)
{
    return halo::items::item_ref(item_index).update();
}

}
