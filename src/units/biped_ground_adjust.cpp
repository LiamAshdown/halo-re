#include "halo/units/unit.hpp"
#include "physics.h"
#include "projectiles.h"

extern "C" {
extern data_array *object_data;
extern tag_instance *tag_instances;
extern real vector3d_normalize_with_length(real_vector3d *v);
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
extern uint8_t real_matrix4x3_rotation_is_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up);
extern void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up);
extern double acos(double x);
extern double sin(double x);
extern double fabs(double x);
extern physics_model ground_adjust_physics_model;
extern uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model);
extern int16_t physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta, physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts, physics_model_contact *contacts);
extern uint8_t physics_point_refresh_leaf(real_point3d *point, float radius);
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
extern double sqrt(double x);
extern void plane3d_from_point_and_normal(real_plane3d *out, const real_vector3d *normal, const real_point3d *point);
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in);
extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m);
extern real_point3d unit_ground_adjust_node_positions[64];
}

namespace halo::units {

namespace biped_ground_adjust_apply_node_rotations_local {

static void biped_ground_adjust_make_orthonormal(real_matrix4x3 *m)
{
    if (!real_matrix4x3_rotation_is_orthonormal(&m->forward, &m->left, &m->up)) {
        real_matrix4x3_rotation_rebuild_orthonormal(&m->forward, &m->left, &m->up);
    }
}

}

/**
 * Engine function biped_ground_adjust_apply_node_rotations.
 *
 * @address 0x558a20
 */
void BipedView::ground_adjust_apply_node_rotations(real_matrix4x3 *nodes, real_point3d *saved_positions)
{
    using namespace biped_ground_adjust_apply_node_rotations_local;
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)&((struct Object *)object_tag)->animation_graph.tag_id & 0xffff].data;
    int32_t i;

    for (i = 0; i < *(int32_t *)&((ModelAnimations *)graph)->nodes.count; i++) {
        uint8_t *graph_nodes = *(uint8_t **)&((ModelAnimations *)graph)->nodes.pointer;
        int16_t parent_index;
        real_vector3d saved;
        real_vector3d current;
        real_vector3d axis;
        float cosine;
        float angle;

        if (i == 0) {
            continue;
        }
        parent_index = *(int16_t *)(graph_nodes + i * 0x40 + 0x24);
        if (graph_nodes[parent_index * 0x40 + 0x28] & 4) {
            continue;
        }
        saved.i = saved_positions[i].x - saved_positions[parent_index].x;
        saved.j = saved_positions[i].y - saved_positions[parent_index].y;
        saved.k = saved_positions[i].z - saved_positions[parent_index].z;
        current.i = nodes[i].position.x - nodes[parent_index].position.x;
        current.j = nodes[i].position.y - nodes[parent_index].position.y;
        current.k = nodes[i].position.z - nodes[parent_index].position.z;
        vector3d_normalize_with_length(&saved);
        vector3d_normalize_with_length(&current);
        axis.i = current.k * saved.j - current.j * saved.k;
        axis.j = current.i * saved.k - current.k * saved.i;
        axis.k = current.j * saved.i - saved.j * current.i;
        vector3d_normalize_with_length(&axis);
        cosine = current.k * saved.k + current.j * saved.j + current.i * saved.i;
        if (fabs((double)(cosine - 1.0f)) < 9.999999747378752e-05) {
            continue;
        }
        angle = (float)acos((double)cosine);
        if (fabs((double)angle) < 9.999999747378752e-05 || !(fabs((double)angle) < 0.7853981852531433)) {
            continue;
        }
        {
            real_matrix4x3 *parent = &nodes[parent_index];
            real sine;

            biped_ground_adjust_make_orthonormal(parent);
            sine = (real)sin((double)angle);
            vector3d_rotate_about_axis(&parent->forward, &axis, sine, cosine);
            vector3d_rotate_about_axis(&parent->up, &axis, sine, cosine);
            vector3d_normalize_with_length(&parent->forward);
            vector3d_normalize_with_length(&parent->up);
            vector3d_cross_product(&parent->left, &parent->forward, &parent->up);
            vector3d_normalize_with_length(&parent->left);
            biped_ground_adjust_make_orthonormal(parent);
        }
    }
}

