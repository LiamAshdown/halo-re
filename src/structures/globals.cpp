/**
 * @file src/structures/globals.cpp
 * Binds halo::structures::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/structures/globals.hpp"
#include "halo/structures/api.hpp"

extern "C" {
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
}

namespace halo::structures {

const Globals structures_globals{
    ::render_leaf_index,
    ::render_cluster_index,
    ::render_cluster_has_sky,
    ::render_cluster_sky_index,
    ::cluster_visible_bits,
    ::visible_clusters,
    ::visible_cluster_count,
    ::surface_visible_bits,
    ::visible_surface_count,
    ::visible_surface_indices,
    ::picked_surfaces_valid,
    ::picked_surfaces_geometry,
    ::picked_leaf_map_leaf,
    ::picked_leaf_map_portal,
    ::geometry_buffer_warning,
    ::debug_count_all_leaf_portals,
    ::debug_render_cluster_pvs,
    ::fog_plane_vector_valid,
    ::fog_plane_vector,
    ::no_subcluster_path_taken,
    ::flood_recursion_bits,
    ::cluster_visible_index,
    ::cluster_flood_in_progress,
    ::cluster_flood_stamp,
    ::cluster_visit_stamp,
    ::detail_objects,
    ::runtime_decals_suppressed,
    ::k_cluster_query_radius_threshold,
    ::k_default_screen_bounds,
};

}  // namespace halo::structures
