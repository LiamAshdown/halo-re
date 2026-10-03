/**
 * @file src/models/animation_view.cpp
 * Sampling and blending of model animation frames (base, overlay and replacement animations).
 * The original author notes and decompiles are in docs/original/models/.
 */

#include "halo/core/crt.hpp"
#include "halo/models/models.hpp"
#include "halo/math/api.hpp"
#include "halo/models/globals.hpp"


namespace halo::models {

void * animation_view::get_frame_data(int16_t frame)
{
    int use_compressed;

    use_compressed = ((self->flags & 1) != 0) && (globals().animation_compressed_data_enabled != 0);
    if (use_compressed) {
        return (uint8_t *)self->frame_data.pointer + self->offset_to_compressed_data;
    }

    return (uint8_t *)self->frame_data.pointer + (int32_t)(int16_t)self->frame_size * (int32_t)frame;
}

void animation_view::get_frame_info_distance(float *dx_to_key_frame, float *dx_total)
{
    int16_t frame_info_type;
    int16_t frame;
    float total;
    float value_at_key_frame;
    float *frame_info;

    total = 0.0f;
    value_at_key_frame = 0.0f;
    frame_info = (float *)self->frame_info.pointer;
    frame = 0;

    if (0 < (int16_t)self->frame_count) {
        frame_info_type = self->frame_info_type;
        do {
            if (frame_info_type == 1) {
                total = total + frame_info[0];
                frame_info += 2;
            } else if (frame_info_type == 2) {
                total = total + frame_info[0];
                frame_info += 3;
            } else if (frame_info_type == 3) {
                total = total + frame_info[0];
                frame_info += 4;
            }
            if (frame == (int16_t)self->key_frame_index) {
                value_at_key_frame = total;
            }
            frame = frame + 1;
        } while (frame < (int16_t)self->frame_count);
    }
    if (dx_total != 0) {
        *dx_total = total;
    }
    if (dx_to_key_frame != 0) {
        *dx_to_key_frame = value_at_key_frame;
    }
}

void animation_view::get_frame_orientations(GBXModel *model, int16_t frame, real_orientation *out_orientations)
{
    uint8_t *frame_cursor;
    uint8_t *default_cursor;
    int use_compressed_frame_base;
    int use_compressed_codec;
    int16_t node_count;
    int16_t node;
    uint32_t translation_mask;
    uint32_t rotation_mask;
    uint32_t scale_mask;
    int16_t rotation_index;
    int16_t translation_index;
    int16_t scale_index;
    real_orientation *out_node;

    if (self->type != 0 ||
        (model != 0 &&
         (((self->node_list_checksum != 0 && self->node_list_checksum != model->node_list_checksum) &&
           model->node_list_checksum != 0) ||
          model->nodes.count != (int32_t)(int16_t)self->node_count))) {
        model_view(model).get_default_transforms(out_orientations);
        return;
    }

    use_compressed_codec = ((self->flags & 1) != 0) &&
                           !((globals().animation_compressed_data_enabled == 0) && (self->offset_to_compressed_data != 0));
    use_compressed_frame_base = ((self->flags & 1) != 0) && (globals().animation_compressed_data_enabled != 0);

    if (use_compressed_frame_base) {
        frame_cursor = (uint8_t *)self->frame_data.pointer + self->offset_to_compressed_data;
    } else {
        frame_cursor = (uint8_t *)self->frame_data.pointer + (int32_t)(int16_t)self->frame_size * (int32_t)frame;
    }

    default_cursor = (uint8_t *)self->default_data.pointer;
    rotation_index = 0;
    translation_index = 0;
    scale_index = 0;
    node_count = self->node_count;
    if (node_count < 1) {
        return;
    }

    for (node = 0; node < node_count; node++) {
        out_node = &out_orientations[node];

        if ((node & 0x1f) == 0) {
            int mask_word = node >> 5;
            translation_mask = self->node_transform_flag_data[mask_word];
            rotation_mask = self->node_rotation_flag_data[mask_word];
            scale_mask = self->node_scale_flag_data[mask_word];
        }

        if ((rotation_mask & 1) == 0) {
            if (!use_compressed_codec) {
                int16_t *src = (int16_t *)default_cursor;
                out_node->rotation.i = (float)src[0] * 3.051851e-05f;
                out_node->rotation.j = (float)src[1] * 3.051851e-05f;
                out_node->rotation.k = (float)src[2] * 3.051851e-05f;
                out_node->rotation.w = (float)src[3] * 3.051851e-05f;
                default_cursor += 8;
            } else {
                animation_compressed_header *header = (animation_compressed_header *)frame_cursor;
                animation_quaternion48 *def = (animation_quaternion48 *)(frame_cursor + header->rotation_defaults +
                                                                          (int)node * (int)sizeof(animation_quaternion48));
                animation_graph::quaternion48_decode(def, &out_node->rotation);
                halo::math::quaternion_normalize(out_node->rotation);
            }
        } else if (use_compressed_codec) {
            animation_view(self).node_get_rotation((float)frame, rotation_index, node, &out_node->rotation);
            rotation_index = rotation_index + 1;
        } else {
            int16_t *src = (int16_t *)frame_cursor;
            out_node->rotation.i = (float)src[0] * 3.051851e-05f;
            out_node->rotation.j = (float)src[1] * 3.051851e-05f;
            out_node->rotation.k = (float)src[2] * 3.051851e-05f;
            out_node->rotation.w = (float)src[3] * 3.051851e-05f;
            frame_cursor += 8;
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) == 0) {
            if (use_compressed_codec) {
                animation_compressed_header *header = (animation_compressed_header *)frame_cursor;
                real_point3d *def = (real_point3d *)(frame_cursor + header->translation_defaults +
                                                       (int)node * (int)sizeof(real_point3d));
                out_node->translation = *def;
            } else {
                real_point3d *src = (real_point3d *)default_cursor;
                out_node->translation = *src;
                default_cursor += sizeof(real_point3d);
            }
        } else if (use_compressed_codec) {
            animation_view(self).node_get_translation((float)frame, translation_index, node, &out_node->translation);
            translation_index = translation_index + 1;
        } else {
            real_point3d *src = (real_point3d *)frame_cursor;
            out_node->translation = *src;
            frame_cursor += sizeof(real_point3d);
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) == 0) {
            if (use_compressed_codec) {
                out_node->scale = 1.0f;
            } else {
                out_node->scale = *(float *)default_cursor;
                default_cursor += sizeof(float);
            }
        } else if (use_compressed_codec) {
            animation_view(self).node_get_scale(scale_index, (float)frame, &out_node->scale);
            scale_index = scale_index + 1;
        } else {
            out_node->scale = *(float *)frame_cursor;
            frame_cursor += sizeof(float);
        }
        scale_mask = scale_mask >> 1;
    }
}