namespace biped_ground_adjust_solve_local {

static int biped_ground_adjust_near_zero(float value)
{
    return fabs((double)value) < 9.999999747378752e-05;
}

}

/**
 * Engine function biped_ground_adjust_solve.
 *
 * @address 0x558000
 */
void BipedView::ground_adjust_solve(real_matrix4x3 *nodes)
{
    using namespace biped_ground_adjust_solve_local;
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)&((struct Object *)object_tag)->animation_graph.tag_id & 0xffff].data;
    float tolerance = ((ModelAnimations *)graph)->limp_body_node_radius;
    uint8_t limit = obj[0x525];
    uint8_t iteration;
    float progress;
    uint32_t success_bits[2];
    int32_t pass;
    physics_model_contact contacts[3];

    if (biped_ground_adjust_near_zero(tolerance) || tolerance < 0.0f || tolerance > 0.07f) {
        tolerance = 0.03f;
    }
    if (limit == 0 || limit >= 0x1e) {
        return;
    }
    iteration = obj[0x524];
    progress = (float)((int32_t)iteration + 1) / (float)(int32_t)limit;
    if (biped_ground_adjust_near_zero(progress) || iteration >= limit) {
        return;
    }

    physics_model_build_from_sphere_query(0xc0a8, (real_point3d *)(obj + 0x5c), ((unit_object *)obj)->base.bounding_radius + 0.0625f,
        0.0f, tolerance, object_index, &ground_adjust_physics_model);

    success_bits[0] = 0;
    success_bits[1] = 0;
    for (pass = 0; pass < 4; pass++) {
        int16_t queue[64];
        int16_t read_index = 0;
        int16_t write_index = 1;

        queue[0] = 0;
        do {
            int16_t node_index = queue[read_index];
            uint8_t *graph_node = *(uint8_t **)&((ModelAnimations *)graph)->nodes.pointer + node_index * 0x40;

            read_index++;
            if (node_index != 0) {
                int16_t parent_index = *(int16_t *)(graph_node + 0x24);
                real_point3d *self = &nodes[node_index].position;
                real_point3d *parent = &nodes[parent_index].position;
                real_vector3d bone;
                real_vector3d delta;
                real_point3d fitted_position;
                real_vector3d fitted_velocity;
                collision_result hit;
                real_point3d segment_start;
                float rest_length;
                float current_length;

                delta.i = 0.0f;
                delta.j = 0.0f;
                delta.k = progress * -0.03208661451935768f;
                if (pass == 0 && !physics_point_refresh_leaf(self, tolerance)) {
                    physics_model_slide_along_contacts(self, &delta, &ground_adjust_physics_model, &fitted_position,
                        &fitted_velocity, 3, contacts);
                    if (biped_ground_adjust_near_zero(fitted_velocity.i) &&
                        biped_ground_adjust_near_zero(fitted_velocity.j)) {
                        BipedView(object_index).ground_adjust_solve_node(&fitted_position, node_index, nodes, self, success_bits);
                    }
                }

                bone.i = self->x - parent->x;
                bone.j = self->y - parent->y;
                bone.k = self->z - parent->z;
                segment_start.x = parent->x - bone.i * 0.015f;
                segment_start.y = parent->y - bone.j * 0.015f;
                segment_start.z = parent->z - bone.k * 0.015f;
                bone.i = bone.i * 1.03f;
                bone.j = bone.j * 1.03f;
                bone.k = bone.k * 1.03f;
                if (collision_test_movement_segment(0xc0a8, &segment_start, &bone, object_index, &hit)) {
                    uint8_t embedded[2];
                    float push[2];
                    real_plane3d *plane = &hit.plane;

                    embedded[0] = physics_point_refresh_leaf(self, 0.03f);
                    embedded[1] = physics_point_refresh_leaf(parent, 0.03f);
                    if ((int32_t)embedded[0] + (int32_t)embedded[1] != 0) {
                        int32_t side;

                        for (side = 0; side < 2; side++) {
                            real_point3d *point = (side == 0) ? self : parent;
                            float length_squared;

                            if (!embedded[side]) {
                                push[side] = 0.0f;
                                continue;
                            }
                            length_squared = plane->normal.j * plane->normal.j + plane->normal.k * plane->normal.k +
                                plane->normal.i * plane->normal.i;
                            push[side] = -((plane->normal.j * point->y + plane->normal.k * point->z +
                                plane->normal.i * point->x - plane->d) / length_squared);
                            if (push[side] != 0.0f) {
                                push[side] = tolerance * 2.5f + push[side];
                            }
                        }
                        if (!biped_ground_adjust_near_zero(push[0])) {
                            float scale = push[0] * progress;

                            self->x = plane->normal.i * scale + self->x;
                            self->y = plane->normal.j * scale + self->y;
                            self->z = plane->normal.k * scale + self->z;
                        }
                        if (!biped_ground_adjust_near_zero(push[1])) {
                            float scale = push[1] * progress;

                            parent->x = plane->normal.i * scale + parent->x;
                            parent->y = plane->normal.j * scale + parent->y;
                            parent->z = plane->normal.k * scale + parent->z;
                        }
                    }
                }

                bone.i = self->x - parent->x;
                bone.j = self->y - parent->y;
                bone.k = self->z - parent->z;
                {
                    uint8_t *model = (uint8_t *)tag_instances[*(datum_index *)&((struct Object *)object_tag)->model.tag_id & 0xffff].data;

                    rest_length = *(float *)(*(uint8_t **)(model + 0xbc) + node_index * 0x9c + 0x44);
                }
                {
                    float dx = parent->x - self->x;
                    float dy = parent->y - self->y;
                    float dz = parent->z - self->z;

                    current_length = (float)sqrt(dz * dz + dy * dy + dx * dx);
                }
                if (rest_length > 0.0f && rest_length <= 10.0f && !(current_length < 0.0f) &&
                    current_length < 20.0f && !biped_ground_adjust_near_zero(current_length) &&
                    !biped_ground_adjust_near_zero(rest_length) && current_length != rest_length) {
                    float stretch = (rest_length - current_length) / current_length;
                    real_vector3d correction;

                    if (*(int16_t *)(graph_node + 0x24) == 0) {
                        correction.i = bone.i * stretch;
                        correction.j = bone.j * stretch;
                        correction.k = bone.k * stretch;
                        if (!physics_point_refresh_leaf(self, tolerance)) {
                            physics_model_slide_along_contacts(self, &correction, &ground_adjust_physics_model, self,
                                &correction, 3, contacts);
                        }
                    } else {
                        float half = stretch * 0.5f;

                        correction.i = bone.i * -half;
                        correction.j = bone.j * -half;
                        correction.k = bone.k * -half;
                        physics_model_slide_along_contacts(parent, &correction, &ground_adjust_physics_model, parent,
                            &correction, 3, contacts);
                        correction.i = bone.i * half;
                        correction.j = bone.j * half;
                        correction.k = bone.k * half;
                        physics_model_slide_along_contacts(self, &correction, &ground_adjust_physics_model, self,
                            &correction, 3, contacts);
                    }
                }
            }

            if (*(uint16_t *)(graph_node + 0x20) != 0xffff) {
                queue[write_index++] = *(int16_t *)(graph_node + 0x20);
            }
            if (*(uint16_t *)(graph_node + 0x22) != 0xffff) {
                queue[write_index++] = *(int16_t *)(graph_node + 0x22);
            }
        } while (read_index != write_index);
    }
}

