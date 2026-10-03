/**
 * @file src/rasterizer/lens_flares.cpp
 * Lens flare instances, occlusion sampling and the flare sprite batcher.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include <cstddef>
#include "halo/core/slot_mask.hpp"
#include "halo/render/d3d9.hpp"
#include "halo/core/datum.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/render/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"




static_assert(offsetof(LensFlare, cos_falloff_angle) == 0x08, "lens flare cosines");
static_assert(offsetof(LensFlare, occlusion_radius) == 0x10, "lens flare occlusion radius");
static_assert(offsetof(LensFlare, near_fade_distance) == 0x18, "lens flare fade distances");
static_assert(offsetof(LensFlare, bitmap) + offsetof(TagDependency, tag_id) == 0x2c, "lens flare bitmap id");
static_assert(offsetof(LensFlare, flags) == 0x30, "lens flare flags");
static_assert(offsetof(LensFlare, rotation_function) == 0x80, "lens flare rotation");
static_assert(offsetof(LensFlare, horizontal_scale) == 0xa0, "lens flare scale");
static_assert(offsetof(LensFlare, reflections) == 0xc4, "lens flare reflections");
static_assert(sizeof(LensFlareReflection) == 0x80, "lens flare reflection size");
static_assert(offsetof(LensFlareReflection, position) == 0x1c, "reflection position");
static_assert(offsetof(LensFlareReflection, radius) == 0x28, "reflection radius");
static_assert(offsetof(LensFlareReflection, brightness) == 0x34, "reflection brightness");
static_assert(offsetof(LensFlareReflection, tint_color) == 0x40, "reflection tint");
static_assert(offsetof(LensFlareReflection, color_lower_bound) == 0x50, "reflection lower color");
static_assert(offsetof(LensFlareReflection, more_flags) == 0x70, "reflection more flags");
static_assert(offsetof(LensFlareReflection, animation_phase) == 0x78, "reflection phase");

namespace halo::rasterizer {

/**
 * Culls a candidate lens flare (by view-space depth against its LensFlare.far_fade_distance, and by a zero
 * alpha) and, if visible and the active list has room, appends it to lens_flare_instances and resolves its per
 * window visibility byte: for a BSP marker flare (candidate->object_index == -1) it rewrites
 * visibility_high/visibility_low from the source marker offset; for an object flare it resets the matching
 * lens_flare_object_visibility slot whenever that slot no longer belongs to this object.
 *
 * Registers: unaff_EBX -> candidate
 *
 * @address 0x5138a0
 */
void lens_flare_add_instance(lens_flare_instance *candidate)
{
    int32_t new_index;
    float depth;
    LensFlare *definition;

    new_index = lens_flare_instance_count;

    if (halo::rasterizer::fields::decals_and_lens_flares_enabled == 0 || halo::rasterizer::fields::screenshot_tile_count >= 2 ||
        (halo::rasterizer::fields::screenshot_tile_count == 1 && screenshot_scale >= 2) || rasterizer_window.type != 1) {
        return;
    }

    if (lens_flare_instance_count >= 0x400) {
        if (lens_flare_instance_overflow == 0) {
            lens_flare_instance_overflow = 1;
        }
        return;
    }

    definition = (LensFlare *)candidate->definition;
    depth = rasterizer_window.camera.forward.i * (candidate->position.x - rasterizer_window.camera.position.x) +
            rasterizer_window.camera.forward.j * (candidate->position.y - rasterizer_window.camera.position.y) +
            rasterizer_window.camera.forward.k * (candidate->position.z - rasterizer_window.camera.position.z);

    if ((definition->far_fade_distance != 0.0f && depth >= definition->far_fade_distance) ||
        (candidate->color & 0xff000000) == 0) {
        return;
    }

    lens_flare_instances[new_index] = *candidate;
    lens_flare_instance_count = new_index + 1;

    if (candidate->object_index == -1) {
        uint16_t marker_visibility_high = (uint16_t)candidate->visibility_high;

        if (marker_visibility_high != 0xffff) {
            int16_t marker_offset = (int16_t)candidate->visibility_low;
            lens_flare_instances[new_index].visibility_low = marker_offset + 8;
            lens_flare_instances[new_index].visibility_high =
                (int16_t)(marker_visibility_high | (uint16_t)(marker_offset >> 0xf) | 0x8000);
            return;
        }
        lens_flare_instances[new_index].visibility_high = (int16_t)0x8000;
    } else {
        if (candidate->object_index != lens_flare_object_visibility_table[lens_flare_instances[new_index].visibility_high].object_index) {
            int16_t object_index = lens_flare_instances[new_index].object_index;
            int i;

            for (i = 0; i < 8; i++) {
                lens_flare_object_visibility_table[lens_flare_instances[new_index].visibility_high].visibility[i] = 0;
            }
            lens_flare_object_visibility_table[lens_flare_instances[new_index].visibility_high].object_index = object_index;
            return;
        }
    }
}

static float dot3(const real_vector3d *a, const real_vector3d *b)
{
    return a->i * b->i + a->j * b->j + a->k * b->k;
}

/**
 * Direct3D 9 back end function lens_flare_compute_rotation. The original author notes are in
 * docs/original/rasterizer/lens_flare_compute_rotation.c.txt.
 *
 * @address 0x513540
 */
