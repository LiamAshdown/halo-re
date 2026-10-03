/**
 * @file src/rasterizer/decals.cpp
 * Decal vertex cache, decal pass state and per-cluster decal drawing.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "halo/render/d3d9.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"




namespace halo::rasterizer {


/**
 * Applies decal-pass depth-bias render states (0xc3/0xaf) from an alternate bias table, for the transparent-
 * geometry-group draw path.
 *
 * @address 0x519530
 */
void chimera__transparent_decal_zbias(void)
{

    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        render_device().set_render_state(halo::d3d9::rs::depth_bias, __builtin_bit_cast(uint32_t, halo::shell::globals().transparent_decal_z_bias));
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        render_device().set_render_state(halo::d3d9::rs::slope_scale_depth_bias, __builtin_bit_cast(uint32_t, halo::shell::globals().transparent_decal_slope_z_bias));
    }
}

/**
 * Clears the lens flare visibility tables and active count, clears the font glyph cache, loads
 * GlobalsRasterizerData from the Globals tag (or NULL if it has none), and resets two cinematic-owned state
 * blocks (including the letterbox height at +0x74 of the first).
 *
 * @address 0x515740
 */
void decal_and_font_system_reset(void)
{
    uint8_t *globals = (uint8_t *)global_globals;
    int32_t i;

    if (*(int32_t *)(globals + 0x134) == 0) {
        rasterizer_globals_data = (GlobalsRasterizerData *)((void *)0);
    } else {
        rasterizer_globals_data = (GlobalsRasterizerData *)(*(void **)(globals + 0x138));
    }

    for (i = 0; i < 0x8c0; i++) {
        ((uint32_t *)lens_flare_object_visibility_table)[i] = 0;
    }
    for (i = 0; i < 0x4002; i++) {
        ((uint32_t *)lens_flare_marker_visibility)[i] = 0;
    }
    lens_flare_instance_count = 0;
    font_glyph_cache_clear_all();

    if (cinematic_screen_effect_state != (cinematic_screen_effect_globals *)0) {
        for (i = 0; i < 0x1e; i++) {
            ((uint32_t *)cinematic_screen_effect_state)[i] = 0;
        }
        ((uint32_t *)cinematic_screen_effect_state)[0x19] = 0x3f800000;
        ((uint32_t *)cinematic_screen_effect_state)[0x1a] = 0x3f800000;
        ((uint32_t *)cinematic_screen_effect_state)[0x1b] = 0x3f800000;
        ((uint32_t *)cinematic_screen_effect_state)[0x1c] = 0x3f800000;
    }
    if (rasterizer_model_ambient_reflection_tint != (float *)0) {
        rasterizer_model_ambient_reflection_tint[0] = 0;
        rasterizer_model_ambient_reflection_tint[1] = 0;
        rasterizer_model_ambient_reflection_tint[2] = 0;
        rasterizer_model_ambient_reflection_tint[3] = 0;
    }
    if (cinematic_screen_effect_state != (cinematic_screen_effect_globals *)0) {
        ((uint32_t *)cinematic_screen_effect_state)[0x1d] = 0;
    }
}

/**
 * Direct3D 9 back end function decal_geometry_cache_restore_procs. The original author notes are in
 * docs/original/rasterizer/decal_geometry_cache_restore_procs.c.txt.
 *
 * @address 0x511ed0
 */
void decal_geometry_cache_restore_procs(void)
{
    *(void **)(rasterizer_decal_vertex_cache_handle + 0x20) = (void *)decal_vertex_cache_release;
    *(void **)(rasterizer_decal_vertex_cache_handle + 0x24) = (void *)decal_vertex_cache_in_use;
}

/**
 * Direct3D 9 back end function decal_vertex_cache_in_use. The original author notes are in
 * docs/original/rasterizer/decal_vertex_cache_in_use.c.txt.
 *
 * @address 0x51a670
 */
uint8_t decal_vertex_cache_in_use(datum_index handle)
{
    uint8_t *element = (uint8_t *)decal_data->data + (uint32_t)(handle & 0xffff) * 0x38;

    decal_vertex_cache_last_queried = handle;
    return (element[2] & 3) != 0;
}

