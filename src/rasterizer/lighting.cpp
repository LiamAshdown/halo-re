/**
 * @file src/rasterizer/lighting.cpp
 * Light constants, lights, fog constants and shader stage configuration.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/shaders/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/rasterizer/constants.hpp"
#include "halo/rasterizer/tag_access.hpp"
#include "halo/core/bit_cast.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/tags/flags.hpp"
#include <cstring>




namespace halo::rasterizer {

namespace {

/** The Light tag a rasterizer light was built from. */
const Light *light_tag(const rasterizer_light *light)
{
    return reinterpret_cast<const Light *>(static_cast<uintptr_t>(light->definition));
}

}  // namespace


/**
 * Direct3D 9 back end function rasterizer_light_cone_draw.
 *
 * @address 0x51dc50
 */
void rasterizer_light_cone_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    rasterizer_effect_slot *effect_slot = &rasterizer_effects[4];
    void *effect = effect_slot->effect;
    uint32_t bump_map_tag;
    BitmapData *bump_bitmap = 0;
    float constants[12];
    float color[4];
    uint32_t pass_count;
    uint32_t pass;

    if (halo::rasterizer::fields::rasterizer_debug_mode_word != 0 || halo::rasterizer::fields::environment_diffuse_lights_enabled == 0 || effect == 0) {
        return;
    }

    bump_map_tag = halo::test_flag(shader->shader_environment_flags, halo::tags::shader_environment_tag_flag::bump_map_is_specular_mask) ? halo::k_dword_none : halo::tag_id_bits(shader->bump_map.tag_id);
    if (halo::rasterizer::fields::bump_mapping_enabled != 0 && bump_map_tag != halo::k_dword_none) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & halo::k_slot_mask].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (bump_bitmap->type != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

        if (default_tag != halo::k_dword_none) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[default_tag & halo::k_slot_mask].data;

            if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                bump_bitmap = tag_block_element<BitmapData>(bitmap->bitmap_data, 3);
            }
        }
    }
    if (bump_bitmap != 0) {
        rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
        rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
        rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
    }

    constants[0] = shader->bump_map_scale_xy.x;
    constants[1] = shader->bump_map_scale_xy.y;
    constants[2] = 1.0f;
    constants[3] = 1.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    halo::shaders::shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, const_cast<ShaderEnvironment *>(shader));
    render_device().set_vertex_shader_constant_f(0xa, constants, 3);

    color[0] = shader->material_color.red;
    color[1] = shader->material_color.green;
    color[2] = shader->material_color.blue;
    color[3] = 1.0f;
    render_device().set_pixel_shader_constant_f(1, color, 1);

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[0].declaration);
    render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                   rasterizer_vertex_declarations[0].usage) & 0x10);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[9].shader);

    render_device().effect_begin(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
    }
    render_device().effect_end(effect);
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

namespace rasterizer_light_cone_set_orientation_constants_impl {


/**
 * Direct3D 9 back end function rasterizer_light_cone_set_orientation_constants.
 *
 * Registers: EAX = light_index
 *
 * @address 0x51da20
 */
void rasterizer_light_cone_set_orientation_constants(int32_t light_index)
{
    rasterizer_light *light;
    real yaw, pitch, roll;
    real_matrix4x3 orientation;
    real_vector3d axis_a, axis_b, axis_c;
    float constants_vs[5][4];
    float constants_ps[1][4];

    if (halo::rasterizer::fields::rasterizer_debug_mode != 0 || halo::rasterizer::fields::environment_diffuse_lights_enabled == 0 ||
        rasterizer_caps.pixel_shader_version <= halo::d3d9::k_pixel_shader_version_1_0) {
        return;
    }

    light = &rasterizer_lights[light_index];

    if (rasterizer_effects[4].effect != 0) {
        rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(light_tag(light)->primary_cube_map.tag_id), 2, 1, 1, 0,
                                              &rasterizer_effects[4]);
    }

