/**
 * @file src/structures/structure_draw.cpp
 * Drawing structure bsp surfaces: leaf face lists, picked polygon and debug draws.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/render/api.hpp"
#include "halo/structures/globals.hpp"


namespace halo::structures {

/** D3DPS_VERSION(1, 1): devices reporting an older pixel shader version need the fixed-function fallback. */
inline constexpr uint32_t k_pixel_shader_version_1_1 = 0xffff0101u;

int32_t structure_draw::build_visible_surface_geometry(int32_t *visible_surface_indices, uint32_t *surface_bits, int16_t visible_surface_count)
{
    void (__stdcall **vtable)(void *);

    if (visible_surface_count > 0) {
        int32_t geometry_handle = halo::rasterizer::rasterizer_dynamic_index_cache_reserve(visible_surface_count);
        if (geometry_handle != -1) {
            void *vertex_buffer = halo::render::rasterizer_dynamic_index_slot_lock(geometry_handle);

            if (surface_bits != 0) {
                structure_draw::leaf_faces_gather_masked(visible_surface_indices, surface_bits, (ScenarioStructureBSPSurface *)vertex_buffer);
            } else {
                structure_draw::leaf_faces_gather_list(visible_surface_count, (ScenarioStructureBSPSurface *)vertex_buffer, visible_surface_indices);
            }

            vtable = *(void (__stdcall ***)(void *))halo::rasterizer::globals().dynamic_index_buffer;
            vtable[0xc](halo::rasterizer::globals().dynamic_index_buffer);
            return geometry_handle;
        }
        if (globals().geometry_buffer_warning != 0) {
            globals().geometry_buffer_warning = 0;
        }
    }
    return -1;
}

void structure_draw::leaf_faces_gather_masked(int32_t *out_surface_indices, uint32_t *surface_bits, ScenarioStructureBSPSurface *out_faces)
{
    int32_t surface_count = halo::scenario::globals().structure_bsp->surfaces.count;
    ScenarioStructureBSPSurface *surfaces = (ScenarioStructureBSPSurface *)halo::scenario::globals().structure_bsp->surfaces.pointer;
    int32_t surface_index = 0;
    int32_t out_count = 0;

    while (surface_index < surface_count) {
        if (*surface_bits == 0) {
            surface_index = surface_index + 32;
        } else {
            int32_t bit;
            for (bit = 0; bit < 32 && surface_index < surface_count; bit = bit + 1, surface_index = surface_index + 1) {
                if ((*surface_bits & (1u << bit)) != 0) {
                    out_surface_indices[out_count] = surface_index;
                    out_faces[out_count] = surfaces[surface_index];
                    out_count = out_count + 1;
                }
            }
        }
        surface_bits = surface_bits + 1;
    }
}

void structure_draw::leaf_faces_gather_list(int16_t face_count, ScenarioStructureBSPSurface *out_faces, int32_t *face_indices)
{
    ScenarioStructureBSPSurface *surfaces = (ScenarioStructureBSPSurface *)halo::scenario::globals().structure_bsp->surfaces.pointer;
    int32_t i;

    halo::cseries::qsort_dword_array((uint32_t)(int32_t)face_count, face_indices, structure_leaf_face_index_compare);

    for (i = 0; i < face_count; i = i + 1) {
        out_faces[i] = surfaces[face_indices[i]];
    }
}

