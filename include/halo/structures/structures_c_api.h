/**
 * @file include/halo/structures/structures_c_api.h
 * The C ABI of the structures module: every original function with its original signature and C linkage.
 * Defined in src/structures/structures_c_api.cpp; documented on the halo::structures C++ API.
 */
#pragma once

#include "halo/structures/structures.hpp"

#ifdef __cplusplus
extern "C" {
#endif

structure_bsp_overlap aabb_overlap_classify(real_rectangle3d *box_a, real_rectangle3d *box_b);
structure_bsp_overlap frustum_planes_classify_box(real_rectangle3d *box, real_plane3d *planes, int16_t plane_count);
void bsp3d_node_bounds_decompress(real_rectangle3d *parent_bounds, uint8_t *compressed_bounds, real_rectangle3d *out);
void polygon2d_bounds_expand(real_bounds *bounds_xy, polygon2d *polygon);
void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index);
int16_t bsp3d_node_query_recursive(int32_t node_index, real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_point3d *point, float radius, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes, int16_t inherited_classification);
int16_t structure_bsp_leaf_query(int32_t raw_child, int16_t inherited_classification, real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes);
int32_t structure_bsp_collect_surfaces_in_clusters(int32_t *out_surfaces, int16_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes, uint32_t *visited_bits, int16_t cluster_count, int16_t *cluster_indices);
int16_t structure_bsp_query_surfaces(real_rectangle3d *query_box, real_point3d *query_point, int32_t *out_surfaces, int32_t max_count, float radius, int16_t plane_count, real_plane3d *planes, int16_t cluster_count, int16_t *cluster_indices);
uint8_t structure_bsp_points_within_band(real_point3d *points, int16_t point_count, float tolerance);
uint8_t structure_bsp_leaf_find_material_surface(real_point3d *point, int32_t accepted_plane, int16_t *out_lightmap_index, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u, void *out_barycentric_v, int32_t raw_child);
uint8_t structure_bsp_resolve_position_to_surface(real_point3d *start_position, real_point3d *position, int16_t *out_lightmap_index, void *out_barycentric_v, real_vector3d *direction, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u);
uint8_t cluster_sound_distance_lookup(int16_t cluster_a, int16_t cluster_b, ScenarioStructureBSP *structure_bsp);
void structure_bsp_expand_visible_clusters_by_plane(ScenarioStructureBSP *tag);
void structure_bsp_expand_visible_clusters_by_subcluster(ScenarioStructureBSP *tag);
uint8_t structure_bsp_portal_sphere_test(ScenarioStructureBSP *structure_bsp, real_point3d *point, int16_t portal_index, float tolerance);
void structure_surface_material_locate(ScenarioStructureBSP *structure_bsp, int32_t surface_index, int16_t *out_material_index, int16_t *out_lightmap_index);
void camera_cluster_portal_flood_recursive(int16_t cluster_index, polygon2d *view_polygon);
int16_t cluster_flood_fill_with_predicate(real_point3d *position, real_vector3d *facing, real max_distance, real sin_angle, real cos_angle, int16_t max_count, int16_t *output, int16_t start_cluster);
int32_t cluster_flood_fill_within_radius(int16_t cluster_index, real_point3d *point, float tolerance, int32_t remaining_budget, int16_t *output);
int32_t structure_bsp_cluster_flood_seed(real_point3d *point, float radius, int16_t start_cluster, int16_t *output, int16_t max_count);
uint8_t structure_bsp_portal_project(real_plane3d *plane, void *camera_ref, real_point3d *vertices, void *camera, uint32_t vertex_count, int16_t winding, polygon2d *out);
uint8_t structure_bsp_portal_test_and_project(char same_side, int16_t portal_index, polygon2d *out);
int16_t structure_weather_polyhedra_find_within_radius(int16_t *out, float radius);
void cluster_partition_new(cluster_reference_group *out, char *name);
void cluster_reference_add_within_radius(uint32_t light_or_object_handle, datum_index *placement_slot, real_point3d *position, float radius, bsp_leaf_reference *leaf_and_cluster, cluster_reference_group *cluster_list);
void cluster_reference_remove_all(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list);
void structure_bsp_camera_visibility_pass(void);
void structure_bsp_cluster_visibility_update(void);
int16_t structure_bsp_collect_visible_objects(int32_t *out_handles, int16_t max_count, structure_bsp_object_iterate_begin_fn iterate_begin, structure_bsp_object_iterate_next_fn iterate_next, structure_bsp_object_get_bounds_fn get_bounds, structure_bsp_object_predicate_fn predicate, structure_bsp_object_accept_fn accept);
void render_camera_update_leaf_and_cluster(real_point3d *camera_position);
uint8_t structure_bsp_mirror_query(void *camera_ref, void *camera, structure_bsp_mirror_result *out);
uint32_t structure_bsp_resolve_fog_tag(int16_t cluster_index, ScenarioStructureBSP *structure_bsp, uint8_t use_sky);
void structure_bsp_build_fog_environment(int16_t cluster_index, structure_fog_environment *out);
void detail_objects_globals_allocate(void);
void detail_objects_invalidate(void);
void detail_objects_update_render_list(void);
ScenarioStructureBSPGlobalDetailObjectCell * detail_object_cell_lower_bound(ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end, detail_object_cell_key *key);
ScenarioStructureBSPGlobalDetailObjectCell * detail_object_cell_upper_bound(ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end, detail_object_cell_key *key);
void bsp_lightmap_sample_vertex_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices);
void bsp_material_sample_base_map_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices);
uint8_t object_lighting_sample_point(uint8_t flags, real_point3d *point, render_lighting *lighting);
void structure_lightmap_uv_rect_build(int16_t sequence_index, int16_t sprite_index, real scale, real *out_extent, real *out_sprite_rect, const Decal *decal_definition);
int32_t chimera__bsp_poly_movsx_2(int32_t *visible_surface_indices, uint32_t *surface_bits, int16_t visible_surface_count);
void structure_leaf_faces_gather_masked(int32_t *out_surface_indices, uint32_t *surface_bits, ScenarioStructureBSPSurface *out_faces);
void structure_leaf_faces_gather_list(int16_t face_count, ScenarioStructureBSPSurface *out_faces, int32_t *face_indices);
void structure_leaf_faces_for_each(int32_t render_context, structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb, structure_lightmap_end_callback lightmap_end, structure_transparent_material_callback transparent_material_cb, int32_t *surface_indices, int16_t surface_index_count);
void structure_leaf_portal_vertex_count_debug(int32_t leaf_index, structure_bsp_leaf_map *leaf_map);
void structure_picked_polygon_refresh(void);
void structure_picked_polygon_draw(void);
void structure_debug_draw_surfaces_in_box(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices);
void structure_debug_draw_surfaces_in_box_alt(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices);
void structure_debug_draw_surfaces_simple(real_point3d *query_point, float radius, real_rectangle3d *query_box, real_plane3d *planes, int16_t plane_count);
uint8_t structure_leaf_face_index_compare(int32_t element, int32_t other);
void structure_picked_polygon_lightmap_begin(void *bitmap_data);
void structure_picked_polygon_material(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t surface_offset, int16_t surface_count, void *material_extra);
void structure_decals_update_switch_transitions(uint32_t *switch_group_a, uint32_t *switch_group_b, int16_t cluster_count);
void structure_runtime_decals_evict(void);
void structure_runtime_decals_mark_dirty(void);

#ifdef __cplusplus
}
#endif
