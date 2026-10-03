#include "halo/objects/object_update.hpp"
#include "halo/models/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "game.h"
#include "models.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"

extern "C" {
extern float angle_delta_wrapped(float from, float to);
extern double atan2(double y, double x);
extern double fabs(double x);
extern float fabsf(float x);
extern double floor(double x);
extern double fmod(double x, double y);
extern double fpatan(double y, double x);
extern game_time_globals *game_time;
extern real_vector3d *global_origin3d_pointer;
extern int16_t network_game_mode;
extern data_array *object_data;
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback);
extern int16_t object_get_first_region_probability_group(uint32_t object_index, GBXModel *model);
extern object_globals *object_globals_pointer;
extern void object_notify_node_array_if_animated(uint32_t object_index);
extern int16_t object_permutation_find_matching_group(ModelRegion *region, int16_t group, int16_t *out);
extern void object_recalculate_bounding_radius(uint32_t object_index);
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index);
extern uint8_t object_regions_initialize_permutations(uint32_t object_index, int16_t group, GBXModel *model);
extern void object_type_definitions_notify_0x38(uint32_t object_index);
extern void object_type_definitions_notify_two_args_0x48(uint32_t object_index, uint32_t event_argument);
extern uint8_t object_type_definitions_query_0x34(uint32_t object_index);
extern uint8_t object_update(uint32_t object_index);
extern void object_update_change_colors(uint32_t object_index);
extern void object_update_functions(uint32_t object_index);
extern void object_update_vitality_and_regeneration(uint32_t object_index);
extern double pow(double base, double exponent);
}

/**
 * Locks or unlocks the object's region permutations.
 *
 * Original register convention: uint32_t object_index in EAX (in_EAX); a bool lock flag in the low byte.
 *
 * @address 0x004f03e0
 */
void halo::objects::ObjectUpdater::regions_reset_permutation_lock(int8_t unlock)
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;
    ModelCollisionGeometry *geometry =
        (ModelCollisionGeometry *)halo::cache::globals().tag_instances[definition->collision_model.tag_id.index].data;
    ModelCollisionGeometryRegion *regions = (ModelCollisionGeometryRegion *)geometry->regions.pointer;
    int32_t region_count = (int32_t)geometry->regions.count;
    int32_t region_index;

    for (region_index = 0; region_index < region_count; region_index++) {
        ModelCollisionGeometryRegion *region = &regions[region_index];

        if ((region->flags & 0x10) != 0 && (int32_t)region->permutations.count > 1) {
            obj->region_permutations[region_index] = (unlock == 0);
        }
    }
}

/**
 * Selects a named permutation for the object's matching regions.
 *
 * @address 0x004f6c60
 */
void halo::objects::ObjectUpdater::set_permutation_by_name(char *name, int16_t region_filter, char use_matched_index)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;

    if (definition->model.tag_id.index == 0xffff) {
        return;
    }

    {
        GBXModel *model = (GBXModel *)halo::cache::globals().tag_instances[definition->model.tag_id.index & 0xffff].data;
        int16_t region_index;
        for (region_index = 0; region_index < (int16_t)model->regions.count; region_index++) {
            ModelRegion *region;
            if (region_filter != -1 && region_filter != region_index) {
                continue;
            }
            region = (ModelRegion *)model->regions.pointer + region_index;
            if ((int16_t)region->permutations.count > 0) {
                ModelRegionPermutation *permutations = (ModelRegionPermutation *)region->permutations.pointer;
                int16_t permutation_index;
                for (permutation_index = 0; permutation_index < (int16_t)region->permutations.count; permutation_index++) {
                    if (_stricmp(permutations[permutation_index].name.string, name) == 0) {
                        int16_t stored = permutation_index;
                        if (use_matched_index == 0) {
                            stored = 0;
                        }
                        obj->region_permutations[region_index] = (uint8_t)stored;
                        break;
                    }
                }
            }
        }
    }
}

/**
 * Per-tick update of one object; returns nonzero when the object is still alive.
 *
 * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
 * "object_update(uint param_1)"), reused as EAX/EBX at various callee call sites within the body per each callee's
 * own established convention.
 *
 * @address 0x004f7ef0
 */
