/**
 * @file src/rasterizer/detail_objects.cpp
 * Detail object vertex buffer and drawing.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "internal/state.hpp"
#include "halo/cache/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"




namespace halo::rasterizer {


/**
 * Direct3D 9 back end function rasterizer_detail_object_vertex_buffer_create. The original author notes are in
 * docs/original/rasterizer/rasterizer_detail_object_vertex_buffer_create.c.txt.
 *
 * @address 0x51b370
 */
uint8_t rasterizer_detail_object_vertex_buffer_create(void)
{
    void *buffer = 0;
    uint32_t usage = (rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                     rasterizer_vertex_declarations[_rasterizer_vertex_type_detail_object].usage | 0x200;
    uint32_t pool = (usage & 0x10) != 0 || (usage & 0x200) != 0 ? 2 : 1;
    int32_t hr = render_device().create_vertex_buffer(0x78000, usage, 0, pool, &buffer, 0);

    rasterizer_detail_object_vertex_buffer = hr < 0 ? 0 : buffer;
    return (uint8_t)(rasterizer_detail_object_vertex_buffer != 0);
}

namespace rasterizer_detail_objects_begin_impl {







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

/**
 * Direct3D 9 back end function rasterizer_detail_objects_begin. The original author notes are in
 * docs/original/rasterizer/rasterizer_detail_objects_begin.c.txt.
 *
 * @address 0x51b3f0
 */
void rasterizer_detail_objects_begin(void)
{
    float constants[24];

    if (halo::rasterizer::fields::detail_objects_enabled == 0 || halo::main::render_local_view_count() > 1) {
        return;
    }

    rasterizer_set_render_state(halo::d3d9::rs::cull_mode, 1);
    rasterizer_set_render_state(halo::d3d9::rs::color_write_enable, 7);
    rasterizer_set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    rasterizer_set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    rasterizer_set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    rasterizer_set_render_state(halo::d3d9::rs::blend_op, 1);
    rasterizer_set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    rasterizer_set_render_state(halo::d3d9::rs::z_enable, 1);
    rasterizer_set_render_state(halo::d3d9::rs::z_func, 4);
    rasterizer_set_render_state(halo::d3d9::rs::z_write_enable, 0);
    rasterizer_set_render_state(halo::d3d9::rs::fog_enable, 0);
    rasterizer_set_sampler_state(0, halo::d3d9::ss::address_u, 3);
    rasterizer_set_sampler_state(0, halo::d3d9::ss::address_v, 3);
    rasterizer_set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    rasterizer_set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    rasterizer_set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);

    constants[0] = 255.01f;
    constants[1] = 0.0f;
    constants[2] = 1.0f;
    constants[3] = 1.0f;
    constants[4] = 8.0f;
    constants[5] = 8.0f;
    constants[6] = 8.0f;
    constants[7] = console_debug_value_689430;
    constants[8] = 1.0f;
    constants[9] = 1.0f;
    constants[10] = 0.5f;
    constants[11] = 0.0f;
    constants[12] = 0.0f;
    constants[13] = 1.0f;
    constants[14] = -0.5f;
    constants[15] = 0.0f;
    constants[16] = 0.0f;
    constants[17] = 0.0f;
    constants[18] = -0.5f;
    constants[19] = 1.0f;
    constants[20] = 1.0f;
    constants[21] = 0.0f;
    constants[22] = 0.5f;
    constants[23] = 1.0f;
    render_device().set_vertex_shader_constant_f(0xd, constants, 6);

    render_device().set_stream_source(0, rasterizer_detail_object_vertex_buffer, 0, 0x14);
    render_device().set_pixel_shader(0);
    rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
    rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
    rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, halo::d3d9::ta::diffuse);
    rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
}

}  // namespace rasterizer_detail_objects_begin_impl