namespace biped_ground_adjust_solve_node_local {

static void biped_ground_adjust_mark(uint32_t *success_bits, int32_t node_index)
{
    success_bits[node_index >> 5] |= 1u << (node_index & 0x1f);
}

static int biped_ground_adjust_is_one(float value)
{
    return fabs((double)(value - 1.0f)) < 9.999999747378752e-05;
}

}

/**
 * Engine function biped_ground_adjust_solve_node.
 *
 * @address 0x557b80
 */
char BipedView::ground_adjust_solve_node(real_point3d *reference_position, int32_t node_index, real_matrix4x3 *nodes, real_point3d *own_position, uint32_t *success_bits)
{
    using namespace biped_ground_adjust_solve_node_local;
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)&((struct Object *)object_tag)->animation_graph.tag_id & 0xffff].data;
    uint8_t *graph_nodes = *(uint8_t **)&((ModelAnimations *)graph)->nodes.pointer;
    uint8_t *self_node = graph_nodes + node_index * 0x40;
    int16_t parent_index = *(int16_t *)(self_node + 0x24);
    uint8_t *parent_node = graph_nodes + parent_index * 0x40;
    float tolerance = ((ModelAnimations *)graph)->limp_body_node_radius;
    char updated = 0;

    if (fabs((double)tolerance) < 9.999999747378752e-05 || tolerance < 0.0f || tolerance > 0.07f) {
        tolerance = 0.03f;
    }

    if (parent_index != 0 && (parent_node[0x28] & 4) == 0) {
        real_matrix4x3 *parent_matrix = &nodes[parent_index];
        real_point3d *parent_position = &parent_matrix->position;
        real_point3d *self_position = &nodes[node_index].position;
        real_vector3d to_self;
        real_vector3d to_reference;
        real_vector3d axis;
        float cosine;

        to_self.i = self_position->x - parent_position->x;
        to_self.j = self_position->y - parent_position->y;
        to_self.k = self_position->z - parent_position->z;
        to_reference.i = reference_position->x - parent_position->x;
        to_reference.j = reference_position->y - parent_position->y;
        to_reference.k = reference_position->z - parent_position->z;
        vector3d_normalize_with_length(&to_self);
        vector3d_normalize_with_length(&to_reference);
        vector3d_cross_product(&axis, &to_reference, &to_self);
        vector3d_normalize_with_length(&axis);
        cosine = to_reference.k * to_self.k + to_reference.j * to_self.j + to_reference.i * to_self.i;

        if (!biped_ground_adjust_is_one(cosine)) {
            float angle = (float)acos((double)cosine);
            float *base = (float *)(parent_node + 0x2c);
            real_matrix4x3 parent_inverse;
            real_matrix4x3 grandparent_inverse;
            real_vector3d local_axis;
            real_vector3d local_forward;

            matrix4x3_inverse(&parent_inverse, parent_matrix);
            matrix4x3_inverse(&grandparent_inverse, &nodes[*(int16_t *)(parent_node + 0x24)]);
            matrix4x3_transform_vector(&local_axis, &axis, &parent_inverse);
            local_forward = parent_matrix->forward;
            matrix4x3_transform_vector(&local_forward, &local_forward, &grandparent_inverse);

            if (parent_node[0x28] & 2) {
                real_vector3d up = parent_matrix->up;
                real_plane3d plane;
                real_point3d projected;
                real_vector3d direction;
                real_vector3d local_direction;
                float distance;

                plane3d_from_point_and_normal(&plane, &up, parent_position);
                distance = (plane.normal.j * reference_position->y + plane.normal.i * reference_position->x +
                    plane.normal.k * reference_position->z - plane.d) * -1.0f;
                projected.x = up.i * distance + reference_position->x;
                projected.y = up.j * distance + reference_position->y;
                projected.z = up.k * distance + reference_position->z;
                direction.i = projected.x - parent_position->x;
                direction.j = projected.y - parent_position->y;
                direction.k = projected.z - parent_position->z;
                vector3d_normalize_with_length(&direction);
                matrix4x3_transform_vector(&local_direction, &direction, &parent_inverse);
                if (!biped_ground_adjust_is_one(local_direction.j * base[1] + local_direction.k * base[2] +
                        local_direction.i * base[0])) {
                    biped_ground_adjust_mark(success_bits, node_index);
                    if (!(own_position->z < projected.z) && !physics_point_refresh_leaf(&projected, tolerance)) {
                        *own_position = projected;
                    }
                    updated = 1;
                }
            } else {
                float alignment;

                vector3d_rotate_about_axis(&local_forward, &local_axis, (real)sin((double)angle), cosine);
                alignment = local_forward.j * base[1] + local_forward.k * base[2] + local_forward.i * base[0];
                if (!biped_ground_adjust_is_one(alignment) &&
                    *(float *)(parent_node + 0x38) > (float)fabs(acos((double)alignment)) &&
                    own_position->z > reference_position->z) {
                    biped_ground_adjust_mark(success_bits, node_index);
                    *own_position = *reference_position;
                    updated = 1;
                }
            }
        }
    }

    if ((parent_node[0x28] & 4) == 0 && updated) {
        return updated;
    }
    if ((success_bits[parent_index >> 5] & (1u << (parent_index & 0x1f))) == 0) {
        return updated;
    }
    if (!(own_position->z > reference_position->z)) {
        return updated;
    }
    biped_ground_adjust_mark(success_bits, node_index);
    *own_position = *reference_position;
    return 1;
}

