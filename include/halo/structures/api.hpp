/**
 * @file include/halo/structures/api.hpp
 * The public C++ API of the structures module (namespace halo::structures): the structure bsp operations (visibility, clusters, fog, lighting, decals, detail objects, debug
 * drawing) as free functions taking the engine's records by pointer, plus the Globals service object.
 */
#pragma once

#include "halo/structures/globals.hpp"
#include "halo/structures/structures.hpp"

namespace halo::structures {

inline void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index)
{
    halo::structures::bsp_bounds::plane_fetch_signed(out, planes_owner, signed_index);
}

inline uint8_t structure_bsp_resolve_position_to_surface(real_point3d *start_position, real_point3d *position, int16_t *out_lightmap_index, void *out_barycentric_v, real_vector3d *direction, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u)
{
    return halo::structures::structure_bsp_query::resolve_position_to_surface(start_position, position, out_lightmap_index, out_barycentric_v, direction, out_material_index, out_surface, out_barycentric_u);
}

inline uint8_t cluster_sound_distance_lookup(int16_t cluster_a, int16_t cluster_b, ScenarioStructureBSP *structure_bsp)
{
    return halo::structures::structure_bsp_query::cluster_sound_distance_lookup(cluster_a, cluster_b, structure_bsp);
}

inline int16_t cluster_flood_fill_with_predicate(real_point3d *position, real_vector3d *facing, real max_distance, real sin_angle, real cos_angle, int16_t max_count, int16_t *output, int16_t start_cluster)
{
    return halo::structures::cluster_flood::fill_with_predicate(position, facing, max_distance, sin_angle, cos_angle, max_count, output, start_cluster);
}

inline int32_t cluster_flood_fill_within_radius(int16_t cluster_index, real_point3d *point, float tolerance, int32_t remaining_budget, int16_t *output)
{
    return halo::structures::cluster_flood::fill_within_radius(cluster_index, point, tolerance, remaining_budget, output);
}

inline int16_t structure_weather_polyhedra_find_within_radius(int16_t *out, float radius)
{
    return halo::structures::cluster_flood::weather_polyhedra_find_within_radius(out, radius);
}

inline void cluster_partition_new(cluster_reference_group *out, const char *name)
{
    halo::structures::cluster_references::partition_new(out, name);
}

inline void cluster_reference_add_within_radius(uint32_t light_or_object_handle, datum_index *placement_slot, real_point3d *position, float radius, bsp_leaf_reference *leaf_and_cluster, cluster_reference_group *cluster_list)
{
    halo::structures::cluster_references::add_within_radius(light_or_object_handle, placement_slot, position, radius, leaf_and_cluster, cluster_list);
}

inline void cluster_reference_remove_all(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list)
{
    halo::structures::cluster_references::remove_all(handle, link, cluster_list);
}

inline void structure_bsp_cluster_visibility_update(void)
{
    halo::structures::structure_visibility::cluster_visibility_update();
}

inline int16_t structure_bsp_collect_visible_objects(int32_t *out_handles, int16_t max_count, structure_bsp_object_iterate_begin_fn iterate_begin, structure_bsp_object_iterate_next_fn iterate_next, structure_bsp_object_get_bounds_fn get_bounds, structure_bsp_object_predicate_fn predicate, structure_bsp_object_accept_fn accept)
{
    return halo::structures::structure_visibility::collect_visible_objects(out_handles, max_count, iterate_begin, iterate_next, get_bounds, predicate, accept);
}

inline void render_camera_update_leaf_and_cluster(real_point3d *camera_position)
{
    halo::structures::structure_visibility::render_camera_update_leaf_and_cluster(camera_position);
}

inline uint8_t structure_bsp_mirror_query(void *camera_ref, void *camera, structure_bsp_mirror_result *out)
{
    return halo::structures::structure_visibility::mirror_query(camera_ref, camera, out);
}

inline void structure_bsp_build_fog_environment(int16_t cluster_index, structure_fog_environment *out)
{
    halo::structures::structure_fog::build_fog_environment(cluster_index, out);
}

inline void detail_objects_globals_allocate(void)
{
    halo::structures::detail_object_system::globals_allocate();
}

inline void detail_objects_invalidate(void)
{
    halo::structures::detail_object_system::invalidate();
}

inline void detail_objects_update_render_list(void)
{
    halo::structures::detail_object_system::update_render_list();
}

inline void bsp_lightmap_sample_vertex_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices)
{
    halo::structures::bsp_lighting::lightmap_sample_vertex_color(bitmap, weight_1, weight_2, out, material, triangle_vertex_indices);
}

inline void bsp_material_sample_base_map_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices)
{
    halo::structures::bsp_lighting::material_sample_base_map_color(bitmap, weight_1, weight_2, out, material, triangle_vertex_indices);
}

inline uint8_t object_lighting_sample_point(uint8_t flags, real_point3d *point, render_lighting *lighting)
{
    return halo::structures::bsp_lighting::object_lighting_sample_point(flags, point, lighting);
}

inline void structure_lightmap_uv_rect_build(int16_t sequence_index, int16_t sprite_index, real scale, real *out_extent, real *out_sprite_rect, const Decal *decal_definition)
{
    halo::structures::bsp_lighting::lightmap_uv_rect_build(sequence_index, sprite_index, scale, out_extent, out_sprite_rect, decal_definition);
}

inline void structure_leaf_faces_for_each(int32_t render_context, structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb, structure_lightmap_end_callback lightmap_end, structure_transparent_material_callback transparent_material_cb, int32_t *surface_indices, int16_t surface_index_count)
{
    halo::structures::structure_draw::leaf_faces_for_each(render_context, lightmap_begin, material_cb, lightmap_end, transparent_material_cb, surface_indices, surface_index_count);
}

inline void structure_picked_polygon_refresh(void)
{
    halo::structures::structure_draw::picked_polygon_refresh();
}

inline void structure_picked_polygon_draw(void)
{
    halo::structures::structure_draw::picked_polygon_draw();
}

inline void structure_debug_draw_surfaces_in_box(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices)
{
    halo::structures::structure_draw::debug_draw_surfaces_in_box(render_point, query_point, radius, cluster_count, cluster_indices);
}

inline void structure_debug_draw_surfaces_in_box_alt(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices)
{
    halo::structures::structure_draw::debug_draw_surfaces_in_box_alt(render_point, query_point, radius, cluster_count, cluster_indices);
}

inline void structure_debug_draw_surfaces_simple(real_point3d *query_point, float radius, real_rectangle3d *query_box, real_plane3d *planes, int16_t plane_count)
{
    halo::structures::structure_draw::debug_draw_surfaces_simple(query_point, radius, query_box, planes, plane_count);
}

inline void structure_decals_update_switch_transitions(uint32_t *switch_group_a, uint32_t *switch_group_b, int16_t cluster_count)
{
    halo::structures::structure_decals::update_switch_transitions(switch_group_a, switch_group_b, cluster_count);
}

inline void structure_runtime_decals_evict(void)
{
    halo::structures::structure_decals::runtime_decals_evict();
}

inline void structure_runtime_decals_mark_dirty(void)
{
    halo::structures::structure_decals::runtime_decals_mark_dirty();
}

}  // namespace halo::structures