    yaw = halo::math::periodic_function_evaluate(light_tag(light)->yaw_function, rasterizer_time.time / light_tag(light)->yaw_period) * 6.2831855f;
    pitch = halo::math::periodic_function_evaluate(light_tag(light)->pitch_function, rasterizer_time.time / light_tag(light)->pitch_period) * 6.2831855f;
    roll = halo::math::periodic_function_evaluate(light_tag(light)->roll_function, rasterizer_time.time / light_tag(light)->roll_period) * 6.2831855f;

    halo::math::matrix4x3_from_euler_angles(orientation, yaw, pitch, roll);
    halo::math::matrix4x3_transform_normal(axis_a, light->forward, orientation);
    halo::math::matrix4x3_transform_normal(axis_b, light->up, orientation);

    halo::math::vector3d_cross_product(axis_c, axis_b, axis_a);
    halo::math::vector3d_normalize_with_length(axis_c);

    constants_vs[0][0] = light->position.x;
    constants_vs[0][1] = light->position.y;
    constants_vs[0][2] = light->position.z;
    constants_vs[0][3] = 0.5f / light->radius;

    constants_vs[1][0] = -axis_a.i;
    constants_vs[1][1] = -axis_a.j;
    constants_vs[1][2] = -axis_a.k;
    constants_vs[1][3] = 1.0f;

    constants_vs[2][0] = -axis_b.i;
    constants_vs[2][1] = -axis_b.j;
    constants_vs[2][2] = -axis_b.k;
    constants_vs[2][3] = 1.0f;

    constants_vs[3][0] = -axis_c.i;
    constants_vs[3][1] = -axis_c.j;
    constants_vs[3][2] = -axis_c.k;
    constants_vs[3][3] = 1.0f;

    constants_vs[4][0] = 0.0f;
    constants_vs[4][1] = 0.0f;
    constants_vs[4][2] = 0.0f;
    constants_vs[4][3] = 1.0f;

    render_device().set_vertex_shader_constant_f(0xd, constants_vs, 5);

    constants_ps[0][0] = light->color.red;
    constants_ps[0][1] = light->color.green;
    constants_ps[0][2] = light->color.blue;
    constants_ps[0][3] = 1.0f;

    render_device().set_pixel_shader_constant_f(0, constants_ps, 1);
}

}  // namespace rasterizer_light_cone_set_orientation_constants_impl

namespace rasterizer_light_cone_set_texture_stage_states_impl {


/**
 * Direct3D 9 back end function rasterizer_light_cone_set_texture_stage_states.
 *
 * @address 0x51d6a0
 */
void rasterizer_light_cone_set_texture_stage_states(void)
{

    if (halo::rasterizer::fields::rasterizer_debug_mode == 0 && halo::rasterizer::fields::environment_diffuse_lights_enabled != 0 &&
        halo::d3d9::k_pixel_shader_version_1_0 < rasterizer_caps.pixel_shader_version &&rasterizer_effects[4].effect != 0) {

        render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 1);
        render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 1);
        render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_w, 3);
        render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);

        chimera__rasterizer_set_texture_direct_d3dx(halo::tag_id_bits(rasterizer_globals_data->distance_attenuation.tag_id), 2, 0,
                                                    &rasterizer_effects[4]);
        if ((rasterizer_caps.texture_address_caps & 8) == 0) {
            render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 3);
            render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 3);
            render_device().set_sampler_state(2, halo::d3d9::ss::address_w, 3);
        } else {
            render_device().set_sampler_state(2, halo::d3d9::ss::border_color, 0);
            render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 4);
            render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 4);
            render_device().set_sampler_state(2, halo::d3d9::ss::address_w, 4);
        }
        render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, 1);
        render_device().set_sampler_state(2, halo::d3d9::ss::mip_filter, 1);

        chimera__rasterizer_set_texture_direct_d3dx(halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id), 3, 0,
                                                    &rasterizer_effects[4]);
        render_device().set_sampler_state(3, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(3, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(3, halo::d3d9::ss::address_w, 3);
        render_device().set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(3, halo::d3d9::ss::min_filter, 1);
        render_device().set_sampler_state(3, halo::d3d9::ss::mip_filter, 1);

        render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
        render_device().set_render_state(halo::d3d9::rs::color_write_enable, 7);
        render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
        render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
        render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::alpha_ref, 0);
        render_device().set_render_state(halo::d3d9::rs::z_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::z_func, 3);
        render_device().set_render_state(halo::d3d9::rs::z_write_enable, 0);
        render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);
    }
}

}  // namespace rasterizer_light_cone_set_texture_stage_states_impl

