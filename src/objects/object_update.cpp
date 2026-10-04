#include <stdio.h>
#include "halo/networking/game_mode.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/objects/object_update.hpp"
#include "halo/tags/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/core/lcg.hpp"
#include "halo/models/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "game.h"
#include "models.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/models/models.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"
#include "halo/ai/api.hpp"
namespace {
/** The function input (selectors 1..4) or output (5..8) value an object function selector names; they are adjacent in the record. */
float &function_slot(object &record, int16_t selector)
{
    return selector <= 4 ? record.function_in_values[selector - 1] : record.function_out_values[selector - 5];
}
}


static auto &global_origin3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
static auto &object_globals_pointer = halo::link::ref<object_globals *>(halo::objects::vars().object_globals_pointer);

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
    object *obj = headers[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    ModelCollisionGeometry *geometry =
        (ModelCollisionGeometry *)halo::cache::globals().tag_instances[definition->collision_model.tag_id.index].data;
    ModelCollisionGeometryRegion *regions = (ModelCollisionGeometryRegion *)geometry->regions.pointer;
    int32_t region_count = (int32_t)geometry->regions.count;
    int32_t region_index;

    for (region_index = 0; region_index < region_count; region_index++) {
        ModelCollisionGeometryRegion *region = &regions[region_index];

        if (test_flag(region->flags, tags::model_collision_geometry_region_tag_flag::disappears_when_shield_is_off) && (int32_t)region->permutations.count > 1) {
            obj->region_permutations[region_index] = (unlock == 0);
        }
    }
}

/**
 * Selects a named permutation for the object's matching regions.
 *
 * @address 0x004f6c60
 */
