#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#include "rasterizer.h"
#include "interface.h"
#include "structures.h"
#include "cutscene.h"
#include "shaders.h"
#include "render.h"
#include <stdint.h>
#include "halo/render/render.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/render/api.hpp"
#include "halo/rasterizer/api.hpp"

extern "C" {
extern float build_sprite_screen_coverage;
extern int16_t build_sprite_large_quad_count;
extern real_vector3d build_sprite_view_up;
extern real_vector3d build_sprite_view_left;
extern render_frustum render_frustum_global;
extern const ColorARGB *global_white_argb;
extern real_rectangle3d *global_null_rectangle3d_pointer;
extern uint32_t color_pack_argb_from_real(ColorARGB *color);
extern double sin(double x);
extern double cos(double x);
extern uint8_t build_sprite_group_warning;
extern double fmod(double x, double y);
extern double atan2(double y, double x);
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots];
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count];
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots];
extern double sqrt(double x);
extern double fabs(double x);
extern render_camera render_camera_global;
extern float unknown_00672f20;
extern data_array *contrail_point_data;
extern data_array *object_data;
extern real_point3d *global_zero_vector3d_pointer;
extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence);
extern data_array *contrail_data;
extern uint8_t particle_spawn_debug_mode;
extern int16_t current_local_player_index;
extern data_array *particle_data;
extern first_person_weapon_interface *first_person_weapon_interfaces;
extern int32_t render_frame_index;
}

typedef int32_t (__stdcall *d3d_unlock_fn)(void *self);

/**
 * Computes the width and packed color of one contrail point from its interpolated state.
 */
static void point_state_width_and_color(ContrailPointState *state, contrail_point *point,
                                        float *width, ColorARGB *color)
{
    float t = 1.0f;

    if ((state->scale_flags & 0x20) != 0) {
        t = point->scale;
    }
    *width = state->width;
    if ((state->scale_flags & 0x10) != 0) {
        *width = *width * point->scale;
    }
    color->alpha = (state->color_upper_bound.alpha - state->color_lower_bound.alpha) * t +
                   state->color_lower_bound.alpha;
    color->red = (state->color_upper_bound.red - state->color_lower_bound.red) * t +
                 state->color_lower_bound.red;
    color->green = (state->color_upper_bound.green - state->color_lower_bound.green) * t +
                   state->color_lower_bound.green;
    color->blue = (state->color_upper_bound.blue - state->color_lower_bound.blue) * t +
                  state->color_lower_bound.blue;
}

/**
 * Finds data's existing group for `bitmap`, or appends a new one (failing once
 * k_maximum_build_sprite_groups is reached) and locks maximum_sprite_count*4 dynamic vertices for
 * it. Returns the group index, or -1 on failure (including when the group has no locked
 * vertices).
 *
 * @address 0x00511520
 */
int16_t halo::render::SpriteBuilder::get_group(BitmapData *bitmap)
{
    build_sprite_data *data = self;
    int16_t count = data->group_count;
    int16_t i;
    build_sprite_group *group;

    for (i = 0; i < count; i++) {
        if (data->groups[i].bitmap == (uint32_t)bitmap) {
            break;
        }
    }

    if (count <= i) {
        if (count > 7) {
            return -1;
        }
        group = &data->groups[i];
        data->group_count = count + 1;
        group->bitmap = (uint32_t)bitmap;

        if (halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
            group->vertices = 0;
            group->vertex_slot = -1;
        } else {
            int32_t vertex_count = (int32_t)data->maximum_sprite_count * 4;
            int16_t vertex_type = (data->flags & _build_sprite_data_screen_space_bit) != 0 ?
                                      _rasterizer_vertex_type_dynamic_screen :
                                      _rasterizer_vertex_type_dynamic_unlit;
            int32_t slot;

            halo::rasterizer::globals().vertex_buffer_lock_state = 0x10;
            slot = halo::rasterizer::rasterizer_dynamic_vertex_cache_reserve(vertex_type, vertex_count);
            group->vertex_slot = slot;
            if (slot == -1) {
                if (build_sprite_group_warning == 0) {
                    build_sprite_group_warning = 1;
                }
                group->vertices = 0;
                halo::rasterizer::globals().vertex_buffer_lock_state = 0;
            } else {
                group->vertices = (uint32_t)(uintptr_t)halo::rasterizer::rasterizer_dynamic_vertex_cache_lock(slot);
                halo::rasterizer::globals().vertex_buffer_lock_state = 0;
            }
        }
        group->quad_count = 0;
    }

    if (i != -1 && data->groups[i].vertices == 0) {
        return -1;
    }
    return i;
}

/**
 * Draws a sprite that has an axis (a spinning or rotating particle): as a blend of an end-on,
 * camera facing quad from the next sequence and a side-on quad rotated to the projected axis,
 * weighted by how directly the axis points at the viewer.
 *
 * @address 0x00511b40
 */