uint8_t halo::objects::ObjectUpdater::update()
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;

    if ((header->flags & _object_header_just_created_bit) != 0) {
        return 1;
    }

    if ((obj->flags & _object_in_tracked_list_bit) != 0) {
        object_globals_pointer->active_garbage_object_count = object_globals_pointer->active_garbage_object_count + 1;
    }

    if (obj->node_function_count != 0) {
        obj->interpolation_frame_index = obj->interpolation_frame_index + 1;
        if (obj->node_function_count <= obj->interpolation_frame_index) {
            obj->node_function_count = 0;
        }
    }

    object_type_definitions_query_0x34(object_index);

    if (definition->collision_model.tag_id.index != 0xffff) {
        object_update_vitality_and_regeneration(object_index);
    }

    object_type_definitions_notify_0x38(object_index);

    if ((obj->flags & 0x800000) == 0) {
        object_recalculate_bounding_radius(object_index);
    }

    object_update_functions(object_index);
    object_update_change_colors(object_index);

    if (((obj->flags & 0x2000) != 0) &&
        (((obj->flags & _object_no_collision_bit) == 0) || (definition->model.tag_id.index == 0xffff))) {
        object_for_each_light_attachment(object_index, 1, 1);
    }

    if (obj->first_child_object != k_datum_index_none) {
        object_update(obj->first_child_object);
    }
    if ((obj->parent_object != k_datum_index_none) && (obj->next_object != k_datum_index_none)) {
        object_update(obj->next_object);
    }

    object_notify_node_array_if_animated(object_index);

    if (network_game_mode == 2) {
        uint8_t *at_rest_flag = (uint8_t *)&obj->at_rest;
        if ((fabsf(obj->velocity.i - global_origin3d_pointer->i) < 0.0001f) &&
            (fabsf(obj->velocity.j - global_origin3d_pointer->j) < 0.0001f) &&
            (fabsf(obj->velocity.k - global_origin3d_pointer->k) < 0.0001f) &&
            (fabsf(obj->angular_velocity.i - global_origin3d_pointer->i) < 0.0001f) &&
            (fabsf(obj->angular_velocity.j - global_origin3d_pointer->j) < 0.0001f) &&
            (fabsf(obj->angular_velocity.k - global_origin3d_pointer->k) < 0.0001f)) {
            *at_rest_flag = 1;
            return 1;
        }
        *at_rest_flag = 0;
    }

    return 1;
}

namespace {
static float clamp_to_one(float value)
{
    return value <= 1.0f ? value : 1.0f;
}
}

/**
 * Evaluates and stores the object's exported function values.
 *
 * Original register convention: stack -> object_index (cdecl).
 *
 * @address 0x004f80d0
 */
void halo::objects::ObjectUpdater::update_export_functions()
{
    datum_index object_index = handle;
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)object & 0xffff].data;
    int32_t i;

    for (i = 0; i < 4; i++) {
        int16_t source = *(int16_t *)(definition + 0x108 + i * 2);
        float *output = (float *)(object + 0x124 + i * 4);
        float value = 0.0f;

        if (source == 0) {
            continue;
        }
        switch (source) {
        case 1: value = *(float *)(object + 0xe0); break;
        case 2: value = clamp_to_one(*(float *)(object + 0xe4)); break;
        case 3: value = *(float *)(object + 0xec); break;
        case 4: value = *(float *)(object + 0xe8); break;
        case 5:
            if (*(uint32_t *)output == 0x3f800000) {
                value = halo::math::random_real();
            }
            break;
        case 18: value = (object[0x106] & 4) != 0 ? 0.0f : 1.0f; break;
        case 19: {
            float *forward = (float *)(object + *(int16_t *)(object + 0x1f2) + 4);

            if (!(fabs(forward[2]) < 0.995)) {
                value = *output;
            } else {
                float yaw = (float)fpatan(forward[0], forward[1]);
                value = angle_delta_wrapped(halo::scenario::globals().scenario->local_north, yaw) * 0.15915494f + 0.5f;
                value = value >= 0.0f ? clamp_to_one(value) : 0.0f;
            }
            break;
        }
        default:
            value = (float)object[0x178 + (source - 10)] * 0.0039215689f;
            break;
        }
        *output = value;
    }
}

namespace {
static uint8_t * &global_scenario__as_object_function_evaluate_input = reinterpret_cast<uint8_t * &>(halo::scenario::globals().scenario);
}

