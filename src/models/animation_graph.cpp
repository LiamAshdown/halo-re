/**
 * @file src/models/animation_graph.cpp
 * Animation graph level helpers: state advance, lookup, keyframe search, quaternion codecs and node matrices.
 * The original author notes and decompiles are in docs/original/models/.
 */

#include "halo/models/models.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"


namespace halo::models {

animation_state_advance_result animation_graph::state_advance(uint32_t animation_graph_tag_index, animation_state *state, int32_t *sound_tag_id, animation_random_stream random_stream)
{
    ModelAnimations *graph;
    ModelAnimationsAnimation *animation;
    ModelAnimationsAnimationGraphSoundReference *sound_references;
    int16_t frame_index;
    int16_t frame_count;
    int16_t loop_frame_index;
    int16_t clamped_loop_frame;

    graph = (ModelAnimations *)halo::cache::globals().tag_instances[animation_graph_tag_index & 0xffff].data;
    animation = (ModelAnimationsAnimation *)((uint8_t *)graph->animations.pointer +
                                              state->animation_index * (int)sizeof(ModelAnimationsAnimation));

    if (sound_tag_id != 0) {
        if ((int16_t)animation->sound == -1 || (int16_t)animation->sound_frame_index != state->frame_index) {
            *sound_tag_id = -1;
        } else {
            sound_references = (ModelAnimationsAnimationGraphSoundReference *)graph->sound_references.pointer;
            *sound_tag_id = *(int32_t *)&sound_references[(int16_t)animation->sound].sound.tag_id;
        }
    }

    state->frame_index = state->frame_index + 1;
    frame_index = state->frame_index;
    frame_count = animation->frame_count;
    if (frame_count <= frame_index) {
        loop_frame_index = animation->loop_frame_index;
        if (0 < loop_frame_index) {
            clamped_loop_frame = frame_count - 1;
            if (loop_frame_index <= frame_count - 1) {
                clamped_loop_frame = loop_frame_index;
            }
            state->frame_index = clamped_loop_frame;
            return _animation_advance_looped;
        }
        state->animation_index = animation_graph::choose_random_permutation(animation_graph_tag_index, (int16_t)animation->main_animation_index, random_stream);
        state->frame_index = 0;
        return _animation_advance_next_animation;
    }
    if (frame_index + 1 == frame_count && animation->loop_frame_index == 0) {
        return _animation_advance_last_frame;
    }
    if (frame_index != (int16_t)animation->key_frame_index && frame_index != (int16_t)animation->second_key_frame_index) {
        return _animation_advance_none;
    }
    return _animation_advance_key_frame;
}

int16_t animation_graph::choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation, animation_random_stream stream)
{
    ModelAnimations *graph;
    ModelAnimationsAnimation *animations;
    random_seed seed;
    real threshold;
    int16_t animation;

    graph = (ModelAnimations *)halo::cache::globals().tag_instances[animation_graph_tag & 0xffff].data;
    animations = (ModelAnimationsAnimation *)graph->animations.pointer;

    if (stream == _animation_random_global) {
        halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * k_random_multiplier + k_random_increment;
        seed = halo::math::globals().random_seed_global;
    } else {
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        seed = halo::math::globals().effect_random_seed;
    }
    threshold = (real)(seed >> k_random_value_shift) * 1.5259022e-05f;

    animation = first_animation;
    while (animation != -1) {
        if (threshold <= animations[animation].relative_weight) {
            break;
        }
        animation = (int16_t)animations[animation].next_animation;
    }
    return animation;
}