void halo::render::SpriteBuilder::rotational(uint32_t flags, int16_t first_sequence_index, int16_t sprite_index,
    real_point3d *origin, real_vector3d *axis, float rotation, float scale, ColorARGB *color, float fade)
{
    build_sprite_data *data = self;
    real_point3d transformed_origin;
    real_vector3d transformed_axis;
    Bitmap *bitmap_group;
    BitmapGroupSequence *sequences;
    real d;
    real t;

    if (color == 0) {
        color = (ColorARGB *)global_white_argb;
    }

    halo::render::render_sprite_transform_point_and_normal(origin, axis, &transformed_axis, data,
                                             (uint8_t)(flags & 1), &transformed_origin);
    d = halo::math::vector3d_angle_between_4cd4f0(transformed_axis, *((real_vector3d *)&transformed_origin)) -
        1.5707964f;
    t = d * d * 0.40528470f;

    if (t < 0.0f) {
        t = 0.0f;
    } else {
        uint8_t draw = 1;

        if (t > 1.0f) {
            t = 1.0f;
        } else if (t <= 0.05f) {
            draw = 0;
        }
        if (draw) {
            int16_t count;
            int16_t sprite;
            float quad_rotation;
            uint32_t quad_flags = 1;

            bitmap_group = (Bitmap *)halo::cache::globals().tag_instances[(uint16_t)data->bitmap_group_index].data;
            sequences = (BitmapGroupSequence *)bitmap_group->bitmap_group_sequence.pointer;
            count = (int16_t)sequences[first_sequence_index + 1].sprites.count;

            if ((flags & 2) != 0) {
                sprite = (int16_t)(int32_t)(fmod((real)count * rotation * 0.15915494f + 0.5f,
                                                 (real)count) + (real)sprite_index);
                quad_rotation = 0.0f;
                if (d < 0.0f) {
                    sprite = (int16_t)(count - sprite_index);
                }
            } else {
                sprite = sprite_index;
                quad_rotation = rotation;
                if (d < 0.0f) {
                    quad_flags = 3;
                }
            }
            halo::render::build_sprite(data, (int16_t)(first_sequence_index + 1), sprite, 0, &transformed_origin,
                         0, quad_rotation, scale, color, t * fade, quad_flags);
        }
    }

    if (1.0f - t > 0.05f) {
        int16_t count;
        int16_t sprite;
        float side_fade = (1.0f - t) * fade;
        float side_rotation;

        bitmap_group = (Bitmap *)halo::cache::globals().tag_instances[(uint16_t)data->bitmap_group_index].data;
        sequences = (BitmapGroupSequence *)bitmap_group->bitmap_group_sequence.pointer;
        count = (int16_t)sequences[first_sequence_index].sprites.count;
        side_rotation = (real)atan2(transformed_axis.j, transformed_axis.i);
        sprite = (int16_t)(int32_t)fmod((real)count * rotation * 0.15915494f + 0.5f, (real)count);
        halo::render::build_sprite(data, first_sequence_index, sprite, 0, &transformed_origin, 0, side_rotation,
                     scale, color, side_fade, 1);
    }
}

/**
 * Builds the orientation basis of a billboard from its transformed position and normal for the given render type.
 *
 * @address 0x005111f0
 */
void halo::render::SpriteBuilder::billboard_build_orientation_basis(int16_t render_type, real_vector3d *position,
    real_vector3d *normal, billboard_basis *out)
{
    build_sprite_data *data = self;
    real_vector3d *axis;
    real dot;

    if ((data->flags & _build_sprite_data_screen_space_bit) != 0) {
        return;
    }

    if (render_type == 0) {
        out->tangent.i = 1.0f;
        out->tangent.j = 0.0f;
        out->tangent.k = 0.0f;
        out->bitangent.i = 0.0f;
        out->bitangent.j = 1.0f;
        out->bitangent.k = 0.0f;
        return;
    }

    if (render_type == 1) {
        out->tangent = *normal;
        halo::math::vector3d_normalize_with_length(out->tangent);
        halo::math::vector3d_cross_product(out->bitangent, out->tangent, *position);
        halo::math::vector3d_normalize_with_length(out->bitangent);
        return;
    }

    if (render_type == 2) {
        dot = build_sprite_view_up.j * normal->j + build_sprite_view_up.i * normal->i +
              build_sprite_view_up.k * normal->k;
        axis = &build_sprite_view_up;
        if ((normal->i * normal->i + normal->j * normal->j + normal->k * normal->k) * unknown_00672f20 < dot * dot) {
            axis = &build_sprite_view_left;
        }

        halo::math::vector3d_cross_product(out->tangent, *axis, *normal);
        halo::math::vector3d_normalize_with_length(out->tangent);

        out->bitangent = out->tangent;
        out->normal = *normal;
        halo::math::vector3d_normalize_with_length(out->normal);
        halo::math::vector3d_rotate_about_axis(out->bitangent, out->normal, -1.0f, 0.0f);
    }
}

/**
 * Computes the length or width scale factor of a billboard segment from its marker count or projected depth.
 *
 * @address 0x00511330
 */
void halo::render::SpriteBuilder::billboard_compute_scale(float *scale, int16_t render_type, real_point3d *position,
    BitmapData *bitmap)
{
    build_sprite_data *data = self;
    int16_t width = (int16_t)bitmap->width;

    if ((data->flags & _build_sprite_data_screen_space_bit) == 0) {
        if (render_type == 0 && *scale == 0.0f) {
            *scale = -(position->z / render_frustum_global.projection_world_to_screen.i);
        }
        *scale = (float)width * *scale;
        return;
    }

    if (*scale == 0.0f) {
        *scale = 1.0f;
        *scale = (float)width * *scale;
        return;
    }
    *scale = (float)width * *scale;
}

/**
 * Transforms a sprite's origin (and, if given, its normal) from world space into view space
 * through render_frustum_global.world_to_view, unless the sprite build is screen-space (data's
 * screen_space flag) or the caller marks the values as already transformed, in which case they
 * are copied through unchanged.
 *
 * @address 0x00511190
 */
