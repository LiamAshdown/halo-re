/**
 * @file src/structures/globals.cpp
 * Binds halo::structures::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/structures/structures.hpp"
#include "halo/memory/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/math/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/render/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/core/lcg.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/crt.hpp"
#include "halo/structures/globals.hpp"
#include "link/structures.hpp"

namespace halo::structures {

const Globals &Service::instance()
{
    static const Globals state{
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
        ::local_player_globals,
        ::current_local_player_index,
        ::camera_forward_x,
        ::k_surface_resolve_step,
        ::portal_visibility_tolerance,
        ::near_clip_plane,
        ::k_plane_side_epsilon,
        ::k_projection_numerator,
        ::decals_enabled,
        ::global_origin3d_pointer,
        ::unknown_007c048c,
        ::object_lighting_default,
        ::object_lightmap_probe_direction,
        ::object_lighting_probe_sideways,
        ::render_camera_global,
        ::render_frustum_global,
    };
    return state;
}

}  // namespace halo::structures
