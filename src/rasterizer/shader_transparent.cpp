/**
 * @file src/rasterizer/shader_transparent.cpp
 * Transparent shader draw passes: chicago, plasma, glass and water.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/shader_access.hpp"
#include "internal/state.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/shaders/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/shaders/shaders.hpp"
#include <cstddef>
#include "halo/core/libm.hpp"
#include "halo/tags/flags.hpp"
#include "halo/core/datum.hpp"

namespace {

constexpr uint32_t k_extra_flag_dont_fade_active_camouflage =
    static_cast<uint32_t>(halo::tags::shader_transparent_chicago_extra_tag_flag::don_t_fade_active_camouflage);
constexpr uint32_t k_extra_flag_numeric_countdown_timer =
    static_cast<uint32_t>(halo::tags::shader_transparent_chicago_extra_tag_flag::numeric_countdown_timer);
constexpr uint16_t k_glass_bump_map_is_specular_mask =
    static_cast<uint16_t>(halo::tags::shader_transparent_glass_tag_flag::bump_map_is_specular_mask);
constexpr uint16_t k_water_base_map_alpha_modulates_reflection =
    static_cast<uint16_t>(halo::tags::shader_transparent_water_tag_flag::base_map_alpha_modulates_reflection);
constexpr uint16_t k_water_base_map_color_modulates_background =
    static_cast<uint16_t>(halo::tags::shader_transparent_water_tag_flag::base_map_color_modulates_background);
constexpr uint16_t k_water_draw_before_fog = static_cast<uint16_t>(halo::tags::shader_transparent_water_tag_flag::draw_before_fog);
constexpr uint16_t k_map_unfiltered = static_cast<uint16_t>(halo::tags::shader_transparent_chicago_map_tag_flag::unfiltered);
constexpr uint16_t k_map_u_clamped = static_cast<uint16_t>(halo::tags::shader_transparent_chicago_map_tag_flag::u_clamped);
constexpr uint16_t k_map_v_clamped = static_cast<uint16_t>(halo::tags::shader_transparent_chicago_map_tag_flag::v_clamped);

}  // namespace

static_assert(offsetof(ShaderTransparentChicagoMap, map) + offsetof(TagDependency, tag_id) == 0x78, "chicago map bitmap id");
static_assert(offsetof(ShaderTransparentChicagoMap, map_u_scale) == 0x54, "chicago map scale");
static_assert(offsetof(ShaderTransparentChicagoMap, u_animation_source) == 0xa4, "chicago map animation");
static_assert(offsetof(ShaderTransparentGlass, perpendicular_brightness) == 0x8c, "glass brightness");
static_assert(offsetof(ShaderTransparentGlass, bump_map_scale) == 0xbc, "glass bump scale");
static_assert(offsetof(ShaderTransparentGlass, bump_map) + offsetof(TagDependency, tag_id) == 0xcc, "glass bump id");
static_assert(offsetof(ShaderTransparentGlass, reflection_map) + offsetof(TagDependency, tag_id) == 0xb8, "glass reflection id");
static_assert(offsetof(ShaderTransparentGlass, diffuse_map) + offsetof(TagDependency, tag_id) == 0x164, "glass diffuse id");
static_assert(offsetof(ShaderTransparentGlass, diffuse_detail_map) + offsetof(TagDependency, tag_id) == 0x178, "glass detail id");
static_assert(offsetof(ShaderTransparentPlasma, tint_color_source) == 0x80, "plasma tint source");
static_assert(offsetof(ShaderTransparentPlasma, primary_animation_period) == 0xc0, "plasma primary");
static_assert(offsetof(ShaderTransparentPlasma, secondary_animation_period) == 0x108, "plasma secondary");
static_assert(offsetof(ShaderTransparentPlasma, primary_noise_map) + offsetof(TagDependency, tag_id) == 0xe0, "plasma noise id");
static_assert(offsetof(ShaderTransparentPlasma, secondary_noise_map) + offsetof(TagDependency, tag_id) == 0x128, "plasma noise id 2");
static_assert(offsetof(ShaderTransparentWater, base_map) + offsetof(TagDependency, tag_id) == 0x58, "water base map id");
static_assert(offsetof(ShaderTransparentWater, view_perpendicular_brightness) == 0x6c, "water brightness");
static_assert(offsetof(ShaderTransparentWater, view_parallel_brightness) == 0x7c, "water parallel brightness");
static_assert(offsetof(ShaderTransparentWater, view_parallel_tint_color) == 0x80, "water parallel tint");
static_assert(offsetof(ShaderTransparentWater, reflection_map) + offsetof(TagDependency, tag_id) == 0xa8, "water reflection id");
static_assert(offsetof(ShaderTransparentWater, ripple_animation_angle) == 0xbc, "water ripple angle");
static_assert(offsetof(ShaderTransparentWater, ripple_scale) == 0xc4, "water ripple scale");
static_assert(offsetof(ShaderTransparentWater, ripple_maps) + offsetof(TagDependency, tag_id) == 0xd4, "water ripple map id");
static_assert(offsetof(ShaderTransparentWater, ripple_mipmap_levels) == 0xd8, "water mipmap levels");
static_assert(offsetof(ShaderTransparentWater, ripples) == 0x124, "water ripples");
static_assert(sizeof(ShaderTransparentWaterRipple) == 0x4c, "water ripple size");
static_assert(offsetof(ShaderTransparentWaterRipple, map_repeats) == 0x38, "ripple repeats");
static_assert(sizeof(ShaderTransparentChicagoMap) == 0xdc, "chicago map size");
static_assert(offsetof(ShaderTransparentChicago, extra_flags) == 0x60, "chicago extra flags");
static_assert(offsetof(ShaderTransparentChicagoExtended, extra_flags) == 0x6c, "chicago extended extra flags");
static_assert(offsetof(ShaderTransparentChicago, shader_transparent_chicago_flags) == 0x29, "chicago flags");
static_assert(offsetof(ShaderTransparentChicago, numeric_counter_limit) == 0x28, "chicago numeric limit");




namespace halo::rasterizer {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);


/**
 * Direct3D 9 back end function rasterizer_glass_diffuse_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_glass_diffuse_draw.c.txt.
 *
 * @address 0x523690
 */
void rasterizer_glass_diffuse_draw(transparent_geometry_group *group)
{
    int16_t vertex_type;
    int16_t shader_index;
    void *declaration;
    int32_t has_lightmap;
    uint32_t pass_index;

    if (rasterizer_effects[109].effect == 0) {
        return;
    }

    vertex_type = -1;
    if (group->vertex_buffer == 0) {
        if (group->dynamic_vertex_slot != -1) {
            vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
        }
    } else {
        vertex_type = group->vertex_buffer->type;
    }

    if (vertex_type == 0 || vertex_type == 2) {
        shader_index = 0;
        declaration = (group->lightmap_bitmap == 0) ?
            rasterizer_vertex_declarations[0].declaration : rasterizer_vertex_declarations[2].declaration;
    } else if (vertex_type == 4) {
        declaration = rasterizer_vertex_declarations[4].declaration;
        shader_index = 1;
    } else {
        shader_index = (int16_t)(uint32_t)group;
        declaration = rasterizer_vertex_declarations[0].declaration;
    }

    render_device().set_vertex_declaration(declaration);

    render_device().set_vertex_shader((uint32_t)rasterizer_vertex_shaders[48 + shader_index].shader);

    render_device().set_vertex_shader_constant_f(10, &group->position, 3);

    chimera__rasterizer_set_texture(halo::tag_id_bits(shader_cast<ShaderTransparentGlass>(group->shader)->diffuse_map.tag_id), 0, 0, 1, (int16_t)group->shader_permutation);
    chimera__rasterizer_set_texture(halo::tag_id_bits(shader_cast<ShaderTransparentGlass>(group->shader)->diffuse_detail_map.tag_id), 1, 0, 2, (int16_t)group->shader_permutation);

    render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);

    has_lightmap = group->lightmap_bitmap != 0;
    if (has_lightmap) {
        rasterizer_bind_texture_d3d9(2, group->lightmap_bitmap);
        render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(2, halo::d3d9::ss::mip_filter, 2);
    }

    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);

    pass_index = 0;
    render_device().effect_begin(rasterizer_effects[109].effect, &pass_index, 3);
    render_device().effect_pass(rasterizer_effects[109].effect, (uint32_t)has_lightmap);
    rasterizer_transparent_geometry_group_draw_vertices(group, (int32_t)has_lightmap);
    render_device().effect_end(rasterizer_effects[109].effect);
}

namespace rasterizer_glass_diffuse_draw_fixed_function_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_glass_diffuse_draw_fixed_function. The original author notes are in
 * docs/original/rasterizer/rasterizer_glass_diffuse_draw_fixed_function.c.txt.
 *
 * @address 0x523d10
 */
void rasterizer_glass_diffuse_draw_fixed_function(transparent_geometry_group *group)
{
    int16_t vertex_type;
    void *declaration;
    uint32_t pass_count;
    uint32_t pass;

    bool has_vertex_type = true;

    if (group->vertex_buffer == 0) {
        if (group->dynamic_vertex_slot == -1) {
            has_vertex_type = false;
        } else {
            vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
        }
    } else {
        vertex_type = group->vertex_buffer->type;
    }

    if (has_vertex_type && vertex_type == 4) {
        rasterizer_glass_diffuse_draw(group);
        return;
    }

    if (rasterizer_effects[109].effect == 0) {
        return;
    }

    declaration = rasterizer_vertex_declarations[13].declaration;
    if (group->lightmap_bitmap == 0) {
        declaration = rasterizer_vertex_declarations[12].declaration;
    }

    render_device().set_vertex_declaration(declaration);

    render_device().set_vertex_shader(0);

    chimera__rasterizer_set_texture(halo::tag_id_bits(shader_cast<ShaderTransparentGlass>(group->shader)->diffuse_map.tag_id), 0, 0, 1, (int16_t)group->shader_permutation);

    if (group->lightmap_bitmap != 0) {
        rasterizer_bind_texture_d3d9(1, group->lightmap_bitmap);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_w, 1);
        render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);
    }

    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);

    render_device().effect_begin(rasterizer_effects[109].effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(rasterizer_effects[109].effect, pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, group->lightmap_bitmap != 0);
    }
    render_device().effect_end(rasterizer_effects[109].effect);
}

}  // namespace rasterizer_glass_diffuse_draw_fixed_function_impl

/**
 * Direct3D 9 back end function rasterizer_glass_draw_procedures_select. The original author notes are in
 * docs/original/rasterizer/rasterizer_glass_draw_procedures_select.c.txt.
 *
 * @address 0x523ec0
 */
void rasterizer_glass_draw_procedures_select(void)
{
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        rasterizer_glass_draw_procedures[1] = (void *)rasterizer_glass_tint_draw_fixed_function;
        rasterizer_glass_draw_procedures[2] = (void *)rasterizer_glass_reflection_draw_fixed_function;
        rasterizer_glass_draw_procedures[0] = (void *)rasterizer_glass_diffuse_draw_fixed_function;
        return;
    }
    rasterizer_glass_draw_procedures[1] = (void *)rasterizer_glass_tint_draw;
    rasterizer_glass_draw_procedures[2] = (void *)rasterizer_glass_reflection_draw;
    rasterizer_glass_draw_procedures[0] = (void *)rasterizer_glass_diffuse_draw;
}

namespace rasterizer_glass_reflection_draw_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);