/**
 * Evaluates one object function input selector, with the input angle and the object's tag data.
 *
 * @address 0x004f8207
 */
void halo::objects::ObjectUpdater::function_evaluate_input(float initial_angle_input, float initial_st0,
    int16_t *selectors, float *out_values, uint8_t *object_tag_data, int32_t object_index_scaled,
    int32_t remaining_count)
{
    float value;
    int16_t selector;

    value = angle_delta_wrapped(initial_angle_input, initial_st0);
    value = value * 0.15915494f + 0.5f;
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        goto clamp_to_one;
    }

store_and_advance:
    *out_values = value;
    for (;;) {
        selectors++;
        out_values++;
        remaining_count--;
        if (remaining_count == 0) {
            return;
        }
        selector = *selectors;
        if (selector != 0) {
            break;
        }
    }

    value = 0.0f;
    switch (selector) {
        case 1:
            value = *(float *)(object_tag_data + 0xe0);
            goto store_and_advance;
        case 2:
            value = *(float *)(object_tag_data + 0xe4);
            if (value <= 1.0f) {
                goto store_and_advance;
            }
            break;
        case 3:
            value = *(float *)(object_tag_data + 0xec);
            goto store_and_advance;
        case 4:
            value = *(float *)(object_tag_data + 0xe8);
            goto store_and_advance;
        case 5:
            if (*out_values == 1.0f) {
                value = halo::math::random_real();
            }
            goto store_and_advance;
        case 0x12:
            if ((*(uint8_t *)(object_tag_data + 0x106) & 4) != 0) {
                value = 0.0f;
                goto store_and_advance;
            }
            break;
        case 0x13: {

            object *obj = *(object **)((uint8_t *)object_data->data + 8 + object_index_scaled);
            real_matrix4x3 *node = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);
            if (0.995f <= fabsf(node->left.i)) {
                value = *out_values;
                goto store_and_advance;
            }
            initial_st0 = (float)atan2((double)node->forward.j, (double)node->forward.k);
            initial_angle_input = *(float *)(global_scenario__as_object_function_evaluate_input + 0x4c);
            value = angle_delta_wrapped(initial_angle_input, initial_st0);
            value = value * 0.15915494f + 0.5f;
            if (value >= 0.0f) {
                goto clamp_to_one;
            }
            value = 0.0f;
            goto store_and_advance;
        }
        default:
            value = (float)*(uint8_t *)(object_tag_data + (int16_t)(selector - 10) + 0x178) * 0.003921569f;
            goto store_and_advance;
    }
    value = 1.0f;
    goto store_and_advance;

clamp_to_one:
    goto store_and_advance;
}

/**
 * Recomputes the bounding radius of an object and its children.
 *
 * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
 * "object_recalculate_bounding_radius_recursive(uint param_1)").
 *
 * @address 0x004f82b0
 */
void halo::objects::ObjectUpdater::recalculate_bounding_radius_recursive()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    datum_index child;

    object_recalculate_bounding_radius(object_index);

    child = obj->first_child_object;
    while (child != k_datum_index_none) {
        object *child_obj = ((object_header *)object_data->data)[child & 0xffff].data;
        object_recalculate_bounding_radius_recursive(child);
        child = child_obj->next_object;
    }
}

namespace {
#define OFS(base, off, type) (*(type *)((uint8_t *)(base) + (off)))
#define TAG_DATA(id) ((uint8_t *)halo::cache::globals().tag_instances[(uint32_t)(id) & 0xffff].data)
static void matrix4x3_set_translation_only(real_matrix4x3 *m, const real_point3d *position)
{
    m->scale = 1.0f;
    m->forward.i = 1.0f; m->forward.j = 0.0f; m->forward.k = 0.0f;
    m->left.i = 0.0f; m->left.j = 1.0f; m->left.k = 0.0f;
    m->up.i = 0.0f; m->up.j = 0.0f; m->up.k = 1.0f;
    m->position = *position;
}
}

/**
 * Recomputes the object's bounding centre and radius from its model nodes.
 *
 * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
 * "object_recalculate_bounding_radius(uint param_1)").
 *
 * @address 0x004f8310
 */