void structure_draw::leaf_faces_for_each(int32_t render_context, structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb, structure_lightmap_end_callback lightmap_end, structure_transparent_material_callback transparent_material_cb, int32_t *surface_indices, int16_t surface_index_count)
{
    int32_t *end = surface_indices + surface_index_count;
    int32_t surface_offset = 0;
    int16_t lightmap_index;

    for (lightmap_index = 0; lightmap_index < halo::scenario::globals().structure_bsp->lightmaps.count; lightmap_index = lightmap_index + 1) {
        ScenarioStructureBSPLightmap *lightmap =
            (ScenarioStructureBSPLightmap *)halo::scenario::globals().structure_bsp->lightmaps.pointer + lightmap_index;
        ScenarioStructureBSPMaterial *materials =
            (ScenarioStructureBSPMaterial *)lightmap->materials.pointer;
        int32_t material_count = lightmap->materials.count;

        if (surface_indices >= end) {
            return;
        }

        if (*surface_indices < materials[material_count - 1].surfaces + materials[material_count - 1].surface_count) {
            void *bitmap_data = 0;

            if (halo::scenario::globals().structure_bsp->lightmaps_bitmap.tag_id.index != k_word_none) {
                uint16_t bitmap_index = lightmap->bitmap;
                Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[halo::scenario::globals().structure_bsp->lightmaps_bitmap.tag_id.index].data;
                if (bitmap != 0 && bitmap_index < bitmap->bitmap_data.count) {
                    bitmap_data = (uint8_t *)bitmap->bitmap_data.pointer + bitmap_index * sizeof(BitmapData);
                }
            }
            if (lightmap_begin != 0) {
                lightmap_begin(bitmap_data);
            }

            {
                int16_t material_index;
                for (material_index = 0; material_index < material_count; material_index = material_index + 1) {
                    ScenarioStructureBSPMaterial *material = &materials[material_index];
                    int32_t material_end = material->surfaces + material->surface_count;

                    if (surface_indices >= end) {
                        break;
                    }
                    if (*surface_indices < material_end) {
                        Shader *shader = (Shader *)halo::cache::globals().tag_instances[material->shader.tag_id.index].data;
                        int32_t *scan = surface_indices;
                        int16_t consumed;

                        do {
                            scan = scan + 1;
                            if (scan >= end) {
                                break;
                            }
                        } while (*scan < material_end);
                        consumed = (int16_t)(scan - surface_indices);

                        if (material->breakable_surface == (uint16_t)-1 ||
                            (halo::physics::globals().breakable_surface_state->active[halo::scenario::globals().structure_bsp_index][bit_array_word(material->breakable_surface)] &
                             bit_array_mask(material->breakable_surface)) != 0) {
                            if (render::shader_type_is_transparent(shader->shader_type)) {
                                if (transparent_material_cb != 0) {
                                    void *coplanar_vector = test_flag(material->flags, tags::scenario_structure_bsp_material_tag_flag::fog_plane)
                                        ? (void *)&halo::structures::globals().fog_plane_vector
                                        : (void *)globals().global_origin3d_pointer;
                                    void *lightmap_vertices = test_flag(material->flags, tags::scenario_structure_bsp_material_tag_flag::coplanar)
                                        ? (void *)&material->plane
                                        : (void *)0;
                                    transparent_material_cb(shader, material->shader_permutation, bitmap_data,
                                        render_context, surface_offset, consumed, &material->rendered_vertices_type,
                                        &material->centroid, lightmap_vertices, coplanar_vector,
                                        &material->ambient_color, 0);
                                }
                            } else if (material_cb != 0) {
                                material_cb(shader, material->shader_permutation, render_context, surface_offset,
                                    consumed, &material->rendered_vertices_type);
                            }
                        }

                        surface_offset = surface_offset + consumed;
                        surface_indices = scan;
                    }
                }
            }

            if (lightmap_end != 0) {
                lightmap_end();
            }
        }
    }
}

void structure_draw::leaf_portal_vertex_count_debug(int32_t leaf_index, structure_bsp_leaf_map *leaf_map)
{
    ScenarioStructureBSPGlobalMapLeaf *leaf =
        (ScenarioStructureBSPGlobalMapLeaf *)leaf_map->leaves.pointer + (leaf_index & k_leaf_index_mask);
    int32_t portal_ref_count = leaf->portal_indices.count;
    int32_t *portal_indices = (int32_t *)leaf->portal_indices.pointer;
    ScenarioStructureBSPGlobalLeafPortal *portals =
        (ScenarioStructureBSPGlobalLeafPortal *)leaf_map->portals.pointer;
    int16_t discarded_count;
    int32_t i;

    for (i = 0; i < portal_ref_count; i = i + 1) {
        int32_t portal_index = portal_indices[i] & k_leaf_index_mask;
        int32_t vertex_count = portals[portal_index].vertices.count;

        discarded_count = 2;
        while (discarded_count < vertex_count) {
            discarded_count = discarded_count + 1;
        }
    }
}