static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void rasterizer_set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void rasterizer_set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static float real_negate_pinned(float x)
{
    float value = 0.5f - x * 0.5f;

    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    return value + value - 1.0f;
}

/**
 * Direct3D 9 back end function rasterizer_glass_reflection_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_glass_reflection_draw.c.txt.
 *
 * @address 0x522c60
 */
void rasterizer_glass_reflection_draw(transparent_geometry_group *group, int16_t reflection_kind)
{
    const ShaderTransparentGlass *glass = shader_cast<ShaderTransparentGlass>(group->shader);
    int16_t vertex_type = -1;
    int16_t shader_variant = 0;

    int16_t shader_base = 0;
    rasterizer_effect_slot *effect_slot;
    uint32_t specular_mask_pass;
    float vectors[16];
    float constants[12];
    int32_t width;
    int32_t height;
    int32_t i;

    if (group->vertex_buffer != 0) {
        vertex_type = (group->vertex_buffer)->type;
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }

    vectors[0] = real_negate_pinned(rasterizer_window.camera.forward.i);
    vectors[1] = real_negate_pinned(rasterizer_window.camera.forward.j);
    vectors[2] = real_negate_pinned(rasterizer_window.camera.forward.k);
    vectors[3] = 1.0f;
    vectors[4] = glass->perpendicular_tint_color.red;
    vectors[5] = glass->perpendicular_tint_color.green;
    vectors[6] = glass->perpendicular_tint_color.blue;
    vectors[7] = glass->perpendicular_brightness;
    vectors[8] = glass->parallel_tint_color.red;
    vectors[9] = glass->parallel_tint_color.green;
    vectors[10] = glass->parallel_tint_color.blue;
    vectors[11] = glass->parallel_brightness;
    for (i = 12; i < 16; i++) {
        vectors[i] = group->parameters.mode == 1 ? 1.0f - group->parameters.blend_factor : 1.0f;
    }

    if (reflection_kind == 0 && ((glass->shader_transparent_glass_flags & k_glass_bump_map_is_specular_mask) != 0 || halo::tag_id_bits(glass->bump_map.tag_id) == halo::k_dword_none)) {
        reflection_kind = 1;
    }
    switch (vertex_type) {
    case 0:
    case 2:
        shader_variant = 0;
        break;
    case 4:
        shader_variant = 1;
        break;
    default:
        break;
    }

    effect_slot = &rasterizer_effects[106];
    switch (reflection_kind) {
    case 0:
        shader_base = 0x32;
        if (effect_slot->constant_handles != 0) {
            void **handles = effect_slot->constant_handles;
            void *effect = effect_slot->effect;

            for (i = 0; i < 4; i++) {
                render_device().effect_set_vector(effect, handles[i], &vectors[i * 4]);
            }
        }
        break;
    case 1:
        effect_slot = &rasterizer_effects[107];
        shader_base = 0x34;
        if (rasterizer_caps.pixel_shader_version >= halo::d3d9::k_pixel_shader_version_1_1) {
            render_device().set_pixel_shader_constant_f(0, vectors, 3);
        }
        break;
    case 2:
        effect_slot = &rasterizer_effects[108];
        shader_base = 0x36;
        if (effect_slot->constant_handles != 0) {
            void **handles = effect_slot->constant_handles;
            void *effect = effect_slot->effect;

            for (i = 0; i < 3; i++) {
                render_device().effect_set_vector(effect, handles[i], &vectors[i * 4]);
            }
        }
        break;
    default:
        break;
    }

    specular_mask_pass = (glass->shader_transparent_glass_flags >> 3) & 1;
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[vertex_type].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[shader_variant + shader_base].shader);
    if (effect_slot->effect == 0) {
        return;
    }

    width = rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left;
    height = rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[9] = 0.0f;
    constants[8] = 0.0f;
    constants[10] = 0.0f;

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {

        constants[0] = 1.0f;
        constants[1] = 1.0f;
        constants[2] = 0.0f;
        constants[3] = 0.0f;
        constants[4] = (float)width * 0.5f;
        constants[5] = (float)height * 0.5f;
        constants[11] = 1.0f;
        render_device().set_vertex_shader_constant_f(0xa, constants, 3);
        chimera__rasterizer_set_texture(halo::tag_id_bits(rasterizer_globals_data->test_1.tag_id), 0, 0, 1,
                                        (int16_t)group->shader_permutation);
        render_device().set_pixel_shader(0);
        rasterizer_set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
        rasterizer_set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        rasterizer_set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        rasterizer_set_render_state(halo::d3d9::rs::texture_factor, 0x3c7f7f7f);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::add);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::tfactor);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse | halo::d3d9::ta::alpha_replicate);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
        rasterizer_set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
        rasterizer_set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        return;
    }

    {
        void *effect = effect_slot->effect;
        float bump_scale = glass->bump_map_scale;
        uint32_t bump_map_tag = halo::tag_id_bits(glass->bump_map.tag_id);
        uint32_t normalization_tag = halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id);
        BitmapData *bump_bitmap = 0;
        uint32_t pass_count;

        constants[0] = group->base_map_u_scale * bump_scale;
        constants[1] = group->base_map_v_scale * bump_scale;
        constants[2] = (float)width * 0.5f;
        constants[3] = (float)height * 0.5f;
        constants[4] = 0.0f;
        constants[9] = 1.0f;
        constants[11] = 0.0f;
        render_device().set_vertex_shader_constant_f(0xa, constants, 3);

        if (halo::rasterizer::fields::bump_mapping_enabled != 0 && bump_map_tag != halo::k_dword_none) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bump_map_tag & halo::k_slot_mask].data;
            int32_t count = (int32_t)bitmap->bitmap_data.count;

            if (count > 0) {
                bump_bitmap = halo::bitmaps::bitmap_group_get_bitmap_data(bump_map_tag,
                                                           (int16_t)((int32_t)(int16_t)group->shader_permutation % count));
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
                    bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
                }
            }
        }
        if (bump_bitmap != 0) {
            rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
            rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
            rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
        }

        for (i = 1; i <= 2; i++) {
            chimera__rasterizer_set_texture_direct_d3dx(normalization_tag, (int16_t)i, 0, effect_slot);
            rasterizer_set_sampler_state((uint32_t)i, 1, 3);
            rasterizer_set_sampler_state((uint32_t)i, 2, 3);
            rasterizer_set_sampler_state((uint32_t)i, 3, 3);
            rasterizer_set_sampler_state((uint32_t)i, 5, 2);
            rasterizer_set_sampler_state((uint32_t)i, 6, 1);
            rasterizer_set_sampler_state((uint32_t)i, 7, 1);
        }

        if (reflection_kind == 2) {
            render_device().effect_set_texture(effect, effect_slot->texture_handles[3], rasterizer_render_targets[2].texture);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::address_u, 3);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::address_v, 3);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::mip_filter, 1);
        } else {
            rasterizer_resolve_and_cache_submap_b(halo::tag_id_bits(glass->reflection_map.tag_id), 2, 3, 0, (int16_t)group->shader_permutation,
                                                  effect_slot);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::address_u, 3);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::address_v, 3);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::address_w, 3);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
            rasterizer_set_sampler_state(3, halo::d3d9::ss::mip_filter, 2);
        }
        rasterizer_set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
        rasterizer_set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        rasterizer_set_render_state(halo::d3d9::rs::alpha_test_enable, 0);

        render_device().effect_begin(effect, &pass_count, 3);
        render_device().effect_pass(effect, specular_mask_pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        render_device().effect_end(effect);
    }
}

}  // namespace rasterizer_glass_reflection_draw_impl

static bool group_vertex_type_is_model(const transparent_geometry_group *group)
{
    if (group->vertex_buffer == 0) {
        return group->dynamic_vertex_slot != -1 &&
               rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type == 4;
    }
    return group->vertex_buffer->type == 4;
}

namespace rasterizer_glass_reflection_draw_fixed_function_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_glass_reflection_draw_fixed_function. The original author notes are
 * in docs/original/rasterizer/rasterizer_glass_reflection_draw_fixed_function.c.txt.
 *
 * @address 0x523b90
 */
void rasterizer_glass_reflection_draw_fixed_function(transparent_geometry_group *group, uint32_t reflection_kind)
{
    if (group_vertex_type_is_model(group)) {
        rasterizer_glass_reflection_draw(group, (int16_t)reflection_kind);
        return;
    }

    chimera__rasterizer_set_texture(halo::tag_id_bits(rasterizer_globals_data->test_1.tag_id), 0, 0, 1, (int16_t)group->shader_permutation);

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[12].declaration);

    render_device().set_vertex_shader(0);

    render_device().set_pixel_shader(0);

    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    render_device().set_render_state(halo::d3d9::rs::texture_factor, 0x3cffffff);

    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::tfactor);
    render_device().set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    render_device().set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::tfactor);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);

    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
}

}  // namespace rasterizer_glass_reflection_draw_fixed_function_impl

namespace rasterizer_glass_tint_draw_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);


/**
 * Direct3D 9 back end function rasterizer_glass_tint_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_glass_tint_draw.c.txt.
 *
 * @address 0x522930
 */
void rasterizer_glass_tint_draw(transparent_geometry_group *group)
{
    int16_t vertex_type;
    int16_t shader_index;
    uint32_t decal_color;
    uint32_t stage4_arg, stage5_arg;

    vertex_type = -1;
    if (group->vertex_buffer == 0) {
        if (group->dynamic_vertex_slot != -1) {
            vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
        }
    } else {
        vertex_type = group->vertex_buffer->type;
    }

    if (vertex_type == 0 || vertex_type == 2) {
        shader_index = 0;
    } else if (vertex_type == 4) {
        shader_index = 1;
    } else {
        shader_index = (int16_t)(uint32_t)group;
    }

    chimera__rasterizer_set_texture(halo::tag_id_bits(shader_cast<ShaderTransparentGlass>(group->shader)->background_tint_map.tag_id), 0, 0, 1, (int16_t)group->shader_permutation);

    render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[vertex_type].declaration);

    render_device().set_vertex_shader((uint32_t)rasterizer_vertex_shaders[55 + shader_index].shader);

    render_device().set_vertex_shader_constant_f(10, &group->position, 3);

    render_device().set_pixel_shader(0);

    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::zero);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::src_color);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);

    decal_color = ((((uint32_t)(int32_t)(group->tint.alpha * 255.0f) & 0xff) << 8 |
                    ((uint32_t)(int32_t)(group->tint.red * 255.0f) & 0xff)) << 8 |
                   ((uint32_t)(int32_t)(group->tint.green * 255.0f) & 0xff)) << 8 |
                  ((uint32_t)(int32_t)(group->tint.blue * 255.0f) & 0xff);
    render_device().set_render_state(halo::d3d9::rs::texture_factor, decal_color);

    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::tfactor);

    if (group->parameters.mode == 1) {
        render_device().set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
        render_device().set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::diffuse);
        stage4_arg = 3;
        stage5_arg = 6;
    } else {
        render_device().set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        stage4_arg = 0;
        stage5_arg = 5;
    }
    render_device().set_texture_stage_state(0, stage5_arg, stage4_arg);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::multiply_add);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse | halo::d3d9::ta::alpha_replicate);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::color_arg0, halo::d3d9::ta::diffuse | halo::d3d9::ta::complement | halo::d3d9::ta::alpha_replicate);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
    render_device().set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    render_device().set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);

    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
}

}  // namespace rasterizer_glass_tint_draw_impl