void halo::objects::ObjectUpdater::recalculate_bounding_radius()
{
    uint32_t object_index = handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *def = TAG_DATA(OFS(obj, 0x0, uint32_t));
    real_matrix4x3 *nodes = (real_matrix4x3 *)(obj + OFS(obj, 0x1f2, int16_t));
    real_orientation local_orientations[k_maximum_nodes_per_model];
    real_orientation *orientations;

    if (((1u << (OFS(obj, 0xb4, uint8_t) & 0x1f)) & 0xfe0u) != 0) {
        orientations = local_orientations;
    } else {
        orientations = (real_orientation *)(obj + OFS(obj, 0x1ee, int16_t));
    }

    if (OFS(def, 0x34, int32_t) == -1) {

        nodes[0].scale = 1.0f;
        nodes[0].forward = OFS(obj, 0x74, real_vector3d);
        nodes[0].up = OFS(obj, 0x80, real_vector3d);
        halo::math::vector3d_cross_product(nodes[0].left, nodes[0].forward, nodes[0].up);
        nodes[0].position = OFS(obj, 0x5c, real_point3d);
    } else {
        uint8_t *model = TAG_DATA(OFS(def, 0x34, uint32_t));
        real_matrix4x3 *parent_matrix = 0;
        uint8_t absolute_root = 0;
        int16_t queue[k_maximum_nodes_per_model];
        int16_t head, tail;

        if (OFS(obj, 0x11c, int32_t) != -1) {
            uint8_t *parent = (uint8_t *)((object_header *)object_data->data)[OFS(obj, 0x11c, uint32_t) & 0xffff].data;
            parent_matrix = (real_matrix4x3 *)(parent + OFS(parent, 0x1f2, int16_t)) + OFS(obj, 0x120, int8_t);
        }

        if (OFS(obj, 0xcc, int32_t) != -1 && OFS(obj, 0xd0, int16_t) != -1) {
            ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(OFS(TAG_DATA(OFS(obj, 0xcc, uint32_t)), 0x78, uint8_t *) +
                (int32_t)OFS(obj, 0xd0, int16_t) * 0xb4);
            int16_t frame_count = OFS(animation, 0x22, int16_t);
            uint32_t frame;
            if (OFS(obj, 0x10, int8_t) < 0 && frame_count > 0) {
                frame = (OFS(game_time, 0xc, uint32_t) + object_index) % (uint32_t)(int32_t)frame_count;
            } else {
                frame = OFS(obj, 0xd2, uint16_t);
            }
            halo::models::animation_get_frame_orientations(animation, (GBXModel *)model, (int16_t)frame, orientations);
            absolute_root = (uint8_t)((OFS(animation, 0x3a, uint8_t) >> 1) & 1);
        } else {
            halo::models::model_nodes_get_default_transforms((GBXModel *)model, orientations);
        }

        if (OFS(def, 0x44, int32_t) != -1) {
            uint8_t *graph = TAG_DATA(OFS(def, 0x44, uint32_t));
            int16_t i;
            for (i = 0; (int32_t)i < OFS(graph, 0x0, int32_t); i++) {
                int16_t *entry = (int16_t *)(OFS(graph, 0x4, uint8_t *) + (int32_t)i * 0x14);
                if (entry[0] == -1 || (int32_t)entry[1] >= OFS(def, 0x158, int32_t)) {
                    continue;
                }
                {
                    ModelAnimationsAnimation *animation =
                        (ModelAnimationsAnimation *)(OFS(graph, 0x78, uint8_t *) + (int32_t)entry[0] * 0xb4);
                    float value = OFS(obj, 0x134 + (int32_t)entry[1] * 4, float);
                    if (entry[2] == 0) {
                        int32_t frames = (int32_t)OFS(animation, 0x22, int16_t);
                        if ((OFS(OFS(def, 0x15c, uint8_t *), (int32_t)entry[1] * 0x168, uint8_t) & 2) == 0) {
                            frames -= 1;
                        }
                        halo::models::animation_overlay_interpolated_frame_orientations(animation, (float)frames * value, orientations);
                    } else if (entry[2] == 1) {
                        uint32_t frame = (OFS(game_time, 0xc, uint32_t) + object_index) %
                            (uint32_t)(int32_t)OFS(animation, 0x22, int16_t);
                        halo::models::animation_overlay_frame_orientations_weighted(animation, (int16_t)frame, value, orientations);
                    }
                }
            }
        }

        if (OFS(obj, 0xb0, float) > 0.0f) {
            float scale = OFS(obj, 0xb0, float);
            orientations[0].scale *= scale;
            orientations[0].translation.x *= scale;
            orientations[0].translation.y *= scale;
            orientations[0].translation.z *= scale;
        }
        if (OFS(def, 0x44, int32_t) != -1) {
            object_type_definitions_notify_two_args_0x48(object_index, (uint32_t)orientations);
        }
        if (OFS(obj, 0xd6, int16_t) > 0) {

            halo::models::model_nodes_blend_transforms(orientations, OFS(model, 0xb8, int16_t),
                (real_orientation *)(obj + OFS(obj, 0x1ea, int16_t)), (int16_t)OFS(obj, 0xd4, uint16_t),
                (int16_t)OFS(obj, 0xd6, uint16_t));
        }

        queue[0] = 0;
        head = 0;
        tail = 1;
        do {
            int16_t node_index = queue[head++];
            uint8_t *node = OFS(model, 0xbc, uint8_t *) + (int32_t)node_index * 0x9c;

            if (node_index == 0) {
                real_matrix4x3 root;
                halo::math::matrix4x3_from_quaternion(orientations[0].rotation, root);
                root.scale = orientations[0].scale;
                root.position = orientations[0].translation;

                if (absolute_root) {
                    nodes[0] = root;
                } else {
                    real_matrix4x3 world;
                    real_matrix4x3 orientation;
                    real_matrix4x3 offset;
                    real_matrix4x3 parent_copy;
                    real_matrix4x3 *base = parent_matrix;

                    matrix4x3_set_translation_only(&world, &OFS(obj, 0x5c, real_point3d));
                    halo::math::matrix4x3_from_forward_up(OFS(obj, 0x80, real_vector3d), OFS(obj, 0x74, real_vector3d), orientation);
                    if ((OFS(obj, 0x10, uint32_t) & 0x1000) != 0) {
                        orientation.left.i = -orientation.left.i;
                        orientation.left.j = -orientation.left.j;
                        orientation.left.k = -orientation.left.k;
                    }
                    if (OFS(def, 0x8c, int32_t) != -1) {
                        uint8_t *tag = TAG_DATA(OFS(def, 0x8c, uint32_t));
                        real_point3d negated;
                        negated.x = -OFS(tag, 0xc, float);
                        negated.y = -OFS(tag, 0x10, float);
                        negated.z = -OFS(tag, 0x14, float);
                        matrix4x3_set_translation_only(&offset, &negated);
                        halo::math::globals().matrix4x3_multiply_procedure(&orientation, &offset, &orientation);
                    }
                    matrix4x3_set_translation_only(&offset, &OFS(def, 0x14, real_point3d));
                    halo::math::globals().matrix4x3_multiply_procedure(&orientation, &offset, &orientation);

                    if (base != 0) {
                        if (base->scale != 1.0f) {
                            world.position.x *= base->scale;
                            world.position.y *= base->scale;
                            world.position.z *= base->scale;
                            parent_copy = *base;
                            parent_copy.scale = 1.0f;
                            base = &parent_copy;
                        }
                        {
                            uint8_t *parent_object = (uint8_t *)((object_header *)object_data->data)
                                [OFS(obj, 0x11c, uint32_t) & 0xffff].data;
                            if ((OFS(parent_object, 0x10, uint32_t) & 0x1000) != 0) {
                                if (base != &parent_copy) {
                                    parent_copy = *base;
                                    base = &parent_copy;
                                }
                                base->left.i = -base->left.i;
                                base->left.j = -base->left.j;
                                base->left.k = -base->left.k;
                            }
                        }
                        halo::math::globals().matrix4x3_multiply_procedure(base, &world, &nodes[0]);
                        halo::math::globals().matrix4x3_multiply_procedure(&nodes[0], &orientation, &nodes[0]);
                        halo::math::globals().matrix4x3_multiply_procedure(&nodes[0], &root, &nodes[0]);
                    } else {
                        halo::math::globals().matrix4x3_multiply_procedure(&world, &orientation, &nodes[0]);
                        halo::math::globals().matrix4x3_multiply_procedure(&nodes[0], &root, &nodes[0]);
                    }
                }
            } else {
                real_matrix4x3 *m = &nodes[node_index];
                halo::math::matrix4x3_from_quaternion(orientations[node_index].rotation, *m);
                m->scale = orientations[node_index].scale;
                m->position = orientations[node_index].translation;
                halo::math::globals().matrix4x3_multiply_procedure(&nodes[OFS(node, 0x24, int16_t)], m, m);
            }

            if (OFS(node, 0x20, int16_t) != -1) {
                queue[tail++] = OFS(node, 0x20, int16_t);
            }
            if (OFS(node, 0x22, int16_t) != -1) {
                queue[tail++] = OFS(node, 0x22, int16_t);
            }
        } while (head != tail);
    }

    halo::math::matrix4x3_transform_point(OFS(obj, 0xa0, real_point3d), OFS(def, 0x8, real_point3d), nodes[0]);
    OFS(obj, 0xac, float) = OFS(def, 0x4, float);
    if (OFS(obj, 0xb0, float) > 0.0f) {
        OFS(obj, 0xac, float) = OFS(def, 0x4, float) * OFS(obj, 0xb0, float);
    }
}
#undef OFS
#undef TAG_DATA

