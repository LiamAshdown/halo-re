/**
 * @file src/structures/structures_c_api.cpp
 * The structures module's C ABI: one extern "C" function per original symbol, same name and signature,
 * forwarding to the halo::structures implementation. The shims do nothing else.
 */

#include "halo/structures/structures_c_api.h"

extern "C" structure_bsp_overlap aabb_overlap_classify(real_rectangle3d *box_a, real_rectangle3d *box_b)
{
    return halo::structures::bsp_bounds::aabb_overlap_classify(box_a, box_b);
}

extern "C" structure_bsp_overlap frustum_planes_classify_box(real_rectangle3d *box, real_plane3d *planes, int16_t plane_count)
{
    return halo::structures::bsp_bounds::frustum_planes_classify_box(box, planes, plane_count);
}

extern "C" void bsp3d_node_bounds_decompress(real_rectangle3d *parent_bounds, uint8_t *compressed_bounds, real_rectangle3d *out)
{
    halo::structures::bsp_bounds::bsp3d_node_bounds_decompress(parent_bounds, compressed_bounds, out);
}

extern "C" void polygon2d_bounds_expand(real_bounds *bounds_xy, polygon2d *polygon)
{
    halo::structures::bsp_bounds::polygon2d_bounds_expand(bounds_xy, polygon);
}

extern "C" void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index)
{
    halo::structures::bsp_bounds::plane_fetch_signed(out, planes_owner, signed_index);
}

extern "C" int16_t bsp3d_node_query_recursive(int32_t node_index, real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_point3d *point, float radius, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes, int16_t inherited_classification)
{
    return halo::structures::structure_bsp_query::node_query_recursive(node_index, parent_bounds, visited_bits, output_array, max_count, point, radius, query_box, plane_count, planes, inherited_classification);
}

extern "C" int16_t structure_bsp_leaf_query(int32_t raw_child, int16_t inherited_classification, real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes)
{
    return halo::structures::structure_bsp_query::leaf_query(raw_child, inherited_classification, parent_bounds, visited_bits, output_array, max_count, query_box, plane_count, planes);
}

extern "C" int32_t structure_bsp_collect_surfaces_in_clusters(int32_t *out_surfaces, int16_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes, uint32_t *visited_bits, int16_t cluster_count, int16_t *cluster_indices)
{
    return halo::structures::structure_bsp_query::collect_surfaces_in_clusters(out_surfaces, max_count, query_box, plane_count, planes, visited_bits, cluster_count, cluster_indices);
}

extern "C" int16_t structure_bsp_query_surfaces(real_rectangle3d *query_box, real_point3d *query_point, int32_t *out_surfaces, int32_t max_count, float radius, int16_t plane_count, real_plane3d *planes, int16_t cluster_count, int16_t *cluster_indices)
{
    return halo::structures::structure_bsp_query::query_surfaces(query_box, query_point, out_surfaces, max_count, radius, plane_count, planes, cluster_count, cluster_indices);
}

extern "C" uint8_t structure_bsp_points_within_band(real_point3d *points, int16_t point_count, float tolerance)
{
    return halo::structures::structure_bsp_query::points_within_band(points, point_count, tolerance);
}

extern "C" uint8_t structure_bsp_leaf_find_material_surface(real_point3d *point, int32_t accepted_plane, int16_t *out_lightmap_index, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u, void *out_barycentric_v, int32_t raw_child)
{
    return halo::structures::structure_bsp_query::leaf_find_material_surface(point, accepted_plane, out_lightmap_index, out_material_index, out_surface, out_barycentric_u, out_barycentric_v, raw_child);
}

extern "C" uint8_t structure_bsp_resolve_position_to_surface(real_point3d *start_position, real_point3d *position, int16_t *out_lightmap_index, void *out_barycentric_v, real_vector3d *direction, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u)
{
    return halo::structures::structure_bsp_query::resolve_position_to_surface(start_position, position, out_lightmap_index, out_barycentric_v, direction, out_material_index, out_surface, out_barycentric_u);
}

extern "C" uint8_t cluster_sound_distance_lookup(int16_t cluster_a, int16_t cluster_b, ScenarioStructureBSP *structure_bsp)
{
    return halo::structures::structure_bsp_query::cluster_sound_distance_lookup(cluster_a, cluster_b, structure_bsp);
}

extern "C" void structure_bsp_expand_visible_clusters_by_plane(ScenarioStructureBSP *tag)
{
    halo::structures::structure_bsp_view(tag).expand_visible_clusters_by_plane();
}

extern "C" void structure_bsp_expand_visible_clusters_by_subcluster(ScenarioStructureBSP *tag)
{
    halo::structures::structure_bsp_view(tag).expand_visible_clusters_by_subcluster();
}

extern "C" uint8_t structure_bsp_portal_sphere_test(ScenarioStructureBSP *structure_bsp, real_point3d *point, int16_t portal_index, float tolerance)
{
    return halo::structures::structure_bsp_view(structure_bsp).portal_sphere_test(point, portal_index, tolerance);
}