namespace rasterizer_light_disable_all_impl {


/**
 * Resets the default material and disables every fixed-function Direct3D light, for the pre-pixel-shader
 * lighting fallback used when the device has no ps_1_1 support.
 *
 * @address 0x526700
 */
void rasterizer_light_disable_all(void)
{
    uint32_t i;

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1 && rasterizer_caps.max_active_lights != 0) {
        render_device().set_material(rasterizer_default_material);

        rasterizer_fixed_function_light_count = 0;

        i = 0;
        if (rasterizer_caps.max_active_lights != 0) {
            do {
                render_device().light_enable(i, 0);
                i = i + 1;
            } while (i < rasterizer_caps.max_active_lights);
        }
    }
}

}  // namespace rasterizer_light_disable_all_impl

namespace rasterizer_light_set_impl {



/**
 * D3DLIGHTTYPE Populates and enables one fixed-function Direct3D light from a game light object, for the pre-
 * pixel-shader lighting fallback used when the device has no ps_1_1 support.
 *
 * @address 0x526760
 */
void rasterizer_light_set(rasterizer_light *light)
{
    uint32_t index;
    d3d_light9 d3dlight;

    index = rasterizer_fixed_function_light_count;
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1 && rasterizer_caps.max_active_lights != 0 &&
        rasterizer_fixed_function_light_count < (int32_t)rasterizer_caps.max_active_lights) {
        std::memset(&d3dlight, 0, sizeof(d3dlight));

        d3dlight.attenuation1 = 1.4f;
        d3dlight.attenuation0 = 0.0f;

        if (halo::tag_id_bits(light_tag(light)->primary_cube_map.tag_id) == halo::k_dword_none) {

            d3dlight.position.x = light->position.x;
            d3dlight.diffuse[0] = light->color.red * 10.0f;
            d3dlight.position.y = light->position.y;
            d3dlight.position.z = light->position.z;
            d3dlight.type = halo::d3d9::k_light_point;
            d3dlight.attenuation2 = 0.0f;
            d3dlight.diffuse[1] = light->color.green * 10.0f;
            d3dlight.diffuse[2] = light->color.blue * 10.0f;
        } else {
            float radius_scale;
            real_vector3d flashlight_offset;

            d3dlight.direction.i = light->forward.i;
            d3dlight.direction.j = light->forward.j;
            d3dlight.position.x = light->position.x;
            d3dlight.position.y = light->position.y;
            radius_scale = light->radius * 2.5f;
            d3dlight.position.z = light->position.z;
            d3dlight.type = halo::d3d9::k_light_spot;
            d3dlight.theta = 1.0f;
            d3dlight.diffuse[0] = radius_scale * light->color.red;
            d3dlight.phi = 3.14f;
            d3dlight.falloff = 2.0f;
            d3dlight.attenuation2 = 1.0f;
            d3dlight.diffuse[1] = radius_scale * light->color.green;
            d3dlight.diffuse[2] = radius_scale * light->color.blue;
            d3dlight.direction.k = light->forward.k;

            if (halo::test_flag(light_tag(light)->flags, halo::tags::light_tag_flag::first_person_flashlight)) {

                halo::math::vector3d_cross_product(flashlight_offset, light->up, light->forward);
                halo::math::vector3d_normalize_with_length(flashlight_offset);
                d3dlight.position.x -= flashlight_offset.i * 0.3f;
                d3dlight.position.y -= flashlight_offset.j * 0.3f;
                d3dlight.position.z -= flashlight_offset.k * 0.3f;
            }
        }

        d3dlight.range = light->radius;

        render_device().set_light(index, &d3dlight);
        render_device().light_enable(rasterizer_fixed_function_light_count, 1);
        rasterizer_fixed_function_light_count = rasterizer_fixed_function_light_count + 1;
    }
}

}  // namespace rasterizer_light_set_impl