int16_t animation_graph::find_animation_by_name(datum_index animation_graph_tag, const char *name)
{
    ModelAnimations *graph;
    ModelAnimationsAnimation *animations;
    int16_t i;

    graph = (ModelAnimations *)halo::cache::globals().tag_instances[animation_graph_tag & 0xffff].data;
    animations = (ModelAnimationsAnimation *)graph->animations.pointer;

    for (i = 0; (int32_t)i < graph->animations.count; i++) {
        if (_stricmp(name, (const char *)&animations[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int16_t animation_graph::keyframe_time_search(uint16_t *times, int16_t count, int16_t frame)
{
    int16_t lo, hi, mid, saved_lo;

    lo = 0;
    hi = (int16_t)(count - 1);
    for (;;) {
        saved_lo = lo;
        mid = (int16_t)((hi + saved_lo) >> 1);
        if ((mid + 1 < count) && ((int16_t)times[mid + 1] <= frame)) {
            lo = mid;
            continue;
        }
        hi = mid;
        lo = saved_lo;
        if ((int16_t)times[mid] <= frame) {
            return mid;
        }
    }
}

void animation_graph::quaternion16_decode(int16_t *source, real_quaternion *out)
{
    out->i = (real)source[0] * 3.051851e-05f;
    out->j = (real)source[1] * 3.051851e-05f;
    out->k = (real)source[2] * 3.051851e-05f;
    out->w = (real)source[3] * 3.051851e-05f;
}

void animation_graph::quaternion48_decode(animation_quaternion48 *source, real_quaternion *out)
{
    uint16_t w0, w1, w2;

    w0 = source->packed[0];
    w1 = source->packed[1];
    w2 = source->packed[2];

    out->i = (real)(int16_t)((w0 >> 0xc) | (w0 & 0xfff0)) * 3.051851e-05f;
    out->j = (real)(int16_t)(((w1 >> 4) & 0xff0) | (w0 & 0xf) | (w0 << 0xc)) * 3.051851e-05f;
    out->k = (real)(int16_t)((((w2 >> 4) & 0xf00) | (w1 & 0xf0)) >> 4 | (w1 << 8)) * 3.051851e-05f;
    out->w = (real)(int16_t)((w2 >> 8 & 0xf) | (w2 << 4)) * 3.051851e-05f;
}

void animation_graph::get_root_node_matrix(real_matrix4x3 *out, int16_t frame, ModelAnimationsAnimation *animation, GBXModel *model)
{
    real_orientation orientations[k_maximum_nodes_per_model];

    animation_view(animation).get_frame_orientations(model, frame, orientations);
    halo::math::matrix4x3_from_quaternion(orientations[0].rotation, *out);
    out->position = orientations[0].translation;
}

void animation_graph::get_frame_delta(int16_t frame, ModelAnimationsAnimation *animation, real_vector3d *out, GBXModel *model)
{
    real_orientation current_frame[k_maximum_nodes_per_model];
    real_orientation previous_frame[k_maximum_nodes_per_model];
    int16_t use_frame;

    use_frame = (frame == 0) ? 1 : frame;
    animation_view(animation).get_frame_orientations(model, use_frame, current_frame);
    animation_view(animation).get_frame_orientations(model, (int16_t)(use_frame - 1), previous_frame);
    out->i = current_frame[0].translation.x - previous_frame[0].translation.x;
    out->j = current_frame[0].translation.y - previous_frame[0].translation.y;
    out->k = current_frame[0].translation.z - previous_frame[0].translation.z;
}

void animation_graph::nodes_build_matrices(datum_index animation_graph_tag, real_point3d *root_position, real_matrix4x3 *out_matrices, real_orientation *orientations, real_vector3d *forward, real_vector3d *up)
{
    ModelAnimations *graph;
    real_matrix4x3 root_parent;
    int16_t queue[k_maximum_nodes_per_model];
    int16_t read_index, write_index;

    graph = (ModelAnimations *)halo::cache::globals().tag_instances[animation_graph_tag & 0xffff].data;

    halo::math::matrix4x3_from_forward_up(*up, *forward, root_parent);
    root_parent.position = *root_position;

    if (graph->nodes.count <= 0) {
        return;
    }

    read_index = 0;
    write_index = 1;
    queue[0] = 0;
    do {
        ModelAnimationsAnimationGraphNode *node_def;
        real_matrix4x3 *parent_matrix;
        real_matrix4x3 local_matrix;
        int16_t node;

        node = queue[read_index];
        read_index = read_index + 1;
        node_def = (ModelAnimationsAnimationGraphNode *)((uint8_t *)graph->nodes.pointer + node * sizeof(ModelAnimationsAnimationGraphNode));

        parent_matrix = (node == 0) ? &root_parent : &out_matrices[(int16_t)node_def->parent_node_index];

        halo::math::matrix4x3_from_quaternion(orientations[node].rotation, local_matrix);
        local_matrix.scale = orientations[node].scale;
        local_matrix.position = orientations[node].translation;

        halo::math::globals().matrix4x3_multiply_procedure(parent_matrix, &local_matrix, &out_matrices[node]);

        if (node_def->next_sibling_node_index != 0xffff) {
            queue[write_index] = (int16_t)node_def->next_sibling_node_index;
            write_index = write_index + 1;
        }
        if (node_def->first_child_node_index != 0xffff) {
            queue[write_index] = (int16_t)node_def->first_child_node_index;
            write_index = write_index + 1;
        }
    } while (read_index != write_index);
}

}  // namespace halo::models