void animation_view::node_get_rotation(real frame, int16_t rotation_index, int16_t node, real_quaternion *out)
{
    uint8_t *header_base;
    animation_compressed_header *header;
    uint32_t keyframe_header;
    int16_t count;
    int32_t first_index;
    animation_quaternion48 *defaults;

    header_base = (uint8_t *)self->frame_data.pointer + self->offset_to_compressed_data;
    header = (animation_compressed_header *)header_base;
    keyframe_header = ((uint32_t *)(header_base + sizeof(animation_compressed_header)))[rotation_index];
    count = (int16_t)(keyframe_header & k_animation_keyframe_count_mask);
    defaults = (animation_quaternion48 *)(header_base + header->rotation_defaults);

    if (count == 0) {
        animation_graph::quaternion48_decode(&defaults[node], out);
        halo::math::quaternion_normalize(*out);
        return;
    }

    first_index = (int16_t)(keyframe_header >> k_animation_keyframe_index_shift);

    {
        uint16_t *times = (uint16_t *)(header_base + header->rotation_keyframe_times) + first_index;
        animation_quaternion48 *keyframes = (animation_quaternion48 *)(header_base + header->rotation_keyframes) + first_index;
        int16_t rounded_frame = (int16_t)floor((double)frame);
        animation_quaternion48 *source_a;
        animation_quaternion48 *source_b;
        int16_t time_a, time_b;

        if ((int32_t)rounded_frame < (int32_t)times[0]) {
            time_a = 0;
            source_a = &defaults[node];
            time_b = (int16_t)times[0];
            source_b = &keyframes[0];
        } else if ((int32_t)rounded_frame == (int32_t)times[count - 1]) {
            time_a = (int16_t)times[count - 1];
            source_a = &keyframes[count - 1];
            time_b = (int16_t)(time_a + 1);
            source_b = &defaults[node];
        } else {
            int16_t index = animation_graph::keyframe_time_search(times, count, rounded_frame);
            time_a = (int16_t)times[index];
            time_b = (int16_t)times[index + 1];
            source_a = &keyframes[index];
            source_b = &keyframes[index + 1];
        }

        if (frame != (real)time_a) {
            real_quaternion quat_a, quat_b;
            real t;

            animation_graph::quaternion48_decode(source_a, &quat_a);
            animation_graph::quaternion48_decode(source_b, &quat_b);
            t = (frame - (real)time_a) / (real)(time_b - time_a);
            halo::math::quaternion_lerp(quat_b, quat_a, *out, t);
            halo::math::quaternion_normalize(*out);
        } else {
            animation_graph::quaternion48_decode(source_a, out);
            halo::math::quaternion_normalize(*out);
        }
    }
}

