/**
 * @file src/rasterizer/dynamic_geometry.cpp
 * Dynamic vertex and index caches and the indexed draw paths.
 */

#include "halo/render/d3d9.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/rasterizer/constants.hpp"
#include "halo/render/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"




namespace halo::rasterizer {

/**
 * Releases rasterizer_misc_vertex_buffer's COM object and frees the transparent-geometry-group pools and their
 * sort-index buffer, resetting all counts to zero.
 *
 * @address 0x515430
 */
void chimera__rasterizer_dispose_free_memory(void)
{
    if (rasterizer_misc_vertex_buffer != nullptr) {
        render_device().release(rasterizer_misc_vertex_buffer);
        rasterizer_misc_vertex_buffer = nullptr;
    }
    if (transparent_geometry_groups != (transparent_geometry_group *)0) {
        GlobalFree(transparent_geometry_groups);
    }
    transparent_geometry_groups = (transparent_geometry_group *)0;
    if (transparent_geometry_group_sorted_indices != nullptr) {
        GlobalFree(transparent_geometry_group_sorted_indices);
    }
    transparent_geometry_group_sorted_indices = nullptr;
    if (transparent_geometry_groups_secondary != (transparent_geometry_group *)0) {
        GlobalFree(transparent_geometry_groups_secondary);
    }
    transparent_geometry_groups_secondary = (transparent_geometry_group *)0;
    transparent_geometry_group_secondary_count = 0;
    transparent_geometry_group_count = 0;
}


/**
 * Direct3D 9 back end function chimera__rasterizer_draw_dynamic_triangles_static_vertices.
 *
 * Registers: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
 *
 * @address 0x51c1c0
 */
void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive)
{
    while (primitive_count > 0) {
        rasterizer_dynamic_index_slot *slot;
        uint32_t stride;
        int32_t chunk;
        void *hardware_buffer;
        uint32_t vertex_desc[6];
        uint32_t index_desc[5];

        if (dynamic_index_slot == -1 || vertex_buffer == 0 || vertex_buffer->hardware_buffer == 0) {
            break;
        }
        slot = &rasterizer_dynamic_index_slots[dynamic_index_slot];
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[vertex_buffer->type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        hardware_buffer = vertex_buffer->hardware_buffer;
        render_device().buffer_get_desc(hardware_buffer, vertex_desc);
        render_device().buffer_get_desc(rasterizer_dynamic_index_buffer, index_desc);

        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                    rasterizer_vertex_declarations[vertex_buffer->type].usage) & 0x10);
        render_device().set_stream_source(0, hardware_buffer, 0, stride);
        render_device().set_indices(rasterizer_dynamic_index_buffer);
        {
            int32_t debug_hr = render_device().draw_indexed_primitive(4, 0, 0, (uint32_t)vertex_buffer->count, (uint32_t)((slot->first_index + first_primitive) * 3), (uint32_t)chunk);
            halo::interface::debug_fp_draw_state_note("st1", debug_hr, 0, (uint32_t)vertex_buffer->count, (uint32_t)chunk);
        }
        first_primitive += chunk;
        primitive_count -= chunk;
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

namespace chimera__rasterizer_draw_dynamic_triangles_static_vertices2_impl {


/**
 * Direct3D 9 back end function chimera__rasterizer_draw_dynamic_triangles_static_vertices2.
 *
 * Registers: EAX -> primitive_count, EDI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive,
 * second_stream)
 *
 * @address 0x51c310
 */
void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive, rasterizer_vertex_buffer *second_stream)
{
    while (primitive_count > 0) {
        rasterizer_dynamic_index_slot *slot;
        uint32_t stride;
        uint32_t second_stride;
        int32_t chunk;

        if (dynamic_index_slot == -1 || vertex_buffer == 0 || vertex_buffer->hardware_buffer == 0 ||
            second_stream == 0 || second_stream->hardware_buffer == 0) {
            break;
        }
        slot = &rasterizer_dynamic_index_slots[dynamic_index_slot];
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[vertex_buffer->type];
        second_stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[second_stream->type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                    rasterizer_vertex_declarations[vertex_buffer->type].usage) & 0x10);
        render_device().set_stream_source(0, vertex_buffer->hardware_buffer, 0, stride);
        if (halo::shell::globals().safe_mode == 0 && rasterizer_caps.max_streams > 1) {
            render_device().set_stream_source(1, second_stream->hardware_buffer, 0, second_stride);
        }
        render_device().set_indices(rasterizer_dynamic_index_buffer);
        {
            int32_t debug_hr = render_device().draw_indexed_primitive(4, 0, 0, (uint32_t)vertex_buffer->count, (uint32_t)((slot->first_index + first_primitive) * 3), (uint32_t)chunk);
            halo::interface::debug_fp_draw_state_note("st2", debug_hr, 0, (uint32_t)vertex_buffer->count, (uint32_t)chunk);
        }
        first_primitive += chunk;
        primitive_count -= chunk;
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace chimera__rasterizer_draw_dynamic_triangles_static_vertices2_impl

/**
 * Carves a `size` byte block from the fixed scratch memory pool, or returns NULL if the pool would overflow.
 * When `source` is non-NULL, copies `size` bytes from it into the new block.
 *
 * Registers: EAX -> source, ECX -> size
 *
 * @address 0x514560
 */
void * chimera__rasterizer_memory_alloc(void *source, uint32_t size)
{
    uint32_t new_used;
    void *block;

    new_used = rasterizer_scratch_memory_used + size;
    block = nullptr;
    if (new_used <= k_scratch_memory_bytes) {
        block = static_cast<uint8_t *>(rasterizer_scratch_memory) + rasterizer_scratch_memory_used;
        rasterizer_scratch_memory_used = new_used;
        if (source != nullptr) {
            memcpy(block, source, size);
        }
    }
    return block;
}

namespace rasterizer_dynamic_geometry_chain_draw_impl {


/**
 * Direct3D 9 back end function rasterizer_dynamic_geometry_chain_draw.
 *
 * Registers: EAX -> vertex_buffer, EDI -> index_buffer, stack -> primitive_count
 *
 * @address 0x51c5f0
 */
void rasterizer_dynamic_geometry_chain_draw(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, rasterizer_index_buffer *index_buffer)
{
    uint32_t start_index = 0;

    while (primitive_count > 0) {
        uint32_t stride;
        int32_t chunk;

        if (index_buffer == 0 || index_buffer->hardware_buffer == 0 || vertex_buffer == 0 || vertex_buffer->hardware_buffer == 0) {
            break;
        }
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[vertex_buffer->type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                       rasterizer_vertex_declarations[vertex_buffer->type].usage) & 0x10);
        render_device().set_stream_source(0, vertex_buffer->hardware_buffer, 0, stride);
        render_device().set_indices(index_buffer->hardware_buffer);
        {
            halo::interface::debug_fp_pre_draw();
            int32_t debug_hr = render_device().draw_indexed_primitive(rasterizer_triangle_buffer_primitive_types[index_buffer->type], 0, 0, (uint32_t)vertex_buffer->count, start_index, (uint32_t)chunk);
            halo::interface::debug_fp_draw_state_note("chn", debug_hr, 0, (uint32_t)vertex_buffer->count, (uint32_t)chunk);
        }
        primitive_count -= chunk;
        switch (index_buffer->type) {
        case 0:
            start_index += (uint32_t)chunk * 3;
            break;
        case 1:
            start_index += (uint32_t)chunk;
            break;
        default:
            break;
        }
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_dynamic_geometry_chain_draw_impl

/**
 * Releases every dynamic vertex buffer currently checked out by the 20 per vertex type dynamic caches,
 * clearing their vertex buffer slots and the associated bookkeeping counters, then releases the shared dynamic
 * index buffer. Only runs while the device is alive.
 *
 * @address 0x51bcd0
 */
void rasterizer_dynamic_geometry_dispose(void)
{
    int32_t type_index;
    int32_t handle;
    rasterizer_vertex_buffer_slot *slot;
    void *object;

    if (rasterizer_device != 0) {
        for (type_index = 0; type_index < k_rasterizer_vertex_type_count; type_index++) {
            handle = rasterizer_dynamic_vertex_caches[type_index].buffer_handle;
            if (handle != 0) {
                slot = &rasterizer_vertex_buffer_slots[handle - 1];
                object = slot->hardware_buffer;
                if (object != 0) {
                    render_device().release(object);
                }
                rasterizer_vertex_buffer_slot_count = rasterizer_vertex_buffer_slot_count - 1;
                slot->hardware_buffer = 0;
                slot->vertex_type = 0;
                slot->length = 0;
                slot->fvf = 0;
                if (rasterizer_vertex_buffer_slot_count == 0) {
                    rasterizer_vertex_buffer_slot_high_water = 0;
                }
                rasterizer_dynamic_vertex_caches[type_index].buffer_handle = 0;
            }
        }
        if (rasterizer_dynamic_index_buffer != 0) {
            object = rasterizer_dynamic_index_buffer;
            render_device().release(object);
            rasterizer_dynamic_index_buffer = 0;
        }
    }
}

/**
 * ECX -> first_primitive, EBX -> dynamic_vertex_slot
 *
 * Registers: stack -> (index_buffer, dynamic_index_slot, vertex_buffer), EAX -> primitive_count,
 *
 * @address 0x51c730
 */
void rasterizer_dynamic_geometry_draw_dispatch(rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, rasterizer_vertex_buffer *vertex_buffer, int32_t primitive_count, int32_t first_primitive, int32_t dynamic_vertex_slot)
{
    if (index_buffer != 0) {
        if (vertex_buffer != 0) {
            rasterizer_dynamic_geometry_chain_draw(primitive_count, vertex_buffer, index_buffer);
        } else {
            rasterizer_dynamic_vertex_draw_indexed(index_buffer, primitive_count, dynamic_vertex_slot);
        }
    } else if (vertex_buffer != 0) {
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, (rasterizer_vertex_buffer *)vertex_buffer, dynamic_index_slot, first_primitive);
    } else {
        rasterizer_dynamic_index_cache_draw(dynamic_index_slot, first_primitive, primitive_count, dynamic_vertex_slot);
    }
}

namespace rasterizer_dynamic_index_cache_draw_impl {


/**
 * Direct3D 9 back end function rasterizer_dynamic_index_cache_draw.
 *
 * @address 0x51c090
 */
void rasterizer_dynamic_index_cache_draw(int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, int32_t dynamic_vertex_slot)
{
    while (primitive_count > 0) {
        rasterizer_dynamic_vertex_slot *vertex_slot;
        rasterizer_dynamic_index_slot *index_slot;
        int16_t type;
        uint32_t stride;
        int32_t chunk;
        int32_t handle;
        void *buffer;

        if (dynamic_index_slot == -1 || dynamic_vertex_slot == -1) {
            break;
        }
        vertex_slot = &rasterizer_dynamic_vertex_slots[dynamic_vertex_slot];
        index_slot = &rasterizer_dynamic_index_slots[dynamic_index_slot];
        type = vertex_slot->vertex_type;
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                       rasterizer_vertex_declarations[type].usage) & 0x10);
        handle = rasterizer_dynamic_vertex_caches[type].buffer_handle;
        buffer = handle == 0 ? 0 : rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer;
        render_device().set_stream_source(0, buffer, 0, stride);
        render_device().set_indices(rasterizer_dynamic_index_buffer);
        {
            int32_t debug_hr = render_device().draw_indexed_primitive(4, vertex_slot->first_vertex, 0, (uint32_t)vertex_slot->vertex_count, (uint32_t)((index_slot->first_index + first_primitive) * 3), (uint32_t)chunk);
            halo::interface::debug_fp_draw_state_note("idc", debug_hr, 0, (uint32_t)vertex_slot->vertex_count, (uint32_t)chunk);
        }
        first_primitive += chunk;
        primitive_count -= chunk;
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_dynamic_index_cache_draw_impl

/**
 * Reserves `count` indices out of the shared per frame dynamic index budget and records the reservation as a
 * new entry in the dynamic index slot table. Returns the new slot's index, or -1 if count is not positive or
 * the reservation would exceed the per frame index budget or the slot table (in which case the sticky overflow
 * flag is set once).
 *
 * Registers: EDX = count
 *
 * @address 0x51bd60
 */
int32_t rasterizer_dynamic_index_cache_reserve(int32_t count)
{
    int32_t slot_index;

    slot_index = rasterizer_dynamic_index_slot_count;
    if (0 < count) {
        if (rasterizer_dynamic_index_count < k_rasterizer_dynamic_index_budget - count &&
            rasterizer_dynamic_index_slot_count < k_rasterizer_dynamic_vertex_slots - 1) {
            rasterizer_dynamic_index_slots[rasterizer_dynamic_index_slot_count].first_index =
                rasterizer_dynamic_index_count;
            rasterizer_dynamic_index_slots[slot_index].index_count = count;
            rasterizer_dynamic_index_count = rasterizer_dynamic_index_count + count;
            rasterizer_dynamic_index_slot_count = rasterizer_dynamic_index_slot_count + 1;
            return slot_index;
        }
        if (rasterizer_dynamic_index_overflow == 0) {
            rasterizer_dynamic_index_overflow = 1;
        }
    }
    return -1;
}

namespace rasterizer_dynamic_light_technique_ps2_set_states_impl {


/**
 * Direct3D 9 back end function rasterizer_dynamic_light_technique_ps2_set_states.
 *
 * @address 0x521cc0
 */
void rasterizer_dynamic_light_technique_ps2_set_states(void)
{

    if (halo::rasterizer::fields::rasterizer_debug_mode != 0 || halo::rasterizer::fields::specular_lightmap_enabled == 0 ||
        render_force_flag != 0 || rasterizer_caps.pixel_shader_version <= halo::d3d9::k_pixel_shader_version_1_3) {
        return;
    }

    render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
    render_device().set_render_state(halo::d3d9::rs::color_write_enable, 7);
    render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::dest_alpha);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::alpha_ref, 0);
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
    render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(2, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_u, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_v, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_w, 3);
    render_device().set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::mip_filter, 2);
}

}  // namespace rasterizer_dynamic_light_technique_ps2_set_states_impl

namespace rasterizer_dynamic_vertex_cache_lock_impl {


/**
 * Locks the vertex buffer range covered by dynamic vertex slot `slot_index` (see
 * rasterizer_dynamic_vertex_cache_reserve) and stores the resulting pointer (or NULL on failure) into the
 * slot's locked_vertices field, which is also the return value.
 *
 * Registers: EAX = slot_index
 *
 * @address 0x51be40
 */
void * rasterizer_dynamic_vertex_cache_lock(int32_t slot_index)
{
    rasterizer_dynamic_vertex_slot *slot;
    rasterizer_dynamic_vertex_cache *cache;
    rasterizer_vertex_buffer_slot *buffer_slot;
    void *locked_data;
    int32_t hresult;
    int32_t stride;

    if (slot_index == -1) {
        return 0;
    }

    slot = &rasterizer_dynamic_vertex_slots[slot_index];
    cache = &rasterizer_dynamic_vertex_caches[slot->vertex_type];
    buffer_slot = &rasterizer_vertex_buffer_slots[cache->buffer_handle - 1];
    stride = rasterizer_vertex_sizes[slot->vertex_type];

    locked_data = 0;
    hresult = render_device().buffer_lock(buffer_slot->hardware_buffer, slot->first_vertex * stride, slot->vertex_count * stride, &locked_data, halo::d3d9::k_lock_discard);

    slot->locked_vertices = hresult < 0 ? NULL : locked_data;
    return slot->locked_vertices;
}

}  // namespace rasterizer_dynamic_vertex_cache_lock_impl

/**
 * Reserves `count` vertices out of the per vertex type dynamic cache `vertex_type` and records the reservation
 * as a new entry in the dynamic vertex slot table. Returns the new slot's index, or -1 if count is not
 * positive or the reservation would exceed the cache's remaining capacity or the slot table (in which case the
 * sticky overflow flag is set once).
 *
 * Registers: AX = vertex_type, ESI = count
 *
 * @address 0x51bdd0
 */
int32_t rasterizer_dynamic_vertex_cache_reserve(int16_t vertex_type, int32_t count)
{
    rasterizer_dynamic_vertex_cache *cache;
    int32_t slot_index;

    if (0 < count) {
        cache = &rasterizer_dynamic_vertex_caches[vertex_type];
        if (cache->used < cache->capacity - count && rasterizer_dynamic_vertex_slot_count < k_rasterizer_dynamic_vertex_slots - 1) {
            slot_index = rasterizer_dynamic_vertex_slot_count;
            rasterizer_dynamic_vertex_slots[slot_index].vertex_type = vertex_type;
            rasterizer_dynamic_vertex_slots[slot_index].first_vertex = cache->used;
            rasterizer_dynamic_vertex_slots[slot_index].vertex_count = count;
            cache->used = cache->used + count;
            rasterizer_dynamic_vertex_slot_count = rasterizer_dynamic_vertex_slot_count + 1;
            return slot_index;
        }
        if (rasterizer_dynamic_vertex_overflow == 0) {
            rasterizer_dynamic_vertex_overflow = 1;
        }
    }
    return -1;
}

namespace rasterizer_dynamic_vertex_draw_impl {


/**
 * Direct3D 9 back end function rasterizer_dynamic_vertex_draw.
 *
 * @address 0x51bec0
 */
void rasterizer_dynamic_vertex_draw(int32_t first_primitive, int32_t primitive_count, int32_t dynamic_vertex_slot, int16_t primitive_kind)
{
    while (primitive_count > 0 && dynamic_vertex_slot != -1) {
        rasterizer_dynamic_vertex_slot *vertex_slot;
        uint32_t primitive_type;
        int16_t type;
        uint32_t stride;
        int32_t chunk;
        int32_t handle;
        void *buffer;

        switch (primitive_kind) {
        case 2:
            primitive_type = 2;
            break;
        case 3:
            primitive_type = 4;
            break;
        case 4: {

            int32_t triangle_count = primitive_count * 2;
            int32_t index_slot = rasterizer_dynamic_index_cache_reserve(triangle_count);
            uint16_t *indices;
            int16_t triangle;

            if (index_slot == -1) {
                return;
            }
            indices = static_cast<uint16_t *>(halo::render::rasterizer_dynamic_index_slot_lock(index_slot));
            for (triangle = 0; triangle < triangle_count; triangle += 2) {
                uint16_t base = (uint16_t)((triangle / 2) * 4);
                uint16_t *quad = indices + triangle * 3;

                quad[0] = base;
                quad[1] = (uint16_t)(base + 1);
                quad[2] = (uint16_t)(base + 2);
                quad[3] = base;
                quad[4] = (uint16_t)(base + 2);
                quad[5] = (uint16_t)(base + 3);
            }
            render_device().buffer_unlock(rasterizer_dynamic_index_buffer);
            rasterizer_dynamic_index_cache_draw(index_slot, 0, triangle_count, dynamic_vertex_slot);
            return;
        }
        default:
            primitive_count = primitive_kind - 2;
            primitive_type = 5;
            break;
        }

        vertex_slot = &rasterizer_dynamic_vertex_slots[dynamic_vertex_slot];
        type = vertex_slot->vertex_type;
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                       rasterizer_vertex_declarations[type].usage) & 0x10);
        handle = rasterizer_dynamic_vertex_caches[type].buffer_handle;
        buffer = handle == 0 ? 0 : rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer;
        render_device().set_stream_source(0, buffer, 0, stride);
        {
            int32_t debug_hr = render_device().draw_primitive(primitive_type, (uint32_t)(primitive_kind * first_primitive + vertex_slot->first_vertex), (uint32_t)chunk);
            halo::interface::debug_fp_draw_state_note("vd", debug_hr, 0, 0, (uint32_t)chunk);
        }
        primitive_count -= chunk;
        first_primitive += chunk;
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_dynamic_vertex_draw_impl

namespace rasterizer_dynamic_vertex_draw_indexed_impl {


/**
 * Direct3D 9 back end function rasterizer_dynamic_vertex_draw_indexed.
 *
 * @address 0x51c490
 */
void rasterizer_dynamic_vertex_draw_indexed(rasterizer_index_buffer *index_buffer, int32_t primitive_count, int32_t dynamic_vertex_slot)
{
    uint32_t start_index = 0;

    while (primitive_count > 0) {
        rasterizer_dynamic_vertex_slot *vertex_slot;
        int16_t type;
        uint32_t stride;
        int32_t chunk;
        int32_t handle;
        void *buffer;

        if (index_buffer == 0 || index_buffer->hardware_buffer == 0 || dynamic_vertex_slot == -1) {
            break;
        }
        vertex_slot = &rasterizer_dynamic_vertex_slots[dynamic_vertex_slot];
        type = vertex_slot->vertex_type;
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                       rasterizer_vertex_declarations[type].usage) & 0x10);
        handle = rasterizer_dynamic_vertex_caches[type].buffer_handle;
        buffer = handle == 0 ? 0 : rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer;
        render_device().set_stream_source(0, buffer, 0, stride);
        render_device().set_indices(index_buffer->hardware_buffer);
        {
            int32_t debug_hr = render_device().draw_indexed_primitive(rasterizer_triangle_buffer_primitive_types[index_buffer->type], vertex_slot->first_vertex, 0, (uint32_t)vertex_slot->vertex_count, start_index, (uint32_t)chunk);
            halo::interface::debug_fp_draw_state_note("vdi", debug_hr, 0, (uint32_t)vertex_slot->vertex_count, (uint32_t)chunk);
        }
        primitive_count -= chunk;
        switch (index_buffer->type) {
        case 0:
            start_index += (uint32_t)chunk * 3;
            break;
        case 1:
            start_index += (uint32_t)chunk;
            break;
        default:
            break;
        }
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_dynamic_vertex_draw_indexed_impl

namespace rasterizer_dynamic_vertex_process_and_get_handle_impl {


/**
 * Direct3D 9 back end function rasterizer_dynamic_vertex_process_and_get_handle.
 *
 * Registers: ESI = vertex_buffer (live-in)
 *
 * @address 0x51c790
 */
void *rasterizer_dynamic_vertex_process_and_get_handle(rasterizer_vertex_buffer *vertex_buffer)
{
    int16_t stride;
    void *handle;

    stride = rasterizer_vertex_sizes[vertex_buffer->type];

    render_device().set_software_vertex_processing((-(uint32_t)(rasterizer_software_vertex_processing != 0) & 0x10) |
        (rasterizer_vertex_declarations[vertex_buffer->type].usage & 0x10));

    render_device().set_stream_source(0, vertex_buffer->hardware_buffer, 0, (uint32_t)stride);

    handle = rasterizer_vertex_buffer_slots[rasterizer_dynamic_vertex_caches[_rasterizer_vertex_type_model_processed].buffer_handle - 1].hardware_buffer;

    render_device().process_vertices(0, 0, (uint32_t)vertex_buffer->count, handle, 0, 1);

    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);

    return handle;
}

}  // namespace rasterizer_dynamic_vertex_process_and_get_handle_impl

namespace rasterizer_geometry_draw_fixed_function_impl {


/**
 * Direct3D 9 back end function rasterizer_geometry_draw_fixed_function.
 *
 * @address 0x528ae0
 */
void rasterizer_geometry_draw_fixed_function(uint32_t flags, int32_t dynamic_vertex_slot, rasterizer_vertex_buffer *vertex_buffer, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count)
{
    if (flags & _group_fixed_function_fog_bit) {
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[14].declaration);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                                                  dynamic_vertex_slot);
    } else {
        rasterizer_vertex_buffer processed = *vertex_buffer;

        render_device().set_vertex_shader(rasterizer_vertex_shaders[27].shader);
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[4].declaration);
        processed.hardware_buffer = index_buffer != NULL ? rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer) : NULL;
        processed.type = _rasterizer_vertex_type_model_processed;
        render_device().set_vertex_shader(0);
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[15].declaration);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                  dynamic_vertex_slot);
    }
}

}  // namespace rasterizer_geometry_draw_fixed_function_impl