/**
 * Copies (or clears) a rasterizer_point_light_constants record at dest_base[slot] from
 * rasterizer_lights[light_index], or zeroes it when light_index == -1.
 *
 * Registers: EAX -> light_index, CX -> slot, EDX -> dest_base
 *
 * @address 0x518c10
 */
void rasterizer_light_set_point_constants(int32_t light_index, int16_t slot, rasterizer_point_light_constants *dest_base)
{
    rasterizer_point_light_constants *dest = &dest_base[slot];

    if (light_index == -1) {
        std::memset(dest, 0, sizeof(*dest));
        return;
    }

    {
        rasterizer_light *light = &rasterizer_lights[light_index];
        const Light *definition = light_tag(light);
        float cos_falloff_angle = definition->cos_falloff_angle;
        float cos_cutoff_angle = definition->cos_cutoff_angle;

        dest->position = light->position;
        dest->inverse_radius_squared = 1.0f / (light->radius * light->radius);
        dest->forward = light->forward;
        dest->color = light->color;

        if (halo::bit_cast<uint32_t>(definition->cos_falloff_angle) != k_float_bits_minus_one) {
            float falloff_scale = 1.0f / (cos_falloff_angle - cos_cutoff_angle);
            dest->falloff_scale = falloff_scale;
            dest->falloff_offset = -(falloff_scale * cos_cutoff_angle);
        } else {
            dest->falloff_scale = 0.0f;
            dest->falloff_offset = 1.0f;
        }
    }
}

namespace rasterizer_prepare_lighting_constants_impl {


typedef struct lighting_constant_block {
    rasterizer_point_light_constants point_lights[2];
    float distant_lights[2][2][4];
    float ambient[4];
} lighting_constant_block;

static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

/**
 * Direct3D 9 back end function rasterizer_prepare_lighting_constants.
 *
 * @address 0x518ce0
 */
void rasterizer_prepare_lighting_constants(render_lighting *lighting)
{
    lighting_constant_block block;
    float ambient_red, ambient_green, ambient_blue;
    int16_t i;

    if (halo::rasterizer::fields::model_lighting_ambient_override > 0.0f) {
        std::memset(&block, 0, sizeof(block));
        ambient_red = ambient_green = ambient_blue = halo::rasterizer::fields::model_lighting_ambient_override;
    } else {
        for (i = 0; i < 2; i++) {
            int32_t light_index = (i < lighting->point_light_count) ? lighting->point_light_indices[i] : -1;

            rasterizer_light_set_point_constants(light_index, i, &block.point_lights[0]);
        }
        for (i = 0; i < 2; i++) {
            const render_distant_light *light = &lighting->distant_lights[i];

            if (i < lighting->distant_light_count) {
                block.distant_lights[i][0][0] = light->direction.i;
                block.distant_lights[i][0][1] = light->direction.j;
                block.distant_lights[i][0][2] = light->direction.k;
                block.distant_lights[i][0][3] = 0.0f;
                block.distant_lights[i][1][0] = light->color.red;
                block.distant_lights[i][1][1] = light->color.green;
                block.distant_lights[i][1][2] = light->color.blue;
                block.distant_lights[i][1][3] = 0.0f;
            } else {
                int32_t k;
                for (k = 0; k < 4; k++) {
                    block.distant_lights[i][0][k] = 0.0f;
                    block.distant_lights[i][1][k] = 0.0f;
                }
            }
        }
        ambient_red = lighting->ambient_color.red;
        ambient_green = lighting->ambient_color.green;
        ambient_blue = lighting->ambient_color.blue;
    }
    block.ambient[0] = ambient_red;
    block.ambient[1] = ambient_green;
    block.ambient[2] = ambient_blue;
    block.ambient[3] = 0.0f;

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        float boost = (float)(halo::rasterizer::fields::fixed_function_ambient_color & 0xff) * 0.003921569f;
        uint32_t red = (uint32_t)(int32_t)(clamp01(zoom_static_tint_r.red + boost + ambient_red) * 255.0f);
        uint32_t green = (uint32_t)(int32_t)(clamp01(zoom_static_tint_r.green + boost + ambient_green) * 255.0f);
        uint32_t blue = (uint32_t)(int32_t)(clamp01(zoom_static_tint_r.blue + boost + ambient_blue) * 255.0f);

        render_device().set_render_state(halo::d3d9::rs::ambient, (((red & 0xff) << 8 | (green & 0xff)) << 8) | (blue & 0xff));
    }
    render_device().set_vertex_shader_constant_f(15, reinterpret_cast<const float *>(&block), 11);
}

}  // namespace rasterizer_prepare_lighting_constants_impl