namespace {
static float clamp_unit(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}
}

/**
 * Initialises the object's change colours from the tag, clamping and blending as configured.
 *
 * Original register convention: EAX -> object_index, stack -> colors (cdecl).
 *
 * @address 0x004f8b70
 */
void halo::objects::ObjectUpdater::initialize_change_colors(ColorRGB *colors)
{
    uint32_t object_index = handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)obj & 0xffff].data;
    float *position = (float *)(obj + 0x5c);
    int32_t i;

    for (i = 0; i < 4; i++) {
        ColorRGB *working = (ColorRGB *)(obj + 0x188 + i * 0xc);
        ColorRGB *final_color = (ColorRGB *)(obj + 0x1b8 + i * 0xc);

        *working = colors[i];
        if (i < *(int32_t *)&((Object *)tag)->change_colors.count) {
            uint8_t *change_color = *(uint8_t **)&((Object *)tag)->change_colors.pointer + i * 0x2c;

            double seed = (double)position[2] * (double)744.12415f + (double)position[0] * (double)315.89313f +
                (double)position[1] * (double)587.12946f + (double)i * (double)431.12894f;
            float weight = (float)fmod(fabs(seed), 1.0);
            int32_t count = *(int32_t *)(change_color + 0x20);
            int16_t p;

            for (p = 0; p < count; p++) {
                uint8_t *permutation = *(uint8_t **)(change_color + 0x24) + p * 0x1c;

                if (weight <= *(float *)permutation) {
                    float t = (float)fmod(fabs(position[1]) + (double)i * (double)0.71210998f, 1.0);

                    halo::bitmaps::color_interpolate((ColorRGB *)(permutation + 0x10), (ColorRGB *)(permutation + 4), working, static_cast<color_interpolation_flags>(1), t);
                    break;
                }
            }
        }
        final_color->red = clamp_unit(working->red);
        final_color->green = clamp_unit(working->green);
        final_color->blue = clamp_unit(working->blue);
    }
}