namespace rasterizer_detail_objects_draw_impl {






/**
 * Direct3D 9 back end function rasterizer_detail_objects_draw. The original author notes are in
 * docs/original/rasterizer/rasterizer_detail_objects_draw.c.txt.
 *
 * @address 0x51b890
 */
void rasterizer_detail_objects_draw(const rasterizer_detail_object_batches *list)
{
    int16_t batch_index;

    if (halo::rasterizer::fields::detail_objects_enabled == 0 || halo::main::render_local_view_count() > 1) {
        return;
    }

    for (batch_index = 0; batch_index < list->batch_count; batch_index++) {
        const rasterizer_detail_object_batch *batch = &((const rasterizer_detail_object_batch *)list->batches)[batch_index];
        const uint8_t *palette = (const uint8_t *)global_scenario->detail_object_collection_palette.pointer;
        uint32_t collection_tag = *(const uint32_t *)(palette + batch->collection_palette_index * 0x30 + 0xc);
        const DetailObjectCollection *collection = (const DetailObjectCollection *)halo::cache::globals().tag_instances[collection_tag & halo::k_slot_mask].data;
        uint32_t sprite_plate_tag = halo::tag_id_bits(collection->sprite_plate.tag_id);
        const Bitmap *sprite_plate = (const Bitmap *)halo::cache::globals().tag_instances[sprite_plate_tag & halo::k_slot_mask].data;
        float type_constants[16][4];
        float sprite_constants[128][4];
        int32_t type_count;
        int16_t type_index;
        int16_t sprite_count = 0;
        int16_t sequence_index;
        int16_t draw_index;

        chimera__rasterizer_set_texture(sprite_plate_tag, 0, 0, 1, 0);

        type_count = (int32_t)collection->types.count;
        for (type_index = 0; type_index < type_count; type_index++) {
            const DetailObjectCollectionObjectType *type =
                &((const DetailObjectCollectionObjectType *)collection->types.pointer)[type_index];
            const BitmapGroupSequence *sequence =
                &((const BitmapGroupSequence *)sprite_plate->bitmap_group_sequence.pointer)[type->sequence_index];
            const BitmapData *first_bitmap =
                &((const BitmapData *)sprite_plate->bitmap_data.pointer)[(int16_t)sequence->first_bitmap_index];
            float fade_scale = type->far_fade_distance - type->near_fade_distance;

            if (fade_scale > 0.0f) {
                fade_scale = 1.0f / fade_scale;
            }
            type_constants[type_index][0] = fade_scale * type->far_fade_distance;
            type_constants[type_index][1] = -fade_scale;
            type_constants[type_index][2] = (float)(int16_t)first_bitmap->width * type->size;
            type_constants[type_index][3] = (float)(int16_t)first_bitmap->height * type->size;
        }

        for (sequence_index = 0; sequence_index < (int32_t)sprite_plate->bitmap_group_sequence.count; sequence_index++) {
            const BitmapGroupSequence *sequence = &((const BitmapGroupSequence *)sprite_plate->bitmap_group_sequence.pointer)[sequence_index];
            int16_t sprite_index;

            for (sprite_index = 0; sprite_index < (int32_t)sequence->sprites.count; sprite_index++) {
                const BitmapGroupSprite *sprite = &((const BitmapGroupSprite *)sequence->sprites.pointer)[sprite_index];

                sprite_constants[sprite_count][0] = sprite->left;
                sprite_constants[sprite_count][1] = sprite->top;
                sprite_constants[sprite_count][2] = sprite->right - sprite->left;
                sprite_constants[sprite_count][3] = sprite->bottom - sprite->top;
                sprite_count++;
            }
        }

        render_device().set_vertex_shader_constant_f(0x13, &type_constants[0][0], (uint32_t)type_count);
        render_device().set_vertex_shader_constant_f(0x1d, &sprite_constants[0][0], (uint32_t)sprite_count);
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_detail_object].declaration);
        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                rasterizer_vertex_declarations[_rasterizer_vertex_type_detail_object].usage) & 0x10);
        render_device().set_vertex_shader(rasterizer_vertex_shaders[3 + collection->collection_type].shader);

        for (draw_index = 0; draw_index < batch->draw_count; draw_index++) {
            const rasterizer_detail_object_draw *draw = &((const rasterizer_detail_object_draw *)batch->draws)[draw_index];

            render_device().draw_primitive(4, (uint32_t)draw->first_vertex, (uint32_t)(draw->quad_count * 2));
        }
        render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
    }
}

}  // namespace rasterizer_detail_objects_draw_impl

/**
 * Direct3D 9 back end function rasterizer_detail_objects_expand_quad_vertices. The original author notes are
 * in docs/original/rasterizer/rasterizer_detail_objects_expand_quad_vertices.c.txt.
 *
 * Registers: EAX -> quad_count, ECX -> vertices, EDX -> instances, stack -> (collection, draw)
 *
 * @address 0x51b150
 */