/**
 * Direct3D 9 back end function rasterizer_projected_light_constants_build.
 *
 * Registers: EAX = light_index
 *
 * @address 0x521750
 */
void rasterizer_projected_light_constants_build(int32_t light_index)
{
    rasterizer_light *light;
    const Light *definition;

    if (halo::rasterizer::fields::rasterizer_debug_mode != 0 || halo::rasterizer::fields::specular_projected_light_enabled == 0 ||
        rasterizer_caps.pixel_shader_version <= halo::d3d9::k_pixel_shader_version_1_3) {
        return;
    }

    light = &rasterizer_lights[light_index];
    definition = light_tag(light);

    if (halo::bit_cast<uint32_t>(definition->cos_falloff_angle) != k_float_bits_minus_one &&
        (halo::tag_id_bits<int32_t>(definition->primary_cube_map.tag_id) != -1 ||
         halo::tag_id_bits<int32_t>(definition->secondary_cube_map.tag_id) != -1)) {
        rasterizer_projected_light_shader_variant = 1;
        rasterizer_projected_light_luminance =
            light->color.red * 0.299f + light->color.green * 0.587f + light->color.blue * 0.114f;
        rasterizer_projected_light_constants_build_cube_map(light_index);
        rasterizer_projected_light_has_cube_map = 1;
        return;
    }

    rasterizer_projected_light.position = light->position;
    rasterizer_projected_light_shader_variant = 0;
    rasterizer_projected_light_has_cube_map = 0;
    rasterizer_projected_light_luminance =
        light->color.red * 0.299f + light->color.green * 0.587f + light->color.blue * 0.114f;

    rasterizer_projected_light.inverse_radius =
        1.0f / (definition->specular_radius_multiplier * light->radius) * 0.5f;

    rasterizer_projected_light.basis[0][0] = 0.0f;
    rasterizer_projected_light.basis[0][1] = 0.0f;
    rasterizer_projected_light.basis[0][2] = 0.0f;
    rasterizer_projected_light.basis[0][3] = 1.0f;
    rasterizer_projected_light.basis[1][0] = 0.0f;
    rasterizer_projected_light.basis[1][1] = 0.0f;
    rasterizer_projected_light.basis[1][2] = 0.0f;
    rasterizer_projected_light.basis[1][3] = 1.0f;
    rasterizer_projected_light.basis[2][0] = 0.0f;
    rasterizer_projected_light.basis[2][1] = 0.0f;
    rasterizer_projected_light.basis[2][2] = 0.0f;
    rasterizer_projected_light.basis[2][3] = 1.0f;
    rasterizer_projected_light.cone_axis.i = 0.0f;
    rasterizer_projected_light.cone_axis.j = 0.0f;
    rasterizer_projected_light.cone_axis.k = 0.0f;
    rasterizer_projected_light.cone_offset = 1.0f;

    rasterizer_projected_light_cube_map = halo::tag_id_bits<int32_t>(rasterizer_globals_data->distance_attenuation.tag_id);
}