/**
 * Finds the permutation group of a model region matching a group number.
 *
 * @address 0x004f8d80
 */
int16_t halo::objects::ObjectUpdater::permutation_find_matching_group(ModelRegion *region, int16_t group,
    int16_t *out)
{
    int16_t count = 0;
    int16_t i;

    for (i = 0; (int32_t)i < (int32_t)region->permutations.count; i++) {
        ModelRegionPermutation *perm = (ModelRegionPermutation *)region->permutations.pointer + i;
        if ((perm->flags & 1) == 0) {

            if (((int16_t)perm->permutation_number == group) ||
                ((group == -1) && ((int16_t)perm->permutation_number < 100))) {
                out[count] = i;
                count++;
            }
        }
    }

    return count;
}

/**
 * Chooses a permutation for each model region of the object from a probability group.
 *
 * @address 0x004f8dd0
 */
uint8_t halo::objects::ObjectUpdater::regions_initialize_permutations(int16_t group, GBXModel *model)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t all_assigned = 1;
    int16_t region_index;

    for (region_index = 0; region_index < (int16_t)model->regions.count; region_index++) {
        ModelRegion *region = (ModelRegion *)model->regions.pointer + region_index;
        int16_t matches[32];
        int16_t match_count = object_permutation_find_matching_group(region, group, matches);
        int16_t chosen;

        if (match_count == 0) {
            if (group != -1) {
                match_count = object_permutation_find_matching_group(region, 0, matches);
            }
            if (match_count == 0) {
                obj->region_permutations[region_index] = 0;
                all_assigned = 0;
                continue;
            }
        }

        if (match_count == 1) {
            chosen = 0;
        } else {
            halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
            chosen = (int16_t)(((int32_t)(halo::math::globals().random_seed_global >> 0x10) * match_count) >> 0x10);
        }
        obj->region_permutations[region_index] = (uint8_t)matches[chosen];
    }

    return all_assigned;
}