namespace rasterizer_glass_tint_draw_fixed_function_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_glass_tint_draw_fixed_function. The original author notes are in
 * docs/original/rasterizer/rasterizer_glass_tint_draw_fixed_function.c.txt.
 *
 * @address 0x523980
 */
void rasterizer_glass_tint_draw_fixed_function(transparent_geometry_group *group)
{
    uint32_t decal_color;
    uint32_t stage4_arg, stage5_arg;

    if (group_vertex_type_is_model(group)) {
        rasterizer_glass_tint_draw(group);
        return;
    }

    chimera__rasterizer_set_texture(halo::tag_id_bits(shader_cast<ShaderTransparentGlass>(group->shader)->background_tint_map.tag_id), 0, 0, 1, (int16_t)group->shader_permutation);

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[12].declaration);

    render_device().set_vertex_shader(0);

    render_device().set_pixel_shader(0);

    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::zero);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::src_color);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);

    {
        ShaderTransparentGlass *glass = shader_cast<ShaderTransparentGlass>(group->shader);
        double factor = (group->parameters.mode == 1) ? (double)group->parameters.blend_factor : 1.0;

        decal_color = (uint32_t)(int32_t)((double)glass->background_tint_color.red * 255.0) & 0xff;
        decal_color |= (uint32_t)(int32_t)(factor * 255.0) << 8;
        decal_color <<= 8;
        decal_color |= (uint32_t)(int32_t)((double)glass->background_tint_color.green * 255.0) & 0xff;
        decal_color <<= 8;
        decal_color |= (uint32_t)(int32_t)((double)glass->background_tint_color.blue * 255.0) & 0xff;
    }
    render_device().set_render_state(halo::d3d9::rs::texture_factor, decal_color);

    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    render_device().set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::tfactor);

    if (group->parameters.mode == 1) {
        render_device().set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
        render_device().set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::diffuse);
        stage4_arg = 0x13;
        stage5_arg = 6;
    } else {
        render_device().set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        stage4_arg = 3;
        stage5_arg = 5;
    }
    render_device().set_texture_stage_state(0, stage5_arg, stage4_arg);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    render_device().set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);

    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
}

}  // namespace rasterizer_glass_tint_draw_fixed_function_impl

namespace rasterizer_shader_transparent_chicago_draw_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);