void halo::objects::ObjectUpdater::set_permutation_by_name(const char *name, int16_t region_filter, char use_matched_index)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    if (definition->model.tag_id.index == halo::k_word_none) {
        return;
    }

    {
        GBXModel *model = (GBXModel *)halo::cache::globals().tag_instances[halo::datum_slot(definition->model.tag_id.index)].data;
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
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

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

    halo::objects::object_type_definitions_query_0x34(object_index);

    if (definition->collision_model.tag_id.index != halo::k_word_none) {
        halo::objects::object_update_vitality_and_regeneration(object_index);
    }

    halo::objects::object_type_definitions_notify_0x38(object_index);

    if (!test_flag(obj->flags, objects::object_flag::do_not_recompute_node_matrices)) {
        halo::objects::object_recalculate_bounding_radius(object_index);
    }

    halo::objects::object_update_functions(object_index);
    halo::objects::object_update_change_colors(object_index);

    if ((test_flag(obj->flags, objects::object_flag::dynamic_lighting_recompute)) &&
        (((obj->flags & _object_no_collision_bit) == 0) || (definition->model.tag_id.index == halo::k_word_none))) {
        halo::objects::object_for_each_light_attachment(object_index, 1, 1);
    }

    if (obj->first_child_object != k_datum_index_none) {
        halo::objects::object_update(obj->first_child_object);
    }
    if ((obj->parent_object != k_datum_index_none) && (obj->next_object != k_datum_index_none)) {
        halo::objects::object_update(obj->next_object);
    }

    halo::objects::object_notify_node_array_if_animated(object_index);

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        uint8_t *at_rest_flag = &obj->at_rest;
        if ((halo::x87::fabsf(obj->velocity.i - global_origin3d_pointer->i) < 0.0001f) &&
            (halo::x87::fabsf(obj->velocity.j - global_origin3d_pointer->j) < 0.0001f) &&
            (halo::x87::fabsf(obj->velocity.k - global_origin3d_pointer->k) < 0.0001f) &&
            (halo::x87::fabsf(obj->angular_velocity.i - global_origin3d_pointer->i) < 0.0001f) &&
            (halo::x87::fabsf(obj->angular_velocity.j - global_origin3d_pointer->j) < 0.0001f) &&
            (halo::x87::fabsf(obj->angular_velocity.k - global_origin3d_pointer->k) < 0.0001f)) {
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
    object *object = halo::objects::object_as<struct object>(object_index);
    Object *definition = halo::objects::tag_as<Object>(object->definition_tag);
    const int16_t sources[4] = {definition->a_in, definition->b_in, definition->c_in, definition->d_in};
    int32_t i;

    for (i = 0; i < 4; i++) {
        int16_t source = sources[i];
        float *output = &object->function_in_values[i];
        float value = 0.0f;

        if (source == 0) {
            continue;
        }
        switch (source) {
        case 1: value = object->body_vitality; break;
        case 2: value = clamp_to_one(object->shield_vitality); break;
        case 3: value = object->current_body_damage; break;
        case 4: value = object->current_shield_damage; break;
        case 5:
            if (*output == 1.0f) {
                value = halo::math::random_real();
            }
            break;
        case 18: value = test_flag(object->vitality_flags, objects::vitality_flag::health_frozen) ? 0.0f : 1.0f; break;
        case 19: {
            const real_vector3d &forward = halo::objects::object_block<real_matrix4x3>(*object, object->nodes)[0].forward;

            if (!(halo::libm::fabs(forward.k) < 0.995)) {
                value = *output;
            } else {
                float yaw = (float)halo::x87::fpatan(forward.i, forward.j);
                value = halo::game::angle_delta_wrapped(halo::scenario::globals().scenario->local_north, yaw) * 0.15915494f + 0.5f;
                value = value >= 0.0f ? clamp_to_one(value) : 0.0f;
            }
            break;
        }
        default:
            value = (float)object->region_vitality[source - 10] * 0.0039215689f;
            break;
        }
        *output = value;
    }
}

/**
 * Evaluates one object function input selector, with the input angle and the object's tag data.
 *
 * @address 0x004f8207
 */
void halo::objects::ObjectUpdater::function_evaluate_input(float initial_angle_input, float initial_st0,
    int16_t *selectors, float *out_values, object *object_record, int32_t object_index_scaled,
    int32_t remaining_count)
{
    float value = halo::game::angle_delta_wrapped(initial_angle_input, initial_st0);

    value = value * 0.15915494f + 0.5f;
    if (value < 0.0f) {
        value = 0.0f;
    }

    for (;;) {
        int16_t selector;

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
            value = object_record->body_vitality;
            continue;
        case 2:
            value = object_record->shield_vitality;
            if (value <= 1.0f) {
                continue;
            }
            break;
        case 3:
            value = object_record->current_body_damage;
            continue;
        case 4:
            value = object_record->current_shield_damage;
            continue;
        case 5:
            if (*out_values == 1.0f) {
                value = halo::math::random_real();
            }
            continue;
        case 0x12:
            if (test_flag(object_record->vitality_flags, vitality_flag::health_frozen)) {
                value = 0.0f;
                continue;
            }
            break;
        case 0x13: {
            object *obj = *(object **)((uint8_t *)object_data->data + 8 + object_index_scaled);
            real_matrix4x3 *node = halo::objects::object_block<real_matrix4x3>(*obj, obj->nodes);

            if (0.995f <= halo::x87::fabsf(node->left.i)) {
                value = *out_values;
                continue;
            }
            initial_st0 = (float)halo::libm::atan2((double)node->forward.j, (double)node->forward.k);
            initial_angle_input = halo::scenario::globals().scenario->local_north;
            value = halo::game::angle_delta_wrapped(initial_angle_input, initial_st0);
            value = value * 0.15915494f + 0.5f;
            if (!(value >= 0.0f)) {
                value = 0.0f;
            }
            continue;
        }
        default:
            value = (float)object_record->region_vitality[(int16_t)(selector - 10)] * 0.003921569f;
            continue;
        }
        value = 1.0f;
    }
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    datum_index child;

    halo::objects::object_recalculate_bounding_radius(object_index);

    child = obj->first_child_object;
    while (child != k_datum_index_none) {
        object *child_obj = ((object_header *)object_data->data)[halo::datum_slot(child)].data;
        halo::objects::object_recalculate_bounding_radius_recursive(child);
        child = child_obj->next_object;
    }
}

namespace {
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
    uint8_t *obj = halo::objects::object_record_bytes(object_index);
    Object *def = halo::objects::tag_as<Object>(((struct object *)obj)->definition_tag);
    real_matrix4x3 *nodes = (real_matrix4x3 *)(obj + ((struct object *)obj)->nodes.offset);
    real_orientation local_orientations[k_maximum_nodes_per_model];
    real_orientation *orientations;