/**
 * Direct3D 9 back end function rasterizer_projected_light_constants_build_cube_map.
 *
 * Registers: EAX = light_index
 *
 * @address 0x5215b0
 */
void rasterizer_projected_light_constants_build_cube_map(int32_t light_index)
{
    rasterizer_light *light;
    const Light *definition;
    int32_t cube_map_tag_index;
    real_vector3d cross_axis;
    float radius;
    float scale;

    if (halo::rasterizer::fields::specular_projected_light_enabled == 0 || rasterizer_caps.pixel_shader_version <= halo::d3d9::k_pixel_shader_version_1_3) {
        return;
    }

    light = &rasterizer_lights[light_index];
    definition = light_tag(light);

    cube_map_tag_index = halo::tag_id_bits<int32_t>(definition->primary_cube_map.tag_id);
    if (cube_map_tag_index == -1) {
        cube_map_tag_index = halo::tag_id_bits<int32_t>(definition->secondary_cube_map.tag_id);
    }

    halo::math::vector3d_cross_product(cross_axis, light->up, light->forward);
    halo::math::vector3d_normalize_with_length(cross_axis);

    rasterizer_projected_light.position = light->position;
    radius = definition->specular_radius_multiplier * light->radius;
    rasterizer_projected_light.basis[0][3] = 1.0f;
    rasterizer_projected_light.basis[1][3] = 1.0f;
    rasterizer_projected_light.basis[2][3] = 1.0f;
    scale = 1.0f / (radius - radius * 0.5f);
    rasterizer_projected_light.inverse_radius = 0.5f / radius;
    rasterizer_projected_light.basis[0][0] = -light->forward.i;
    rasterizer_projected_light.basis[0][1] = -light->forward.j;
    rasterizer_projected_light.basis[0][2] = -light->forward.k;
    rasterizer_projected_light.basis[1][0] = -cross_axis.i;
    rasterizer_projected_light.basis[1][1] = -cross_axis.j;
    rasterizer_projected_light.basis[1][2] = -cross_axis.k;
    rasterizer_projected_light.basis[2][0] = -light->up.i;
    rasterizer_projected_light.basis[2][1] = -light->up.j;
    rasterizer_projected_light.basis[2][2] = -light->up.k;
    rasterizer_projected_light.cone_axis.i = light->forward.i * scale;
    rasterizer_projected_light.cone_axis.j = light->forward.j * scale;
    rasterizer_projected_light.cone_axis.k = light->forward.k * scale;
    rasterizer_projected_light.cone_offset = -(scale * radius * 0.5f);
    rasterizer_projected_light_cube_map = cube_map_tag_index;
}