static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void tss(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static int32_t numeric_value(float limit, float value)
{
    float rounded = (float)halo::libm::floor(limit * value + 0.5f);

    return (int32_t)rounded;
}

/**
 * Direct3D 9 back end function rasterizer_shader_transparent_chicago_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_transparent_chicago_draw.c.txt.
 *
 * @address 0x531ed0
 */
void rasterizer_shader_transparent_chicago_draw(transparent_geometry_group *group, uint8_t attached)
{
    ShaderTransparentChicago *shader = shader_cast<ShaderTransparentChicago>(group->shader);
    ShaderTransparentChicagoMap *maps;
    uint8_t ok = 1;
    int16_t permutation;
    int16_t vertex_type;
    int16_t frame;
    int16_t first_map_type;
    int32_t map_count;
    int16_t layer;
    int16_t map;
    uint32_t fade_argument;
    int16_t stage;
    float map_constants[8][4];
    float fade_constants[3][4];

    permutation = halo::shaders::shader_view(const_cast<Shader *>(&shader->base)).vertex_shader_permutation();
    vertex_type = -1;
    if (group->vertex_buffer != 0) {
        vertex_type = group->vertex_buffer->type;
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    frame = (int16_t)group->shader_permutation;
    maps = (ShaderTransparentChicagoMap *)(uintptr_t)shader->maps.pointer;
    if (maps == NULL || maps->map.path_pointer == 0) {
        return;
    }
    if (render_device().set_vertex_shader(rasterizer_vertex_shaders[rasterizer_transparent_vertex_shader_table[vertex_type * 6 + permutation]].shader) < 0) {
        ok = 0;
    }
    if (render_device().set_vertex_declaration(rasterizer_vertex_declarations[vertex_type].declaration) < 0) {
        ok = 0;
    }
    if (render_device().set_pixel_shader(0) < 0) {
        ok = 0;
    }

    for (layer = 0; layer < (int32_t)shader->extra_layers.count; layer++) {
        transparent_geometry_group copy = *group;
        const ShaderTransparentExtraLayer *layers = (const ShaderTransparentExtraLayer *)(uintptr_t)shader->extra_layers.pointer;
        uint32_t tag_id = halo::tag_id_bits(layers[layer].shader.tag_id);

        copy.sorted_index = -1;
        copy.shader = static_cast<Shader *>(halo::cache::globals().tag_instances[tag_id & halo::k_slot_mask].data);
        rasterizer_transparent_geometry_group_draw(&copy, attached);
    }

    set_render_state(halo::d3d9::rs::cull_mode, (shader->shader_transparent_chicago_flags & _shader_transparent_two_sided_bit) ? 1 : 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, shader->shader_transparent_chicago_flags & _shader_transparent_alpha_tested_bit);
    set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    chimera__rasterizer_set_framebuffer_blend_function(shader->framebuffer_blend_function);

    if ((shader->shader_transparent_chicago_flags & _shader_transparent_numeric_bit) != 0 && group->lighting_extra != 0 && (int32_t)shader->maps.count > 0) {
        const Bitmap *bitmap = (const Bitmap *)halo::cache::globals().tag_instances[halo::tag_id_bits(maps->map.tag_id) & halo::k_slot_mask].data;
        int16_t base = (int16_t)bitmap->bitmap_data.count;

        if (shader->extra_flags & k_extra_flag_numeric_countdown_timer) {
            frame = halo::shaders::numeric_countdown_timer::get_digit((int16_t)group->shader_permutation);
        } else {
            const float *function_values = *(const float **)(uintptr_t)(group->lighting_extra + 4);
            int32_t limit = (int16_t)shader->numeric_counter_limit;
            int value_index = (base != 8) ? 0 : 3;
            int16_t value;
            int16_t digit;

            if (numeric_value((float)limit, function_values[value_index]) < 0) {
                value = 0;
            } else if (numeric_value((float)limit, function_values[value_index]) > limit) {
                value = (int16_t)limit;
            } else {
                value = (int16_t)numeric_value((float)limit, function_values[value_index]);
            }
            for (digit = (int16_t)group->shader_permutation; digit > 0; digit--) {
                value = (int16_t)(value / base);
            }
            frame = (int16_t)(value % base);
        }
    }

    first_map_type = shader->first_map_type;
    for (map = 0; map < 4; map++) {
        map_count = (int32_t)shader->maps.count;
        if (map < map_count) {
            ShaderTransparentChicagoMap *entry = &maps[map];
            int16_t bitmap_type = (map != 0) ? 0 : rasterizer_first_map_bitmap_types[first_map_type];
            uint32_t address_u, address_v, address_w;
            uint32_t filter = (entry->flags & k_map_unfiltered) ? 1 : 2;

            chimera__rasterizer_set_texture(halo::tag_id_bits(entry->map.tag_id), map, bitmap_type, 0, frame);
            if (bitmap_type == 0 && (entry->flags & k_map_u_clamped)) {
                address_u = 3;
            } else {
                address_u = (map != 0) ? 1 : rasterizer_first_map_address_modes[first_map_type];
            }
            if (bitmap_type == 0 && (entry->flags & k_map_v_clamped)) {
                address_v = 3;
            } else {
                address_v = (map != 0) ? 1 : rasterizer_first_map_address_modes[first_map_type];
            }
            address_w = (map != 0) ? 1 : rasterizer_first_map_address_modes[first_map_type];
            set_sampler_state(map, halo::d3d9::ss::address_u, address_u);
            set_sampler_state(map, halo::d3d9::ss::address_v, address_v);
            set_sampler_state(map, halo::d3d9::ss::address_w, address_w);
            set_sampler_state(map, halo::d3d9::ss::mag_filter, 2);
            set_sampler_state(map, halo::d3d9::ss::min_filter, filter);
            set_sampler_state(map, halo::d3d9::ss::mip_filter, filter);
        }
        map_count = (int32_t)shader->maps.count;
        if (map < map_count && (map > 0 || first_map_type == 0)) {
            ShaderTransparentChicagoMap *entry = &maps[map];
            float u_scale = entry->map_u_scale;
            float v_scale = entry->map_v_scale;

            if (map == 0) {
                if (shader->shader_transparent_chicago_flags & _shader_transparent_scale_first_map_with_distance_bit) {
                    u_scale = -(u_scale * group->depth);
                    v_scale = -(v_scale * group->depth);
                }
                if (!(shader->shader_transparent_chicago_flags & _shader_transparent_first_map_is_in_screenspace_bit)) {
                    u_scale *= group->base_map_u_scale;
                    v_scale *= group->base_map_v_scale;
                }
            } else {
                u_scale *= group->base_map_u_scale;
                v_scale *= group->base_map_v_scale;
            }
            halo::shaders::shader_texture_animation_evaluate(group->lighting_extra, reinterpret_cast<shader_texture_animation *>(&entry->u_animation_source),
                                              map_constants[map * 2], map_constants[map * 2 + 1], u_scale, v_scale,
                                              entry->map_u_offset, entry->map_v_offset,
                                              entry->map_rotation, (float)rasterizer_time.time);
        } else if (map < map_count && (shader->shader_transparent_chicago_flags & _shader_transparent_first_map_is_in_screenspace_bit)) {

            const real_matrix4x3 *view_to_world = &rasterizer_window.frustum.view_to_world;

            map_constants[0][0] = view_to_world->forward.i;
            map_constants[0][1] = view_to_world->forward.j;
            map_constants[0][2] = view_to_world->forward.k;
            map_constants[1][0] = view_to_world->left.i;
            map_constants[1][1] = view_to_world->left.j;
            map_constants[1][2] = view_to_world->left.k;
            map_constants[0][3] = 0.0f;
            map_constants[1][3] = 0.0f;
        } else {
            map_constants[map * 2][0] = 1.0f;
            map_constants[map * 2][1] = 0.0f;
            map_constants[map * 2][2] = 0.0f;
            map_constants[map * 2 + 1][0] = 0.0f;
            map_constants[map * 2 + 1][1] = 1.0f;
            map_constants[map * 2 + 1][2] = 0.0f;
            map_constants[map * 2][3] = 0.0f;
            map_constants[map * 2 + 1][3] = 0.0f;
        }
    }
    if (render_device().set_vertex_shader_constant_f(0xd, &map_constants[0][0], 8) >= 0 && ok) {
        rasterizer_shader_transparent_chicago_set_texture_stages((const ShaderTransparentChicago *)shader);
    }

    stage = (int16_t)(int32_t)shader->maps.count;
    if (!((group->flags & 0x10) && shader->framebuffer_blend_function == 0)) {
        {
            int16_t fade_source = shader->framebuffer_fade_source;
            int i;

            for (i = 0; i < 3; i++) {
                fade_constants[i][0] = 0.0f;
                fade_constants[i][1] = 0.0f;
                fade_constants[i][2] = 0.0f;
                fade_constants[i][3] = 0.0f;
            }
            fade_constants[2][2] = 1.0f;
            if (group->parameters.mode == 1 && !(shader->extra_flags & k_extra_flag_dont_fade_active_camouflage)) {
                float fade = 1.0f - group->parameters.blend_factor;

                fade_constants[2][2] = fade < 0.0f ? 0.0f : (fade > 1.0f ? 1.0f : fade);
            }
            if (fade_source > 0 && group->lighting_extra != 0) {
                const float *function_values = *(const float **)(uintptr_t)(group->lighting_extra + 4);

                if (function_values != NULL) {
                    const float *value = &function_values[fade_source - 1];

                    if (*value == 0.0f && rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
                        return;
                    }
                    fade_constants[2][2] *= *value;
                }
            }
            render_device().set_vertex_shader_constant_f(10, &fade_constants[0][0], 3);
        }
        switch (shader->framebuffer_fade_mode) {
        case 0: fade_argument = 0x20; break;
        case 1: fade_argument = 0x24; break;
        case 2: fade_argument = 4; break;
        default: fade_argument = (uint32_t)(uint16_t)first_map_type; break;
        }

        map_count = (int32_t)shader->maps.count;
        switch (shader->framebuffer_blend_function) {
        case 0:
            if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
                stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
                tss(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                tss(0, 6, fade_argument);
            } else {
                stage = (int16_t)map_count;
                tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
                tss(stage, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
                tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
                tss(stage, 6, fade_argument);
            }
            stage++;
            break;
        case 1:
        case 5:
            stage = (int16_t)(rasterizer_caps.max_simultaneous_textures > 2 ? map_count
                                                                           : (map_count - 1 > 1 ? map_count - 1 : 1));
            tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::multiply_add);
            tss(stage, 2, fade_argument | 0x10);
            tss(stage, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
            tss(stage, 0x1a, fade_argument);
            tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            stage++;
            break;
        case 2:
            stage = (int16_t)(rasterizer_caps.max_simultaneous_textures > 2 ? map_count
                                                                           : (map_count - 1 > 1 ? map_count - 1 : 1));
            set_render_state(halo::d3d9::rs::texture_factor, 0x7f7f7f7f);
            tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::lerp);
            tss(stage, 2, fade_argument);
            tss(stage, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
            tss(stage, halo::d3d9::ts::color_arg0, halo::d3d9::ta::tfactor);
            tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            stage++;
            break;
        case 3:
        case 4:
        case 6:
            if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
                stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
                tss(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                tss(0, 3, fade_argument);
            } else {
                stage = (int16_t)map_count;
                tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                tss(stage, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
                tss(stage, 3, fade_argument);
                tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
                tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            }
            stage++;
            break;
        case 7:
            if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
                stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
                tss(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                tss(0, 3, fade_argument);
                tss(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                tss(0, 6, fade_argument);
            } else {
                stage = (int16_t)map_count;
                tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                tss(stage, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
                tss(stage, 3, fade_argument);
                tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
                tss(stage, 6, fade_argument);
            }
            stage++;
            break;
        default:
            break;
        }
    }

    render_device().set_texture((uint32_t)(int32_t)stage, 0);
    tss((uint32_t)(int32_t)stage, 1, 1);
    tss((uint32_t)(int32_t)stage, 4, 1);
    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    set_render_state(halo::d3d9::rs::blend_op, 1);
}

}  // namespace rasterizer_shader_transparent_chicago_draw_impl

namespace rasterizer_shader_transparent_chicago_extended_draw_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);



static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void tss(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static int32_t numeric_value(float limit, float value)
{
    float rounded = (float)halo::libm::floor(limit * value + 0.5f);

    return (int32_t)rounded;
}

/**
 * Direct3D 9 back end function rasterizer_shader_transparent_chicago_extended_draw. The original author notes
 * are in docs/original/rasterizer/rasterizer_shader_transparent_chicago_extended_draw.c.txt.
 *
 * @address 0x532a40
 */
void rasterizer_shader_transparent_chicago_extended_draw(transparent_geometry_group *group, uint8_t attached)
{
    ShaderTransparentChicagoExtended *shader = shader_cast<ShaderTransparentChicagoExtended>(group->shader);
    ShaderTransparentChicagoMap *maps;
    ShaderTransparentChicagoMap *map_list[4];
    uint8_t ok = 1;
    int16_t permutation;
    int16_t vertex_type;
    int16_t frame;
    int16_t first_map_type;
    int32_t map_count;
    int16_t layer;
    int16_t map;
    uint32_t fade_argument;
    int16_t stage;
    float map_constants[8][4];
    float fade_constants[3][4];

    permutation = halo::shaders::shader_view(const_cast<Shader *>(&shader->base)).vertex_shader_permutation();
    vertex_type = -1;
    if (group->vertex_buffer != 0) {
        vertex_type = group->vertex_buffer->type;
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    frame = (int16_t)group->shader_permutation;
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        maps = (ShaderTransparentChicagoMap *)(uintptr_t)shader->maps_2_stage.pointer;
    } else {
        maps = (ShaderTransparentChicagoMap *)(uintptr_t)shader->maps_4_stage.pointer;
    }
    if (maps == NULL || maps->map.path_pointer == 0) {
        return;
    }
    if (render_device().set_vertex_shader(rasterizer_vertex_shaders[rasterizer_transparent_extended_vertex_shader_table[vertex_type * 6 + permutation]].shader) < 0) {
        ok = 0;
    }
    if (render_device().set_vertex_declaration(rasterizer_vertex_declarations[vertex_type].declaration) < 0) {
        ok = 0;
    }
    if (render_device().set_pixel_shader(0) < 0) {
        ok = 0;
    }

    for (layer = 0; layer < (int32_t)shader->extra_layers.count; layer++) {
        transparent_geometry_group copy = *group;
        const ShaderTransparentExtraLayer *layers = (const ShaderTransparentExtraLayer *)(uintptr_t)shader->extra_layers.pointer;
        uint32_t tag_id = halo::tag_id_bits(layers[layer].shader.tag_id);

        copy.sorted_index = -1;
        copy.shader = static_cast<Shader *>(halo::cache::globals().tag_instances[tag_id & halo::k_slot_mask].data);
        rasterizer_transparent_geometry_group_draw(&copy, attached);
    }

    set_render_state(halo::d3d9::rs::cull_mode, (shader->shader_transparent_chicago_extended_flags & _shader_transparent_two_sided_bit) ? 1 : 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, shader->shader_transparent_chicago_extended_flags & _shader_transparent_alpha_tested_bit);
    set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    chimera__rasterizer_set_framebuffer_blend_function(shader->framebuffer_blend_function);

    if ((shader->shader_transparent_chicago_extended_flags & _shader_transparent_numeric_bit) != 0 && group->lighting_extra != 0 && (int32_t)shader->maps_4_stage.count > 0) {
        const ShaderTransparentChicagoMap *maps_4_stage = (const ShaderTransparentChicagoMap *)(uintptr_t)shader->maps_4_stage.pointer;
        const Bitmap *bitmap = (const Bitmap *)halo::cache::globals().tag_instances[halo::tag_id_bits(maps_4_stage->map.tag_id) & halo::k_slot_mask].data;
        int16_t base = (int16_t)bitmap->bitmap_data.count;

        if (shader->extra_flags & k_extra_flag_numeric_countdown_timer) {
            frame = halo::shaders::numeric_countdown_timer::get_digit((int16_t)group->shader_permutation);
        } else {
            const float *function_values = *(const float **)(uintptr_t)(group->lighting_extra + 4);
            int32_t limit = (int16_t)shader->numeric_counter_limit;
            int value_index = (base != 8) ? 0 : 3;
            int16_t value;
            int16_t digit;

            if (numeric_value((float)limit, function_values[value_index]) < 0) {
                value = 0;
            } else if (numeric_value((float)limit, function_values[value_index]) > limit) {
                value = (int16_t)limit;
            } else {
                value = (int16_t)numeric_value((float)limit, function_values[value_index]);
            }
            for (digit = (int16_t)group->shader_permutation; digit > 0; digit--) {
                value = (int16_t)(value / base);
            }
            frame = (int16_t)(value % base);
        }
    }

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        map_count = (int16_t)(int32_t)shader->maps_2_stage.count;
        maps = (ShaderTransparentChicagoMap *)(uintptr_t)shader->maps_2_stage.pointer;
    } else {
        map_count = (int16_t)(int32_t)shader->maps_4_stage.count;
        maps = (ShaderTransparentChicagoMap *)(uintptr_t)shader->maps_4_stage.pointer;
    }
    for (map = 0; map < map_count; map++) {
        map_list[map] = &maps[map];
    }

    first_map_type = shader->first_map_type;
    for (map = 0; map < map_count; map++) {
        ShaderTransparentChicagoMap *entry = map_list[map];
        int16_t bitmap_type = (map != 0) ? 0 : rasterizer_extended_first_map_bitmap_types[first_map_type];
        uint32_t address_u, address_v, address_w;
        uint32_t filter = (entry->flags & k_map_unfiltered) ? 1 : 2;

        chimera__rasterizer_set_texture(halo::tag_id_bits(entry->map.tag_id), map, bitmap_type, 0, frame);
        if (bitmap_type == 0 && (entry->flags & k_map_u_clamped)) {
            address_u = 3;
        } else {
            address_u = (map != 0) ? 1 : rasterizer_extended_first_map_address_modes[first_map_type];
        }
        if (bitmap_type == 0 && (entry->flags & k_map_v_clamped)) {
            address_v = 3;
        } else {
            address_v = (map != 0) ? 1 : rasterizer_extended_first_map_address_modes[first_map_type];
        }
        address_w = (map != 0) ? 1 : rasterizer_extended_first_map_address_modes[first_map_type];
        set_sampler_state(map, halo::d3d9::ss::address_u, address_u);
        set_sampler_state(map, halo::d3d9::ss::address_v, address_v);
        set_sampler_state(map, halo::d3d9::ss::address_w, address_w);
        set_sampler_state(map, halo::d3d9::ss::mag_filter, 2);
        set_sampler_state(map, halo::d3d9::ss::min_filter, filter);
        set_sampler_state(map, halo::d3d9::ss::mip_filter, filter);

        if (map > 0 || first_map_type == 0) {
            float u_scale = entry->map_u_scale;
            float v_scale = entry->map_v_scale;

            if (map == 0) {
                if (shader->shader_transparent_chicago_extended_flags & _shader_transparent_scale_first_map_with_distance_bit) {
                    u_scale = -(u_scale * group->depth);
                    v_scale = -(v_scale * group->depth);
                }
                if (!(shader->shader_transparent_chicago_extended_flags & _shader_transparent_first_map_is_in_screenspace_bit)) {
                    u_scale *= group->base_map_u_scale;
                    v_scale *= group->base_map_v_scale;
                }
            } else {
                u_scale *= group->base_map_u_scale;
                v_scale *= group->base_map_v_scale;
            }
            halo::shaders::shader_texture_animation_evaluate(group->lighting_extra, reinterpret_cast<shader_texture_animation *>(&entry->u_animation_source),
                                              map_constants[map * 2], map_constants[map * 2 + 1], u_scale, v_scale,
                                              entry->map_u_offset, entry->map_v_offset,
                                              entry->map_rotation, (float)rasterizer_time.time);
        } else if (shader->shader_transparent_chicago_extended_flags & _shader_transparent_first_map_is_in_screenspace_bit) {
            const real_matrix4x3 *view_to_world = &rasterizer_window.frustum.view_to_world;

            map_constants[0][0] = view_to_world->forward.i;
            map_constants[0][1] = view_to_world->forward.j;
            map_constants[0][2] = view_to_world->forward.k;
            map_constants[1][0] = view_to_world->left.i;
            map_constants[1][1] = view_to_world->left.j;
            map_constants[1][2] = view_to_world->left.k;
            map_constants[0][3] = 0.0f;
            map_constants[1][3] = 0.0f;
        } else {
            map_constants[map * 2][0] = 1.0f;
            map_constants[map * 2][1] = 0.0f;
            map_constants[map * 2][2] = 0.0f;
            map_constants[map * 2][3] = 0.0f;
            map_constants[map * 2 + 1][0] = 0.0f;
            map_constants[map * 2 + 1][1] = 1.0f;
            map_constants[map * 2 + 1][2] = 0.0f;
            map_constants[map * 2 + 1][3] = 0.0f;
        }
    }
    if (render_device().set_vertex_shader_constant_f(0xd, &map_constants[0][0], 8) >= 0 && ok) {
        rasterizer_shader_transparent_chicago_extended_set_texture_stages((const ShaderTransparentChicagoExtended *)shader);
    }

    stage = (int16_t)map_count;
    if (!((group->flags & 0x10) && shader->framebuffer_blend_function == 0)) {
        {
            int16_t fade_source = shader->framebuffer_fade_source;
            int i;

            for (i = 0; i < 3; i++) {
                fade_constants[i][0] = 0.0f;
                fade_constants[i][1] = 0.0f;
                fade_constants[i][2] = 0.0f;
                fade_constants[i][3] = 0.0f;
            }
            fade_constants[2][2] = 1.0f;
            if (group->parameters.mode == 1 && !(shader->extra_flags & k_extra_flag_dont_fade_active_camouflage)) {
                float fade = 1.0f - group->parameters.blend_factor;

                fade_constants[2][2] = fade < 0.0f ? 0.0f : (fade > 1.0f ? 1.0f : fade);
            }
            if (fade_source > 0 && group->lighting_extra != 0) {
                const float *function_values = *(const float **)(uintptr_t)(group->lighting_extra + 4);

                if (function_values != NULL) {
                    const float *value = &function_values[fade_source - 1];

                    if (*value == 0.0f && rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
                        return;
                    }
                    fade_constants[2][2] *= *value;
                }
            }
            render_device().set_vertex_shader_constant_f(10, &fade_constants[0][0], 3);
        }
        switch (shader->framebuffer_fade_mode) {
        case 0: fade_argument = 0x20; break;
        case 1: fade_argument = 0x24; break;
        case 2: fade_argument = 4; break;
        default: fade_argument = (uint32_t)(uintptr_t)shader; break;
        }

        switch (shader->framebuffer_blend_function) {
        case 0:
            if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
                stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
                tss(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                tss(0, 6, fade_argument);
            } else {
                stage = (int16_t)map_count;
                tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
                tss(stage, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
                tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
                tss(stage, 6, fade_argument);
            }
            stage++;
            break;
        case 1:
        case 5:
            stage = (int16_t)(rasterizer_caps.max_simultaneous_textures > 2 ? map_count
                                                                           : (map_count - 1 > 1 ? map_count - 1 : 1));
            tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::multiply_add);
            tss(stage, 2, fade_argument | 0x10);
            tss(stage, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
            tss(stage, 0x1a, fade_argument);
            tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            stage++;
            break;
        case 2:
            stage = (int16_t)(rasterizer_caps.max_simultaneous_textures > 2 ? map_count
                                                                           : (map_count - 1 > 1 ? map_count - 1 : 1));
            set_render_state(halo::d3d9::rs::texture_factor, 0x7f7f7f7f);
            tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::lerp);
            tss(stage, 2, fade_argument);
            tss(stage, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
            tss(stage, halo::d3d9::ts::color_arg0, halo::d3d9::ta::tfactor);
            tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            stage++;
            break;
        case 3:
        case 4:
        case 6:
            if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
                stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
                tss(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                tss(0, 3, fade_argument);
            } else {
                stage = (int16_t)map_count;
                tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                tss(stage, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
                tss(stage, 3, fade_argument);
                tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
                tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            }
            stage++;
            break;
        case 7:
            if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
                stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
                tss(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                tss(0, 3, fade_argument);
                tss(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                tss(0, 6, fade_argument);
            } else {
                stage = (int16_t)map_count;
                tss(stage, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                tss(stage, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
                tss(stage, 3, fade_argument);
                tss(stage, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
                tss(stage, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
                tss(stage, 6, fade_argument);
            }
            stage++;
            break;
        default:
            break;
        }
    }

    render_device().set_texture((uint32_t)(int32_t)stage, 0);
    tss((uint32_t)(int32_t)stage, 1, 1);
    tss((uint32_t)(int32_t)stage, 4, 1);
    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    set_render_state(halo::d3d9::rs::blend_op, 1);
}

}  // namespace rasterizer_shader_transparent_chicago_extended_draw_impl

namespace rasterizer_shader_transparent_chicago_extended_set_texture_stages_impl {

typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * VERIFIED against disassembly 0x537d60..0x537f6d (2026-09-30): list selection (caps halo::d3d9::k_pixel_shader_version_1_1), the pointer
 * array copy, the per-map stage numbering (stage = index + 1, last map on stage 0), all six
 * SetTextureStageState calls of both branches (vtable +0x10c, stdcall) and the return values match. The
 * unbounded 4-entry array is the original's own behaviour, so a difftest with a random count smashes the stack
 * of either version.
 *
 * @address 0x537d60
 */
uint8_t rasterizer_shader_transparent_chicago_extended_set_texture_stages(const ShaderTransparentChicagoExtended *shader)
{
    const ShaderTransparentChicagoMap *maps[4];
    const TagReflexive *list;
    int16_t count;
    int16_t map_index;

    if ((int32_t)shader->maps_4_stage.count <= 0) {
        return 0;
    }
    list = (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) ? &shader->maps_2_stage : &shader->maps_4_stage;
    count = (int16_t)list->count;
    if (count <= 0) {
        return 1;
    }
    for (map_index = 0; map_index < count; map_index++) {
        maps[map_index] = (const ShaderTransparentChicagoMap *)(uintptr_t)list->pointer + map_index;
    }
    for (map_index = 0; map_index < count; map_index++) {
        const ShaderTransparentChicagoMap *map = maps[map_index];
        uint32_t replicate = (*(const uint8_t *)&map->flags & 2) << 4;

        if (map_index == count - 1) {
            set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, halo::d3d9::ta::diffuse);
        } else {
            uint32_t stage = (uint32_t)map_index + 1;
            const uint32_t *color = rasterizer_chicago_color_function_stage_states[map->color_function];
            const uint32_t *alpha = rasterizer_chicago_color_function_stage_states[map->alpha_function];

            set_texture_stage_state(stage, halo::d3d9::ts::color_op, color[0]);
            set_texture_stage_state(stage, halo::d3d9::ts::color_arg1, color[1] | replicate);
            set_texture_stage_state(stage, halo::d3d9::ts::color_arg2, color[2]);
            set_texture_stage_state(stage, halo::d3d9::ts::alpha_op, alpha[0]);
            set_texture_stage_state(stage, halo::d3d9::ts::alpha_arg1, alpha[1]);
            set_texture_stage_state(stage, halo::d3d9::ts::alpha_arg2, alpha[2]);
        }
    }
    return 1;
}

}  // namespace rasterizer_shader_transparent_chicago_extended_set_texture_stages_impl

namespace rasterizer_shader_transparent_chicago_set_texture_stages_impl {

typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_transparent_chicago_set_texture_stages. The original author
 * notes are in docs/original/rasterizer/rasterizer_shader_transparent_chicago_set_texture_stages.c.txt.
 *
 * @address 0x537bb0
 */
uint8_t rasterizer_shader_transparent_chicago_set_texture_stages(const ShaderTransparentChicago *shader)
{
    int16_t map_index;

    if ((int32_t)shader->maps.count <= 0) {
        return 0;
    }
    for (map_index = 0; map_index < (int32_t)shader->maps.count; map_index++) {
        const ShaderTransparentChicagoMap *map =
            (const ShaderTransparentChicagoMap *)(uintptr_t)shader->maps.pointer + map_index;
        uint32_t replicate = (*(const uint8_t *)&map->flags & 2) << 4;

        if (map_index == (int32_t)shader->maps.count - 1) {
            set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, halo::d3d9::ta::diffuse);
        } else {
            uint32_t stage = (uint32_t)map_index + 1;
            const uint32_t *color = rasterizer_chicago_color_function_stage_states[map->color_function];
            const uint32_t *alpha = rasterizer_chicago_color_function_stage_states[map->alpha_function];

            set_texture_stage_state(stage, halo::d3d9::ts::color_op, color[0]);
            set_texture_stage_state(stage, halo::d3d9::ts::color_arg1, color[1] | replicate);
            set_texture_stage_state(stage, halo::d3d9::ts::color_arg2, color[2]);
            set_texture_stage_state(stage, halo::d3d9::ts::alpha_op, alpha[0]);
            set_texture_stage_state(stage, halo::d3d9::ts::alpha_arg1, alpha[1]);
            set_texture_stage_state(stage, halo::d3d9::ts::alpha_arg2, alpha[2]);
        }
    }
    return 1;
}

}  // namespace rasterizer_shader_transparent_chicago_set_texture_stages_impl

namespace rasterizer_shader_transparent_plasma_draw_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);






static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_shader_transparent_plasma_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_transparent_plasma_draw.c.txt.
 *
 * @address 0x52c4a0
 */
void rasterizer_shader_transparent_plasma_draw(transparent_geometry_group *group)
{
    const ShaderTransparentPlasma *shader;
    const ColorRGB *tint;
    float intensity;
    float offset;
    float primary_phase, secondary_phase;
    float primary_scale, secondary_scale;
    float vertex_constants[6][4];
    float color_constants[3][4];
    void *effect;
    uint32_t passes;
    uint32_t pass;
    int16_t vertex_type;
    int i;

    if (!console_debug_toggle_689423) {
        return;
    }
    shader = shader_cast<ShaderTransparentPlasma>(group->shader);
    tint = global_white_color;
    intensity = 1.0f;
    offset = 0.0f;
    if (group->lighting_extra != 0) {
        const ColorRGB *colors = animation_change_colors(group->lighting_extra);
        const float *function_values = animation_function_values(group->lighting_extra);
        int16_t source;

        source = shader->tint_color_source;
        if (colors != NULL && source >= 1 && source <= 4) {
            tint = &colors[source - 1];
        }
        if (function_values != NULL) {
            source = shader->intensity_source;
            if (source >= 1 && source <= 4) {
                intensity = (float)halo::libm::pow(function_values[source - 1], shader->intensity_exponent);
            }
            source = shader->offset_source;
            if (source >= 1 && source <= 4) {
                offset = (float)halo::libm::pow(function_values[source - 1], shader->offset_exponent) * shader->offset_amount;
            }
        }
    }
    effect = rasterizer_effects[44].effect;
    if (effect == NULL) {
        return;
    }

    secondary_scale = shader->secondary_noise_map_scale;
    primary_phase = (float)(rasterizer_time.time / shader->primary_animation_period);
    secondary_phase = (float)(rasterizer_time.time / shader->secondary_animation_period);
    primary_scale = shader->primary_noise_map_scale;
    if (offset < 0.0005f) {
        offset = 0.0f;
    }

    for (i = 0; i < 6; i++) {
        vertex_constants[i][0] = 0.0f;
        vertex_constants[i][1] = 0.0f;
        vertex_constants[i][2] = 0.0f;
    }
    vertex_constants[0][0] = primary_scale;
    vertex_constants[1][1] = primary_scale;
    vertex_constants[2][2] = primary_scale;
    vertex_constants[0][3] = primary_phase * shader->primary_animation_direction.i;
    vertex_constants[1][3] = primary_phase * shader->primary_animation_direction.j;
    vertex_constants[2][3] = primary_phase * shader->primary_animation_direction.k;
    vertex_constants[3][3] = secondary_phase * shader->secondary_animation_direction.i;
    vertex_constants[4][3] = secondary_phase * shader->secondary_animation_direction.j;
    vertex_constants[5][3] = secondary_phase * shader->secondary_animation_direction.k;
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {

        vertex_constants[0][2] = 0.01f;
        vertex_constants[3][0] = 0.4f;
        vertex_constants[4][1] = 0.4f;
        vertex_constants[5][2] = secondary_scale;
        render_device().set_vertex_shader_constant_f(0xd, &vertex_constants[0][0], 6);
        chimera__rasterizer_set_texture(halo::tag_id_bits(rasterizer_globals_data->glow.tag_id), 0, 0, 0,
                                        (int16_t)group->shader_permutation);
        chimera__rasterizer_set_texture(halo::tag_id_bits(rasterizer_globals_data->video_noise_map.tag_id), 1, 0, 0,
                                        (int16_t)group->shader_permutation);
    } else {
        vertex_constants[0][2] = offset;
        vertex_constants[3][0] = secondary_scale;
        vertex_constants[4][1] = secondary_scale;
        vertex_constants[5][2] = secondary_scale;
        render_device().set_vertex_shader_constant_f(0xd, &vertex_constants[0][0], 6);
        chimera__rasterizer_set_texture(halo::tag_id_bits(shader->primary_noise_map.tag_id), 0, 1, 0,
                                        (int16_t)group->shader_permutation);
        set_sampler_state(0, halo::d3d9::ss::address_w, 1);
        chimera__rasterizer_set_texture(halo::tag_id_bits(shader->secondary_noise_map.tag_id), 1, 1, 0,
                                        (int16_t)group->shader_permutation);
        set_sampler_state(1, halo::d3d9::ss::address_w, 1);
    }
    for (i = 0; i < 2; i++) {
        set_sampler_state(i, halo::d3d9::ss::address_u, 1);
        set_sampler_state(i, halo::d3d9::ss::address_v, 1);
        set_sampler_state(i, halo::d3d9::ss::mag_filter, 2);
        set_sampler_state(i, halo::d3d9::ss::min_filter, 2);
        set_sampler_state(i, halo::d3d9::ss::mip_filter, 2);
    }
    set_render_state(halo::d3d9::rs::cull_mode, 1);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 1);
    set_render_state(halo::d3d9::rs::z_write_enable, 0);
    set_render_state(halo::d3d9::rs::z_func, 4);
    set_render_state(halo::d3d9::rs::fog_enable, 0);

    for (i = 0; i < 4; i++) {
        color_constants[0][i] = 1.0f;
    }
    color_constants[1][0] = (shader->perpendicular_tint_color.red - shader->parallel_tint_color.red) * tint->red;
    color_constants[1][1] = (shader->perpendicular_tint_color.green - shader->parallel_tint_color.green) * tint->green;
    color_constants[1][2] = (shader->perpendicular_tint_color.blue - shader->parallel_tint_color.blue) * tint->blue;
    color_constants[1][3] = (shader->perpendicular_brightness - shader->parallel_brightness) * intensity;
    color_constants[2][0] = tint->red * shader->parallel_tint_color.red;
    color_constants[2][1] = shader->parallel_tint_color.green * tint->green;
    color_constants[2][2] = shader->parallel_tint_color.blue * tint->blue;
    color_constants[2][3] = intensity * shader->parallel_brightness;
    render_device().set_vertex_shader_constant_f(10, &color_constants[0][0], 3);

    render_device().effect_begin(effect, &passes, 3);
    vertex_type = (int16_t)transparent_geometry_group_get_vertex_type_reference(group);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[vertex_type].declaration);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[59].shader);
    for (pass = 0; pass < passes; pass++) {
        effect = rasterizer_effects[44].effect;
        render_device().effect_pass(effect, pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    }
    effect = rasterizer_effects[44].effect;
    render_device().effect_end(effect);
}

}  // namespace rasterizer_shader_transparent_plasma_draw_impl

namespace rasterizer_water_draw_fixed_function_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_stage0_samplers(uint32_t filter)
{
    d3d_call3_fn set_sampler_state = halo::d3d9::device_function<d3d_call3_fn>(rasterizer_device, halo::d3d9::device_method::set_sampler_state);

    set_sampler_state(rasterizer_device, 0, 1, filter);
    set_sampler_state(rasterizer_device, 0, 2, filter);
    set_sampler_state(rasterizer_device, 0, 5, 2);
    set_sampler_state(rasterizer_device, 0, 6, 2);
    set_sampler_state(rasterizer_device, 0, 7, 2);
}

static void effect_draw(void *effect, int32_t only_pass, transparent_geometry_group *group)
{
    uint32_t passes = 0;
    uint32_t pass;

    render_device().effect_begin(effect, &passes, 3);
    if (only_pass >= 0) {
        render_device().effect_pass(effect, (uint32_t)only_pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    } else {
        for (pass = 0; pass < passes; pass++) {
            render_device().effect_pass(effect, pass);
            rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        }
    }
    render_device().effect_end(effect);
}

/**
 * Direct3D 9 back end function rasterizer_water_draw_fixed_function. The original author notes are in
 * docs/original/rasterizer/rasterizer_water_draw_fixed_function.c.txt.
 *
 * @address 0x5358b0
 */
void rasterizer_water_draw_fixed_function(transparent_geometry_group *group)
{
    ShaderTransparentWater *water = shader_cast<ShaderTransparentWater>(group->shader);
    uint16_t frame = group->shader_permutation;
    int16_t vertex_type = -1;
    void *declaration;
    uint8_t z_write;
    void *effect;

    if (rasterizer_water_enabled == 0) {
        return;
    }
    if (group->vertex_buffer != 0) {
        vertex_type = group->vertex_buffer->type;
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    declaration = rasterizer_vertex_declarations[vertex_type].declaration;

    if ((water->water_flags & k_water_draw_before_fog) != 0 && (group->flags & (_group_immediate_bit | _group_flag_10_bit)) == 0) {
        d3d_call3_fn set_texture_stage_state;

        set_render_state(halo::d3d9::rs::cull_mode, 1);
        set_render_state(halo::d3d9::rs::color_write_enable, 0);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        set_render_state(halo::d3d9::rs::z_write_enable, 1);
        set_render_state(halo::d3d9::rs::fog_enable, 0);
        render_device().set_vertex_declaration(declaration);
        render_device().set_vertex_shader(0);
        render_device().set_pixel_shader(0);
        set_render_state(halo::d3d9::rs::texture_factor, 0xffffffff);
        set_texture_stage_state = halo::d3d9::device_function<d3d_call3_fn>(rasterizer_device, halo::d3d9::device_method::set_texture_stage_state);
        set_texture_stage_state(rasterizer_device, 0, 1, 2);
        set_texture_stage_state(rasterizer_device, 0, 2, 3);
        set_texture_stage_state(rasterizer_device, 0, 4, 2);
        set_texture_stage_state(rasterizer_device, 0, 5, 3);
        set_texture_stage_state(rasterizer_device, halo::d3d9::ts::color_op, 1, 1);
        set_texture_stage_state(rasterizer_device, halo::d3d9::ts::color_op, 4, 1);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        return;
    }

    z_write = (uint8_t)((group->flags & _group_flag_10_bit) == 0 && (water->water_flags & k_water_draw_before_fog) == 0);

    effect = rasterizer_effects[102].effect;
    if (effect != 0) {
        render_device().set_vertex_declaration(declaration);
        render_device().set_vertex_shader(0);
        if (water->water_flags & k_water_base_map_alpha_modulates_reflection) {
            set_render_state(halo::d3d9::rs::cull_mode, 1);
            set_render_state(halo::d3d9::rs::color_write_enable, 8);
            set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
            set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
            set_render_state(halo::d3d9::rs::z_enable, 1);
            set_render_state(halo::d3d9::rs::z_func, 4);
            set_render_state(halo::d3d9::rs::z_write_enable, z_write);
            set_render_state(halo::d3d9::rs::fog_enable, 0);
            chimera__rasterizer_set_texture(halo::tag_id_bits(water->base_map.tag_id), 0, 0, 1, (int16_t)frame);
            set_stage0_samplers(3);
            effect_draw(effect, 0, group);
        }
        if (water->water_flags & k_water_base_map_color_modulates_background) {
            chimera__rasterizer_set_texture(halo::tag_id_bits(water->base_map.tag_id), 0, 0, 1, (int16_t)frame);
            set_stage0_samplers(3);
            set_render_state(halo::d3d9::rs::cull_mode, 1);
            set_render_state(halo::d3d9::rs::color_write_enable, 7);
            set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
            set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::zero);
            set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::src_color);
            set_render_state(halo::d3d9::rs::blend_op, 1);
            set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
            set_render_state(halo::d3d9::rs::z_enable, 1);
            set_render_state(halo::d3d9::rs::z_func, 4);
            set_render_state(halo::d3d9::rs::z_write_enable, z_write);
            set_render_state(halo::d3d9::rs::fog_enable, (water->water_flags >> 2) & 1);
            effect_draw(effect, 1, group);
        }
    }

    effect = rasterizer_effects[103].effect;
    if (effect != 0) {
        set_render_state(halo::d3d9::rs::cull_mode, 1);
        set_render_state(halo::d3d9::rs::color_write_enable, 7);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, (~(group->flags >> 4)) & 1);
        set_render_state(halo::d3d9::rs::src_blend, (water->water_flags & k_water_base_map_alpha_modulates_reflection) ? 7 : 2);
        set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        set_render_state(halo::d3d9::rs::blend_op, 1);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        set_render_state(halo::d3d9::rs::z_write_enable, z_write);
        set_render_state(halo::d3d9::rs::fog_enable, (water->water_flags >> 2) & 1);
        render_device().set_vertex_declaration(declaration);
        render_device().set_vertex_shader(0);
        chimera__rasterizer_set_texture_direct_d3d9(halo::tag_id_bits(rasterizer_globals_data->test_0.tag_id), 0, 0);
        set_stage0_samplers(1);
        effect_draw(effect, -1, group);
    }
}

}  // namespace rasterizer_water_draw_fixed_function_impl

namespace rasterizer_water_draw_pixel_shader_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);



static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(stage, type, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_stage_samplers(uint32_t stage, uint32_t filter, uint8_t with_mip)
{
    set_sampler_state(stage, halo::d3d9::ss::address_u, filter);
    set_sampler_state(stage, halo::d3d9::ss::address_v, filter);
    if (with_mip) {
        set_sampler_state(stage, halo::d3d9::ss::address_w, filter);
    }
    set_sampler_state(stage, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(stage, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(stage, halo::d3d9::ss::mip_filter, 2);
}

static void effect_draw_all_passes(void *effect, transparent_geometry_group *group)
{
    uint32_t passes = 0;
    uint32_t pass;

    render_device().effect_begin(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        render_device().effect_pass(effect, pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    }
    render_device().effect_end(effect);
}

static void effect_draw_pass(void *effect, uint32_t pass, transparent_geometry_group *group)
{
    uint32_t passes = 0;

    render_device().effect_begin(effect, &passes, 3);
    render_device().effect_pass(effect, pass);
    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    render_device().effect_end(effect);
}

/**
 * Direct3D 9 back end function rasterizer_water_draw_pixel_shader. The original author notes are in
 * docs/original/rasterizer/rasterizer_water_draw_pixel_shader.c.txt.
 *
 * @address 0x535fd0
 */
void rasterizer_water_draw_pixel_shader(transparent_geometry_group *group)
{
    ShaderTransparentWater *water = shader_cast<ShaderTransparentWater>(group->shader);
    uint16_t frame = group->shader_permutation;
    int16_t vertex_type = -1;
    int16_t shader_index = 0;
    void *declaration;
    uint8_t z_write;
    void *effect;

    if (rasterizer_caps_flag_689 != 0 || rasterizer_water_enabled == 0) {
        return;
    }
    if (group->vertex_buffer != 0) {
        vertex_type = group->vertex_buffer->type;
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    if (vertex_type == 0 || vertex_type == 2) {
        shader_index = 0;
    } else if (vertex_type == 4) {
        shader_index = 1;
    }
    declaration = rasterizer_vertex_declarations[vertex_type].declaration;

    if ((water->water_flags & k_water_draw_before_fog) != 0 && (group->flags & (_group_immediate_bit | _group_flag_10_bit)) == 0) {
        set_render_state(halo::d3d9::rs::cull_mode, 1);
        set_render_state(halo::d3d9::rs::color_write_enable, 0);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        set_render_state(halo::d3d9::rs::z_write_enable, 1);
        set_render_state(halo::d3d9::rs::fog_enable, 0);
        render_device().set_vertex_declaration(declaration);
        render_device().set_vertex_shader(rasterizer_vertex_shaders[60 + shader_index].shader);
        render_device().set_pixel_shader(0);
        set_render_state(halo::d3d9::rs::texture_factor, 0xffffffff);
        set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::tfactor);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::tfactor);
        set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
        set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        return;
    }

    z_write = (uint8_t)((group->flags & _group_flag_10_bit) == 0 && (water->water_flags & k_water_draw_before_fog) == 0);
    if (halo::rasterizer::fields::water_ripple_update_pending != 0) {
        rasterizer_water_update_ripple_texture(water);
        halo::rasterizer::fields::water_ripple_update_pending = 0;
    }

    effect = rasterizer_effects[102].effect;
    if (effect != 0) {
        float constants[8];

        constants[0] = constants[1] = constants[2] = constants[3] = water->view_perpendicular_brightness;
        constants[4] = constants[5] = constants[6] = constants[7] = water->view_parallel_brightness;
        render_device().set_vertex_declaration(declaration);
        render_device().set_vertex_shader(rasterizer_vertex_shaders[60 + shader_index].shader);
        if (water->water_flags & k_water_base_map_alpha_modulates_reflection) {
            set_render_state(halo::d3d9::rs::cull_mode, 1);
            set_render_state(halo::d3d9::rs::color_write_enable, 8);
            set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
            set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
            set_render_state(halo::d3d9::rs::z_enable, 1);
            set_render_state(halo::d3d9::rs::z_func, 4);
            set_render_state(halo::d3d9::rs::z_write_enable, z_write);
            set_render_state(halo::d3d9::rs::fog_enable, 0);
            chimera__rasterizer_set_texture(halo::tag_id_bits(water->base_map.tag_id), 0, 0, 1, (int16_t)frame);
            set_stage_samplers(0, 3, 0);
            chimera__rasterizer_set_texture_direct_d3d9(halo::tag_id_bits(rasterizer_globals_data->vector_normalization.tag_id), 1, 0);
            set_stage_samplers(1, 3, 1);
            render_device().set_pixel_shader_constant_f(0, constants, 2);
            effect_draw_pass(effect, 0, group);
        }
        if (water->water_flags & k_water_base_map_color_modulates_background) {
            chimera__rasterizer_set_texture(halo::tag_id_bits(water->base_map.tag_id), 0, 0, 1, (int16_t)frame);
            set_stage_samplers(0, 3, 0);
            set_render_state(halo::d3d9::rs::cull_mode, 1);
            set_render_state(halo::d3d9::rs::color_write_enable, 7);
            set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
            set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::zero);
            set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::src_color);
            set_render_state(halo::d3d9::rs::blend_op, 1);
            set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
            set_render_state(halo::d3d9::rs::z_enable, 1);
            set_render_state(halo::d3d9::rs::z_func, 4);
            set_render_state(halo::d3d9::rs::z_write_enable, z_write);
            set_render_state(halo::d3d9::rs::fog_enable, (water->water_flags >> 2) & 1);
            render_device().set_pixel_shader_constant_f(0, constants, 2);
            effect_draw_pass(effect, 1, group);
        }
    }

    effect = rasterizer_effects[103].effect;
    if (effect != 0) {
        float vertex_constants[12];

        set_render_state(halo::d3d9::rs::cull_mode, 1);
        set_render_state(halo::d3d9::rs::color_write_enable, 7);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, (~(group->flags >> 4)) & 1);
        set_render_state(halo::d3d9::rs::src_blend, (water->water_flags & k_water_base_map_alpha_modulates_reflection) ? 7 : 2);
        set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        set_render_state(halo::d3d9::rs::blend_op, 1);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, 1);
        set_render_state(halo::d3d9::rs::z_func, 4);
        set_render_state(halo::d3d9::rs::z_write_enable, z_write);
        set_render_state(halo::d3d9::rs::fog_enable, (water->water_flags >> 2) & 1);
        render_device().set_vertex_declaration(declaration);
        render_device().set_vertex_shader(rasterizer_vertex_shaders[62 + shader_index].shader);
        memset(vertex_constants, 0, sizeof vertex_constants);
        vertex_constants[0] = water->ripple_scale;
        vertex_constants[1] = water->ripple_scale;
        vertex_constants[2] = (float)(halo::libm::cos((double)water->ripple_animation_angle) * water->ripple_animation_velocity * rasterizer_time.time);
        vertex_constants[3] = (float)(halo::libm::sin((double)water->ripple_animation_angle) * water->ripple_animation_velocity * rasterizer_time.time);
        render_device().set_vertex_shader_constant_f(10, vertex_constants, 3);

        if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
            chimera__rasterizer_set_texture_direct_d3d9(halo::tag_id_bits(rasterizer_globals_data->test_0.tag_id), 0, 0);
            set_stage_samplers(0, 1, 0);
            effect_draw_all_passes(effect, group);
            return;
        }
        {
            float pixel_constant[4];
            real_vector3d *normal = (real_vector3d *)&group->tint;

            render_device().set_texture(0, rasterizer_render_targets[8].texture);
            set_stage_samplers(0, 1, 0);
            chimera__rasterizer_set_texture(halo::tag_id_bits(water->reflection_map.tag_id), 3, 2, 0, (int16_t)frame);
            set_stage_samplers(3, 3, 1);
            if (halo::math::vector3d_length(*normal) > 0.0f) {
                float facing = -(rasterizer_camera_forward[1] * normal->j + rasterizer_camera_forward[2] * normal->k +
                    rasterizer_camera_forward[0] * normal->i);
                float t;
                float one_minus_t;

                if (facing < 0.0f) {
                    t = 0.0f;
                } else if (!(facing <= 1.0f)) {
                    t = 1.0f;
                } else {
                    t = facing;
                }
                one_minus_t = 1.0f - t;
                pixel_constant[0] = one_minus_t * water->view_parallel_tint_color.red + t * water->view_perpendicular_tint_color.red;
                pixel_constant[1] = one_minus_t * water->view_parallel_tint_color.green + t * water->view_perpendicular_tint_color.green;
                pixel_constant[2] = t * water->view_perpendicular_tint_color.blue + one_minus_t * water->view_parallel_tint_color.blue;
            } else {
                pixel_constant[0] = 1.0f;
                pixel_constant[1] = 1.0f;
                pixel_constant[2] = 1.0f;
            }
            pixel_constant[3] = 0.0f;
            render_device().set_pixel_shader_constant_f(0, pixel_constant, 1);
            effect_draw_all_passes(effect, group);
        }
    }
}

}  // namespace rasterizer_water_draw_pixel_shader_impl