void structure_draw::picked_polygon_refresh(void)
{
    structure_bsp_leaf_map *leaf_map = leaf_map_of(halo::scenario::globals().structure_bsp);

    globals().picked_surfaces_geometry = structure_draw::build_visible_surface_geometry(globals().visible_surface_indices, globals().surface_visible_bits, (int16_t)globals().visible_surface_count);
    globals().picked_surfaces_valid = globals().picked_surfaces_geometry != -1;

    if (globals().picked_leaf_map_leaf > -1 && globals().picked_leaf_map_leaf < leaf_map->leaves.count) {
        structure_draw::leaf_portal_vertex_count_debug(globals().picked_leaf_map_leaf, leaf_map);
    }

    if (globals().picked_leaf_map_portal > -1 && globals().picked_leaf_map_portal < leaf_map->portals.count) {
        ScenarioStructureBSPGlobalLeafPortal *portals =
            (ScenarioStructureBSPGlobalLeafPortal *)leaf_map->portals.pointer;
        int32_t vertex_count = portals[globals().picked_leaf_map_portal].vertices.count;
        int16_t discarded_count = 2;
        while (discarded_count < vertex_count) {
            discarded_count = discarded_count + 1;
        }
    }

    if (globals().debug_count_all_leaf_portals != 0) {
        ScenarioStructureBSPGlobalLeafPortal *portals =
            (ScenarioStructureBSPGlobalLeafPortal *)leaf_map->portals.pointer;
        int32_t i;
        for (i = 0; i < leaf_map->portals.count; i = i + 1) {
            int32_t vertex_count = portals[i].vertices.count;
            int16_t discarded_count = 2;
            while (discarded_count < vertex_count) {
                discarded_count = discarded_count + 1;
            }
        }
    }

    globals().fog_plane_vector_valid = 0;
    globals().fog_plane_vector = *(const real_vector3d *)globals().global_origin3d_pointer;
}

void structure_draw::picked_polygon_draw(void)
{
    int16_t saved_render_flag;

    if (globals().picked_surfaces_valid == 0) {
        return;
    }

    saved_render_flag = halo::render::globals().force_flag;
    if (halo::scenario::globals().structure_bsp->lightmaps_bitmap.tag_id.index == k_word_none && saved_render_flag == 0) {
        halo::render::globals().force_flag = 1;
    }

    halo::rasterizer::rasterizer_underwater_tint_set_states();

    structure_draw::leaf_faces_for_each(globals().picked_surfaces_geometry, (structure_lightmap_begin_callback)structure_picked_polygon_lightmap_begin, (structure_material_callback)structure_picked_polygon_material, (structure_lightmap_end_callback)halo::cseries::function_do_nothing, (structure_transparent_material_callback)0, globals().visible_surface_indices, (int16_t)globals().visible_surface_count);

    if (halo::rasterizer::globals().device_version < k_pixel_shader_version_1_1) {
        void **device = (void **)halo::rasterizer::globals().device;
        (*(void (__stdcall **)(void *, int32_t, int32_t))((uint8_t *)device + 0xe4))(device, 0x89, 0);
    }

    halo::render::globals().force_flag = saved_render_flag;
}

void structure_draw::debug_draw_surfaces_in_box(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices)
{
    int32_t local_surface_indices[k_maximum_query_surfaces];
    int32_t geometry_handle;
    int32_t *surface_indices;
    int16_t surface_count;

    geometry_handle = -1;
    if (cluster_indices != 0) {
        surface_count = structure_bsp_query::query_surfaces(0, query_point, local_surface_indices, k_maximum_query_surfaces, radius, 0, 0, cluster_count, cluster_indices);
        surface_indices = local_surface_indices;
        geometry_handle = -1;
        if (surface_count > 0) {
            geometry_handle = halo::rasterizer::rasterizer_dynamic_index_cache_reserve(surface_count);
            if (geometry_handle == -1) {
                if (globals().geometry_buffer_warning != 0) {
                    globals().geometry_buffer_warning = 0;
                }
            } else {
                void *vertex_buffer = halo::render::rasterizer_dynamic_index_slot_lock(geometry_handle);
                structure_draw::leaf_faces_gather_list(surface_count, (ScenarioStructureBSPSurface *)vertex_buffer, local_surface_indices);
                (*(void (__stdcall **)(void *))((uint8_t *)*(void **)halo::rasterizer::globals().dynamic_index_buffer + 0x30))(halo::rasterizer::globals().dynamic_index_buffer);
            }
        }
    } else {
        geometry_handle = globals().picked_surfaces_geometry;
        surface_count = (int16_t)globals().visible_surface_count;
        surface_indices = globals().visible_surface_indices;
    }

    if (geometry_handle != -1) {
        halo::rasterizer::rasterizer_projected_light_constants_build((int32_t)render_point);
        structure_draw::leaf_faces_for_each(geometry_handle, (structure_lightmap_begin_callback)0, (structure_material_callback)halo::render::render_window_structure_material_0x511f80, (structure_lightmap_end_callback)0, (structure_transparent_material_callback)0, surface_indices, surface_count);
    }
}