    if (((1u << ((uint8_t)((struct object *)obj)->type & 0x1f)) & 0xfe0u) != 0) {
        orientations = local_orientations;
    } else {
        orientations = (real_orientation *)(obj + ((struct object *)obj)->node_function_defaults.offset);
    }

    if ((int32_t)halo::objects::tag_handle(def->model) == -1) {

        nodes[0].scale = 1.0f;
        nodes[0].forward = ((struct object *)obj)->forward;
        nodes[0].up = ((struct object *)obj)->up;
        halo::math::vector3d_cross_product(nodes[0].left, nodes[0].forward, nodes[0].up);
        nodes[0].position = ((struct object *)obj)->position;
    } else {
        GBXModel *model = halo::objects::tag_as<GBXModel>(halo::objects::tag_handle(def->model));
        real_matrix4x3 *parent_matrix = 0;
        uint8_t absolute_root = 0;
        int16_t queue[k_maximum_nodes_per_model];
        int16_t head, tail;

        if ((int32_t)((struct object *)obj)->parent_object != -1) {
            uint8_t *parent = halo::objects::object_record_bytes(((struct object *)obj)->parent_object);
            parent_matrix = (real_matrix4x3 *)(parent + ((struct object *)parent)->nodes.offset) + (int8_t)((struct object *)obj)->parent_marker_index;
        }

        if ((int32_t)((struct object *)obj)->animation_graph != -1 && ((struct object *)obj)->animation_index != -1) {
            ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(*(uint8_t * *)((uint8_t *)(halo::objects::tag_record_bytes(((struct object *)obj)->animation_graph)) + 0x78) +
                (int32_t)((struct object *)obj)->animation_index * 0xb4);
            int16_t frame_count = (int16_t)animation->frame_count;
            uint32_t frame;
            if (test_flag(((struct object *)obj)->flags, objects::object_flag::animates_automatically) && frame_count > 0) {
                frame = ((uint32_t)halo::game::globals().game_time->game_time + object_index) % (uint32_t)(int32_t)frame_count;
            } else {
                frame = (uint16_t)((struct object *)obj)->animation_frame;
            }
            halo::models::animation_view(animation).get_frame_orientations((GBXModel *)model, (int16_t)frame, orientations);
            absolute_root = (uint8_t)(((uint8_t)animation->flags >> 1) & 1);
        } else {
            halo::models::model_view((GBXModel *)model).get_default_transforms(orientations);
        }

        if ((int32_t)halo::objects::tag_handle(def->animation_graph) != -1) {
            ModelAnimations *graph = halo::objects::tag_as<ModelAnimations>(halo::objects::tag_handle(def->animation_graph));
            int16_t i;
            for (i = 0; (int32_t)i < graph->objects.count; i++) {
                ModelAnimationsAnimationGraphObjectOverlay *entry = &halo::objects::block_element<ModelAnimationsAnimationGraphObjectOverlay>(graph->objects, (int32_t)i);
                if (entry->animation == k_word_none || (int32_t)entry->function >= (int32_t)def->functions.count) {
                    continue;
                }
                {
                    ModelAnimationsAnimation *animation = &halo::objects::block_element<ModelAnimationsAnimation>(graph->animations, (int32_t)entry->animation);
                    float value = ((struct object *)obj)->function_out_values[entry->function];
                    if (entry->function_controls == 0) {
                        int32_t frames = (int32_t)(int16_t)animation->frame_count;
                        if (!test_flag(halo::objects::block_element<ObjectFunction>(def->functions, entry->function).flags, tags::object_function_tag_flag::additive)) {
                            frames -= 1;
                        }
                        halo::models::animation_view(animation).overlay_interpolated_frame_orientations((float)frames * value, orientations);
                    } else if (entry->function_controls == 1) {
                        uint32_t frame = ((uint32_t)halo::game::globals().game_time->game_time + object_index) %
                            (uint32_t)(int32_t)(int16_t)animation->frame_count;
                        halo::models::animation_view(animation).overlay_frame_orientations_weighted((int16_t)frame, value, orientations);
                    }
                }
            }
        }

        if (((struct object *)obj)->scale > 0.0f) {
            float scale = ((struct object *)obj)->scale;
            orientations[0].scale *= scale;
            orientations[0].translation.x *= scale;
            orientations[0].translation.y *= scale;
            orientations[0].translation.z *= scale;
        }
        if ((int32_t)halo::objects::tag_handle(def->animation_graph) != -1) {
            halo::objects::object_type_definitions_notify_two_args_0x48(object_index, (uint32_t)orientations);
        }
        if (((struct object *)obj)->node_function_count > 0) {

            halo::models::model_skeleton::blend_transforms(orientations, (int16_t)model->nodes.count, (real_orientation *)(obj + ((struct object *)obj)->node_function_values.offset), (int16_t)(uint16_t)((struct object *)obj)->interpolation_frame_index, (int16_t)(uint16_t)((struct object *)obj)->node_function_count);
        }

        queue[0] = 0;
        head = 0;
        tail = 1;
        do {
            int16_t node_index = queue[head++];
            ModelNode *node = &halo::objects::block_element<ModelNode>(model->nodes, (int32_t)node_index);

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

                    matrix4x3_set_translation_only(&world, &((struct object *)obj)->position);
                    halo::math::matrix4x3_from_forward_up(((struct object *)obj)->up, ((struct object *)obj)->forward, orientation);
                    if (test_flag(((struct object *)obj)->flags, objects::object_flag::mirrored_geometry)) {
                        orientation.left.i = -orientation.left.i;
                        orientation.left.j = -orientation.left.j;
                        orientation.left.k = -orientation.left.k;
                    }
                    if ((int32_t)halo::objects::tag_handle(def->physics) != -1) {
                        Physics *tag = halo::objects::tag_as<Physics>(halo::objects::tag_handle(def->physics));
                        real_point3d negated;
                        negated.x = -tag->center_of_mass.x;
                        negated.y = -tag->center_of_mass.y;
                        negated.z = -tag->center_of_mass.z;
                        matrix4x3_set_translation_only(&offset, &negated);
                        halo::math::globals().matrix4x3_multiply_procedure(&orientation, &offset, &orientation);
                    }
                    matrix4x3_set_translation_only(&offset, reinterpret_cast<const real_point3d *>(&def->origin_offset));
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
                            struct object *parent_object = ((object_header *)object_data->data)
                                [halo::datum_slot(((struct object *)obj)->parent_object)].data;
                            if (test_flag(parent_object->flags, objects::object_flag::mirrored_geometry)) {
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
                halo::math::globals().matrix4x3_multiply_procedure(&nodes[node->parent_node_index], m, m);
            }

            if (node->next_sibling_node_index != halo::k_word_none) {
                queue[tail++] = node->next_sibling_node_index;
            }
            if (node->first_child_node_index != halo::k_word_none) {
                queue[tail++] = node->first_child_node_index;
            }
        } while (head != tail);
    }