/**
 * Snapshots the current world position of every skeleton node into the shared
 * unit_ground_adjust_node_positions buffer, runs the ground-contact/bone-length solve
 * (biped_ground_adjust_solve) and the node-basis update (biped_ground_adjust_apply_node_rotations), then
 * advances the per-biped ground-adjust iteration counter, saturating at 0x7f. Returns 1 once
 * biped_data.ground_adjust_iteration has already reached ground_adjust_iteration_limit (nothing done this
 * call), 0 if an iteration ran.
 *
 * @address 0x557a90
 */
uint32_t BipedView::ground_adjust_step()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    ModelAnimations *graph = (ModelAnimations *)tag_instances[object_tag->animation_graph.tag_id.index].data;
    real_matrix4x3 *nodes = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);
    uint32_t already_capped = biped->ground_adjust_iteration_limit <= biped->ground_adjust_iteration;

    if (!already_capped) {
        int32_t i;
        for (i = 0; i < (int32_t)graph->nodes.count; i++) {
            unit_ground_adjust_node_positions[i] = nodes[i].position;
        }
        BipedView(object_index).ground_adjust_solve(nodes);
        BipedView(object_index).ground_adjust_apply_node_rotations(nodes, unit_ground_adjust_node_positions);
        if (biped->ground_adjust_iteration < 0x7f) {
            biped->ground_adjust_iteration = biped->ground_adjust_iteration + 1;
        }
    }
    return already_capped;
}