/**
 * Returns the first permutation probability group of a model's regions.
 *
 * @address 0x004f8ef0
 */
int16_t halo::objects::ObjectUpdater::get_first_region_probability_group(GBXModel *model)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    int16_t group = 0;
    int16_t region_index;

    for (region_index = 0; (group == 0) && (region_index < (int16_t)model->regions.count); region_index++) {
        ModelRegion *region = (ModelRegion *)model->regions.pointer + region_index;
        uint8_t active_permutation = obj->region_permutations[region_index];
        if (active_permutation < (int16_t)region->permutations.count) {
            ModelRegionPermutation *perm = (ModelRegionPermutation *)region->permutations.pointer + active_permutation;
            group = perm->permutation_number;
        }
    }

    return group;
}

/**
 * Re-applies the object's region permutations.
 *
 * Original register convention: object index in EBX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f8f5f mov
 * eax,ebx at entry with no stack access at all. // blam-cc: EBX -> object_index.
 *
 * @address 0x004f8f50
 */
void halo::objects::ObjectUpdater::refresh_region_permutations()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;

    if (definition->model.tag_id.index != 0xffff) {
        GBXModel *model = (GBXModel *)halo::cache::globals().tag_instances[definition->model.tag_id.index & 0xffff].data;
        int16_t *cached_group = (int16_t *)((uint8_t *)obj + 0xbe);

        if ((*cached_group <= 0) || (object_regions_initialize_permutations(object_index, *cached_group, model) == 0)) {
            int16_t new_group;
            object_regions_initialize_permutations(object_index, -1, model);
            new_group = object_get_first_region_probability_group(object_index, model);
            *cached_group = new_group;
            if (new_group > 0) {
                object_regions_initialize_permutations(object_index, new_group, model);
            }
        }
    }
}

/**
 * Updates the object's change colours for the tick.
 *
 * @address 0x004f9110
 */
void halo::objects::ObjectUpdater::update_change_colors()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;

    if ((definition->scales_change_colors & 1) != 0) {
        int32_t count = definition->change_colors.count;
        int32_t i;

        for (i = 0; i < count; i++) {
            ObjectChangeColors *tag_color = (ObjectChangeColors *)((uint8_t *)definition->change_colors.pointer + i * 0x2c);
            ColorRGB *out = &obj->change_colors[i];
            int c;

            if (tag_color->scale_by != 0) {
                float t = *(float *)((uint8_t *)obj + 0x120 + tag_color->scale_by * 4);

                halo::bitmaps::color_interpolate((ColorRGB *)((uint8_t *)tag_color + 0x14), (ColorRGB *)((uint8_t *)tag_color + 8), out,
                    static_cast<color_interpolation_flags>(*(uint32_t *)&((struct ObjectChangeColors *)tag_color)->flags), t);
            }
            if (tag_color->darken_by != 0) {
                float scale = *(float *)((uint8_t *)obj + 0x120 + tag_color->darken_by * 4);
                out->red *= scale;
                out->green *= scale;
                out->blue *= scale;
            }

            for (c = 0; c < 3; c++) {
                float *component = &out->red + c;
                if (*component < 0.0f) {
                    *component = 0.0f;
                } else if (*component > 1.0f) {
                    *component = 1.0f;
                }
            }
        }
    }
}

namespace {
static float function_scale_input(uint8_t *obj, int16_t selector)
{
    return *(float *)(obj + 0x120 + selector * 4);
}
}