void animation_view::node_get_translation(real frame, int16_t translation_index, int16_t node, real_point3d *out)
{
    uint8_t *header_base;
    animation_compressed_header *header;
    uint32_t keyframe_header;
    int16_t count;
    real_point3d *defaults_node;

    header_base = (uint8_t *)self->frame_data.pointer + self->offset_to_compressed_data;
    header = (animation_compressed_header *)header_base;
    keyframe_header = ((uint32_t *)(header_base + header->translation_keyframe_headers))[translation_index];
    defaults_node = (real_point3d *)(header_base + header->translation_defaults) + node;
    count = (int16_t)(keyframe_header & k_animation_keyframe_count_mask);

    if (count == 0) {
        *out = *defaults_node;
        return;
    }

    {
        int32_t first_index = (int16_t)(keyframe_header >> k_animation_keyframe_index_shift);
        uint16_t *times = (uint16_t *)(header_base + header->translation_keyframe_times) + first_index;
        real_point3d *keyframes = (real_point3d *)(header_base + header->translation_keyframes) + first_index;
        int16_t rounded_frame = (int16_t)floor((double)frame);
        real_point3d *source_a;
        real_point3d *source_b;
        int16_t time_a, time_b;

        if ((int32_t)rounded_frame < (int32_t)times[0]) {
            time_a = 0;
            source_a = defaults_node;
            time_b = (int16_t)times[0];
            source_b = &keyframes[0];
        } else if ((int32_t)rounded_frame == (int32_t)times[count - 1]) {
            time_a = (int16_t)times[count - 1];
            source_a = &keyframes[count - 1];
            time_b = (int16_t)(time_a + 1);
            source_b = defaults_node;
        } else {
            int16_t index = animation_graph::keyframe_time_search(times, count, rounded_frame);
            time_a = (int16_t)times[index];
            time_b = (int16_t)times[index + 1];
            source_a = &keyframes[index];
            source_b = &keyframes[index + 1];
        }

        if (frame == (real)time_a) {
            *out = *source_a;
        } else {
            real t = (frame - (real)time_a) / (real)(time_b - time_a);
            real one_minus_t = 1.0f - t;

            out->x = t * source_b->x + one_minus_t * source_a->x;
            out->y = t * source_b->y + one_minus_t * source_a->y;
            out->z = t * source_b->z + one_minus_t * source_a->z;
        }
    }
}

void animation_view::node_get_scale(int16_t scale_index, real frame, real *out)
{
    uint8_t *header_base;
    animation_compressed_header *header;
    uint32_t keyframe_header;
    int16_t count;
    real *default_value;

    header_base = (uint8_t *)self->frame_data.pointer + self->offset_to_compressed_data;
    header = (animation_compressed_header *)header_base;
    keyframe_header = ((uint32_t *)(header_base + header->scale_keyframe_headers))[scale_index];
    default_value = &((real *)(header_base + header->scale_defaults))[scale_index];
    count = (int16_t)(keyframe_header & k_animation_keyframe_count_mask);

    if (count == 0) {
        *out = *default_value;
        return;
    }

    {
        int32_t first_index = (int16_t)(keyframe_header >> k_animation_keyframe_index_shift);
        uint16_t *times = (uint16_t *)(header_base + header->scale_keyframe_times) + first_index;
        real *keyframes = (real *)(header_base + header->scale_keyframes) + first_index;
        int16_t rounded_frame = (int16_t)floor((double)frame);
        real value_a, value_b;
        int16_t time_a, time_b;

        if ((int32_t)rounded_frame < (int32_t)times[0]) {
            time_a = 0;
            value_a = *default_value;
            time_b = (int16_t)times[0];
            value_b = keyframes[0];
        } else if ((int32_t)rounded_frame == (int32_t)times[count - 1]) {
            time_a = (int16_t)times[count - 1];
            value_a = keyframes[count - 1];
            time_b = (int16_t)(time_a + 1);
            value_b = *default_value;
        } else {
            int16_t index = animation_graph::keyframe_time_search(times, count, rounded_frame);
            time_a = (int16_t)times[index];
            time_b = (int16_t)times[index + 1];
            value_a = keyframes[index];
            value_b = keyframes[index + 1];
        }

        if (frame == (real)time_a) {
            *out = value_a;
        } else {
            real t = (frame - (real)time_a) / (real)(time_b - time_a);
            *out = t * value_b + (1.0f - t) * value_a;
        }
    }
}