/**
 * 0x44e3c0, blam-cc: EDX -> decal_index
 *
 * @address 0x51a660
 */
void decal_vertex_cache_release(datum_index handle)
{
    halo::effects::decal_delete(handle);
}

/**
 * Applies the decal-pass depth-bias render states (0xc3, 0xaf) from this bias table when the corresponding
 * decal-bias flags (raster_caps bits) are set.
 *
 * @address 0x5194e0
 */
void rasterizer_apply_decal_zbias(void)
{

    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        render_device().set_render_state(halo::d3d9::rs::depth_bias, __builtin_bit_cast(uint32_t, halo::shell::globals().decal_z_bias));
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        render_device().set_render_state(halo::d3d9::rs::slope_scale_depth_bias, __builtin_bit_cast(uint32_t, halo::shell::globals().decal_slope_z_bias));
    }
}

/**
 * Resets the decal depth-bias render states (0xc3, 0xaf) back to zero when the corresponding bias flags are
 * active.
 *
 * @address 0x519580
 */
void rasterizer_clear_decal_zbias(void)
{

    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        render_device().set_render_state(halo::d3d9::rs::depth_bias, 0);
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        render_device().set_render_state(halo::d3d9::rs::slope_scale_depth_bias, 0);
    }
}

namespace rasterizer_decal_index_buffer_initialize_impl {


/**
 * Creates the shared dynamic index buffer (0x30000 bytes of D3DFMT_INDEX16, write-only | dynamic, software
 * processing when enabled, D3DPOOL_SYSTEMMEM) and, while everything succeeds, one vertex buffer slot per
 * vertex type that has a dynamic cache: vertex size * capacity bytes of that type's FVF. Returns whether all
 * of it succeeded.
 *
 * @address 0x51bb90
 */
uint8_t rasterizer_decal_index_buffer_initialize(void)
{
    uint32_t usage = (rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | 0x208;
    uint8_t ok = 1;
    int32_t type;

    if (render_device().create_index_buffer(0x30000, usage, 0x65, 2, &rasterizer_dynamic_index_buffer, 0) < 0) {
        ok = 0;
    }
    if (rasterizer_dynamic_index_buffer == (void *)0 || !ok) {
        ok = 0;
        rasterizer_dynamic_index_buffer = (void *)0;
    }

    for (type = 0; ok && (int16_t)type < k_rasterizer_vertex_type_count; type++) {
        rasterizer_dynamic_vertex_cache *cache = &rasterizer_dynamic_vertex_caches[(int16_t)type];
        int32_t capacity;

        switch (type) {
        case 4: capacity = 0x800; break;
        case 6: case 15: capacity = 0x2000; break;
        case 7: capacity = 2; break;
        case 8: capacity = 0x4000; break;
        default:
            cache->capacity = 0;
            cache->buffer_handle = 0;
            continue;
        }
        cache->buffer_handle = rasterizer_vertex_buffer_slot_allocate(type, rasterizer_vertex_declarations[type].fvf,
                                                                      rasterizer_vertex_sizes[type] * capacity);
        if (cache->buffer_handle == 0) {
            ok = 0;
        }
        cache->capacity = capacity;
    }
    return ok;
}

}  // namespace rasterizer_decal_index_buffer_initialize_impl

namespace rasterizer_decal_pass_begin_impl {


/**
 * Direct3D 9 back end function rasterizer_decal_pass_begin. The original author notes are in
 * docs/original/rasterizer/rasterizer_decal_pass_begin.c.txt.
 *
 * Registers: unaff_DI -> stage
 *
 * @address 0x51a810
 */
void rasterizer_decal_pass_begin(int16_t stage)
{
    uint8_t proceed = 1;

    if (decals_for_all_responses == 0 && stage != 3) {
        proceed = 0;
    }
    if (*(int16_t *)&halo::rasterizer::fields::rasterizer_debug_mode != 0 || !proceed) {
        rasterizer_decal_layer = stage;
        return;
    }

    rasterizer_decal_blend_mode = 0xffff;
    rasterizer_decal_bitmap_frame = 0xffff;
    rasterizer_decal_bitmap_tag = 0xffffffff;
    halo::rasterizer::fields::decal_fog_state_applied = 0;
    rasterizer_decal_layer = stage;

    {
        TagID *fallback_tag_id = (TagID *)((uint8_t *)rasterizer_globals_data + 0xb8);
        if (*(uint32_t *)fallback_tag_id != 0xffffffff) {
            Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[fallback_tag_id->index].data;
            if (bitmap != (Bitmap *)0 && bitmap->bitmap_data.count > 1) {
                uint8_t *first_submap = (uint8_t *)bitmap->bitmap_data.pointer;
                if (first_submap != (uint8_t *)(uint32_t)-0x30) {
                    rasterizer_bind_texture_d3d9(0, (BitmapData *)(first_submap + 0x30));
                    rasterizer_bound_bitmap_size_a[0] = *(int16_t *)(first_submap + 0x34);
                    rasterizer_bound_bitmap_size_a[1] = *(int16_t *)(first_submap + 0x36);
                }
            }
        }
    }

    {
        render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    }
    {
        render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
        render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::z_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::z_write_enable, 0);
        render_device().set_render_state(halo::d3d9::rs::z_func, 4);
        render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);
    }

