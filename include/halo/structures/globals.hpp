/**
 * @file include/halo/structures/globals.hpp
 * The structures module's engine globals as one service object. The variables live at fixed addresses in the data
 * image (standalone/data) under their original link names; Globals holds a reference to each, so no other file
 * declares them.
 */
#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

struct player_globals;
struct render_lighting;

namespace halo::structures {

/**
 * References to the structures module's engine variables.
 *
 * render_leaf_index and render_cluster_index (0x007c3344, 0x007c3348) locate the render camera in the bsp, -1 when it
 * is outside. The cluster, surface and visible_* members are the per-frame visibility results; the picked_* and
 * debug_* members drive the picked-polygon visualisation; flood_recursion_bits, cluster_visible_index and the
 * cluster_flood_* members are the portal and cluster flood working state; detail_objects and
 * runtime_decals_suppressed point into game state.
 */
struct Globals {
    int32_t &render_leaf_index;
    int32_t &render_cluster_index;
    uint8_t &render_cluster_has_sky;
    int16_t &render_cluster_sky_index;
    uint32_t (&cluster_visible_bits)[0x10];
    structure_bsp_visible_cluster (&visible_clusters)[k_maximum_visible_clusters];
    int16_t &visible_cluster_count;
    uint32_t (&surface_visible_bits)[k_maximum_visible_surface_bits];
    int16_t &visible_surface_count;
    int32_t (&visible_surface_indices)[k_maximum_visible_surfaces];
    uint8_t &picked_surfaces_valid;
    int32_t &picked_surfaces_geometry;
    int32_t &picked_leaf_map_leaf;
    int32_t &picked_leaf_map_portal;
    int16_t &geometry_buffer_warning;
    uint8_t &debug_count_all_leaf_portals;
    uint8_t &debug_render_cluster_pvs;
    uint8_t &fog_plane_vector_valid;
    real_vector3d &fog_plane_vector;
    uint8_t &no_subcluster_path_taken;
    uint32_t *&flood_recursion_bits;
    int16_t (&cluster_visible_index)[0x200];
    uint8_t &cluster_flood_in_progress;
    int32_t &cluster_flood_stamp;
    int32_t (&cluster_visit_stamp)[0x200];
    detail_object_globals *&detail_objects;
    uint8_t *&runtime_decals_suppressed;
    float &k_cluster_query_radius_threshold;
    real_bounds *&k_default_screen_bounds;
    player_globals *&local_player_globals;
    int16_t &current_local_player_index;
    real_vector3d &camera_forward_x;
    float &k_surface_resolve_step;
    float &portal_visibility_tolerance;
    real_plane3d &near_clip_plane;
    double &k_plane_side_epsilon;
    float &k_projection_numerator;
    uint8_t &decals_enabled;
    const real_point3d *&global_origin3d_pointer;
    void *&unknown_007c048c;
    render_lighting &object_lighting_default;
    real_vector3d (&object_lightmap_probe_direction)[1];
    real_vector3d (&object_lighting_probe_sideways)[4];
};

/**
 * The structures service singleton. instance() builds the Globals reference table on first use (Meyers singleton); the state it
 * refers to lives in the data image. globals() is the short form every caller uses.
 */
class Service {
public:
    static const Globals &instance();
};

inline const Globals &globals() { return Service::instance(); }

}  // namespace halo::structures