/**
 * Evaluates the object's function inputs and updates its node function blocks.
 *
 * Original register convention: object index in EAX. Consistent with every other single-register accessor in this
 * module and with this function's own Ghidra signature ("in_EAX" only). // blam-cc: EAX -> object_index.
 *
 * @address 0x004f92f0
 */
void halo::objects::ObjectUpdater::update_functions()
{
    uint32_t object_index = handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[*(datum_index *)obj & 0xffff].data;
    float phase = (float)(int32_t)((object_index & 0xffff) * 0x39 + game_time->game_time) * 0.033333335f;
    int16_t i;

    for (i = 0; i < (int32_t)definition->functions.count; i++) {
        ObjectFunction *fn = (ObjectFunction *)((uint8_t *)definition->functions.pointer + i * 0x168);
        float period = fn->inverse_period;
        float value;
        uint8_t valid = 1;

        if (fn->scale_period_by != 0) {
            float scale = function_scale_input(obj, fn->scale_period_by);
            if (scale > 0.0f) {
                period = period / scale;
            }
        }
        value = halo::math::periodic_function_evaluate(fn->function, (double)(period * phase));
        if (fn->scale_function_by != 0) {
            value = function_scale_input(obj, fn->scale_function_by) * value;
        }
        if (fn->flags & 1) {
            value = 1.0f - value;
        }
        if (fn->wobble_magnitude != 0.0f) {
            float wobble = halo::math::periodic_function_evaluate(fn->wobble_function, (double)(phase * fn->wobble_period));
            wobble = (wobble - 0.5f) * fn->wobble_magnitude;
            value = wobble + wobble + value;
        }
        if (fn->square_wave_threshold != 0.0f) {
            value = value > fn->square_wave_threshold ? 1.0f : 0.0f;
        }
        if (fn->step_count > 1) {
            value = (float)(floor((double)((float)fn->step_count * value)) * fn->inverse_step);
        }
        if (fn->inverse_sawtooth > 0.0f) {
            value = (float)fmod((double)value, (double)fn->inverse_sawtooth);
        }
        if (fn->add != 0) {
            value = function_scale_input(obj, fn->add) + value;
            if (value > 1.0f) {
                value = 1.0f;
            }
        }
        if (fn->scale_result_by != 0) {
            value = function_scale_input(obj, fn->scale_result_by) * value;
        }
        value = halo::math::transition_function_evaluate(fn->map_to, value);
        if (fn->scale_by > 0.0f) {
            value = value * fn->scale_by;
        }
        if (fn->bounds_mode == 2) {
            value = (fn->bounds[1] - fn->bounds[0]) * value + fn->bounds[0];
            if (fn->bounds[0] + 0.0001f >= value) {
                valid = (uint8_t)((fn->flags >> 2) & 1);
            }
        } else {
            if (fn->bounds[0] + 0.0001f >= value) {
                valid = (uint8_t)((fn->flags >> 2) & 1);
                value = fn->bounds[0];
            }
            if (value > fn->bounds[1]) {
                value = fn->bounds[1];
            }
            if (fn->bounds_mode == 1) {
                value = (value - fn->bounds[0]) * fn->inverse_bounds;
            }
        }
        if (fn->turn_off_with != -1 &&
            (obj[0x123] & (uint8_t)(1u << (fn->turn_off_with & 0x1f))) == 0) {
            valid = 0;
        }
        if (fn->flags & 2) {
            value = (float)fmod((double)(value + *(float *)(obj + 0x134 + i * 4)), 1.0);
        }
        *(float *)(obj + 0x134 + i * 4) = value;
        if (valid) {
            obj[0x123] = (uint8_t)(obj[0x123] | (1u << i));
        } else {
            obj[0x123] = (uint8_t)(obj[0x123] & ~(1u << i));
        }
    }
}

/**
 * Raises value to exponent with the engine's input conventions and returns the result; used when evaluating object
 * function curves.
 *
 * Original register convention: two float stack parameters (Ghidra shows them cleanly).
 *
 * @address 0x004fea50
 */
float halo::objects::ObjectUpdater::curve_apply_exponent(float value, float exponent)
{
    if (exponent != 1.0f) {
        return (float)pow((double)value, (double)exponent);
    }
    return value;
}