namespace rasterizer_water_fade_compute_and_set_states_impl {


typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

/**
 * Direct3D 9 back end function rasterizer_water_fade_compute_and_set_states. The original author notes are in
 * docs/original/rasterizer/rasterizer_water_fade_compute_and_set_states.c.txt.
 *
 * @address 0x51eb20
 */
void rasterizer_water_fade_compute_and_set_states(void)
{
    float plane_distance;

    if (halo::rasterizer::fields::rasterizer_debug_mode != 0 || rasterizer_fog_enabled == 0 ||
        rasterizer_caps.pixel_shader_version <= halo::d3d9::k_pixel_shader_version_1_0) {
        return;
    }

    plane_distance = rasterizer_window.fog.plane.normal.i * rasterizer_window.camera.position.x +
                      rasterizer_window.fog.plane.normal.j * rasterizer_window.camera.position.y +
                      rasterizer_window.fog.plane.normal.k * rasterizer_window.camera.position.z -
                      rasterizer_window.fog.plane.d;
    water_fade_plane_distance = plane_distance;

    water_fade_factor_a = plane_distance / rasterizer_window.fog.atmospheric_maximum_distance;
    if (0.0f <= water_fade_factor_a) {
        if (1.0f < water_fade_factor_a) {
            water_fade_factor_a = 1.0f;
        }
    } else {
        water_fade_factor_a = 0.0f;
    }

    water_fade_factor_b = -(plane_distance / rasterizer_window.fog.planar_maximum_depth);
    if (0.0f <= water_fade_factor_b) {
        if (1.0f < water_fade_factor_b) {
            water_fade_factor_b = 1.0f;
        }
    } else {
        water_fade_factor_b = 0.0f;
    }

    if ((rasterizer_window.fog.flags & 2) != 0) {
        water_fade_factor_a = 1.0f;
    }

    if (rasterizer_effects[112].effect != 0) {
        render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
        render_device().set_render_state(halo::d3d9::rs::color_write_enable, 7);
        render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
        render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::alpha_ref, 0);
        render_device().set_render_state(halo::d3d9::rs::z_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::z_func, 3);
        render_device().set_render_state(halo::d3d9::rs::z_write_enable, 0);

        if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
            render_device().set_render_state(halo::d3d9::rs::fog_enable, 1);
            render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
            render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_color);
            return;
        }

        chimera__rasterizer_set_texture_direct_d3dx(halo::tag_id_bits(rasterizer_globals_data->atmospheric_fog_density.tag_id), 0, 0,
                                                    &rasterizer_effects[112]);
        render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);

        chimera__rasterizer_set_texture_direct_d3dx(halo::tag_id_bits(rasterizer_globals_data->planar_fog_density.tag_id), 1, 0,
                                                    &rasterizer_effects[112]);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);

        render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);
        render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
        render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    }
}

}  // namespace rasterizer_water_fade_compute_and_set_states_impl

