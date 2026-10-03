#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Starting location queries and waypoint collection. Stateless service class: every function is a static
 * member and the state it acts on lives in the engine globals.
 */
class SpawnLocations {
public:
    static void build_visible_cluster_bitmask(uint32_t *out_bitmask, uint8_t local_players_only);
    static int16_t collect_matching_waypoints(int32_t candidate, float *out_positions, uint8_t *out_slots, int16_t max_count);
    static int32_t find_nearest_unused_type4_location(int32_t *excluded_indices, int32_t excluded_count, real_point3d *reference_point);
    static int32_t find_one_valid_starting_location(int16_t type, int16_t team, real_point3d *origin, float max_horizontal_dist, float max_height_delta);
    static int find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results);
    static uint8_t location_blocked_by_vehicle(real_point3d *point);
};

}