float lens_flare_compute_rotation(lens_flare_instance *flare, int16_t mode)
{
    const real_vector3d *forward = &rasterizer_window.frustum.view_to_world.forward;
    const real_vector3d *up = &rasterizer_window.frustum.view_to_world.up;
    real_vector3d unpacked;
    real_vector3d direction;
    real_vector3d reference;
    real_vector3d basis;
    float y = 0.0f;
    float x = 1.0f;

    direction = *vector3d_unpack_normal_11_11_10(&unpacked, flare->packed_direction);
    switch (mode) {
    case 1:
    case 3:
        halo::math::vector3d_cross_product(basis, *forward, direction);
        halo::math::vector3d_cross_product(basis, direction, basis);
        if (mode == 1) {
            reference = rasterizer_window.camera.forward;
        } else {
            reference.i = flare->position.x - rasterizer_window.camera.position.x;
            reference.j = flare->position.y - rasterizer_window.camera.position.y;
            reference.k = flare->position.z - rasterizer_window.camera.position.z;
        }
        y = dot3(&basis, &reference);
        x = -dot3(&direction, &reference);
        break;
    case 2:
        reference.i = -direction.i;
        reference.j = -direction.j;
        reference.k = -direction.k;
        y = dot3(forward, &reference);
        x = -dot3(up, &reference);
        break;
    case 4:
        reference.i = flare->position.x - rasterizer_window.camera.position.x;
        reference.j = flare->position.y - rasterizer_window.camera.position.y;
        reference.k = flare->position.z - rasterizer_window.camera.position.z;
        y = dot3(forward, &reference);
        x = -dot3(up, &reference);
        break;
    default:
        break;
    }
    if (mode != 0 && y != 0.0f) {
        return (float)(halo::libm::atan2((double)y, (double)x) * 0.15915493667125702);
    }
    return 0.0f;
}

/**
 * Resolves a lens flare instance's smoothed visibility byte: for a BSP marker flare (visibility_high < 0) it
 * indexes lens_flare_marker_visibility by the packed marker offset in visibility_high/visibility_low plus the
 * window bits of window_flags; for an object flare it indexes lens_flare_object_visibility_bytes by object
 * index (stride 0x0a, +2 for the per window visibility array) plus the same window bits.
 *
 * Registers: ECX -> flare
 *
 * @address 0x5134f0
 */
uint8_t * lens_flare_get_visibility_byte(lens_flare_instance *flare)
{
    if (flare->visibility_high < 0) {
        return lens_flare_marker_visibility +
               (flare->window_flags & 0xffffff7f) +
               (((uint32_t)(uint16_t)flare->visibility_high & 0x7fff) << 0x10 |
                (uint32_t)(int32_t)flare->visibility_low);
    }
    return (uint8_t *)lens_flare_object_visibility_table +
           (flare->window_flags & 0xffffff7f) + (int32_t)flare->visibility_high * 10 + 2 +
           (int32_t)flare->visibility_low;
}

static float lens_flare_clamp01(float x)
{
    if (!(x >= 0.0f)) {
        return 0.0f;
    }
    if (x > 1.0f) {
        return 1.0f;
    }
    return x;
}

/**
 * Direct3D 9 back end function lens_flare_render_all. The original author notes are in
 * docs/original/rasterizer/lens_flare_render_all.c.txt.
 *
 * @address 0x513cf0
 */