namespace rasterizer_water_ripple_draw_impl {







/**
 * Direct3D 9 back end function rasterizer_water_ripple_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_water_ripple_draw.c.txt.
 *
 * Registers: EAX -> vertex_buffer
 *
 * @address 0x51ee60
 */
void rasterizer_water_ripple_draw(rasterizer_vertex_buffer *vertex_buffer, const Shader *shader, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count)
{
    void *effect = rasterizer_effects[112].effect;
    const render_fog *fog = &rasterizer_window.fog;
    uint8_t succeeded = 1;
    int16_t permutation;
    float constants[16];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&halo::rasterizer::fields::rasterizer_debug_mode != 0 || rasterizer_fog_enabled == 0 ||
        rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1 || effect == 0) {
        return;
    }

    if (render_device().set_vertex_declaration(rasterizer_vertex_declarations[vertex_buffer->type].declaration) < 0) {
        succeeded = 0;
    }
    permutation = halo::shaders::shader_view(const_cast<Shader *>(shader)).vertex_shader_permutation();
    if (render_device().set_vertex_shader(rasterizer_vertex_shaders[11 + permutation].shader) < 0 ||
        !succeeded) {
        return;
    }

    constants[0] = fog->atmospheric_maximum_density;
    constants[1] = fog->atmospheric_maximum_density;
    constants[2] = fog->atmospheric_maximum_density;
    constants[3] = water_fade_factor_a * fog->atmospheric_maximum_density;
    constants[4] = (1.0f - water_fade_factor_b) * fog->planar_maximum_density;
    constants[5] = constants[4];
    constants[6] = constants[4];
    constants[7] = fog->planar_maximum_density * water_fade_factor_b;
    constants[8] = fog->atmospheric_color.red;
    constants[9] = fog->atmospheric_color.green;
    constants[10] = fog->atmospheric_color.blue;
    constants[11] = water_fade_factor_a;
    constants[12] = fog->planar_color.red;
    constants[13] = fog->planar_color.green;
    constants[14] = fog->planar_color.blue;
    constants[15] = 1.0f - water_fade_factor_a;