void animation_view::overlay_frame_orientations(int16_t frame, real_orientation *out_orientations)
{
    uint8_t *frame_cursor;
    int use_compressed_codec;
    int16_t node_count;
    int16_t node;
    uint32_t translation_mask;
    uint32_t rotation_mask;
    uint32_t scale_mask;
    int16_t rotation_index;
    int16_t translation_index;
    int16_t scale_index;
    real_orientation *out_node;
    real_quaternion new_rotation;
    real_point3d new_translation;
    float new_scale;

    if (self->type != 1) {
        return;
    }
    if (frame < 0 || (int16_t)self->frame_count <= frame) {
        return;
    }

    use_compressed_codec = ((self->flags & 1) != 0) &&
                           !((globals().animation_compressed_data_enabled == 0) && (self->offset_to_compressed_data != 0));

    frame_cursor = (uint8_t *)animation_view(self).get_frame_data(frame);

    rotation_index = 0;
    translation_index = 0;
    scale_index = 0;
    node_count = self->node_count;
    if (node_count <= 0) {
        return;
    }

    for (node = 0; node < node_count; node++) {
        out_node = &out_orientations[node];

        if ((node & 0x1f) == 0) {
            int mask_word = node >> 5;
            translation_mask = self->node_transform_flag_data[mask_word];
            rotation_mask = self->node_rotation_flag_data[mask_word];
            scale_mask = self->node_scale_flag_data[mask_word];
        }

        if ((rotation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_rotation((float)frame, rotation_index, node, &new_rotation);
                rotation_index = rotation_index + 1;
            } else {
                animation_graph::quaternion16_decode((int16_t *)frame_cursor, &new_rotation);
                frame_cursor += 8;
            }
            halo::math::quaternion_multiply(&out_node->rotation, &new_rotation, &out_node->rotation);
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_translation((float)frame, translation_index, node, &new_translation);
                translation_index = translation_index + 1;
            } else {
                new_translation = *(real_point3d *)frame_cursor;
                frame_cursor += sizeof(real_point3d);
            }
            out_node->translation.x = new_translation.x + out_node->translation.x;
            out_node->translation.y = new_translation.y + out_node->translation.y;
            out_node->translation.z = new_translation.z + out_node->translation.z;
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_scale(scale_index, (float)frame, &new_scale);
                scale_index = scale_index + 1;
            } else {
                new_scale = *(float *)frame_cursor;
                frame_cursor += sizeof(float);
            }
            out_node->scale = new_scale * out_node->scale;
        }
        scale_mask = scale_mask >> 1;
    }
}

void animation_view::overlay_frame_orientations_weighted(int16_t frame, float weight, real_orientation *out_orientations)
{
    uint8_t *frame_cursor;
    int use_compressed_codec;
    int16_t node_count;
    int16_t node;
    uint32_t translation_mask;
    uint32_t rotation_mask;
    uint32_t scale_mask;
    int16_t rotation_index;
    int16_t translation_index;
    int16_t scale_index;
    real_orientation *out_node;
    real_quaternion new_rotation;
    real_point3d new_translation;
    float new_scale;
    float one_minus_weight;

    one_minus_weight = 1.0f - weight;

    if (self->type != 1) {
        return;
    }
    if (frame < 0 || (int16_t)self->frame_count <= frame) {
        return;
    }

    use_compressed_codec = ((self->flags & 1) != 0) &&
                           !((globals().animation_compressed_data_enabled == 0) && (self->offset_to_compressed_data != 0));

    frame_cursor = (uint8_t *)animation_view(self).get_frame_data(frame);

    rotation_index = 0;
    translation_index = 0;
    scale_index = 0;
    node_count = self->node_count;
    if (node_count <= 0) {
        return;
    }

    for (node = 0; node < node_count; node++) {
        out_node = &out_orientations[node];

        if ((node & 0x1f) == 0) {
            int mask_word = node >> 5;
            translation_mask = self->node_transform_flag_data[mask_word];
            rotation_mask = self->node_rotation_flag_data[mask_word];
            scale_mask = self->node_scale_flag_data[mask_word];
        }

        if ((rotation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_rotation((float)frame, rotation_index, node, &new_rotation);
                rotation_index = rotation_index + 1;
            } else {
                animation_graph::quaternion16_decode((int16_t *)frame_cursor, &new_rotation);
                frame_cursor += 8;
            }
            halo::math::quaternion_lerp(new_rotation, *globals().global_identity_quaternion_pointer, new_rotation, weight);
            halo::math::quaternion_multiply(&out_node->rotation, &new_rotation, &out_node->rotation);
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_translation((float)frame, translation_index, node, &new_translation);
                translation_index = translation_index + 1;
            } else {
                new_translation = *(real_point3d *)frame_cursor;
                frame_cursor += sizeof(real_point3d);
            }
            out_node->translation.x = new_translation.x * weight + out_node->translation.x;
            out_node->translation.y = new_translation.y * weight + out_node->translation.y;
            out_node->translation.z = new_translation.z * weight + out_node->translation.z;
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_scale(scale_index, (float)frame, &new_scale);
                scale_index = scale_index + 1;
            } else {
                new_scale = *(float *)frame_cursor;
                frame_cursor += sizeof(float);
            }
            out_node->scale = (new_scale * weight + one_minus_weight) * out_node->scale;
        }
        scale_mask = scale_mask >> 1;
    }
}