void lens_flare_render_all(void)
{
    const float *camera = &rasterizer_window.camera.position.x;
    const float *forward = &rasterizer_window.camera.forward.i;
    const float *axis_a = &rasterizer_window.frustum.view_to_world.forward.i;
    const float *axis_b = &rasterizer_window.frustum.view_to_world.left.i;
    int16_t i;

    if (halo::rasterizer::fields::decals_and_lens_flares_enabled == 0 || rasterizer_window.type != 1 || lens_flare_instance_count <= 0) {
        return;
    }
    if (lens_flare_occlusion_queries_supported != 1) {
        chimera__rasterizer_set_frustum_z_func(0x3d000200, 0x45800000);
    }
    rasterizer_lens_flare_batching_select_mode(5, 0);

    for (i = 0; i < lens_flare_instance_count; i++) {
        lens_flare_instance *instance = &lens_flare_instances[i];
        uint8_t *visibility_byte = lens_flare_get_visibility_byte(instance);
        real_vector3d unpacked;
        real_vector3d normal;
        LensFlare *definition;
        uint8_t alpha;
        real_point3d position;
        real_vector3d d;
        float depth, off[3], visibility, fade, base, rotation, angle, span, inv, bias, falloff[4];
        float intensity;
        int16_t j;

        normal = *vector3d_unpack_normal_11_11_10(&unpacked, instance->packed_direction);
        if ((int16_t)(instance->window_flags & _lens_flare_window_index_mask) != rasterizer_window.window_index) {
            continue;
        }
        definition = (LensFlare *)(uintptr_t)instance->definition;
        if (instance->sample_count <= 0) {
            continue;
        }
        alpha = (uint8_t)(instance->color >> 24);
        if (alpha == 0 || (int32_t)definition->reflections.count <= 0) {
            continue;
        }

        position = instance->position;
        d.i = position.x - camera[0];
        d.j = position.y - camera[1];
        d.k = position.z - camera[2];
        depth = forward[2] * d.k + forward[1] * d.j + d.i * forward[0];
        off[0] = (forward[0] * depth - d.i) * 2.0f;
        off[1] = (forward[1] * depth - d.j) * 2.0f;
        off[2] = (forward[2] * depth - d.k) * 2.0f;
        if (lens_flare_occlusion_queries_supported == 0) {
            depth = depth * 0.5f;
        }

        visibility = (float)*visibility_byte * 0.003921569f;
        {
            float fade_start = definition->far_fade_distance;
            float fade_end = definition->near_fade_distance;

            if (!(fade_start > 0.0f)) {
                fade = 1.0f;
            } else {
                float t = (depth - fade_start) / (fade_end - fade_start);

                if (!(t >= 0.0f)) {
                    fade = 0.0f;
                } else if (t > 1.0f) {
                    fade = 1.0f;
                } else {
                    fade = t;
                }
            }
        }
        base = visibility * fade * (float)alpha * 0.003921569f;

        rotation = lens_flare_compute_rotation(instance, definition->rotation_function) *
                   definition->rotation_function_scale;
        angle = (float)halo::x87::fpatan((double)(axis_a[2] * d.k + axis_a[1] * d.j + d.i * axis_a[0]),
                              (double)(axis_b[2] * d.k + axis_b[1] * d.j + d.i * axis_b[0])) * 57.29578f;

        span = definition->cos_falloff_angle - definition->cos_cutoff_angle;
        inv = (span != 0.0f) ? 1.0f / span : 0.0f;
        bias = -(inv * definition->cos_cutoff_angle);
        halo::math::vector3d_normalize_with_length(d);
        falloff[0] = 1.0f;
        falloff[1] = lens_flare_clamp01(bias - (forward[2] * normal.k + forward[1] * normal.j + normal.i * forward[0]) * inv);
        falloff[2] = lens_flare_clamp01(bias - (normal.k * d.k + normal.j * d.j + normal.i * d.i) * inv);
        falloff[3] = lens_flare_clamp01((forward[2] * d.k + forward[1] * d.j + d.i * forward[0]) * inv + bias);

        if (!(base > 0.0f)) {
            continue;
        }
        intensity = (float)instance->intensity * 0.003921569f;

        for (j = 0; j < (int32_t)definition->reflections.count; j++) {
            LensFlareReflection *reflection = (LensFlareReflection *)(uintptr_t)definition->reflections.pointer + (int32_t)j;
            float r34 = reflection->brightness[0];
            float brightness = ((reflection->brightness[1] - r34) * intensity + r34) *
                               falloff[reflection->brightness_scaled_by] * base;
            float r28, radius, specular, reflection_rotation, scale[2];
            uint32_t colour;
            real_point3d vertex;
            uint16_t flags;

            if (j == 0) {
                base = brightness;
            }
            if (!(brightness > 0.0f)) {
                continue;
            }
            r28 = reflection->radius[0];
            radius = (reflection->radius[1] - r28) * intensity + r28;

            if (reflection->tint_color.alpha == 0.0f && reflection->tint_color.red == 0.0f &&
                reflection->tint_color.green == 0.0f && reflection->tint_color.blue == 0.0f) {
                colour = ((uint32_t)color_channel_real_to_byte(brightness) << 24) |
                         (instance->color & 0xffffff);
                specular = 1.0f;
            } else {
                ColorARGB tint;

                tint.alpha = brightness;
                tint.red = reflection->tint_color.red;
                tint.green = reflection->tint_color.green;
                tint.blue = reflection->tint_color.blue;
                if (reflection->animation_function > 1) {
                    ColorRGB animated;
                    float t = (float)halo::math::periodic_function_evaluate(
                        (periodic_function_t)reflection->animation_function,
                        ((double)reflection->animation_phase + *(double *)&rasterizer_time) /
                            (double)reflection->animation_period);

                    halo::bitmaps::color_interpolate((ColorRGB *)&reflection->color_upper_bound.red, (ColorRGB *)&reflection->color_lower_bound.red, &animated,
                                      static_cast<color_interpolation_flags>(reflection->more_flags & 3), t);
                    tint.alpha = ((1.0f - t) * reflection->color_lower_bound.alpha + t * reflection->color_upper_bound.alpha) *
                                 tint.alpha;
                    tint.red = tint.red * animated.red;
                    tint.green = tint.green * animated.green;
                    tint.blue = tint.blue * animated.blue;
                }
                colour = halo::interface::color_pack_argb_from_real(&tint);
                specular = reflection->tint_color.alpha;
            }

            if (j == 0) {
                reflection_rotation = rotation + reflection->rotation_offset;
                scale[0] = definition->horizontal_scale;
                scale[1] = definition->vertical_scale;
            } else {
                reflection_rotation = reflection->rotation_offset;
                scale[0] = 1.0f;
                scale[1] = 1.0f;
            }
            flags = reflection->flags;
            if ((flags & 1) != 0) {
                reflection_rotation = reflection_rotation + angle;
            }
            if ((flags & 4) != 0) {
                radius = (visibility + 1.0f) * radius * 0.5f;
            }
            if ((flags & 2) != 0) {
                radius = radius * depth;
            }
            {
                float along = reflection->position;

                vertex.x = off[0] * along + position.x;
                vertex.y = off[1] * along + position.y;
                vertex.z = off[2] * along + position.z;
            }

            if (halo::render::rasterizer_lens_flare_set_current_key(halo::tag_id_bits<int32_t>(definition->bitmap.tag_id), 0,
                                                      (int16_t)reflection->bitmap_index) != 0) {
                break;
            }
            halo::render::rasterizer_lens_flare_set_vertex_specular(specular);
            halo::rasterizer::fields::lens_flare_batch_mode = ((flags & 8) != 0 && (instance->window_flags & _lens_flare_window_flag_80_bit) != 0) ? 2 : 0;
            rasterizer_lens_flare_quad_add(scale, colour, &vertex, radius, reflection_rotation * 0.017453292f);
        }
    }

    rasterizer_set_shader_stage_config(0);
    if (lens_flare_occlusion_queries_supported != 1) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
    rasterizer_lens_flare_batch_flush_all();

    if (rasterizer_effect_pool_scratch != 0 && *(void **)rasterizer_effect_pool_scratch != 0) {
        void *obj = *(void **)rasterizer_effect_pool_scratch;
        void **vtable = *(void ***)obj;
        ((void (__stdcall *)(void *))vtable[0x108 / 4])(obj);
    }
    rasterizer_effect_pool_scratch = 0;
    {
        render_device().set_fvf(0);
    }

    if (rasterizer_caps_flag_68a == 0 && halo::rasterizer::fields::lens_flare_occlusion_enabled != 0) {
        for (i = 0; i < lens_flare_instance_count; i++) {
            lens_flare_instance *instance = &lens_flare_instances[i];

            if (instance->sample_count > 0 && (int16_t)(instance->window_flags & _lens_flare_window_index_mask) == rasterizer_window.window_index) {
                LensFlare *definition = (LensFlare *)(uintptr_t)instance->definition;

                if (definition->occlusion_radius == 50.0f || (definition->flags & 1) != 0) {
                    rasterizer_sun_glow_render(instance);
                }
            }
        }
    }
}

