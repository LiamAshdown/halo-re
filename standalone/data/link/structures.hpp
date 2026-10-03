/**
 * @file standalone/data/link/structures.hpp
 * Link names of the engine variables the structures module binds in halo::structures::Globals (src/structures/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern player_globals *local_player_globals;
extern int16_t current_local_player_index;
extern int32_t render_leaf_index;
extern int32_t render_cluster_index;
extern uint8_t render_cluster_has_sky;
extern int16_t render_cluster_sky_index;
extern uint32_t cluster_visible_bits[0x10];
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters];
extern int16_t visible_cluster_count;
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits];
extern int16_t visible_surface_count;
extern int32_t visible_surface_indices[k_maximum_visible_surfaces];
extern uint8_t picked_surfaces_valid;
extern int32_t picked_surfaces_geometry;
extern int32_t picked_leaf_map_leaf;
extern int32_t picked_leaf_map_portal;
extern int16_t geometry_buffer_warning;
extern uint8_t debug_count_all_leaf_portals;
extern uint8_t debug_render_cluster_pvs;
extern uint8_t fog_plane_vector_valid;
extern real_vector3d fog_plane_vector;
extern uint8_t no_subcluster_path_taken;
extern uint32_t *flood_recursion_bits;
extern int16_t cluster_visible_index[0x200];
extern uint8_t cluster_flood_in_progress;
extern int32_t cluster_flood_stamp;
extern int32_t cluster_visit_stamp[0x200];
extern detail_object_globals *detail_objects;
extern uint8_t *runtime_decals_suppressed;
extern float k_cluster_query_radius_threshold;
extern real_bounds *k_default_screen_bounds;
extern real_vector3d camera_forward_x;
extern float k_surface_resolve_step;
extern float portal_visibility_tolerance;
extern real_plane3d near_clip_plane;
extern double k_plane_side_epsilon;
extern float k_projection_numerator;
extern uint8_t decals_enabled;
extern const real_point3d *global_origin3d_pointer;
extern void *unknown_007c048c;
extern render_lighting object_lighting_default;
extern real_vector3d object_lightmap_probe_direction[1];
extern real_vector3d object_lighting_probe_sideways[4];
}