void structure_draw::debug_draw_surfaces_in_box_alt(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices)
{
    int32_t local_surface_indices[k_maximum_query_surfaces];
    int32_t geometry_handle;
    int32_t *surface_indices;
    int16_t surface_count;

    geometry_handle = -1;
    if (cluster_indices != 0) {
        surface_count = structure_bsp_query::query_surfaces(0, query_point, local_surface_indices, k_maximum_query_surfaces, radius, 0, 0, cluster_count, cluster_indices);
        surface_indices = local_surface_indices;
        geometry_handle = -1;
        if (surface_count > 0) {
            geometry_handle = halo::rasterizer::rasterizer_dynamic_index_cache_reserve(surface_count);
            if (geometry_handle == -1) {
                if (globals().geometry_buffer_warning != 0) {
                    globals().geometry_buffer_warning = 0;
                }
            } else {
                void *vertex_buffer = halo::render::rasterizer_dynamic_index_slot_lock(geometry_handle);
                structure_draw::leaf_faces_gather_list(surface_count, (ScenarioStructureBSPSurface *)vertex_buffer, local_surface_indices);
                (*(void (__stdcall **)(void *))((uint8_t *)*(void **)halo::rasterizer::globals().dynamic_index_buffer + 0x30))(halo::rasterizer::globals().dynamic_index_buffer);
            }
        }
    } else {
        geometry_handle = globals().picked_surfaces_geometry;
        surface_count = (int16_t)globals().visible_surface_count;
        surface_indices = globals().visible_surface_indices;
    }

    if (geometry_handle != -1) {
        halo::rasterizer::rasterizer_light_cone_set_orientation_constants((int32_t)render_point);
        structure_draw::leaf_faces_for_each(geometry_handle, (structure_lightmap_begin_callback)0, (structure_material_callback)halo::render::render_window_structure_material_0x511f40, (structure_lightmap_end_callback)0, (structure_transparent_material_callback)0, surface_indices, surface_count);
    }
}

void structure_draw::debug_draw_surfaces_simple(real_point3d *query_point, float radius, real_rectangle3d *query_box, real_plane3d *planes, int16_t plane_count)
{
    int32_t local_surface_indices[k_maximum_query_surfaces];
    int16_t surface_count = structure_bsp_query::query_surfaces(query_box, query_point, local_surface_indices, k_maximum_query_surfaces, radius, plane_count, planes, 0, 0);

    if (surface_count > 0) {
        int32_t geometry_handle = halo::rasterizer::rasterizer_dynamic_index_cache_reserve(surface_count);
        if (geometry_handle == -1) {
            if (globals().geometry_buffer_warning != 0) {
                globals().geometry_buffer_warning = 0;
            }
        } else {
            void *vertex_buffer = halo::render::rasterizer_dynamic_index_slot_lock(geometry_handle);
            structure_draw::leaf_faces_gather_list(surface_count, (ScenarioStructureBSPSurface *)vertex_buffer, local_surface_indices);
            (*(void (__stdcall **)(void *))((uint8_t *)*(void **)halo::rasterizer::globals().dynamic_index_buffer + 0x30))(halo::rasterizer::globals().dynamic_index_buffer);

            if (geometry_handle != -1) {
                structure_draw::leaf_faces_for_each(geometry_handle, (structure_lightmap_begin_callback)0, (structure_material_callback)halo::render::render_window_structure_material_0x511f50, (structure_lightmap_end_callback)0, (structure_transparent_material_callback)0, local_surface_indices, surface_count);
            }
        }
    }
}

uint8_t structure_leaf_face_index_compare(int32_t element, int32_t other)
{
    return element > other;
}

void structure_picked_polygon_lightmap_begin(void *bitmap_data)
{
    halo::rasterizer::rasterizer_underwater_tint_jitter_update((BitmapData *)bitmap_data);
}

void structure_picked_polygon_material(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t surface_offset, int16_t surface_count, void *material_extra)
{
    ((structure_material_callback)globals().unknown_007c048c)(shader_data, shader_permutation, render_context, surface_offset,
        surface_count, material_extra);
}

}  // namespace halo::structures