/**
 * Takes no arguments. For every lens flare instance of the current window computes the occlusion sample point (behind
 * the camera forward axis, along the flare normal or at the instance position, per the definition's occlusion offset
 * direction) and issues the occlusion query, storing the returned sample count in the instance.
 *
 * @address 0x513ba0
 */
void lens_flare_update_samples(void)
{
    int16_t i;
    real_vector3d unpack_scratch;
    real_vector3d normal;
    real_point3d sample_point = { 0.0f, 0.0f, 0.0f };
    LensFlare *definition;
    float radius;

    if (halo::rasterizer::fields::decals_and_lens_flares_enabled == 0 || halo::rasterizer::fields::screenshot_tile_count > 1 ||
        (halo::rasterizer::fields::screenshot_tile_count == 1 && screenshot_scale > 1) ||
        rasterizer_window.type != 1 || lens_flare_instance_count <= 0) {
        return;
    }

    rasterizer_lens_flare_batching_select_mode(6, 1);

    for (i = 0; i < lens_flare_instance_count; i++) {
        lens_flare_instance *instance = &lens_flare_instances[i];

        definition = (LensFlare *)instance->definition;
        normal = *vector3d_unpack_normal_11_11_10(&unpack_scratch, instance->packed_direction);
        if ((int16_t)(instance->window_flags & 0x7f) != rasterizer_window.window_index) {
            continue;
        }
        radius = definition->occlusion_radius;
        switch (definition->occlusion_offset_direction) {
        case 0:
            halo::math::point3d_add_scaled(sample_point, *((real_vector3d *)&rasterizer_window.camera.forward), instance->position,
                               -radius);
            break;
        case 1:
            halo::math::point3d_add_scaled(sample_point, normal, instance->position, radius * 1.4142135f);
            break;
        case 2:
            sample_point = instance->position;
            break;
        }
        instance->sample_count = halo::render::render_rasterizer_dispatch_537800(i, &sample_point, radius);
    }

    halo::render::rasterizer_effect_slot_release_active();
}

/**
 * 0x537b40, ESI slot = the loop index (0x5137c0) Per-frame smoothing pass: while occlusion queries are enabled
 * (halo::rasterizer::fields::decals_and_lens_flares_enabled) and the window/mode gate allows it, blends each active lens flare's visibility byte
 * toward its freshly sampled occlusion percentage (0 when it has no samples), then clears the active count.
 *
 * @address 0x513780
 */
void lens_flare_update_visibility(void)
{
    int32_t i;
    uint8_t *slot;
    int32_t percent;
    uint8_t old_value;
    uint8_t new_value;

    if (halo::rasterizer::fields::decals_and_lens_flares_enabled == 0 || halo::rasterizer::fields::screenshot_tile_count >= 2 ||
        (halo::rasterizer::fields::screenshot_tile_count == 1 && screenshot_scale >= 2)) {
        return;
    }

    for (i = 0; i < lens_flare_instance_count; i++) {
        slot = lens_flare_get_visibility_byte(&lens_flare_instances[i]);

        if (lens_flare_instances[i].sample_count < 1) {
            *slot = 0;
            continue;
        }

        percent = (rasterizer_lens_flare_occlusion_query_get_result(i) * 0xff +
                   (lens_flare_instances[i].sample_count >> 1)) /
                  lens_flare_instances[i].sample_count;
        if (percent < 0xff) {
            new_value = (uint8_t)percent;
            if (new_value == 0) {
                *slot = 0;
                continue;
            }
        } else {
            new_value = 0xff;
        }

        old_value = *slot;
        if (old_value < new_value) {
            new_value = (uint8_t)(((uint32_t)old_value * 3 + (uint32_t)new_value) >> 2);
        } else if (old_value > new_value) {
            new_value = (uint8_t)(((uint32_t)old_value + (uint32_t)new_value) / 2);
        } else {
            continue;
        }
        *slot = new_value;
    }

    lens_flare_instance_count = 0;
}

/**
 * Finds (or LRU-evicts and reassigns) a batching slot matching the current material key for the screen-space
 * sprite rendering system by comparing it against the last-applied key and only re-binding the texture/shader-
 * stage state that actually changed.
 *
 * @address 0x536b70
 */
uint8_t rasterizer_lens_flare_batch_apply_material(lens_flare_batch_key *key)
{
    uint8_t ok = 1;

    if (lens_flare_applied_key.bitmap_tag_index != key->bitmap_tag_index ||
        lens_flare_applied_key.second_bitmap_tag_index != key->second_bitmap_tag_index ||
        lens_flare_applied_key.bitmap_index != key->bitmap_index) {
        uint8_t failed;
        if (key->second_bitmap_tag_index == -1) {

            failed = rasterizer_validate_and_rebind_texture(0xffffffffu, (int16_t)(uint16_t)key->bitmap_tag_index,
                (int16_t)(uint16_t)key->bitmap_index);
        } else {

            failed = rasterizer_resolve_and_cache_submap_c((uint32_t)key->second_bitmap_tag_index, 0,
                (int16_t)(uint16_t)key->bitmap_tag_index, 1, (int16_t)(uint16_t)key->bitmap_index);
        }
        ok = (failed == 0);
        lens_flare_applied_key.bitmap_tag_index = key->bitmap_tag_index;
        lens_flare_applied_key.second_bitmap_tag_index = key->second_bitmap_tag_index;
        lens_flare_applied_key.bitmap_index = key->bitmap_index;
    }

    if (lens_flare_applied_key.shader_stage_config != key->shader_stage_config) {
        rasterizer_set_shader_stage_config((int16_t)key->shader_stage_config);
        lens_flare_applied_key.shader_stage_config = key->shader_stage_config;
    }

    return ok;
}