    rasterizer_apply_decal_zbias();

    if (stage == 3) {
        render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
        rasterizer_set_shader_stage_config(4);
    } else {
        if ((console_debug_toggle_689441 == 0 || rasterizer_window.fog.atmospheric_maximum_density != 1.0f) && halo::rasterizer::fields::decal_fog_state_applied == 0) {
            render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
            goto stream_source;
        }
        halo::rasterizer::fields::decal_fog_state_applied = 1;
        render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
        render_device().set_render_state(halo::d3d9::rs::alpha_ref, 0);
    }

stream_source:
    render_device().set_stream_source(0, rasterizer_decal_vertex_cache, 0, 0x10);
}

}  // namespace rasterizer_decal_pass_begin_impl

namespace rasterizer_decal_vertex_cache_lock_impl {


/**
 * Direct3D 9 back end function rasterizer_decal_vertex_cache_lock. The original author notes are in
 * docs/original/rasterizer/rasterizer_decal_vertex_cache_lock.c.txt.
 *
 * Registers: EAX -> decal_index, stack -> byte_count
 *
 * @address 0x51a770
 */
void * rasterizer_decal_vertex_cache_lock(uint32_t decal_index, int32_t byte_count)
{
    uint8_t *cache = rasterizer_decal_vertex_cache_handle;
    data_array *blocks = *(data_array **)(cache + 0x3c);
    uint32_t offset = *(uint32_t *)((uint8_t *)blocks->data + (decal_index & 0xffff) * 0x1c + 8)
                      << (*(uint32_t *)(cache + 0x2c) & 0x1f);
    void *buffer = rasterizer_decal_vertex_cache;
    void *data = 0;
    uint8_t succeeded = 1;
    int32_t size;

    rasterizer_vertex_buffer_lock_state = 5;
    size = (int32_t)(long long)((double)byte_count * 1.5);
    if (render_device().buffer_lock(buffer, (uint32_t)(int32_t)(long long)((double)offset * 1.5), (uint32_t)size, &data, 0) < 0) {
        succeeded = 0;
    }
    rasterizer_vertex_buffer_lock_state = 4;
    return succeeded ? data : 0;
}

}  // namespace rasterizer_decal_vertex_cache_lock_impl

/**
 * Returns whether either decal depth-bias flag is currently set in raster_caps.
 *
 * @address 0x5195d0
 */
int rasterizer_decal_zbias_active(void)
{
    if ((rasterizer_caps.raster_caps & 0x6000000) == 0) {
        return 0;
    }
    return 1;
}