    render_device().effect_begin(effect, &pass_count, 3);
    render_device().set_pixel_shader_constant_f(0, constants, 4);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
    }
    render_device().effect_end(effect);
}

}  // namespace rasterizer_water_ripple_draw_impl

namespace rasterizer_water_update_ripple_texture_impl {



typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);



static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_linear_clamped_stage(uint32_t stage)
{
    d3d_call3_fn set_sampler_state = halo::d3d9::device_function<d3d_call3_fn>(rasterizer_device, halo::d3d9::device_method::set_sampler_state);

    set_sampler_state(rasterizer_device, stage, 1, 1);
    set_sampler_state(rasterizer_device, stage, 2, 1);
    set_sampler_state(rasterizer_device, stage, 5, 2);
    set_sampler_state(rasterizer_device, stage, 6, 2);
    set_sampler_state(rasterizer_device, stage, 7, 2);
}

static void bind_ripple_bitmap(uint32_t stage, BitmapData *bitmap)
{
    halo::cache::texture_cache_get(bitmap, 1, 1);
    render_device().set_texture(stage, bitmap->hardware_texture);
    rasterizer_bound_bitmap_size_a[0] = (int16_t)bitmap->width;
    rasterizer_bound_bitmap_size_a[1] = (int16_t)bitmap->height;
}