    halo::math::matrix4x3_transform_point(((struct object *)obj)->bounding_center, *reinterpret_cast<const real_point3d *>(&def->bounding_offset), nodes[0]);
    ((struct object *)obj)->bounding_radius = def->bounding_radius;
    if (((struct object *)obj)->scale > 0.0f) {
        ((struct object *)obj)->bounding_radius = def->bounding_radius * ((struct object *)obj)->scale;
    }
}

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
    uint8_t *obj = halo::objects::object_record_bytes(object_index);
    Object *tag = halo::objects::tag_as<Object>(*(datum_index *)obj);
    const real_point3d &position = ((struct object *)obj)->position;
    int32_t i;

    for (i = 0; i < 4; i++) {
        object &record = *reinterpret_cast<object *>(obj);
        ColorRGB *working = &reinterpret_cast<ColorRGB *>(record.unknown_188)[i];
        ColorRGB *final_color = &record.change_colors[i];

        *working = colors[i];
        if (i < (int32_t)tag->change_colors.count) {
            ObjectChangeColors *change_color = &halo::objects::block_element<ObjectChangeColors>(tag->change_colors, i);

            double seed = (double)position.z * (double)744.12415f + (double)position.x * (double)315.89313f +
                (double)position.y * (double)587.12946f + (double)i * (double)431.12894f;
            float weight = (float)halo::libm::fmod(halo::libm::fabs(seed), 1.0);
            int32_t count = (int32_t)change_color->permutations.count;
            int16_t p;

            for (p = 0; p < count; p++) {
                ObjectChangeColorsPermutation *permutation = &halo::objects::block_element<ObjectChangeColorsPermutation>(change_color->permutations, p);

                if (weight <= permutation->weight) {
                    float t = (float)halo::libm::fmod(halo::libm::fabs(position.y) + (double)i * (double)0.71210998f, 1.0);

                    halo::bitmaps::color_interpolate((ColorRGB *)&permutation->color_upper_bound, (ColorRGB *)&permutation->color_lower_bound, working, (color_interpolation_flags)1, t);
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
        if (!test_flag(perm->flags, tags::model_region_permutation_tag_flag::cannot_be_chosen_randomly)) {

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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    uint8_t all_assigned = 1;
    int16_t region_index;

    for (region_index = 0; region_index < (int16_t)model->regions.count; region_index++) {
        ModelRegion *region = (ModelRegion *)model->regions.pointer + region_index;
        int16_t matches[32];
        int16_t match_count = halo::objects::object_permutation_find_matching_group(region, group, matches);
        int16_t chosen;

        if (match_count == 0) {
            if (group != -1) {
                match_count = halo::objects::object_permutation_find_matching_group(region, 0, matches);
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
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            chosen = (int16_t)(((int32_t)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * match_count) >> 0x10);
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    if (definition->model.tag_id.index != halo::k_word_none) {
        GBXModel *model = (GBXModel *)halo::cache::globals().tag_instances[halo::datum_slot(definition->model.tag_id.index)].data;
        int16_t *cached_group = &obj->permutation_group;

        if ((*cached_group <= 0) || (halo::objects::object_regions_initialize_permutations(object_index, *cached_group, model) == 0)) {
            int16_t new_group;
            halo::objects::object_regions_initialize_permutations(object_index, -1, model);
            new_group = halo::objects::object_get_first_region_probability_group(object_index, model);
            *cached_group = new_group;
            if (new_group > 0) {
                halo::objects::object_regions_initialize_permutations(object_index, new_group, model);
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    if ((definition->scales_change_colors & 1) != 0) {
        int32_t count = definition->change_colors.count;
        int32_t i;

        for (i = 0; i < count; i++) {
            ObjectChangeColors *tag_color = &halo::objects::block_element<ObjectChangeColors>(definition->change_colors, i);
            ColorRGB *out = &obj->change_colors[i];
            int c;

            if (tag_color->scale_by != 0) {
                float t = function_slot(*obj, tag_color->scale_by);

                halo::bitmaps::color_interpolate(&tag_color->color_upper_bound, &tag_color->color_lower_bound, out,
                    (color_interpolation_flags)tag_color->flags, t);
            }
            if (tag_color->darken_by != 0) {
                float scale = function_slot(*obj, tag_color->darken_by);
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
    return function_slot(*reinterpret_cast<object *>(obj), selector);
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
    uint8_t *obj = halo::objects::object_record_bytes(object_index);
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    float phase = (float)(int32_t)(halo::datum_slot(object_index) * 0x39 + halo::game::globals().game_time->game_time) * 0.033333335f;
    int16_t i;

    for (i = 0; i < (int32_t)definition->functions.count; i++) {
        ObjectFunction *fn = &halo::objects::block_element<ObjectFunction>(definition->functions, i);
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
            value = (float)(halo::libm::floor((double)((float)fn->step_count * value)) * fn->inverse_step);
        }
        if (fn->inverse_sawtooth > 0.0f) {
            value = (float)halo::libm::fmod((double)value, (double)fn->inverse_sawtooth);
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
            (((struct object *)obj)->function_valid_flags & (uint8_t)(1u << (fn->turn_off_with & 0x1f))) == 0) {
            valid = 0;
        }
        if (fn->flags & 2) {
            value = (float)halo::libm::fmod((double)(value + reinterpret_cast<object *>(obj)->function_out_values[i]), 1.0);
        }
        reinterpret_cast<object *>(obj)->function_out_values[i] = value;
        if (valid) {
            ((struct object *)obj)->function_valid_flags = (uint8_t)(((struct object *)obj)->function_valid_flags | (1u << i));
        } else {
            ((struct object *)obj)->function_valid_flags = (uint8_t)(((struct object *)obj)->function_valid_flags & ~(1u << i));
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
        return (float)halo::libm::pow((double)value, (double)exponent);
    }
    return value;
}