/**
 * Once the Biped tag's "requires ground adjust" flag (biped_flags bit 0x200, UNSURE) is clear and the
 * ground-adjust dirty bit is still set, clears it along with its object.flags mirror.
 *
 * @address 0x55ad70
 */
void UnitView::clear_ground_adjust_dirty()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((tag->biped_flags & 0x200) != 0 && (biped->flags & 0x20) != 0) {
        obj->flags &= ~0x800000u;
        biped->flags &= ~0x20u;
    }
}

/**
 * When the Biped tag requests ground adjustment (biped_flags bit 0x200, UNSURE) and the object is attached to
 * a parent (object.flags bit 0x20) but the ground-adjust dirty bit isn't already set and the biped isn't
 * already grounded, resets the iteration counter, seeds the iteration limit to 0x14, and marks both the dirty
 * bit and its object.flags mirror.
 *
 * @address 0x55ad00
 */
void UnitView::reset_ground_adjust_state()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if (((tag->biped_flags >> 9) & 1) != 0 && (obj->flags & 0x20) != 0 && (biped->flags & 0x21) == 0) {
        biped->ground_adjust_iteration = 0;
        biped->ground_adjust_iteration_limit = 0x14;
        obj->flags |= 0x800000;
        biped->flags |= 0x20;
    }
}

}