void halo::render::SpriteBuilder::sprite_transform_point_and_normal(real_point3d *position, real_vector3d *normal,
    real_vector3d *out_normal, uint8_t flags, real_point3d *out_position)
{
    build_sprite_data *data = self;
    if ((data->flags & _build_sprite_data_screen_space_bit) != 0) {
        return;
    }

    if ((flags & _build_sprite_already_transformed_bit) == 0) {
        halo::math::matrix4x3_transform_point(*out_position, *position, render_frustum_global.world_to_view);
        if (normal != 0) {
            halo::math::matrix4x3_transform_normal(*out_normal, *normal, render_frustum_global.world_to_view);
        }
    } else {
        *out_position = *position;
        if (normal != 0) {
            *out_normal = *normal;
        }
    }
}

namespace halo::render::billboard {

/**
 * Per-frame initialization for the billboard/sprite build system: resets the large-quad
 * occlusion accumulators and computes the world up/left axes in view space (through
 * render_frustum_global.world_to_view) for use by build_sprite's render-type-2 orientation.
 *
 * @address 0x00511410
 */
void frame_init(void)
{
    build_sprite_screen_coverage = 0.0f;
    build_sprite_large_quad_count = 0;

    halo::math::matrix4x3_transform_normal(build_sprite_view_up, *(real_vector3d *)halo::math::globals().global_up3d_pointer,
                                render_frustum_global.world_to_view);
    halo::math::matrix4x3_transform_normal(build_sprite_view_left, *(real_vector3d *)halo::math::globals().global_left3d_pointer,
                                render_frustum_global.world_to_view);
}

/**
 * Computes the 0..1 view-to-normal alignment fade of a billboard segment, inverted for one render type.
 *
 * @address 0x005113b0
 */
real compute_view_fade(real_vector3d *a, real_vector3d *b, int16_t render_type)
{
    real fade;
    real dot;
    real length;

    if (render_type == 0) {
        return 1.0f;
    }

    dot = a->i * b->i + a->j * b->j + a->k * b->k;
    length = (real)sqrt((double)(a->i * a->i + a->j * a->j + a->k * a->k));
    fade = (real)fabs((double)(dot / length));

    if (render_type == 2) {
        fade = 1.0f - fade;
    }
    return fade;
}

}  // namespace halo::render::billboard