/**
 * Draws and clears one batched vertex-quad slot of the screen-space sprite (lens-flare/decal) rendering
 * system, if it has any vertices queued.
 *
 * @address 0x536c10
 */
void rasterizer_lens_flare_batch_draw_slot(int32_t batch_index)
{
    lens_flare_batch *batch = &lens_flare_batches[batch_index];

    if (batch->vertex_count != 0) {
        if (rasterizer_lens_flare_batch_apply_material(&batch->key) != 0) {
            render_device().draw_primitive_up(4, (uint32_t)batch->vertex_count / 3, batch->vertices, 0x20);
        }
    }
    batch->last_used = 0;
    batch->vertex_count = 0;
}

static int key_equal(const lens_flare_batch_key *a, const lens_flare_batch_key *b)
{
    return a->bitmap_tag_index == b->bitmap_tag_index &&
           a->second_bitmap_tag_index == b->second_bitmap_tag_index &&
           a->bitmap_index == b->bitmap_index &&
           *(const uint32_t *)&a->shader_stage_config == *(const uint32_t *)&b->shader_stage_config;
}

/**
 * Finds (or LRU-evicts and reassigns) a batching slot matching the current material key for the screen-space
 * sprite rendering system, returning its slot index.
 *
 * @address 0x536cb0
 */
int32_t rasterizer_lens_flare_batch_find_slot(void)
{
    int32_t i;
    int32_t lru_index = 0;
    uint32_t lru_last_used = lens_flare_batches[0].last_used;

    for (i = 0; i < k_lens_flare_batch_slots; i++) {
        if (key_equal(&lens_flare_batches[i].key, &lens_flare_current_key)) {
            lens_flare_batches[i].last_used = lens_flare_batch_clock + 1;
            lens_flare_batch_clock++;
            return i;
        }
        if (lens_flare_batches[i].last_used < lru_last_used) {
            lru_last_used = lens_flare_batches[i].last_used;
            lru_index = i;
        }
    }

    rasterizer_lens_flare_batch_draw_slot(lru_index);
    lens_flare_batches[lru_index].key = lens_flare_current_key;
    lens_flare_batches[lru_index].last_used = lens_flare_batch_clock + 1;
    lens_flare_batch_clock++;
    return lru_index;
}

/**
 * Flushes (draws) every non-empty batch slot of the screen-space sprite rendering system, typically once per
 * frame.
 *
 * @address 0x536c80
 */
void rasterizer_lens_flare_batch_flush_all(void)
{
    int i;
    for (i = 0; i < k_lens_flare_batch_slots; i++) {
        if (lens_flare_batches[i].vertex_count != 0) {
            rasterizer_lens_flare_batch_draw_slot(i);
        }
    }
}

namespace rasterizer_lens_flare_batching_select_mode_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(stage, type, value);
}

/**
 * Selects a rendering technique/mode (values 5 and 6 observed) for the screen-space sprite (lens-flare/decal)
 * batching system and, for mode 5, resets all of its per-frame state.
 *
 * @address 0x537130
 */
void rasterizer_lens_flare_batching_select_mode(int16_t mode, uint32_t flags)
{
    if (mode == 6) {
        set_render_state(halo::d3d9::rs::cull_mode, 3);
        set_render_state(halo::d3d9::rs::color_write_enable, (console_debug_toggle_689425 != 0) ? 7u : 0u);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        set_render_state(halo::d3d9::rs::z_write_enable, console_debug_toggle_689425);
        set_render_state(halo::d3d9::rs::fog_enable, 0);
        set_render_state(halo::d3d9::rs::texture_factor, 0xffff0000);

        set_texture_stage_state(0, halo::d3d9::ts::color_op, 2);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg1, 3);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_op, 2);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, 1);
        set_texture_stage_state(1, halo::d3d9::ts::color_op, 1);
        set_texture_stage_state(1, halo::d3d9::ts::alpha_op, 1);

        render_device().set_vertex_shader(0);
        render_device().set_pixel_shader(0);
        render_device().set_fvf(0x144);
        return;
    }
    if (mode != 5) {
        return;
    }

    {
        uint32_t z_enable = (lens_flare_occlusion_queries_supported == 0) ? 1u : (flags & 1);

        set_render_state(halo::d3d9::rs::cull_mode, 3);
        set_render_state(halo::d3d9::rs::color_write_enable, 7);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
        set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        set_render_state(halo::d3d9::rs::blend_op, 1);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, z_enable);
        set_render_state(halo::d3d9::rs::z_func, 4);
        set_render_state(halo::d3d9::rs::z_write_enable, (flags >> 1) & 1);
        set_render_state(halo::d3d9::rs::fog_enable, 0);
    }

    rasterizer_effect_pool_scratch = &halo::rasterizer::fields::lens_flare_effect;
    if (halo::rasterizer::fields::lens_flare_effect != 0) {
        uint32_t pass_count = 0;
        render_device().effect_begin(halo::rasterizer::fields::lens_flare_effect, &pass_count, 3);

        render_device().effect_pass(*(void **)rasterizer_effect_pool_scratch, 0);
    } else {
        set_texture_stage_state(0, halo::d3d9::ts::color_op, 4);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg1, 2);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg2, 0);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_op, 2);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, 0);
        set_texture_stage_state(1, halo::d3d9::ts::color_op, 1);
        set_texture_stage_state(1, halo::d3d9::ts::alpha_op, 1);
        render_device().set_pixel_shader(0);
    }

    {
        uint32_t address_mode = (rasterizer_caps.texture_address_caps & 8) ? 4u : 3u;
        set_sampler_state(0, halo::d3d9::ss::address_u, address_mode);
        set_sampler_state(0, halo::d3d9::ss::address_v, address_mode);
    }
    set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    render_device().set_vertex_shader(0);
    render_device().set_fvf(0x1c4);

    rasterizer_set_shader_stage_config(0);

    lens_flare_batch_clock = 0;
    lens_flare_current_key.bitmap_tag_index = 0;
    lens_flare_current_key.second_bitmap_tag_index = 0;
    lens_flare_current_key.bitmap_index = 0;
    lens_flare_current_key.shader_stage_config = 0;
    lens_flare_applied_key.bitmap_tag_index = 0;
    lens_flare_applied_key.second_bitmap_tag_index = 0;
    lens_flare_applied_key.bitmap_index = 0;
    lens_flare_applied_key.shader_stage_config = 0;
    lens_flare_vertex_specular = 0xffffffff;
}

}  // namespace rasterizer_lens_flare_batching_select_mode_impl

