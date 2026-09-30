// biped_ground_adjust_solve  (Ghidra: biped_ground_adjust_solve, renamed)
// address 0x558000, size 2123 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// REWRITTEN from objdump 0x558000..0x55885e (the draft passed a 12-byte local as the physics model,
//   took a third argument the binary never has, and guessed the helper signatures).
// VERIFIED against disassembly 0x558000..0x55885e (2026-09-30): tolerance / progress gates, sphere query and slide call arguments
//   (EAX/EBX/stack bindings), the 4-pass BFS with its 64-entry queue, segment start 0.015 / length 1.03, the push-out
//   arithmetic (2.5 * tolerance), the stretch restoration and every constant (checked in the image). Only the summation order of
//   the plane-distance dot product differs inside the both-embedded case (i, k, j for the node; the C keeps j, k, i).
// blam-cc: stack -> object_index, nodes (biped_ground_adjust_step 0x557b48 pushes exactly these two)
// The limp body pass for a biped (object +0x524 iteration, +0x525 limit, 0 < limit < 30):
//   - tolerance = the animation graph's limp body node radius (+0x60), 0.03 when ~0, negative or > 0.07;
//     progress = (iteration + 1) / limit, nothing when ~0 or iteration >= limit;
//   - physics_model_build_from_sphere_query (0xc0a8, object position +0x5c, bounding radius +0xac + 0.0625,
//     0, tolerance, object, the global physics model 0x006e4d08) gathers the surroundings once;
//   - four breadth-first passes over the graph nodes (0x40 each; parent +0x24, first child +0x20, next
//     sibling +0x22; 64-entry word queue) handle every non-root node against its parent (node matrices
//     0x34 each, position +0x28):
//       on pass 0, a node not embedded (physics_point_refresh_leaf, EDX point, stack tolerance) is slid by
//       (0, 0, progress * -0.0320866) (physics_model_slide_along_contacts, EAX start, stack delta, model,
//       out position, out velocity, 3, contacts); when the out velocity's i and j are ~0 the node is
//       ground-fitted (biped_ground_adjust_solve_node, EAX object, EBX the slide's out position);
//       the bone, started 1.5% behind the parent and 103% long, is tested with
//       collision_test_movement_segment (0xc0a8, object); on a hit, each end that is embedded
//       (physics_point_refresh_leaf 0.03) is pushed out along the hit plane (+0x24 normal, +0x30 d) by its
//       signed distance plus 2.5 * tolerance, scaled by progress;
//       the bone is then restored toward the model node's distance from parent (GBXModel +0xbc nodes,
//       0x9c each, +0x44) when that is in (0, 10] and the current length in [0, 20) and both are ~nonzero
//       and differ: a root child moves only itself (when not embedded) by the full stretch, other nodes
//       move the parent by -half and the node by +half, each through physics_model_slide_along_contacts.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "physics.h"
#include "projectiles.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern physics_model ground_adjust_physics_model; // 0x006e4d08

extern uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius,
    float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model); // 0x506440
extern int16_t physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta,
    physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts,
    physics_model_contact *contacts); // 0x5067b0, EAX start, stack
extern uint8_t physics_point_refresh_leaf(real_point3d *point, float radius); // 0x505540, EDX point, stack radius
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880
extern char biped_ground_adjust_solve_node(uint32_t object_index, real_point3d *reference_position,
    int32_t node_index, real_matrix4x3 *nodes, real_point3d *own_position, uint32_t *success_bits); // 0x557b80, EAX, EBX

extern double sqrt(double x);
extern double fabs(double x);

static int biped_ground_adjust_near_zero(float value)
{
    return fabs((double)value) < 9.999999747378752e-05;
}

void biped_ground_adjust_solve(uint32_t object_index, real_matrix4x3 *nodes)
{
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
                        biped_ground_adjust_solve_node(object_index, &fitted_position, node_index, nodes, self,
                            success_bits);
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