void animation_view::overlay_interpolated_frame_orientations(float frame, real_orientation *out_orientations)
{
    float weight;
    float base_frame_f;
    int16_t frame_count;
    int16_t base_frame;
    int16_t next_frame;
    int use_compressed_codec;
    uint8_t *base_cursor;
    uint8_t *next_cursor;
    int16_t node_count;
    int16_t node;
    uint32_t translation_mask;
    uint32_t rotation_mask;
    uint32_t scale_mask;
    int16_t rotation_index;
    int16_t translation_index;
    int16_t scale_index;
    real_orientation *out_node;
    real_quaternion new_rotation;
    real_point3d new_translation;
    float new_scale;

    weight = (float)fmod((double)frame, 1.0);
    base_frame_f = (float)floor(fabs((double)frame));
    base_frame = (int16_t)base_frame_f;
    frame_count = self->frame_count;
    if (frame_count <= base_frame) {
        base_frame = frame_count - 1;
        base_frame_f = (float)base_frame;
        weight = 1.0f;
        frame = base_frame_f;
    }

    if (self->type != 1) {
        return;
    }

    use_compressed_codec = ((self->flags & 1) != 0) &&
                           !((globals().animation_compressed_data_enabled == 0) && (self->offset_to_compressed_data != 0));

    next_frame = (base_frame == frame_count - 1) ? 0 : (int16_t)(base_frame + 1);

    base_cursor = (uint8_t *)animation_view(self).get_frame_data(base_frame);
    next_cursor = (uint8_t *)animation_view(self).get_frame_data(next_frame);

    rotation_index = 0;
    translation_index = 0;
    scale_index = 0;
    node_count = self->node_count;
    if (node_count <= 0) {
        return;
    }

    for (node = 0; node < node_count; node++) {
        out_node = &out_orientations[node];

        if ((node & 0x1f) == 0) {
            int mask_word = node >> 5;
            translation_mask = self->node_transform_flag_data[mask_word];
            rotation_mask = self->node_rotation_flag_data[mask_word];
            scale_mask = self->node_scale_flag_data[mask_word];
        }

        if ((rotation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_rotation((float)base_frame, rotation_index, node, &new_rotation);
                rotation_index = rotation_index + 1;
            } else {
                real_quaternion corner_base;
                real_quaternion corner_next;
                animation_graph::quaternion16_decode((int16_t *)base_cursor, &corner_base);
                base_cursor += 8;
                animation_graph::quaternion16_decode((int16_t *)next_cursor, &corner_next);
                next_cursor += 8;
                halo::math::quaternion_lerp(corner_next, corner_base, new_rotation, weight);
                halo::math::quaternion_normalize(new_rotation);
            }
            halo::math::quaternion_multiply(&out_node->rotation, &new_rotation, &out_node->rotation);
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_translation(frame, translation_index, node, &new_translation);
                translation_index = translation_index + 1;
            } else {
                real_point3d *corner_base = (real_point3d *)base_cursor;
                real_point3d *corner_next = (real_point3d *)next_cursor;
                new_translation.x = weight * corner_next->x + (1.0f - weight) * corner_base->x;
                new_translation.y = weight * corner_next->y + (1.0f - weight) * corner_base->y;
                new_translation.z = weight * corner_next->z + (1.0f - weight) * corner_base->z;
                base_cursor += sizeof(real_point3d);
                next_cursor += sizeof(real_point3d);
            }
            out_node->translation.x = new_translation.x + out_node->translation.x;
            out_node->translation.y = new_translation.y + out_node->translation.y;
            out_node->translation.z = new_translation.z + out_node->translation.z;
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_scale(scale_index, frame, &new_scale);
                scale_index = scale_index + 1;
            } else {
                float base_scale = *(float *)base_cursor;
                float next_scale = *(float *)next_cursor;
                new_scale = weight * next_scale + (1.0f - weight) * base_scale;
                base_cursor += sizeof(float);
                next_cursor += sizeof(float);
            }
            out_node->scale = new_scale * out_node->scale;
        }
        scale_mask = scale_mask >> 1;
    }
}