namespace rasterizer_lens_flare_occlusion_queries_create_impl {


/**
 * Creates one Direct3D occlusion query per lens-flare slot (up to 1024), used to fade flares based on their
 * visibility, disabling the feature if the driver does not support occlusion queries.
 *
 * @address 0x536f70
 */
uint8_t rasterizer_lens_flare_occlusion_queries_create(void)
{
    uint8_t ok = 1;
    int i;

    lens_flare_occlusion_queries_supported = 1;
    for (i = 0; i < k_lens_flare_occlusion_queries; i++) {
        lens_flare_occlusion_queries[i] = 0;
    }

    for (i = 0; i < k_lens_flare_occlusion_queries; i++) {
        void *query = 0;
        int32_t hr;
        if (!ok) {
            break;
        }
        hr = render_device().create_query(9, &query);
        if (hr < 0) {
            ok = 0;
            if (hr == (int32_t)0x8876086a) {
                lens_flare_occlusion_queries_supported = 0;
            }
        } else {
            lens_flare_occlusion_queries[i] = query;
        }
    }
    return (lens_flare_occlusion_queries_supported != 0) == ok;
}

}  // namespace rasterizer_lens_flare_occlusion_queries_create_impl

namespace rasterizer_lens_flare_occlusion_query_get_result_impl {


/**
 * Polls the occlusion query for one lens-flare slot until a result is available, returning the query's
 * visible-pixel-count result. Note: the original overwrites its own "query pointer" stack slot in place
 * with the 4-byte GetData result and returns that slot's value reinterpreted as a pointer, rather than
 * returning through a separate out-parameter; the same in-place reuse is reproduced here via `slot`.
 *
 * @address 0x537b40
 */
int32_t rasterizer_lens_flare_occlusion_query_get_result(int32_t slot_index)
{
    int32_t value = -1;
    void *query;
    int32_t hr;

    if (console_debug_toggle_689424 == 0) {
        return 1;
    }

    query = lens_flare_occlusion_queries[slot_index];
    if (query != 0 && slot_index < k_lens_flare_occlusion_queries) {

        hr = render_device().query_get_data(query, &value, 4, 1);
        while (hr == 1) {
            query = lens_flare_occlusion_queries[slot_index];
            hr = render_device().query_get_data(query, &value, 4, 1);
        }
        return value;
    }
    return 2;
}

}  // namespace rasterizer_lens_flare_occlusion_query_get_result_impl

/**
 * Registers one occlusion-test sample point (camera-relative offset plus caller-supplied ids) for a lens
 * flare/light source to be tested this frame, up to a fixed capacity of 384. `procedure` is stored as the
 * group's callback (a NULL-shader transparent_geometry_group), so it doubles as the "do nothing" guard.
 *
 * @address 0x536ff0
 */
void rasterizer_lens_flare_occlusion_sample_add(void *procedure, const real_point3d *position, uint32_t id_1, uint32_t id_2)
{
    int32_t index;
    transparent_geometry_group *group;
    float dx, dy, dz;

    if (procedure == 0) {
        return;
    }
    if (transparent_geometry_group_count >= k_rasterizer_maximum_transparent_groups) {
        if (transparent_geometry_group_overflow_d == 0) {
            transparent_geometry_group_overflow_d = 1;
        }
        return;
    }

    index = transparent_geometry_group_count;
    transparent_geometry_groups[index].sorted_index = index;
    group = &transparent_geometry_groups[index];
    transparent_geometry_group_count++;

    dx = position->x - rasterizer_window.camera.position.x;
    dy = position->y - rasterizer_window.camera.position.y;
    dz = position->z - rasterizer_window.camera.position.z;

    group->dynamic_index_slot = -1;
    group->index_buffer = (uint32_t)procedure;
    group->first_index = (int32_t)id_1;
    group->primitive_count = (int32_t)id_2;
    group->dynamic_vertex_slot = -1;

    group->flags = 0;
    group->object_index = 0;
    group->sort_key = 0;
    group->shader = 0;
    group->shader_permutation = 0;
    group->unknown_12 = 0;
    group->vertex_buffer = 0;
    group->lightmap_bitmap = 0;

    group->depth = -(rasterizer_window.camera.forward.i * dx + rasterizer_window.camera.forward.k * dz +
                     rasterizer_window.camera.forward.j * dy);
    group->position.x = position->x;
    group->position.y = position->y;
    group->position.z = position->z;
    group->tint.alpha = 0.0f;
    group->tint.red = 0.0f;
    group->tint.green = 0.0f;
    group->tint.blue = 0.0f;
    group->previous_group_index = -1;
    group->next_group_index = -1;

    group->base_map_v_scale = 1.0f;
    group->base_map_u_scale = 1.0f;
    group->parent_sort_key = 0;
    group->first_person = 0;
    group->node_matrices = 0;
    group->node_count = 0;
    group->lighting = 0;
    group->lighting_extra = 0;
}