extern "C" void structure_surface_material_locate(ScenarioStructureBSP *structure_bsp, int32_t surface_index, int16_t *out_material_index, int16_t *out_lightmap_index)
{
    halo::structures::structure_bsp_view(structure_bsp).surface_material_locate(surface_index, out_material_index, out_lightmap_index);
}

extern "C" void camera_cluster_portal_flood_recursive(int16_t cluster_index, polygon2d *view_polygon)
{
    halo::structures::cluster_flood::camera_portal_flood_recursive(cluster_index, view_polygon);
}

extern "C" int16_t cluster_flood_fill_with_predicate(real_point3d *position, real_vector3d *facing, real max_distance, real sin_angle, real cos_angle, int16_t max_count, int16_t *output, int16_t start_cluster)
{
    return halo::structures::cluster_flood::fill_with_predicate(position, facing, max_distance, sin_angle, cos_angle, max_count, output, start_cluster);
}

extern "C" int32_t cluster_flood_fill_within_radius(int16_t cluster_index, real_point3d *point, float tolerance, int32_t remaining_budget, int16_t *output)
{
    return halo::structures::cluster_flood::fill_within_radius(cluster_index, point, tolerance, remaining_budget, output);
}

extern "C" int32_t structure_bsp_cluster_flood_seed(real_point3d *point, float radius, int16_t start_cluster, int16_t *output, int16_t max_count)
{
    return halo::structures::cluster_flood::seed(point, radius, start_cluster, output, max_count);
}

extern "C" uint8_t structure_bsp_portal_project(real_plane3d *plane, void *camera_ref, real_point3d *vertices, void *camera, uint32_t vertex_count, int16_t winding, polygon2d *out)
{
    return halo::structures::cluster_flood::portal_project(plane, camera_ref, vertices, camera, vertex_count, winding, out);
}

extern "C" uint8_t structure_bsp_portal_test_and_project(char same_side, int16_t portal_index, polygon2d *out)
{
    return halo::structures::cluster_flood::portal_test_and_project(same_side, portal_index, out);
}

extern "C" int16_t structure_weather_polyhedra_find_within_radius(int16_t *out, float radius)
{
    return halo::structures::cluster_flood::weather_polyhedra_find_within_radius(out, radius);
}

extern "C" void cluster_partition_new(cluster_reference_group *out, char *name)
{
    halo::structures::cluster_references::partition_new(out, name);
}

extern "C" void cluster_reference_add_within_radius(uint32_t light_or_object_handle, datum_index *placement_slot, real_point3d *position, float radius, bsp_leaf_reference *leaf_and_cluster, cluster_reference_group *cluster_list)
{
    halo::structures::cluster_references::add_within_radius(light_or_object_handle, placement_slot, position, radius, leaf_and_cluster, cluster_list);
}

extern "C" void cluster_reference_remove_all(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list)
{
    halo::structures::cluster_references::remove_all(handle, link, cluster_list);
}

extern "C" void structure_bsp_camera_visibility_pass(void)
{
    halo::structures::structure_visibility::camera_visibility_pass();
}

extern "C" void structure_bsp_cluster_visibility_update(void)
{
    halo::structures::structure_visibility::cluster_visibility_update();
}

extern "C" int16_t structure_bsp_collect_visible_objects(int32_t *out_handles, int16_t max_count, structure_bsp_object_iterate_begin_fn iterate_begin, structure_bsp_object_iterate_next_fn iterate_next, structure_bsp_object_get_bounds_fn get_bounds, structure_bsp_object_predicate_fn predicate, structure_bsp_object_accept_fn accept)
{
    return halo::structures::structure_visibility::collect_visible_objects(out_handles, max_count, iterate_begin, iterate_next, get_bounds, predicate, accept);
}

extern "C" void render_camera_update_leaf_and_cluster(real_point3d *camera_position)
{
    halo::structures::structure_visibility::render_camera_update_leaf_and_cluster(camera_position);
}

extern "C" uint8_t structure_bsp_mirror_query(void *camera_ref, void *camera, structure_bsp_mirror_result *out)
{
    return halo::structures::structure_visibility::mirror_query(camera_ref, camera, out);
}

extern "C" uint32_t structure_bsp_resolve_fog_tag(int16_t cluster_index, ScenarioStructureBSP *structure_bsp, uint8_t use_sky)
{
    return halo::structures::structure_fog::resolve_fog_tag(cluster_index, structure_bsp, use_sky);
}

extern "C" void structure_bsp_build_fog_environment(int16_t cluster_index, structure_fog_environment *out)
{
    halo::structures::structure_fog::build_fog_environment(cluster_index, out);
}

extern "C" void detail_objects_globals_allocate(void)
{
    halo::structures::detail_object_system::globals_allocate();
}

extern "C" void detail_objects_invalidate(void)
{
    halo::structures::detail_object_system::invalidate();
}

extern "C" void detail_objects_update_render_list(void)
{
    halo::structures::detail_object_system::update_render_list();
}