void animation_view::overlay_interpolated_frame_orientations_weighted(float frame, float weight, real_orientation *out_orientations)
{
    float one_minus_weight;
    float frame_weight;
    float base_frame_f;
    int16_t frame_count;
    int16_t base_frame;
    int16_t next_frame;
    int use_compressed_codec;
    uint8_t *base_cursor;
    uint8_t *next_cursor;
    int16_t node_count;
    int16_t node;
    uint32_t translation_mask;
    uint32_t rotation_mask;
    uint32_t scale_mask;
    int16_t rotation_index;
    int16_t translation_index;
    int16_t scale_index;
    real_orientation *out_node;
    real_quaternion new_rotation;
    real_point3d new_translation;
    float new_scale;

    one_minus_weight = 1.0f - weight;

    frame_weight = (float)fmod((double)frame, 1.0);
    base_frame_f = (float)floor((double)frame);
    base_frame = (int16_t)base_frame_f;
    frame_count = self->frame_count;
    if (frame_count <= base_frame) {
        base_frame = frame_count - 1;
        base_frame_f = (float)base_frame;
        frame_weight = 1.0f;
        frame = base_frame_f;
    }

    if (self->type != 1) {
        return;
    }

    use_compressed_codec = ((self->flags & 1) != 0) &&
                           !((globals().animation_compressed_data_enabled == 0) && (self->offset_to_compressed_data != 0));

    next_frame = (base_frame == frame_count - 1) ? 0 : (int16_t)(base_frame + 1);

    base_cursor = (uint8_t *)animation_view(self).get_frame_data(base_frame);
    next_cursor = (uint8_t *)animation_view(self).get_frame_data(next_frame);

    rotation_index = 0;
    translation_index = 0;
    scale_index = 0;
    node_count = self->node_count;
    if (node_count <= 0) {
        return;
    }

    for (node = 0; node < node_count; node++) {
        out_node = &out_orientations[node];

        if ((node & 0x1f) == 0) {
            int mask_word = node >> 5;
            translation_mask = self->node_transform_flag_data[mask_word];
            rotation_mask = self->node_rotation_flag_data[mask_word];
            scale_mask = self->node_scale_flag_data[mask_word];
        }

        if ((rotation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_rotation((float)base_frame, rotation_index, node, &new_rotation);
                rotation_index = rotation_index + 1;
            } else {
                real_quaternion corner_base;
                real_quaternion corner_next;
                animation_graph::quaternion16_decode((int16_t *)base_cursor, &corner_base);
                base_cursor += 8;
                animation_graph::quaternion16_decode((int16_t *)next_cursor, &corner_next);
                next_cursor += 8;
                halo::math::quaternion_lerp(corner_next, corner_base, new_rotation, frame_weight);
                halo::math::quaternion_normalize(new_rotation);
            }
            halo::math::quaternion_lerp(new_rotation, *globals().global_identity_quaternion_pointer, new_rotation, weight);
            halo::math::quaternion_normalize(new_rotation);
            halo::math::quaternion_multiply(&out_node->rotation, &new_rotation, &out_node->rotation);
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_translation(frame, translation_index, node, &new_translation);
                translation_index = translation_index + 1;
            } else {
                real_point3d *corner_base = (real_point3d *)base_cursor;
                real_point3d *corner_next = (real_point3d *)next_cursor;
                new_translation.x = frame_weight * corner_next->x + (1.0f - frame_weight) * corner_base->x;
                new_translation.y = frame_weight * corner_next->y + (1.0f - frame_weight) * corner_base->y;
                new_translation.z = frame_weight * corner_next->z + (1.0f - frame_weight) * corner_base->z;
                base_cursor += sizeof(real_point3d);
                next_cursor += sizeof(real_point3d);
            }
            out_node->translation.x = new_translation.x * weight + out_node->translation.x;
            out_node->translation.y = new_translation.y * weight + out_node->translation.y;
            out_node->translation.z = new_translation.z * weight + out_node->translation.z;
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_scale(scale_index, frame, &new_scale);
                scale_index = scale_index + 1;
            } else {
                float base_scale = *(float *)base_cursor;
                float next_scale = *(float *)next_cursor;
                new_scale = frame_weight * next_scale + (1.0f - frame_weight) * base_scale;
                base_cursor += sizeof(float);
                next_cursor += sizeof(float);
            }
            out_node->scale = (new_scale * weight + one_minus_weight) * out_node->scale;
        }
        scale_mask = scale_mask >> 1;
    }
}