namespace rasterizer_geometry_part_draw_impl {


/**
 * Direct3D 9 back end function rasterizer_geometry_part_draw.
 *
 * @address 0x533730
 */
void rasterizer_geometry_part_draw(transparent_geometry_group *group)
{
    if ((group->flags & _group_immediate_bit) == 0) {
        rasterizer_node_matrices nodes;

        if (group->node_matrices != 0 && group->node_count != 0) {
            nodes.matrices = group->node_matrices;
            nodes.node_count = group->node_count;
        } else {
            nodes.matrices = k_render_identity_matrix_ptr;
            nodes.node_count = 1;
        }
        chimera__rasterizer_set_model_skinning((uint8_t)((group->flags & _group_node_parts_bit) == 0), &nodes);
        if (group->flags & _group_node_parts_bit) {
            chimera__rasterizer_set_up_node_parts(group->node_part_count, group->node_part_indices);
        }
        if (group->lighting != 0) {
            rasterizer_prepare_lighting_constants(group->lighting);
        }
    }
    if ((group->flags & _group_sort_first_bit) != 0 && group->parameters.mode == 1) {
        chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
    }
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        rasterizer_geometry_draw_fixed_function(group->flags, group->dynamic_vertex_slot,
                                                group->vertex_buffer,
                                                group->index_buffer,
                                                group->dynamic_index_slot, group->primitive_count);
    } else {
        render_device().set_vertex_declaration(rasterizer_vertex_declarations[4].declaration);
        render_device().set_vertex_shader(rasterizer_depth_prepass_vertex_shader);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    }
    if ((group->flags & _group_sort_first_bit) != 0 && group->parameters.mode == 1) {
        chimera__rasterizer_set_frustum_z_func(0.0f, 0.0f);
    }
}

}  // namespace rasterizer_geometry_part_draw_impl

}  // namespace halo::rasterizer