extern "C" ScenarioStructureBSPGlobalDetailObjectCell * detail_object_cell_lower_bound(ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end, detail_object_cell_key *key)
{
    return halo::structures::detail_object_system::cell_lower_bound(begin, end, key);
}

extern "C" ScenarioStructureBSPGlobalDetailObjectCell * detail_object_cell_upper_bound(ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end, detail_object_cell_key *key)
{
    return halo::structures::detail_object_system::cell_upper_bound(begin, end, key);
}

extern "C" void bsp_lightmap_sample_vertex_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices)
{
    halo::structures::bsp_lighting::lightmap_sample_vertex_color(bitmap, weight_1, weight_2, out, material, triangle_vertex_indices);
}

extern "C" void bsp_material_sample_base_map_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices)
{
    halo::structures::bsp_lighting::material_sample_base_map_color(bitmap, weight_1, weight_2, out, material, triangle_vertex_indices);
}

extern "C" uint8_t object_lighting_sample_point(uint8_t flags, real_point3d *point, render_lighting *lighting)
{
    return halo::structures::bsp_lighting::object_lighting_sample_point(flags, point, lighting);
}

extern "C" void structure_lightmap_uv_rect_build(int16_t sequence_index, int16_t sprite_index, real scale, real *out_extent, real *out_sprite_rect, const Decal *decal_definition)
{
    halo::structures::bsp_lighting::lightmap_uv_rect_build(sequence_index, sprite_index, scale, out_extent, out_sprite_rect, decal_definition);
}

extern "C" int32_t chimera__bsp_poly_movsx_2(int32_t *visible_surface_indices, uint32_t *surface_bits, int16_t visible_surface_count)
{
    return halo::structures::structure_draw::build_visible_surface_geometry(visible_surface_indices, surface_bits, visible_surface_count);
}

extern "C" void structure_leaf_faces_gather_masked(int32_t *out_surface_indices, uint32_t *surface_bits, ScenarioStructureBSPSurface *out_faces)
{
    halo::structures::structure_draw::leaf_faces_gather_masked(out_surface_indices, surface_bits, out_faces);
}

extern "C" void structure_leaf_faces_gather_list(int16_t face_count, ScenarioStructureBSPSurface *out_faces, int32_t *face_indices)
{
    halo::structures::structure_draw::leaf_faces_gather_list(face_count, out_faces, face_indices);
}

extern "C" void structure_leaf_faces_for_each(int32_t render_context, structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb, structure_lightmap_end_callback lightmap_end, structure_transparent_material_callback transparent_material_cb, int32_t *surface_indices, int16_t surface_index_count)
{
    halo::structures::structure_draw::leaf_faces_for_each(render_context, lightmap_begin, material_cb, lightmap_end, transparent_material_cb, surface_indices, surface_index_count);
}

extern "C" void structure_leaf_portal_vertex_count_debug(int32_t leaf_index, structure_bsp_leaf_map *leaf_map)
{
    halo::structures::structure_draw::leaf_portal_vertex_count_debug(leaf_index, leaf_map);
}

extern "C" void structure_picked_polygon_refresh(void)
{
    halo::structures::structure_draw::picked_polygon_refresh();
}

extern "C" void structure_picked_polygon_draw(void)
{
    halo::structures::structure_draw::picked_polygon_draw();
}

extern "C" void structure_debug_draw_surfaces_in_box(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices)
{
    halo::structures::structure_draw::debug_draw_surfaces_in_box(render_point, query_point, radius, cluster_count, cluster_indices);
}

extern "C" void structure_debug_draw_surfaces_in_box_alt(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices)
{
    halo::structures::structure_draw::debug_draw_surfaces_in_box_alt(render_point, query_point, radius, cluster_count, cluster_indices);
}

extern "C" void structure_debug_draw_surfaces_simple(real_point3d *query_point, float radius, real_rectangle3d *query_box, real_plane3d *planes, int16_t plane_count)
{
    halo::structures::structure_draw::debug_draw_surfaces_simple(query_point, radius, query_box, planes, plane_count);
}

extern "C" uint8_t structure_leaf_face_index_compare(int32_t element, int32_t other)
{
    return halo::structures::structure_leaf_face_index_compare(element, other);
}

extern "C" void structure_picked_polygon_lightmap_begin(void *bitmap_data)
{
    halo::structures::structure_picked_polygon_lightmap_begin(bitmap_data);
}

extern "C" void structure_picked_polygon_material(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t surface_offset, int16_t surface_count, void *material_extra)
{
    halo::structures::structure_picked_polygon_material(shader_data, shader_permutation, render_context, surface_offset, surface_count, material_extra);
}

extern "C" void structure_decals_update_switch_transitions(uint32_t *switch_group_a, uint32_t *switch_group_b, int16_t cluster_count)
{
    halo::structures::structure_decals::update_switch_transitions(switch_group_a, switch_group_b, cluster_count);
}

extern "C" void structure_runtime_decals_evict(void)
{
    halo::structures::structure_decals::runtime_decals_evict();
}

extern "C" void structure_runtime_decals_mark_dirty(void)
{
    halo::structures::structure_decals::runtime_decals_mark_dirty();
}