void animation_view::replace_frame_orientations(int16_t frame, real_orientation *out_orientations)
{
    uint8_t *frame_cursor;
    int use_compressed_codec;
    int16_t node_count;
    int16_t node;
    uint32_t translation_mask;
    uint32_t rotation_mask;
    uint32_t scale_mask;
    int16_t rotation_index;
    int16_t translation_index;
    int16_t scale_index;
    real_orientation *out_node;

    if (self->type != 2) {
        return;
    }
    if (frame < 0 || (int16_t)self->frame_count <= frame) {
        return;
    }

    use_compressed_codec = ((self->flags & 1) != 0) &&
                           !((globals().animation_compressed_data_enabled == 0) && (self->offset_to_compressed_data != 0));

    frame_cursor = (uint8_t *)animation_view(self).get_frame_data(frame);

    rotation_index = 0;
    translation_index = 0;
    scale_index = 0;
    node_count = self->node_count;
    if (node_count <= 0) {
        return;
    }

    for (node = 0; node < node_count; node++) {
        out_node = &out_orientations[node];

        if ((node & 0x1f) == 0) {
            int mask_word = node >> 5;
            translation_mask = self->node_transform_flag_data[mask_word];
            rotation_mask = self->node_rotation_flag_data[mask_word];
            scale_mask = self->node_scale_flag_data[mask_word];
        }

        if ((rotation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_rotation((float)frame, rotation_index, node, &out_node->rotation);
                rotation_index = rotation_index + 1;
            } else {
                animation_graph::quaternion16_decode((int16_t *)frame_cursor, &out_node->rotation);
                frame_cursor += 8;
            }
        }
        rotation_mask = rotation_mask >> 1;

        if ((translation_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_translation((float)frame, translation_index, node, &out_node->translation);
                translation_index = translation_index + 1;
            } else {
                out_node->translation = *(real_point3d *)frame_cursor;
                frame_cursor += sizeof(real_point3d);
            }
        }
        translation_mask = translation_mask >> 1;

        if ((scale_mask & 1) != 0) {
            if (use_compressed_codec) {
                animation_view(self).node_get_scale(scale_index, (float)frame, &out_node->scale);
                scale_index = scale_index + 1;
            } else {
                out_node->scale = *(float *)frame_cursor;
                frame_cursor += sizeof(float);
            }
        }
        scale_mask = scale_mask >> 1;
    }
}

static void aiming_screen_frame_split(real value, real divisor, int32_t *out_frame, real *out_frac)
{
    real quotient;
    int32_t frame;
    real frac;

    quotient = (divisor == 0.0f) ? 0.0f : value / divisor;
    frame = static_cast<int>((double)quotient);
    frac = (real)fmod((double)quotient, 1.0);
    if (frac < 0.0f) {
        frame = frame - 1;
        frac = frac + 1.0f;
    }
    *out_frame = frame;
    *out_frac = frac;
}