namespace halo::render::sprite {

/**
 * Appends one quad for sprite `sprite_index` of sequence `sequence_index` of the bitmap of `data`
 * to the vertex group of its texture page: the corners are the sprite rectangle around its
 * registration point, rotated by `rotation`, optionally mirrored (flags bits 1 and 2), scaled and
 * laid out on the orientation basis of `mode` around the transformed origin (screen space builds
 * skip the basis and write x/y only). Colour is `color` (white when NULL) with its alpha scaled by
 * `fade`, and by the view angle fade of the shader's framebuffer fade mode.
 *
 * @address 0x00511700
 */
void draw(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode, real_point3d *origin,
    real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade, uint32_t flags)
{
    Bitmap *bitmap_group;
    BitmapGroupSequence *sequence;
    BitmapGroupSprite *sprite;
    BitmapData *bitmap;
    build_sprite_group *group;
    LightningShader *shader;
    int16_t group_index;
    int16_t vertex_index;
    real_rectangle3d bounds;
    real sin_rotation;
    real cos_rotation;
    real_point3d transformed_origin;
    real_vector3d transformed_direction;
    billboard_basis basis;
    uint32_t packed_color;
    uint32_t vertex_color;
    uint32_t mirror_u;
    uint32_t mirror_v;
    int16_t corner;

    bitmap_group = (Bitmap *)halo::cache::globals().tag_instances[(uint16_t)data->bitmap_group_index].data;
    if (color == 0) {
        color = (ColorARGB *)global_white_argb;
    }

    if (data->sprite_count >= data->maximum_sprite_count) {
        return;
    }
    if (sequence_index < 0 || (int32_t)sequence_index >= (int32_t)bitmap_group->bitmap_group_sequence.count) {
        return;
    }
    sequence = &((BitmapGroupSequence *)bitmap_group->bitmap_group_sequence.pointer)[sequence_index];
    if ((int16_t)sequence->first_bitmap_index == -1) {
        return;
    }
    if (sprite_index < 0 || (int32_t)sprite_index >= (int32_t)sequence->sprites.count) {
        return;
    }
    sprite = &((BitmapGroupSprite *)sequence->sprites.pointer)[sprite_index];
    bitmap = &((BitmapData *)bitmap_group->bitmap_data.pointer)[(int16_t)sprite->bitmap_index];

    group_index = halo::render::build_sprite_get_group(data, bitmap);
    if (group_index == -1) {
        return;
    }
    group = &data->groups[group_index];
    if (group->quad_count >= data->maximum_sprite_count) {
        return;
    }
    vertex_index = (int16_t)(group->quad_count * 4);

    bounds = *global_null_rectangle3d_pointer;
    sin_rotation = 0.0f;
    cos_rotation = 1.0f;
    if (rotation != 0.0f) {
        sin_rotation = (real)sin(rotation);
        cos_rotation = (real)cos(rotation);
    }

    halo::render::render_sprite_transform_point_and_normal(origin, direction, &transformed_direction, data,
                                             (uint8_t)flags, &transformed_origin);
    halo::render::render_billboard_build_orientation_basis(data, mode, (real_vector3d *)&transformed_origin,
                                             &transformed_direction, &basis);
    halo::render::render_billboard_compute_scale(data, &scale, mode, &transformed_origin, bitmap);

    shader = (LightningShader *)data->shader;
    if (shader != 0 && shader->framebuffer_fade_mode != 0 && mode != 0) {
        halo::math::vector3d_cross_product(basis.normal, basis.bitangent, basis.tangent);
        fade = halo::render::render_billboard_compute_view_fade((real_vector3d *)&transformed_origin,
                                                  &basis.normal,
                                                  shader->framebuffer_fade_mode) * fade;
    }

    packed_color = color_pack_argb_from_real(color);
    if (shader != 0 && shader->framebuffer_blend_function != 0 && (shader->shader_flags & 2) == 0) {
        vertex_color = (uint32_t)(uint8_t)(int32_t)(fade * 255.0f);
    } else {
        vertex_color = (uint32_t)(uint8_t)(int32_t)((real)(packed_color >> 24) * fade);
    }
    vertex_color = (vertex_color << 24) | (packed_color & 0xffffff);

    mirror_u = flags & _build_sprite_mirror_u_bit;
    mirror_v = flags & _build_sprite_mirror_v_bit;

    for (corner = 0; corner < 4; corner++) {
        rasterizer_dynamic_screen_vertex *vertex;
        real u;
        real v;
        real x;
        real y;
        real a;
        real b;

        u = (((corner >> 1) ^ corner) & 1) != 0 ? sprite->right : sprite->left;
        v = (corner & 2) != 0 ? sprite->top : sprite->bottom;
        x = u - (sprite->registration_point.x + sprite->left);
        y = (sprite->registration_point.y + sprite->top) - v;
        a = x * cos_rotation - y * sin_rotation;
        b = y * cos_rotation + sin_rotation * x;
        if (mirror_u != 0) {
            a = -a;
        }
        if (mirror_v != 0) {
            b = -b;
        }

        vertex = &((rasterizer_dynamic_screen_vertex *)group->vertices)[vertex_index];
        if ((data->flags & _build_sprite_data_screen_space_bit) != 0) {
            vertex->x = a * scale + transformed_origin.x;
            vertex->color = vertex_color;
            vertex->y = b * scale + transformed_origin.y;
            vertex->u = u;
            vertex->v = v;
        } else {
            real_point3d p;

            p.x = (basis.bitangent.i * b + basis.tangent.i * a) * scale + transformed_origin.x;
            p.y = (basis.bitangent.j * b + basis.tangent.j * a) * scale + transformed_origin.y;
            p.z = (basis.bitangent.k * b + basis.tangent.k * a) * scale + transformed_origin.z;
            if (p.x < bounds.x.lower) {
                bounds.x.lower = p.x;
            }
            if (p.x > bounds.x.upper) {
                bounds.x.upper = p.x;
            }
            if (p.y < bounds.y.lower) {
                bounds.y.lower = p.y;
            }
            if (p.y > bounds.y.upper) {
                bounds.y.upper = p.y;
            }
            if (p.z < bounds.z.lower) {
                bounds.z.lower = p.z;
            }
            if (p.z > bounds.z.upper) {
                bounds.z.upper = p.z;
            }
            vertex->u = u;
            vertex->x = p.x;
            vertex->v = v;
            vertex->y = p.y;
            vertex->z = p.z;
            vertex->color = vertex_color;
        }
        vertex_index++;
    }

    data->centroid.x = transformed_origin.x + data->centroid.x;
    data->centroid.y = transformed_origin.y + data->centroid.y;
    data->centroid.z = transformed_origin.z + data->centroid.z;
    group->quad_count++;
    data->sprite_count++;

    if ((data->flags & _build_sprite_data_screen_space_bit) == 0) {
        real area = halo::render::render_frustum_compute_box_overlap_area(&bounds, &render_frustum_global);

        build_sprite_screen_coverage = build_sprite_screen_coverage + area;
        if (area > 0.5f) {
            int16_t previous = build_sprite_large_quad_count;

            build_sprite_large_quad_count = previous + 1;
            if (previous > k_build_sprite_large_quad_limit) {
                group->quad_count--;
                data->sprite_count--;
            }
        }
    }
}

/**
 * VERIFIED against disassembly 0x511620..0x5116fd (2026-09-30): the centroid average (0 when there are no sprites), the
 *   transform by 0x7c31ac, the group walk (stride 0x10 at +0x24), the vertex buffer Unlock (vtable +0x30) chain
 *   (slot -> type -> cache handle -> buffer slot) and the 7 argument append call (flags ((f & 2) << 6) | 0x20, -4 index slot,
 *   quads * 2) match. The single 1/105 difftest difference is a NaN in the accumulated origin (0 * NaN), not a logic difference.
 * Averages data's accumulated view-space sprite origins back into its centroid (now in world
 * space), then unlocks and queues each of data's groups (screen space builds are not queued),
 * finally clearing build_sprite_data_flags bit 2.
 *
 * @address 0x00511620
 */
void sprites_end(build_sprite_data *data)
{
    real scale;
    int16_t i;

    if (data->sprite_count == 0) {
        scale = 0.0f;
    } else {
        scale = 1.0f / (real)data->sprite_count;
    }
    data->centroid.x = scale * data->centroid.x;
    data->centroid.y = scale * data->centroid.y;
    data->centroid.z = scale * data->centroid.z;
    halo::math::matrix4x3_transform_point(data->centroid, data->centroid, render_frustum_global.view_to_world);

    for (i = 0; i < data->group_count; i++) {
        build_sprite_group *group = &data->groups[i];

        if (group->vertex_slot != -1) {
            int16_t vertex_type = rasterizer_dynamic_vertex_slots[group->vertex_slot].vertex_type;
            int32_t handle = rasterizer_dynamic_vertex_caches[vertex_type].buffer_handle;

            if (handle != 0) {
                void *buffer = (void *)(uintptr_t)rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer;

                ((d3d_unlock_fn)(*(void ***)buffer)[0x30 / 4])(buffer);
            }
        }

        if (group->quad_count != 0 && (data->flags & _build_sprite_data_screen_space_bit) == 0) {
            halo::rasterizer::rasterizer_transparent_object_append(group->bitmap, -4, group->vertex_slot,
                                                 (int32_t)group->quad_count * 2,
                                                 ((data->flags & 0xff & 2) << 6) | 0x20,
                                                 &data->centroid,
                                                 (Shader *)(uintptr_t)data->shader);
        }
    }

    data->flags = data->flags & ~(uint32_t)_build_sprite_data_flag_2_bit;
}

}  // namespace halo::render::sprite