namespace rasterizer_lens_flare_occlusion_test_issue_impl {


static int16_t floor_clamped(float value)
{
    if (value < -32767.0f) {
        value = -32767.0f;
    } else if (value > 32767.0f) {
        value = 32767.0f;
    }
    return (int16_t)(int32_t)(float)halo::libm::floor((double)value);
}

static void set_vertex(rasterizer_screen_vertex *vertex, int16_t x, int16_t y, float z, float rhw, float u, float v)
{
    vertex->x = (float)x;
    vertex->y = (float)y;
    vertex->z = z;
    vertex->rhw = rhw;
    vertex->diffuse = 0xffffffff;
    vertex->u = u;
    vertex->v = v;
}

/**
 * Direct3D 9 back end function rasterizer_lens_flare_occlusion_test_issue. The original author notes are in
 * docs/original/rasterizer/rasterizer_lens_flare_occlusion_test_issue.c.txt.
 *
 * @address 0x537800
 */
int32_t rasterizer_lens_flare_occlusion_test_issue(int32_t slot_index, const real_point3d *position, float radius)
{
    float screen[3];
    float inverse_w;
    float half_size[2];
    int16_t x0, y0, x1, y1;
    int32_t area;

    if (console_debug_toggle_689424 == 0) {
        return 1;
    }
    if (slot_index >= k_lens_flare_occlusion_queries) {
        return 0;
    }
    if ((uint8_t)rasterizer_lens_flare_project_to_screen(position, radius, screen, &inverse_w, half_size) == 0) {
        return 0;
    }
    if (1.0f > half_size[0]) {
        half_size[0] = 1.0f;
    }
    if (1.0f > half_size[1]) {
        half_size[1] = 1.0f;
    }
    x0 = floor_clamped(screen[0] - half_size[0]);
    y0 = floor_clamped(screen[1] - half_size[1]);
    x1 = floor_clamped(screen[0] + half_size[0]);
    y1 = floor_clamped(screen[1] + half_size[1]);
    area = ((int32_t)x1 - (int32_t)x0) * ((int32_t)y1 - (int32_t)y0);
    if (area < 0) {
        area = 0;
    }
    if (lens_flare_occlusion_queries_supported == 0) {
        return 4;
    }
    if (area > 0 && lens_flare_occlusion_queries[slot_index] != 0) {
        void *query = lens_flare_occlusion_queries[slot_index];
        rasterizer_screen_vertex quad[4];

        render_device().query_issue(query, 2);
        set_vertex(&quad[0], x0, y0, screen[2], inverse_w, 0.0f, 0.0f);
        set_vertex(&quad[1], x1, y0, screen[2], inverse_w, 1.0f, 0.0f);
        set_vertex(&quad[2], x1, y1, screen[2], inverse_w, 1.0f, 1.0f);
        set_vertex(&quad[3], x0, y1, screen[2], inverse_w, 0.0f, 1.0f);
        render_device().draw_primitive_up(6, 2, quad, sizeof(rasterizer_screen_vertex));
        query = lens_flare_occlusion_queries[slot_index];
        render_device().query_issue(query, 1);
    }
    return area;
}

}  // namespace rasterizer_lens_flare_occlusion_test_issue_impl

/**
 * Projects a world-space point and radius into screen-space position plus billboard size, used to place a
 * screen-space sprite (lens flare / decal) quad.
 *
 * @address 0x536d80
 */
uint8_t rasterizer_lens_flare_project_to_screen(const real_point3d *position, float radius, float *out_screen, float *out_inverse_w, float *out_billboard_size)
{
    real_point3d view_point;
    float clip_w, clip_z;
    float projected_x, projected_y;
    int16_t width, height;
    const float (*proj)[4] = rasterizer_window.frustum.projection;

    if (radius <= 0.0f) {
        return 0;
    }

    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);

    halo::math::matrix4x3_transform_point(view_point, *(real_point3d *)position, rasterizer_window.frustum.world_to_view);

    projected_x = proj[0][1] * view_point.x + proj[1][1] * view_point.y + proj[2][1] * view_point.z + proj[3][1];
    clip_w = proj[0][2] * view_point.x + proj[1][2] * view_point.y + proj[2][2] * view_point.z + proj[3][2];
    projected_y = proj[0][0] * radius;
    radius = proj[1][1] * radius;

    if (clip_w > 0.0f) {
        float inv_w = 1.0f / (proj[0][3] * view_point.x + proj[1][3] * view_point.y + proj[2][3] * view_point.z +
                              proj[3][3]);
        float screen_x = (((proj[0][0] * view_point.x + proj[1][0] * view_point.y + proj[2][0] * view_point.z +
                            proj[3][0]) * inv_w + 1.0f) * (float)width - 1.0f) * 0.5f;
        float height_f = (float)height;
        float screen_y = ((1.0f - inv_w * projected_x) * height_f - 1.0f) * 0.5f;
        float depth = inv_w * clip_w;

        if (depth >= 1.0f) {
            depth = 1.0f;
        }

        out_screen[0] = screen_x;
        out_screen[1] = screen_y;
        out_screen[2] = depth;
        *out_inverse_w = inv_w;
        out_billboard_size[0] = (float)width * inv_w * projected_y * 0.5f;
        out_billboard_size[1] = height_f * inv_w * radius * 0.5f;
        return 1;
    }
    return 0;
}

/**
 * FSIN Builds and appends one screen-space sprite quad (lens flare / decal), with optional rotation and non-
 * uniform scale, into the appropriate material batch, flushing the batch when full.
 *
 * @address 0x537550
 */