namespace rasterizer_set_fog_constants_impl {


static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static float real_pin_unit(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static uint32_t real_bits(float value)
{
    return halo::bit_cast<uint32_t>(value);
}

/**
 * Direct3D 9 back end function rasterizer_set_fog_constants.
 *
 * Registers: EDX -> fog
 *
 * @address 0x5176d0
 */
void rasterizer_set_fog_constants(const render_fog *fog)
{
    render_fog *window_fog = &rasterizer_window.fog;
    const real_point3d *camera_position = &rasterizer_window.camera.position;
    const real_vector3d *camera_forward = &rasterizer_window.camera.forward;
    float constants[16];
    float atmospheric_scale;
    float camera_depth;
    float planar_distance_scale;
    float planar_depth_scale;

    *window_fog = *fog;

    if (window_fog->atmospheric_maximum_density <= 0.0f) {
        window_fog->atmospheric_maximum_density = 1.0f;
    }
    if (window_fog->atmospheric_maximum_distance == 0.0f || halo::rasterizer::fields::rasterizer_fog_atmosphere == 0) {
        window_fog->atmospheric_maximum_distance = rasterizer_window.camera.z_far + rasterizer_window.camera.z_far;
        window_fog->atmospheric_maximum_density = 0.0f;
        window_fog->atmospheric_minimum_distance = rasterizer_window.camera.z_far;
    }

    if (window_fog->planar_maximum_density <= 0.0f) {
        window_fog->planar_maximum_density = 1.0f;
    }
    if (window_fog->planar_mode == 0 || (fog->flags & _render_fog_no_planar_bit) != 0 ||
        halo::rasterizer::fields::rasterizer_fog_plane == 0) {

        window_fog->planar_mode = 0;
        window_fog->planar_maximum_density = 0.0f;
        window_fog->planar_maximum_distance = 1.0f;
        window_fog->planar_maximum_depth = 1.0f;
        window_fog->planar_color = *global_white_color;
        window_fog->plane.normal = *camera_forward;
        window_fog->plane.d = camera_position->x * camera_forward->i +
                              camera_position->y * camera_forward->j +
                              camera_position->z * camera_forward->k;
    } else if (window_fog->planar_mode == 2) {

        window_fog->planar_maximum_depth = 1.0f;
        halo::math::plane3d_from_point_and_normal(window_fog->plane, *camera_forward, *camera_position);
        window_fog->plane.d = window_fog->plane.d + rasterizer_window.camera.z_far;
    }

    atmospheric_scale = 1.0f / (window_fog->atmospheric_maximum_distance - window_fog->atmospheric_minimum_distance);
    camera_depth = camera_position->x * camera_forward->i + camera_position->y * camera_forward->j +
                   camera_position->z * camera_forward->k;
    planar_distance_scale = 1.0f / window_fog->planar_maximum_distance;
    planar_depth_scale = 1.0f / window_fog->planar_maximum_depth;

    constants[0] = camera_forward->i * atmospheric_scale;
    constants[1] = camera_forward->j * atmospheric_scale;
    constants[2] = camera_forward->k * atmospheric_scale;
    constants[3] = -((window_fog->atmospheric_minimum_distance + camera_depth) * atmospheric_scale);

    constants[4] = -(window_fog->plane.normal.i * planar_depth_scale);
    constants[5] = -(window_fog->plane.normal.j * planar_depth_scale);
    constants[6] = -(window_fog->plane.normal.k * planar_depth_scale);
    constants[7] = window_fog->plane.d * planar_depth_scale;

    constants[8] = camera_forward->i * planar_distance_scale;
    constants[9] = camera_forward->j * planar_distance_scale;
    constants[10] = camera_forward->k * planar_distance_scale;
    constants[11] = -(planar_distance_scale * camera_depth);

    constants[12] = real_pin_unit(window_fog->atmospheric_maximum_density);
    constants[13] = real_pin_unit(-(planar_depth_scale *
                                    ((camera_position->x * window_fog->plane.normal.i +
                                      camera_position->y * window_fog->plane.normal.j +
                                      camera_position->z * window_fog->plane.normal.k) - window_fog->plane.d)));
    constants[14] = real_pin_unit(window_fog->planar_maximum_density);
    constants[15] = 3.0f;

    render_device().set_vertex_shader_constant_f(6, constants, 4);

    rasterizer_fog_enabled = console_debug_toggle_6893fc;
    rasterizer_set_render_state(halo::d3d9::rs::fog_enable, rasterizer_fog_enabled);
    rasterizer_set_render_state(halo::d3d9::rs::fog_color, halo::interface::color_rgb_float_to_int(&window_fog->atmospheric_color.red));
    rasterizer_set_render_state(halo::d3d9::rs::fog_table_mode, 0);
    rasterizer_set_render_state(halo::d3d9::rs::fog_vertex_mode, 3);
    rasterizer_set_render_state(halo::d3d9::rs::fog_start, real_bits(window_fog->atmospheric_minimum_distance));
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {

        float z_far = rasterizer_window.frustum.z_far;
        rasterizer_set_render_state(halo::d3d9::rs::fog_end, real_bits((z_far - window_fog->atmospheric_maximum_density * z_far) +
                                                    window_fog->atmospheric_maximum_distance));
    } else {
        rasterizer_set_render_state(halo::d3d9::rs::fog_end, real_bits(window_fog->atmospheric_maximum_distance));
    }
}

}  // namespace rasterizer_set_fog_constants_impl

namespace rasterizer_set_shader_stage_config_impl {


/**
 * Applies a cached, mode-dependent bundle of texture stage render states, doing nothing if `mode` already
 * matches the cached configuration.
 *
 * Registers: AX -> mode
 *
 * @address 0x519200
 */
void rasterizer_set_shader_stage_config(int16_t mode)
{
    namespace rs = halo::d3d9::rs;
    auto set_render_state = [](uint32_t state, uint32_t value) { return render_device().set_render_state(state, value); };
    auto set_stencil = [&](uint32_t fail, uint32_t z_fail, uint32_t pass, uint32_t func, uint32_t ref) {
        set_render_state(rs::stencil_enable, 1);
        set_render_state(rs::stencil_fail, fail);
        set_render_state(rs::stencil_z_fail, z_fail);
        set_render_state(rs::stencil_pass, pass);
        set_render_state(rs::stencil_func, func);
        set_render_state(rs::stencil_ref, ref);
    };

    if (halo::rasterizer::fields::shader_stage_config_enabled == 0) {
        mode = 0;
    }
    if (mode == rasterizer_shader_stage_config) {
        return;
    }

    switch (mode) {
    case 0:
        set_render_state(rs::stencil_enable, 0);
        break;
    case 1:
        set_stencil(1, 1, 3, 8, 1);
        set_render_state(rs::stencil_mask, 1);
        set_render_state(rs::stencil_write_mask, 1);
        break;
    case 2:
        set_stencil(1, 1, 1, 3, 0);
        set_render_state(rs::stencil_mask, 1);
        set_render_state(rs::stencil_write_mask, 0);
        break;
    case 3:
        set_stencil(1, 1, 1, 6, 0);
        set_render_state(rs::stencil_mask, 1);
        set_render_state(rs::stencil_write_mask, 0);
        break;
    case 4:
        set_stencil(1, 1, 3, 3, 2);
        set_render_state(rs::stencil_mask, 1);
        set_render_state(rs::stencil_write_mask, 2);
        break;
    case 5:
        set_stencil(1, 1, 1, 3, 0);
        set_render_state(rs::stencil_mask, 3);
        set_render_state(rs::stencil_write_mask, 0);
        break;
    default:
        break;
    }
    rasterizer_shader_stage_config = mode;
}

}  // namespace rasterizer_set_shader_stage_config_impl

namespace rasterizer_shader_technique_for_name_impl {


/**
 * VERIFIED against disassembly 0x530120..0x5301ae (2026-09-30): version split (0x7c118c =
 * caps.PixelShaderVersion, major = bits 8..15, minor = bits 0..7), loop order/reset to minor 9, sprintf
 * argument order, both stdcall vtable calls (+0x34 by name, +0xf4 validate) and the hr >= 0 test match. A
 * difftest "died" needs a real ID3DXEffect behind EDI.
 *
 * @address 0x530120
 */
void * rasterizer_shader_technique_for_name(void *effect, const char *name)
{
    char full_name[128];
    void *technique = 0;
    int32_t major, minor;
    int32_t found = 0;

    major = (rasterizer_caps.pixel_shader_version >> 8) & 0xff;
    minor = rasterizer_caps.pixel_shader_version & 0xff;
    for (; !found && major >= 0; major--, minor = 9) {
        for (; !found && minor >= 0; minor--) {
            sprintf(full_name, "%s_ps_%d_%d", name, major, minor);
            technique = d3d_handle(render_device().effect_get_technique_by_name(effect, full_name));
            if (technique != 0) {
                int32_t hr = render_device().effect_validate_technique(effect, technique);
                found = hr >= 0;
            }
        }
    }
    return technique;
}

}  // namespace rasterizer_shader_technique_for_name_impl

}  // namespace halo::rasterizer