/**
 * Direct3D 9 back end function rasterizer_water_update_ripple_texture. The original author notes are in
 * docs/original/rasterizer/rasterizer_water_update_ripple_texture.c.txt.
 *
 * @address 0x534f80
 */
void rasterizer_water_update_ripple_texture(void *water_shader)
{
    const ShaderTransparentWater *water = (const ShaderTransparentWater *)water_shader;
    void *effect;

    if (rasterizer_water_enabled == 0) {
        return;
    }
    effect = rasterizer_effects[104].effect;
    if (effect != 0) {
        static const float quad[4][6] = {
            {-1.0078125f, 1.0078125f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.9921875f, 1.0078125f, 0.0f, 0.0f, 1.0f, 0.0f},
            {0.9921875f, -0.9921875f, 0.0f, 0.0f, 1.0f, 1.0f},
            {-1.0078125f, -0.9921875f, 0.0f, 0.0f, 0.0f, 1.0f},
        };
        ShaderTransparentWaterRipple layers[4];
        float pixel_constants[16];
        float vertex_constants[32];
        int16_t ripple_count;
        int16_t pass_index;
        int32_t k;

        memcpy(rasterizer_water_ripple_quad, quad, sizeof quad);
        for (k = 0; k < 4; k++) {
            *(uint32_t *)&rasterizer_water_ripple_quad[k][3] = 0xffffffff;
        }

        set_render_state(halo::d3d9::rs::cull_mode, 3);
        set_render_state(halo::d3d9::rs::color_write_enable, 7);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, 0);
        set_render_state(halo::d3d9::rs::fog_enable, 0);
        set_linear_clamped_stage(0);
        render_device().set_vertex_declaration((uint32_t)rasterizer_vertex_declarations[8].declaration);
        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | rasterizer_vertex_declarations[8].usage) & 0x10);
        render_device().set_vertex_shader(rasterizer_vertex_shaders[0].shader);

        ripple_count = water->ripple_mipmap_levels;
        if (ripple_count > 4) {
            ripple_count = 4;
        }
        memset(pixel_constants, 0, sizeof pixel_constants);
        set_linear_clamped_stage(1);
        set_linear_clamped_stage(2);
        set_linear_clamped_stage(3);

        for (k = 0; k < 4; k++) {
            if (k < (int32_t)water->ripples.count) {
                layers[k] = ((const ShaderTransparentWaterRipple *)(uintptr_t)water->ripples.pointer)[k];
            } else {
                memset(&layers[k], 0, sizeof(layers[k]));
                layers[k].map_repeats = 1;
            }
        }
        if (layers[0].contribution_factor == 0.0f && layers[1].contribution_factor == 0.0f) {
            layers[1].contribution_factor = 1.0f;
        }
        if (layers[2].contribution_factor == 0.0f && layers[3].contribution_factor == 0.0f) {
            layers[3].contribution_factor = 1.0f;
        }

        for (k = 0; k < 4; k++) {
            float frame = (float)(int32_t)(int16_t)layers[k].map_repeats;
            float angle = layers[k].animation_angle;
            float *r = &vertex_constants[k * 8];

            r[0] = frame;
            r[1] = 0.0f;
            r[2] = 0.0f;
            r[3] = (float)(halo::libm::cos((double)angle) * layers[k].animation_velocity * rasterizer_time.time + layers[k].map_offset.i);
            r[4] = 0.0f;
            r[5] = frame;
            r[6] = 0.0f;
            r[7] = (float)(halo::libm::sin((double)angle) * layers[k].animation_velocity * rasterizer_time.time + layers[k].map_offset.j);
        }
        render_device().set_vertex_shader_constant_f(0xd, vertex_constants, 8);

        {
            float sum01 = layers[1].contribution_factor + layers[0].contribution_factor;
            float sum23 = layers[3].contribution_factor + layers[2].contribution_factor;

            pixel_constants[3] = layers[0].contribution_factor / sum01;
            pixel_constants[7] = layers[2].contribution_factor / sum23;
            pixel_constants[11] = sum01 / (layers[1].contribution_factor + layers[0].contribution_factor + sum23);
        }
        rasterizer_set_shader_stage_config(0);

        for (pass_index = 0; pass_index < ripple_count; pass_index++) {
            uint32_t passes = 0;
            uint32_t pass;
            int32_t stage;

            if (water->ripple_mipmap_levels > 1) {
                float fraction = (float)(int32_t)pass_index / (float)(int32_t)(water->ripple_mipmap_levels - 1);
                float alpha = fraction * water->ripple_mipmap_fade_factor;
                uint32_t packed = ((uint32_t)halo::libm::lrint((double)alpha * 255.0) << 24) | 0x8080ff;

                pixel_constants[12] = (float)(int32_t)((packed >> 16) & 0xff) * 0.003921569f;
                pixel_constants[13] = (float)(int32_t)((packed >> 8) & 0xff) * 0.003921569f;
                pixel_constants[14] = (float)(int32_t)(packed & 0xff) * 0.003921569f;
                pixel_constants[15] = fraction * water->ripple_mipmap_fade_factor;
            } else {
                pixel_constants[12] = 0.5019608f;
                pixel_constants[13] = 0.5019608f;
                pixel_constants[14] = 1.0f;
                pixel_constants[15] = 0.0f;
            }

            {
                void *surface = rasterizer_render_targets[8].surface;
                uint32_t desc[8];
                uint32_t viewport[6];

                render_device().set_render_target(0, (uint32_t)surface);
                rasterizer_active_render_target = 8;
                render_device().surface_get_desc(surface, desc);
                viewport[0] = 0;
                viewport[1] = 0;
                viewport[2] = desc[6];
                viewport[3] = desc[7];
                *(float *)&viewport[4] = 0.0f;
                *(float *)&viewport[5] = 1.0f;
                render_device().set_viewport((uint32_t)viewport);
            }

            for (stage = 0; stage < 4; stage++) {
                datum_index ripple_bitmap = (stage < (int32_t)water->ripples.count) ?
                    halo::tag_id_bits(water->ripple_maps.tag_id) : k_datum_index_none;
                uint8_t bound = 0;

                if (halo::rasterizer::fields::bump_mapping_enabled != 0 && ripple_bitmap != k_datum_index_none) {
                    const Bitmap *bitmap_tag = (const Bitmap *)halo::cache::globals().tag_instances[ripple_bitmap & halo::k_slot_mask].data;
                    int32_t bitmap_count = (int32_t)bitmap_tag->bitmap_data.count;

                    if (bitmap_count > 0) {
                        int16_t index = (int16_t)((int16_t)layers[stage].map_index % bitmap_count);
                        BitmapData *bitmap = 0;

                        if (bitmap_tag != 0 && index >= 0 && index < bitmap_count) {
                            bitmap = (BitmapData *)(uintptr_t)bitmap_tag->bitmap_data.pointer + index;
                        }
                        if (bitmap->type == 0) {
                            bind_ripple_bitmap((uint32_t)stage, bitmap);
                            bound = 1;
                        }
                    }
                }
                if (!bound) {
                    datum_index fallback = halo::tag_id_bits(rasterizer_globals_data->default_2d.tag_id);

                    if (fallback != k_datum_index_none) {
                        const Bitmap *bitmap_tag = (const Bitmap *)halo::cache::globals().tag_instances[fallback & halo::k_slot_mask].data;

                        if (bitmap_tag != 0 && (int32_t)bitmap_tag->bitmap_data.count > 3) {
                            bind_ripple_bitmap((uint32_t)stage, (BitmapData *)(uintptr_t)bitmap_tag->bitmap_data.pointer + 3);
                        }
                    }
                }
            }

            render_device().set_pixel_shader_constant_f(0, pixel_constants, 4);
            {

                render_device().effect_begin(effect, &passes, 3);
                for (pass = 0; pass < passes; pass++) {
                    render_device().effect_pass(effect, pass);
                    halo::d3d9::device_function<int32_t (__stdcall *)(void *, uint32_t, uint32_t, const void *, uint32_t)>(rasterizer_device, halo::d3d9::device_method::draw_primitive_up)(
                        rasterizer_device, 6, 2, rasterizer_water_ripple_quad, 0x18);
                }
                render_device().effect_end(effect);
            }
        }
        render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
    }
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    rasterizer_set_shader_stage_config(2);
}

}  // namespace rasterizer_water_update_ripple_texture_impl

}  // namespace halo::rasterizer