namespace rasterizer_decals_draw_cluster_impl {


static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void rasterizer_set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Direct3D 9 back end function rasterizer_decals_draw_cluster. The original author notes are in
 * docs/original/rasterizer/rasterizer_decals_draw_cluster.c.txt.
 *
 * @address 0x51aa50
 */
void rasterizer_decals_draw_cluster(int16_t cluster_index)
{
    uint8_t succeeded = 1;
    uint8_t layer_enabled = 1;
    uint32_t decal_index;

    if (decals_for_all_responses == 0 && rasterizer_decal_layer != 3) {
        layer_enabled = 0;
    }
    if (*(uint16_t *)&halo::rasterizer::fields::rasterizer_debug_mode != 0 || !layer_enabled) {
        return;
    }

    decal_index = decal_grid_block[rasterizer_decal_layer * 0x200 + cluster_index];
    while (decal_index != 0xffffffff) {
        uint8_t *decal = (uint8_t *)decal_data->data + (decal_index & 0xffff) * 0x38;
        uint32_t definition_tag = *(uint32_t *)&((struct decal *)decal)->definition_index;
        uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[definition_tag & 0xffff].data + 0xbc;
        int16_t type = *(int16_t *)(definition + 4);

        if (rasterizer_decal_blend_mode != type) {
            rasterizer_decal_blend_mode = type;
            if (type == 1 || type == 2) {
                rasterizer_set_render_state(halo::d3d9::rs::color_write_enable, 0xf);
            } else {
                rasterizer_set_render_state(halo::d3d9::rs::color_write_enable, 7);
            }
            chimera__rasterizer_set_framebuffer_blend_function(rasterizer_decal_blend_mode);
        }

        if (succeeded) {
            uint8_t *cache = rasterizer_decal_vertex_cache_handle;
            data_array *blocks = *(data_array **)(cache + 0x3c);
            uint32_t first_offset = *(uint32_t *)((uint8_t *)blocks->data + (decal_index & 0xffff) * 0x1c + 8)
                                    << (*(uint32_t *)(cache + 0x2c) & 0x1f);
            uint32_t color = ((struct decal *)decal)->color;
            uint32_t alpha = (((struct decal *)decal)->alpha * (color >> 24) + 0x7f) >> 8;
            int32_t primitive_count;
            int32_t first_vertex;
            int8_t frame;
            float constants[4];

            switch (rasterizer_decal_blend_mode) {
            case 0:
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, 4);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg2, 0);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, 4);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, 0x10);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, 1);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, 1);
                break;
            case 1:
            case 5:
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, 4);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg2, 0);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, 2);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, 1);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, 1);
                break;
            case 2:
                rasterizer_set_render_state(halo::d3d9::rs::texture_factor, 0x7f7f7f7f);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, 0x19);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg2, 1);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg0, 0x10);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, 3);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, 1);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, 1);
                break;
            case 3:
            case 4:
            case 6:
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, 4);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg2, 0);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, 1);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, 4);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_arg1, 1);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_arg2, 0x30);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, 2);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, 1);
                rasterizer_set_texture_stage_state(2, halo::d3d9::ts::alpha_op, 1);
                rasterizer_set_texture_stage_state(2, halo::d3d9::ts::color_op, 1);
                break;
            case 7:
                rasterizer_set_render_state(halo::d3d9::rs::texture_factor, 0xffff0000);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, 3);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, 2);
                rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, 3);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, 1);
                rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, 1);
                break;
            default:
                break;
            }

            primitive_count = ((struct decal *)decal)->triangle_count * 2;
            first_vertex = (int32_t)(long long)((double)(first_offset >> 4) * 1.5);

            frame = *(int8_t *)&((struct decal *)decal)->sprite_bitmap_index;
            if (rasterizer_decal_bitmap_tag != *(uint32_t *)(definition + 0x28) ||
                rasterizer_decal_bitmap_frame != (int16_t)frame) {
                rasterizer_decal_bitmap_tag = *(uint32_t *)(definition + 0x28);
                rasterizer_decal_bitmap_frame = (int16_t)frame;
                chimera__rasterizer_set_texture(rasterizer_decal_bitmap_tag, 0, 0, 1, rasterizer_decal_bitmap_frame);
            }

            constants[0] = (float)((double)((color >> 16) & 0xff) * (1.0 / 255.0));
            constants[1] = (float)((double)((color >> 8) & 0xff) * (1.0 / 255.0));
            constants[2] = (float)((double)(color & 0xff) * (1.0 / 255.0));
            constants[3] = (float)((double)(uint32_t)(0xff - alpha) * (1.0 / 255.0));
            if (render_device().set_vertex_shader_constant_f(10, constants, 1) < 0) {
                succeeded = 0;
            }
            if (render_device().set_vertex_declaration((void *)rasterizer_vertex_declarations[_rasterizer_vertex_type_decal].declaration) < 0) {
                succeeded = 0;
            }
            if (render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                        rasterizer_vertex_declarations[_rasterizer_vertex_type_decal].usage) & 0x10) < 0) {
                succeeded = 0;
            }
            if (render_device().set_vertex_shader((void *)rasterizer_vertex_shaders[2].shader) < 0) {
                succeeded = 0;
            }
            if (render_device().set_pixel_shader(0) < 0) {
                succeeded = 0;
            }
            if (render_device().draw_primitive(4, (uint32_t)first_vertex, (uint32_t)primitive_count) < 0) {
                succeeded = 0;
            }
            if (render_device().set_software_vertex_processing(rasterizer_software_vertex_processing) < 0) {
                succeeded = 0;
            }
        }
        decal_index = *(uint32_t *)&((struct decal *)decal)->next_decal;
    }
}

}  // namespace rasterizer_decals_draw_cluster_impl