void animation_view::aiming_screen_blend(animation_aiming_screen *screen, real yaw, real pitch, real_orientation *orientation_out)
{
    int16_t total_columns;
    int16_t total_rows;

    total_columns = (int16_t)(screen->left_frame_count + screen->right_frame_count + 1);
    total_rows = (int16_t)(screen->down_pitch_frame_count + screen->up_pitch_frame_count + 1);

    if (self->type != 1) {
        return;
    }

    if ((int32_t)(int16_t)self->frame_count < (int32_t)total_columns * (int32_t)total_rows) {
        return;
    }

    {
        int use_interpolated;
        int32_t yaw_frame, pitch_frame;
        real yaw_frac, pitch_frac;

        use_interpolated = ((self->flags & 1) != 0) &&
                            !((globals().animation_compressed_data_enabled == 0) &&
                              (self->offset_to_compressed_data != 0));

        aiming_screen_frame_split(yaw, (0.0f < yaw) ? screen->left_yaw_per_frame : screen->right_yaw_per_frame,
                                   &yaw_frame, &yaw_frac);
        if ((int16_t)screen->left_frame_count <= (int16_t)yaw_frame) {
            yaw_frame = (int16_t)screen->left_frame_count - 1;
            yaw_frac = 1.0f;
        }
        if ((int16_t)yaw_frame < -(int16_t)screen->right_frame_count) {
            yaw_frame = -(int16_t)screen->right_frame_count;
            yaw_frac = 0.0f;
        }
        yaw_frame = yaw_frame + screen->right_frame_count;

        aiming_screen_frame_split(pitch, (0.0f < pitch) ? screen->up_pitch_per_frame : screen->down_pitch_per_frame,
                                   &pitch_frame, &pitch_frac);
        if ((int16_t)screen->up_pitch_frame_count <= (int16_t)pitch_frame) {
            pitch_frame = (int16_t)screen->up_pitch_frame_count - 1;
            pitch_frac = 1.0f;
        }
        if ((int16_t)pitch_frame < -(int16_t)screen->down_pitch_frame_count) {
            pitch_frame = -(int16_t)screen->down_pitch_frame_count;
            pitch_frac = 0.0f;
        }
        pitch_frame = pitch_frame + screen->down_pitch_frame_count;

        if (0 <= pitch_frame && pitch_frame < total_rows && 0 <= yaw_frame && yaw_frame < total_columns) {
            int16_t column0, column1, row0_index, row1_index;
            int16_t f00, f01, f10, f11;
            uint8_t *p00, *p01, *p10, *p11;
            int16_t rotation_index, translation_index;
            int16_t node;
            uint32_t rotation_mask, translation_mask;

            column0 = (int16_t)yaw_frame;
            column1 = (int16_t)(column0 + 1);
            if (column1 == total_columns) {
                column1 = column0;
            }
            row0_index = (int16_t)pitch_frame;
            row1_index = (int16_t)(row0_index + 1);
            if (row1_index == total_rows) {
                row1_index = row0_index;
            }

            f00 = (int16_t)(row0_index * total_columns + column0);
            f01 = (int16_t)(row0_index * total_columns + column1);
            f10 = (int16_t)(row1_index * total_columns + column0);
            f11 = (int16_t)(row1_index * total_columns + column1);

            p00 = (uint8_t *)animation_view(self).get_frame_data(f00);
            p01 = (uint8_t *)animation_view(self).get_frame_data(f01);
            p10 = (uint8_t *)animation_view(self).get_frame_data(f10);
            p11 = (uint8_t *)animation_view(self).get_frame_data(f11);

            rotation_index = 0;
            translation_index = 0;
            rotation_mask = 0;
            translation_mask = 0;

            for (node = 0; node < (int16_t)self->node_count; node++) {
                real_orientation *out_node = &orientation_out[node];

                if ((node & 0x1f) == 0) {
                    int mask_word = node >> 5;
                    translation_mask = self->node_transform_flag_data[mask_word];
                    rotation_mask = self->node_rotation_flag_data[mask_word];
                }

                if ((rotation_mask & 1) != 0) {
                    real_quaternion q00, q01, q10, q11;
                    real_quaternion row0, row1, blended;

                    if (use_interpolated) {
                        animation_view(self).node_get_rotation((real)f00, rotation_index, node, &q00);
                        animation_view(self).node_get_rotation((real)f01, rotation_index, node, &q01);
                        animation_view(self).node_get_rotation((real)f10, rotation_index, node, &q10);
                        animation_view(self).node_get_rotation((real)f11, rotation_index, node, &q11);
                        rotation_index = rotation_index + 1;
                    } else {
                        animation_graph::quaternion16_decode((int16_t *)p00, &q00); p00 += 8;
                        animation_graph::quaternion16_decode((int16_t *)p01, &q01); p01 += 8;
                        animation_graph::quaternion16_decode((int16_t *)p10, &q10); p10 += 8;
                        animation_graph::quaternion16_decode((int16_t *)p11, &q11); p11 += 8;
                    }

                    halo::math::quaternion_lerp(q01, q00, row0, yaw_frac);
                    halo::math::quaternion_normalize(row0);
                    halo::math::quaternion_lerp(q11, q10, row1, yaw_frac);
                    halo::math::quaternion_normalize(row1);
                    halo::math::quaternion_lerp(row1, row0, blended, pitch_frac);
                    halo::math::quaternion_normalize(blended);
                    halo::math::quaternion_multiply(&out_node->rotation, &blended, &out_node->rotation);
                }
                rotation_mask = rotation_mask >> 1;

                if ((translation_mask & 1) != 0) {
                    real_point3d t00, t01, t10, t11;
                    real one_minus_yaw, one_minus_pitch;

                    if (use_interpolated) {
                        animation_view(self).node_get_translation((real)f00, translation_index, node, &t00);
                        animation_view(self).node_get_translation((real)f01, translation_index, node, &t01);
                        animation_view(self).node_get_translation((real)f10, translation_index, node, &t10);
                        animation_view(self).node_get_translation((real)f11, translation_index, node, &t11);
                        translation_index = translation_index + 1;
                    } else {
                        t00 = *(real_point3d *)p00; p00 += 12;
                        t01 = *(real_point3d *)p01; p01 += 12;
                        t10 = *(real_point3d *)p10; p10 += 12;
                        t11 = *(real_point3d *)p11; p11 += 12;
                    }

                    one_minus_yaw = 1.0f - yaw_frac;
                    one_minus_pitch = 1.0f - pitch_frac;
                    out_node->translation.x = (t01.x * yaw_frac + t00.x * one_minus_yaw) * one_minus_pitch +
                                               (t11.x * yaw_frac + t10.x * one_minus_yaw) * pitch_frac +
                                               out_node->translation.x;
                    out_node->translation.y = (t01.y * yaw_frac + t00.y * one_minus_yaw) * one_minus_pitch +
                                               (t11.y * yaw_frac + t10.y * one_minus_yaw) * pitch_frac +
                                               out_node->translation.y;
                    out_node->translation.z = (t11.z * yaw_frac + t10.z * one_minus_yaw) * pitch_frac +
                                               (t01.z * yaw_frac + t00.z * one_minus_yaw) * one_minus_pitch +
                                               out_node->translation.z;
                }
                translation_mask = translation_mask >> 1;
            }
        }
    }
}

}  // namespace halo::models