namespace halo::render::contrails {

/**
 * Computes a 0..1 fade factor for one contrail segment based on the angle between the camera-to-
 * point direction and the segment's own axis (direction): a segment seen edge-on (axis
 * perpendicular to the view) fades to 0, one seen face-on fades to 1. fade_mode 0 disables the
 * fade entirely (returns 1.0); fade_mode 2 inverts the curve.
 *
 * @address 0x0050e000
 */
real compute_edge_fade_factor(real_vector3d *direction, real_point3d *point, int16_t fade_mode, uint8_t *flags)
{
    real_vector3d to_camera;
    float fade;

    if (fade_mode == 0) {
        return 1.0f;
    }

    to_camera.i = render_camera_global.position.x - point->x;
    to_camera.j = render_camera_global.position.y - point->y;
    to_camera.k = render_camera_global.position.z - point->z;

    fade = (float)fabs((double)((to_camera.i * direction->i + to_camera.k * direction->k +
                                  to_camera.j * direction->j) /
                                 sqrt((double)(to_camera.k * to_camera.k + to_camera.j * to_camera.j +
                                               to_camera.i * to_camera.i))));

    if ((*flags & 0x40) != 0) {
        fade = halo::math::transition_function_evaluate(_transition_function_very_early, fade);
    }
    if (fade_mode == 2) {
        fade = 1.0f - fade;
    }
    return fade;
}

/**
 * Builds the triangle list of one contrail point list (instance) as a strip of vertex pairs
 * oriented by the Contrail render type, and queues it as a transparent draw.
 *
 * @address 0x0050e090
 */
void draw(contrail *c, Contrail *definition, int16_t instance)
{
    BitmapData *bitmap;
    int16_t segment_count;
    int32_t primitive_count;
    int32_t vertex_count;
    int32_t index_slot;
    int32_t vertex_slot;
    uint16_t *indices;
    rasterizer_dynamic_screen_vertex *vertices;
    uint8_t has_fade;
    Shader *shader;
    real_point3d centroid;
    float u;
    float u_step;
    float v_top;
    float v_bottom;
    datum_index point_index;
    contrail_point *previous = 0;
    ContrailPointState *states;
    real_vector3d fade_normal;

    bitmap = bitmap_group_sequence_get_bitmap_data(*(datum_index *)&definition->bitmap.tag_id,
                                                   c->frame_index, c->sequence_index);
    halo::rasterizer::globals().vertex_buffer_lock_state = 0xf;
    if (halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
        halo::rasterizer::globals().vertex_buffer_lock_state = 0;
        return;
    }

    segment_count = (int16_t)(c->point_count[instance] - 1);
    primitive_count = (int32_t)(int16_t)(segment_count * 2);
    index_slot = halo::rasterizer::rasterizer_dynamic_index_cache_reserve(primitive_count);
    vertex_count = (int32_t)(int16_t)(segment_count * 2 + 2);
    vertex_slot = halo::rasterizer::rasterizer_dynamic_vertex_cache_reserve(_rasterizer_vertex_type_dynamic_unlit,
                                                          vertex_count);
    if (index_slot == -1 || vertex_slot == -1) {
        halo::rasterizer::globals().vertex_buffer_lock_state = 0;
        return;
    }
    indices = (uint16_t *)halo::render::rasterizer_dynamic_index_slot_lock(index_slot);
    vertices = (rasterizer_dynamic_screen_vertex *)halo::rasterizer::rasterizer_dynamic_vertex_cache_lock(vertex_slot);

    has_fade = definition->framebuffer_fade_mode != 0;
    centroid = *global_zero_vector3d_pointer;
    shader = (Shader *)((uint8_t *)definition + 0x84);
    u = c->texture_offset_u;
    u_step = definition->texture_repeats_u;
    if ((definition->scale_flags & 0x40) != 0) {
        u_step = u_step * c->scale;
    }
    u_step = -u_step;
    v_bottom = c->texture_offset_v;
    v_top = definition->texture_repeats_v;
    if ((definition->scale_flags & 0x80) != 0) {
        v_top = v_top * c->scale;
    }
    v_top = v_top + v_bottom;
    states = (ContrailPointState *)definition->point_states.pointer;

    for (point_index = c->first_point[instance]; point_index != 0xffffffff;
         point_index = previous->next_point) {
        contrail_point *point = &((contrail_point *)halo::effects::globals().contrail_point_data->data)[(uint16_t)point_index];
        ContrailPointState *state = &states[point->state_index];
        float width;
        float half_width;
        ColorARGB color;
        contrail_point *next = 0;

        point_state_width_and_color(state, point, &width, &color);
        if ((point->flags & 0x02) != 0) {
            float next_width;
            ColorARGB next_color;
            float t = point->age;

            point_state_width_and_color(&states[point->state_index + 1], point, &next_width,
                                        &next_color);
            width = (next_width - width) * t + width;
            color.alpha = (next_color.alpha - color.alpha) * t + color.alpha;
            color.red = (next_color.red - color.red) * t + color.red;
            color.green = (next_color.green - color.green) * t + color.green;
            color.blue = (next_color.blue - color.blue) * t + color.blue;
        }
        if (c->object_index != 0xffffffff) {
            object *o = ((object_header *)object_data->data)[(uint16_t)c->object_index].data;
            Object *object_definition = (Object *)halo::cache::globals().tag_instances[o->definition_tag & 0xffff].data;
            int16_t change_color = (int16_t)(*(int16_t *)((uint8_t *)object_definition->attachments.pointer +
                                             (int32_t)c->attachment_index * 0x48 + 0x34) - 1);

            if (change_color != -1) {
                color.red = color.red * o->change_colors[change_color].red;
                color.green = color.green * o->change_colors[change_color].green;
                color.blue = color.blue * o->change_colors[change_color].blue;
            }
        }
        half_width = width * 0.5f;

        vertices[0].u = u;
        vertices[0].v = v_top;
        vertices[1].u = u;
        vertices[1].v = v_bottom;
        centroid.x = centroid.x + point->position.x;
        centroid.y = centroid.y + point->position.y;
        centroid.z = centroid.z + point->position.z;
        if (previous == 0) {
            next = &((contrail_point *)halo::effects::globals().contrail_point_data->data)[(uint16_t)point->next_point];
        }

        switch (definition->render_type) {
        case 0:
            vertices[0].x = point->position.x;
            vertices[0].y = point->position.y;
            vertices[0].z = point->position.z - half_width;
            vertices[1].x = point->position.x;
            vertices[1].y = point->position.y;
            vertices[1].z = half_width + point->position.z;
            if (has_fade) {
                if (previous != 0) {
                    fade_normal.k = 0.0f;
                    fade_normal.i = previous->position.y - point->position.y;
                    fade_normal.j = point->position.x - previous->position.x;
                } else {
                    fade_normal.i = point->position.y - next->position.y;
                    fade_normal.j = next->position.x - point->position.x;
                    fade_normal.k = 0.0f;
                }
                halo::math::vector3d_normalize_with_length(fade_normal);
            }
            break;
        case 1:
        case 2: {
            real_vector2d side;

            if (previous != 0) {
                side.i = previous->position.y - point->position.y;
                side.j = point->position.x - previous->position.x;
            } else {
                side.i = point->position.y - next->position.y;
                side.j = next->position.x - point->position.x;
            }
            halo::math::vector2d_normalize_with_length(side);
            vertices[0].x = point->position.x - side.i * half_width;
            vertices[0].y = point->position.y - side.j * half_width;
            vertices[0].z = point->position.z;
            vertices[1].x = side.i * half_width + point->position.x;
            vertices[1].y = side.j * half_width + point->position.y;
            vertices[1].z = point->position.z;
            if (has_fade) {
                fade_normal = *halo::math::globals().global_up3d_pointer;
            }
            break;
        }
        case 4: {
            real_vector3d to_camera;
            real_vector3d segment;
            real_vector3d side;
            real_point3d *from = previous != 0 ? &previous->position : &point->position;

            to_camera.i = render_camera_global.position.x - from->x;
            to_camera.j = render_camera_global.position.y - from->y;
            to_camera.k = render_camera_global.position.z - from->z;
            if (previous != 0) {
                segment.i = point->position.x - previous->position.x;
                segment.j = point->position.y - previous->position.y;
                segment.k = point->position.z - previous->position.z;
            } else {
                segment.i = next->position.x - point->position.x;
                segment.j = next->position.y - point->position.y;
                segment.k = next->position.z - point->position.z;
            }
            side.i = segment.k * to_camera.j - segment.j * to_camera.k;
            side.j = segment.i * to_camera.k - segment.k * to_camera.i;
            side.k = segment.j * to_camera.i - segment.i * to_camera.j;
            halo::math::vector3d_normalize_with_length(side);
            vertices[0].x = point->position.x - side.i * half_width;
            vertices[0].y = point->position.y - side.j * half_width;
            vertices[0].z = point->position.z - side.k * half_width;
            vertices[1].x = side.i * half_width + point->position.x;
            vertices[1].y = side.j * half_width + point->position.y;
            vertices[1].z = side.k * half_width + point->position.z;
            if (has_fade) {
                halo::math::vector3d_cross_product(fade_normal, side, segment);
                halo::math::vector3d_normalize_with_length(fade_normal);
            }
            break;
        }
        default:
            return;
        }

        color.alpha = halo::render::contrail_compute_edge_fade_factor(&fade_normal, &point->position,
                                                        definition->framebuffer_fade_mode,
                                                        (uint8_t *)&definition->flags) * color.alpha;
        if (color.alpha < 0.0f) {
            color.alpha = 0.0f;
        } else if (color.alpha > 1.0f) {
            color.alpha = 1.0f;
        }
        vertices[1].color = color_pack_argb_from_real(&color);
        vertices[0].color = vertices[1].color;
        u = u_step + u;

        previous = point;
        vertices += 2;
    }

    vertices -= vertex_count;
    if ((definition->flags & 1) == 0) {
        ((uint8_t *)&vertices[0].color)[3] = 0;
        ((uint8_t *)&vertices[1].color)[3] = 0;
    }
    if ((definition->flags & 2) == 0) {
        ((uint8_t *)&vertices[vertex_count - 1].color)[3] = 0;
        ((uint8_t *)&vertices[vertex_count - 2].color)[3] = 0;
    }
    if (segment_count > 0) {
        uint16_t a = 1;
        uint16_t k;

        for (k = 0; k < (uint16_t)segment_count; k++) {
            indices[0] = (uint16_t)(a - 1);
            indices[1] = a;
            indices[2] = (uint16_t)(a + 1);
            indices[4] = a;
            indices[3] = (uint16_t)(a + 1);
            indices[5] = (uint16_t)(a + 2);
            a = (uint16_t)(a + 2);
            indices += 6;
        }
    }

    {
        float inverse = 1.0f / (float)(int32_t)c->point_count[instance];
        void *vertex_buffer;
        int32_t buffer_handle;

        centroid.x = centroid.x * inverse;
        centroid.y = centroid.y * inverse;
        centroid.z = inverse * centroid.z;
        ((d3d_unlock_fn)(*(void ***)halo::rasterizer::globals().dynamic_index_buffer)[0x30 / 4])(
            halo::rasterizer::globals().dynamic_index_buffer);
        buffer_handle = rasterizer_dynamic_vertex_caches[
            rasterizer_dynamic_vertex_slots[vertex_slot].vertex_type].buffer_handle;
        if (buffer_handle != 0) {
            vertex_buffer = (void *)(uintptr_t)rasterizer_vertex_buffer_slots[buffer_handle - 1].hardware_buffer;
            ((d3d_unlock_fn)(*(void ***)vertex_buffer)[0x30 / 4])(vertex_buffer);
        }
        halo::rasterizer::rasterizer_transparent_object_append((uint32_t)(uintptr_t)bitmap, index_slot, vertex_slot,
                                             primitive_count, 0, &centroid, shader);
    }
    halo::rasterizer::globals().vertex_buffer_lock_state = 0;
}

/**
 * this module (cdecl)
 * Draws every live contrail whose Contrail render type is selected by render_type_flags, one
 * point list (of the four marker permutations) at a time.
 *
 * @address 0x0050df20
 */
void render_all(uint32_t render_type_flags)
{
    datum_index index = halo::memory::datum_next(-1, halo::effects::globals().contrail_data);

    while (index != k_datum_index_none) {
        contrail *c = &((contrail *)halo::effects::globals().contrail_data->data)[(uint16_t)index];
        Contrail *definition = (Contrail *)halo::cache::globals().tag_instances[(uint16_t)c->definition_index].data;
        int16_t i;

        for (i = 0; i < 4; i++) {
            if ((render_type_flags & (1u << ((uint8_t)definition->render_type & 0x1f))) != 0 &&
                c->point_count[i] >= 2) {
                halo::render::render_contrail(c, definition, i);
            }
        }

        index = halo::memory::datum_next((int16_t)index, halo::effects::globals().contrail_data);
    }
}

}  // namespace halo::render::contrails