void rasterizer_lens_flare_quad_add(const float *scale, uint32_t diffuse, const real_point3d *position, float radius, float rotation_degrees)
{
    float screen[3];
    float inverse_w;
    float billboard[2];

    if (radius <= 0.0f) {
        return;
    }
    if (!rasterizer_lens_flare_project_to_screen(position, radius, screen, &inverse_w, billboard)) {
        return;
    }

    {
        float axis_u, axis_v;
        float scale_x, scale_y;
        int32_t slot, count;
        lens_flare_vertex *v;

        if (rotation_degrees == 0.0f) {
            axis_u = billboard[0];
            axis_v = billboard[1];
        } else {
            double angle = (double)rotation_degrees * 0.017453292;
            double c = halo::x87::fcos(angle);
            double s = halo::x87::fsin(angle);
            axis_u = (float)((double)billboard[0] * c - (double)billboard[1] * s);
            axis_v = (float)((double)billboard[1] * c + (double)billboard[0] * s);
        }

        if (scale == 0) {
            scale_x = 1.0f;
            scale_y = 1.0f;
        } else {
            scale_x = scale[0];
            scale_y = scale[1];
        }

        slot = rasterizer_lens_flare_batch_find_slot();
        count = lens_flare_batches[slot].vertex_count;
        v = &lens_flare_batches[slot].vertices[count];

        {

            float corner1_x = screen[0] - scale_x * axis_u, corner1_y = screen[1] - scale_y * axis_v;
            float corner2_x = screen[0] + scale_x * axis_v, corner2_y = screen[1] - scale_y * axis_u;
            float corner3_x = screen[0] + scale_x * axis_u, corner3_y = screen[1] + scale_y * axis_v;
            float corner4_x = screen[0] - scale_x * axis_v, corner4_y = screen[1] + scale_y * axis_u;

            v[0].x = corner1_x; v[0].y = corner1_y; v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].x = corner2_x; v[1].y = corner2_y; v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].x = corner3_x; v[2].y = corner3_y; v[2].u = 1.0f; v[2].v = 1.0f;
            v[3].x = corner1_x; v[3].y = corner1_y; v[3].u = 0.0f; v[3].v = 0.0f;
            v[4].x = corner3_x; v[4].y = corner3_y; v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].x = corner4_x; v[5].y = corner4_y; v[5].u = 0.0f; v[5].v = 1.0f;

            {
                int i;
                for (i = 0; i < 6; i++) {
                    v[i].z = screen[2];
                    v[i].rhw = inverse_w;
                    v[i].diffuse = diffuse;
                    v[i].specular = lens_flare_vertex_specular;
                }
            }
        }

        lens_flare_batches[slot].vertex_count = count + 6;
        if (count + 6 == k_lens_flare_batch_vertices) {
            rasterizer_lens_flare_batch_draw_slot(slot);
        }
    }
}

/**
 * Direct3D 9 back end function structure_cluster_add_lens_flares. The original author notes are in
 * docs/original/rasterizer/structure_cluster_add_lens_flares.c.txt.
 *
 * Registers: CX -> cluster_index
 *
 * @address 0x513a00
 */
void structure_cluster_add_lens_flares(int16_t cluster_index)
{
    ScenarioStructureBSP *bsp;
    const uint8_t *cluster;
    uint32_t marker_ordinal;

    if (halo::rasterizer::fields::decals_and_lens_flares_enabled == 0 || halo::rasterizer::fields::screenshot_tile_count > 1 || (halo::rasterizer::fields::screenshot_tile_count == 1 && screenshot_scale > 1)) {
        return;
    }

    bsp = global_structure_bsp;
    cluster = (const uint8_t *)((struct ScenarioStructureBSP *)bsp)->clusters.pointer + cluster_index * 0x68;
    for (marker_ordinal = 0; marker_ordinal < *(const uint16_t *)(cluster + 0x42); marker_ordinal++) {
        uint32_t marker_index = *(const uint16_t *)(cluster + 0x40) + marker_ordinal;
        const ScenarioStructureBSPLensFlareMarker *marker =
            (const ScenarioStructureBSPLensFlareMarker *)((struct ScenarioStructureBSP *)bsp)->lens_flare_markers.pointer + marker_index;
        const uint8_t *palette = (const uint8_t *)((struct ScenarioStructureBSP *)bsp)->lens_flares.pointer + marker->lens_flare_index * 0x10;
        real_vector3d direction;
        real_vector3d up;
        lens_flare_instance candidate;

        direction.i = (float)marker->direction_i_component * (1.0f / 127.0f);
        direction.j = (float)marker->direction_j_component * (1.0f / 127.0f);
        direction.k = (float)marker->direction_k_component * (1.0f / 127.0f);
        halo::math::vector3d_build_perpendicular(up, direction);
        halo::math::vector3d_normalize_with_length(direction);
        halo::math::vector3d_normalize_with_length(up);

        candidate.packed_direction = vector3d_pack_normal_11_11_10(&direction);
        candidate.packed_up = vector3d_pack_normal_11_11_10(&up);
        candidate.definition = (uint32_t)halo::cache::globals().tag_instances[*(const uint32_t *)(palette + 0xc) & halo::k_slot_mask].data;
        candidate.position.x = marker->position.x;
        candidate.position.y = marker->position.y;
        candidate.position.z = marker->position.z;
        candidate.color = 0xffffffff;
        candidate.object_index = -1;
        candidate.visibility_high = (int16_t)((int32_t)marker_index >> 16);
        candidate.visibility_low = (int16_t)marker_index;
        candidate.window_flags = (uint8_t)render_window_index;
        candidate.intensity = 0;
        lens_flare_add_instance(&candidate);
    }
}

}  // namespace halo::rasterizer