void rasterizer_detail_objects_expand_quad_vertices(int32_t quad_count, uint32_t *vertices, const uint8_t *instances, const DetailObjectCollection *collection, const rasterizer_detail_object_draw *draw)
{
    rasterizer_detail_object_vertex *vertex = (rasterizer_detail_object_vertex *)vertices;
    const rasterizer_detail_object_instance *instance = (const rasterizer_detail_object_instance *)instances;

    for (; quad_count > 0; quad_count--, instance++) {
        const float *plane = (const float *)draw->z_reference;
        float fx = (float)instance->x * (1.0f / 255.0f);
        float fy = (float)instance->y * (1.0f / 255.0f);
        float fz = (float)instance->z * (1.0f / 255.0f);
        float height = fz * plane[2] + fy * plane[1] + fx * plane[0] + plane[3];
        uint32_t w = instance->packed_normal;
        uint32_t normal;
        int32_t type_index;
        const DetailObjectCollectionObjectType *type;
        uint32_t sprite;
        uint32_t packed;
        rasterizer_detail_object_vertex corner;
        int32_t i;

        corner.position.x = (float)(draw->cell_x << 3) + fx * 8.0f;
        corner.position.y = (float)(draw->cell_y << 3) + fy * 8.0f;
        corner.position.z = draw->base_z * 8.0f + height * 8.0f;

        normal = ((w & 0xfffff800) | 0xffff0000) << 3;
        normal = (normal | (w & 0x7e0)) << 2;
        normal = (normal | (w & 0xffffe01f)) << 3;
        normal |= (((w >> 1) & 0xe) | (w & 0x600)) >> 1;
        corner.normal = normal;

        type_index = (int16_t)((int32_t)(instance->type_and_sprite >> 4) % (int32_t)collection->types.count);
        type = &((const DetailObjectCollectionObjectType *)collection->types.pointer)[type_index];
        sprite = (uint8_t)((int32_t)(instance->type_and_sprite & 0xf) % (int32_t)type->sprite_count +
                           type->first_sprite_index);
        packed = ((((sprite & 0xff) | 0x100) << 8) | ((uint32_t)type_index & 0xff)) << 8;

        for (i = 0; i < 6; i++) {
            static const uint8_t k_corner[6] = { 0, 1, 2, 0, 2, 3 };

            vertex[i] = corner;
            vertex[i].sprite = packed + k_corner[i];
        }
        vertex += 6;
    }
}

namespace rasterizer_detail_objects_vertex_buffer_fill_impl {



/**
 * Direct3D 9 back end function rasterizer_detail_objects_vertex_buffer_fill. The original author notes are in
 * docs/original/rasterizer/rasterizer_detail_objects_vertex_buffer_fill.c.txt.
 *
 * @address 0x51b6f0
 */
void rasterizer_detail_objects_vertex_buffer_fill(rasterizer_detail_object_batches *list)
{
    Scenario *scenario;
    uint8_t *vertices = 0;
    void *buffer;

    if (halo::rasterizer::fields::detail_objects_enabled == 0 || halo::main::render_local_view_count() > 1) {
        return;
    }

    scenario = global_scenario;
    buffer = rasterizer_detail_object_vertex_buffer;
    if (render_device().buffer_lock(buffer, 0, 0x78000, &vertices, 0) >= 0 && vertices != 0) {
        uint8_t *detail_objects = *(uint32_t *)((uint8_t *)global_structure_bsp + 0x24c) != 0
                                      ? (uint8_t *)*(uint32_t *)((uint8_t *)global_structure_bsp + 0x250) : (uint8_t *)0;
        const uint8_t *instances = (const uint8_t *)*(uint32_t *)(detail_objects + 0x10);
        int32_t vertex_cursor = 0;
        int32_t quads_used = 0;
        uint8_t overflow = 0;
        int16_t batch_index;

        for (batch_index = 0; batch_index < list->batch_count; batch_index++) {
            rasterizer_detail_object_batch *batch = &((rasterizer_detail_object_batch *)list->batches)[batch_index];
            const uint8_t *palette = (const uint8_t *)scenario->detail_object_collection_palette.pointer;
            uint32_t collection_tag = *(const uint32_t *)(palette + batch->collection_palette_index * 0x30 + 0xc);
            const DetailObjectCollection *collection =
                (const DetailObjectCollection *)halo::cache::globals().tag_instances[collection_tag & halo::k_slot_mask].data;
            int16_t draw_index;

            for (draw_index = 0; draw_index < batch->draw_count; draw_index++) {
                rasterizer_detail_object_draw *draw = &((rasterizer_detail_object_draw *)batch->draws)[draw_index];
                int32_t quad_count = draw->quad_count;

                if (quad_count > 0x1000 - quads_used) {
                    quad_count = 0x1000 - quads_used;
                }
                rasterizer_detail_objects_expand_quad_vertices(quad_count, (uint32_t *)(vertices + vertex_cursor * 0x14),
                                                               instances + draw->first_instance * 6, collection, draw);
                draw->first_vertex = vertex_cursor;
                vertex_cursor += draw->quad_count * 6;
                if (draw->quad_count > quad_count) {
                    draw->quad_count = quad_count * 2;
                    if (!overflow) {
                        overflow = 1;
                    }
                }
                quads_used += quad_count;
            }
        }
    }
    render_device().buffer_unlock(rasterizer_detail_object_vertex_buffer);
}

}  // namespace rasterizer_detail_objects_vertex_buffer_fill_impl

}  // namespace halo::rasterizer