namespace rasterizer_decals_initialize_impl {


/**
 * 0x4d1750, blam-cc: EBX -> name, stack -> the rest
 *
 * @address 0x51a6a0
 */
void rasterizer_decals_initialize(void)
{
    void *buffer = 0;
    uint32_t usage = (rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                     rasterizer_vertex_declarations[_rasterizer_vertex_type_decal].usage | 0x200;
    uint32_t pool = (usage & 0x10) != 0 || (usage & 0x200) != 0 ? 2 : 1;
    uint32_t region_size = 0xe07c;
    uint8_t *block;
    int32_t hr = render_device().create_vertex_buffer(0x3c000, usage, 0, pool, &buffer, 0);

    rasterizer_decal_vertex_cache = hr < 0 ? 0 : buffer;

    block = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + 0xe07c;
    halo::memory::crc32_update(&game_state_crc, &region_size, 4);

    halo::memory::cache_new((char *)"decal vertex cache", (::cache *)block, 0xa00, 6, 0x800, (void *)decal_vertex_cache_release,
              (void *)decal_vertex_cache_in_use);
    rasterizer_decal_vertex_cache_handle = block;
}

}  // namespace rasterizer_decals_initialize_impl

/**
 * Cleans up render state after a decal pass: clears the clip-plane enable state and, where active, the depth-
 * bias states, then restores the shader stage configuration if the pass tracker reads 3.
 *
 * @address 0x51b0e0
 */
void rasterizer_end_decal_pass(void)
{

    render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);
    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        render_device().set_render_state(halo::d3d9::rs::depth_bias, 0);
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        render_device().set_render_state(halo::d3d9::rs::slope_scale_depth_bias, 0);
    }
    if (rasterizer_decal_layer == 3) {
        rasterizer_set_shader_stage_config(2);
    }
}

namespace rasterizer_shader_decal_pass_set_states_impl {


/**
 * Direct3D 9 back end function rasterizer_shader_decal_pass_set_states. The original author notes are in
 * docs/original/rasterizer/rasterizer_shader_decal_pass_set_states.c.txt.
 *
 * @address 0x520020
 */
void rasterizer_shader_decal_pass_set_states(void)
{

    if (halo::rasterizer::fields::rasterizer_debug_mode != 0 || halo::rasterizer::fields::specular_enabled == 0 ||
        rasterizer_caps.pixel_shader_version <= halo::d3d9::k_pixel_shader_version_1_0) {
        return;
    }

    render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
    render_device().set_render_state(halo::d3d9::rs::color_write_enable, 7);
    render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::dest_alpha);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    render_device().set_render_state(halo::d3d9::rs::z_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::z_func, 3);
    render_device().set_render_state(halo::d3d9::rs::z_write_enable, 0);
    render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);

    render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 1);
    render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 1);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, 1);
    render_device().set_sampler_state(2, halo::d3d9::ss::mip_filter, 1);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::mip_filter, 2);
}

}  // namespace rasterizer_shader_decal_pass_set_states_impl

}  // namespace halo::rasterizer