namespace halo::render::frame {

/**
 * Draws every visible particle: collects them, sorts them into runs that share a definition,
 * cluster and first person state, and builds one sprite batch per run.
 *
 * @address 0x0050fd90
 */
void particles(void)
{
    rendered_particle_datum records[k_maximum_rendered_particles];
    uint16_t group_counts[k_maximum_rendered_particle_groups];
    build_sprite_data data;
    int16_t record_count = 0;
    int16_t viewer;
    int32_t viewer_value;
    datum_index index;

    if (!halo::effects::globals().particle_spawn_debug_mode) {
        return;
    }
    viewer = current_local_player_index;
    if (viewer == -1 || !(uint8_t)halo::render::render_local_player_gunner_seat_visible(viewer)) {
        viewer = 1;
    }
    viewer_value = (int32_t)viewer;

    for (index = halo::memory::datum_next(-1, halo::effects::globals().particle_data); index != 0xffffffff;
         index = halo::memory::datum_next((int16_t)index, halo::effects::globals().particle_data)) {
        particle *p = &((particle *)halo::effects::globals().particle_data->data)[(uint16_t)index];
        int32_t cluster = (int32_t)p->location.cluster_index;
        uint8_t owned = (int32_t)p->first_person_weapon_index == viewer_value;

        if ((halo::structures::globals().cluster_visible_bits[cluster >> 5] & (1u << (cluster & 0x1f))) == 0) {
            continue;
        }
        if ((p->flags & 0x10) != 0 && owned) {
            continue;
        }
        if ((p->flags & 0x20) != 0 && !owned) {
            continue;
        }
        records[record_count].particle_index = (uint16_t)index;
        records[record_count].definition_index = (uint16_t)p->definition_index;
        records[record_count].cluster_index = p->location.cluster_index;
        records[record_count].first_person = (owned && (p->flags & 0x20) != 0) ? 1 : 0;
        record_count++;
    }

    if (record_count <= 0) {
        return;
    }
    halo::render::sort_introsort_loop(records, records + record_count, record_count, viewer_value);

    {
        int16_t group_count = 0;
        int16_t remaining = record_count;
        uint16_t key_definition = 0xffff;
        uint16_t key_cluster = 0xffff;
        uint8_t key_first_person = 0;
        uint16_t *current_count = 0;
        rendered_particle_datum *record = records;
        rendered_particle_datum *group_first;
        int16_t group;

        do {
            remaining--;
            if (record->definition_index == key_definition &&
                (uint16_t)record->cluster_index == key_cluster &&
                record->first_person == key_first_person) {
                (*current_count)++;
            } else {
                if (group_count >= k_maximum_rendered_particle_groups) {
                    break;
                }
                key_cluster = (uint16_t)record->cluster_index;
                key_first_person = record->first_person;
                current_count = &group_counts[group_count];
                group_count++;
                *current_count = 1;
                key_definition = record->definition_index;
            }
            record++;
        } while (remaining > 0);

        group_first = records;
        for (group = 0; group < group_count; group++) {
            Particle *definition = (Particle *)halo::cache::globals().tag_instances[group_first->definition_index].data;
            int16_t in_group = (int16_t)group_counts[group];
            int16_t drawn = 0;
            float radius_sum = 0.0f;
            float average;
            int16_t k;

            data.bitmap_group_index = *(datum_index *)&definition->bitmap.tag_id;
            data.maximum_sprite_count = in_group;
            data.shader = (uint32_t)(uintptr_t)((uint8_t *)definition + 0xb0);
            data.centroid = *global_zero_vector3d_pointer;
            data.flags = (group_first->first_person ? _build_sprite_data_flag_1_bit : 0) |
                         _build_sprite_data_flag_2_bit;
            data.sprite_count = 0;
            data.group_count = 0;

            for (k = 0; k < in_group; k++, group_first++) {
                uint16_t particle_index = group_first->particle_index;
                particle *p = &((particle *)halo::effects::globals().particle_data->data)[particle_index];
                Particle *pd = (Particle *)halo::cache::globals().tag_instances[(uint16_t)p->definition_index].data;
                float radius = ((pd->radius_animation[1] - pd->radius_animation[0]) *
                                (p->age / p->lifespan) + pd->radius_animation[0]) * p->scale;
                real_point3d origin;
                real_vector3d direction;
                float view_z;
                float pixels;

                if (p->object_index == 0xffffffff) {
                    origin = p->position;
                    direction = p->direction;
                    p->object_index = 0xffffffff;
                } else {
                    real_matrix4x3 *m = 0;
                    real_point3d pos;
                    real_vector3d dir;

                    if ((p->flags & _particle_first_person_bit) != 0) {
                        m = (real_matrix4x3 *)((uint8_t *)&first_person_weapon_interfaces[
                                p->first_person_weapon_index] + 0x108c +
                                (int32_t)p->marker_index * 0x34);
                    } else {
                        int16_t object_slot = (int16_t)p->object_index;
                        int16_t salt = (int16_t)(p->object_index >> 16);

                        if (object_slot >= 0 && object_slot < object_data->maximum_count) {
                            object_header *header = (object_header *)((uint8_t *)object_data->data +
                                (int32_t)object_data->size * (int32_t)object_slot);

                            if (header->identifier != 0 &&
                                (salt == 0 || header->identifier == salt) &&
                                (1 << (header->type & 0x1f)) != 0 && header->data != 0) {
                                object *o = ((object_header *)object_data->data)[
                                    (uint16_t)p->object_index].data;

                                m = (real_matrix4x3 *)((uint8_t *)o + o->nodes.offset +
                                                       (int32_t)p->marker_index * 0x34);
                            }
                        }
                        if (m == 0) {
                            halo::memory::datum_delete(halo::effects::globals().particle_data, (datum_index)(int32_t)(int16_t)particle_index);
                            continue;
                        }
                    }

                    pos = p->position;
                    if (*(uint32_t *)&m->scale != 0x3f800000) {
                        pos.x = pos.x * m->scale;
                        pos.y = pos.y * m->scale;
                        pos.z = pos.z * m->scale;
                    }
                    origin.x = pos.z * m->up.i + pos.y * m->left.i + pos.x * m->forward.i + m->position.x;
                    origin.y = pos.x * m->forward.j + pos.z * m->up.j + pos.y * m->left.j + m->position.y;
                    origin.z = pos.z * m->up.k + pos.y * m->left.k + pos.x * m->forward.k + m->position.z;
                    dir = p->direction;
                    direction.i = dir.k * m->up.i + dir.j * m->left.i + dir.i * m->forward.i;
                    direction.j = dir.k * m->up.j + dir.j * m->left.j + dir.i * m->forward.j;
                    direction.k = dir.k * m->up.k + dir.j * m->left.k + dir.i * m->forward.k;
                }

                view_z = origin.z * render_frustum_global.world_to_view.up.k +
                         origin.y * render_frustum_global.world_to_view.left.k +
                         origin.x * render_frustum_global.world_to_view.forward.k +
                         render_frustum_global.world_to_view.position.z;
                if (view_z < 0.0f) {
                    view_z = -view_z;
                }
                if (view_z <= 0.1f) {
                    view_z = 0.1f;
                }
                pixels = render_frustum_global.projection_world_to_screen.j / view_z * radius;
                pixels = pixels + pixels;

                if (pixels > definition->fade_end_size) {
                    float fade = 1.0f;
                    float scale = (radius + radius) * definition->sprite_size;
                    float remaining_life = p->lifespan - p->age;
                    uint32_t flags;

                    drawn++;
                    radius_sum = radius + radius_sum;
                    if (pixels < definition->minimum_size) {
                        scale = (definition->minimum_size / pixels) * scale;
                    }
                    if (definition->fade_in_time > 0.0f && p->age < definition->fade_in_time) {
                        fade = p->age / definition->fade_in_time;
                    }
                    if (definition->fade_out_time > 0.0f && remaining_life < definition->fade_out_time) {
                        fade = (remaining_life / definition->fade_out_time) * fade;
                    }
                    flags = ((uint32_t)(uint8_t)p->flags >> 1) & _build_sprite_mirror_u_bit;
                    if ((p->flags & _particle_mirror_vertical_bit) != 0) {
                        flags |= _build_sprite_mirror_v_bit;
                    } else {
                        flags &= ~(uint32_t)_build_sprite_mirror_v_bit;
                    }
                    halo::render::build_sprite(&data, p->sequence_index, p->frame_index,
                                 (int16_t)(uint16_t)definition->orientation, &origin, &direction,
                                 p->rotation, scale, &p->color, fade, flags);
                    p->last_update_tick = render_frame_index;
                }
            }

            if (drawn != 0) {
                average = radius_sum / (float)drawn;
            } else {
                average = 0.0f;
            }
            ((shader_effect *)(uintptr_t)data.shader)->average_particle_radius = average;
            halo::render::build_sprites_end(&data);
        }
    }
}

}  // namespace halo::render::frame
