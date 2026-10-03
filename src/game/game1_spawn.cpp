/**
 * Starting location queries and waypoint collection.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include <stdint.h>
#include "cache.h"

#include "halo/game/game1_spawn.hpp"
#include "halo/math/api.hpp"

extern "C" {
extern ScenarioStructureBSP *global_structure_bsp;
extern data_array *object_data;
extern int16_t objects_get_ambient_cluster(void);
extern data_array *player_data;
extern void *data_iterator_next(data_iterator *iterator);
extern game_engine_definition *current_game_engine;
extern game_variant game_engine_variant;
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints];
extern uint8_t custom_waypoint_matches_filter(int32_t candidate, player *reference_player,
    int32_t slot_index);
extern Scenario *global_scenario;
extern int game_engine_find_valid_starting_locations(real_point3d *origin,
    float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type,
    int32_t max_results, int32_t *results);
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern int32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point);
extern int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output);
}

namespace halo::game::engine1 {

/**
 * Builds the bitmask of the structure clusters visible to the local players.
 *
 * @address 0x4782a0
 */
void SpawnLocations::build_visible_cluster_bitmask(uint32_t *out_bitmask, uint8_t local_players_only)
{
    uint8_t *bsp_info = (uint8_t *)global_structure_bsp;
    int32_t i;
    data_iterator iterator;
    void *p;
    int16_t player_gate_result;

    for (i = 0; i < 0x10; i++) {
        out_bitmask[i] = 0;
    }

    player_gate_result = objects_get_ambient_cluster();

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = data_iterator_next(&iterator);
    while (p != 0) {
        player *pl = (player *)p;
        if (local_players_only == 0 || pl->local_player_index != -1) {
            if (pl->unit != (datum_index)0xffffffff) {
                uint32_t current = (uint32_t)pl->unit;
                object *root;
                do {
                    root = (object *)((object_header *)object_data->data)[current & 0xffff].data;
                    current = (uint32_t)root->parent_object;
                } while (current != 0xffffffff);

                if (root->location_cluster_index != -1) {
                    pl->bsp_cluster = root->location_cluster_index;
                }
            }

            if (pl->bsp_cluster != -1) {
                int32_t cluster_count = *(int32_t *)(bsp_info + 0x134);
                uint8_t *cluster_bits_table = *(uint8_t **)(bsp_info + 0x14c);
                int32_t stride_dwords = (cluster_count + 0x1f) >> 5;
                int16_t words = (int16_t)stride_dwords;
                int32_t j;

                for (j = words - 1; j >= 0; j--) {
                    out_bitmask[j] |= *(uint32_t *)(cluster_bits_table + stride_dwords * pl->bsp_cluster * 4 + j * 4);
                }
            }
        }
        p = data_iterator_next(&iterator);
    }

    if (player_gate_result != -1) {
        int32_t cluster_count = *(int32_t *)(bsp_info + 0x134);
        uint32_t *row = (uint32_t *)(*(uint8_t **)(bsp_info + 0x14c) +
            ((cluster_count + 0x1f) >> 5) * player_gate_result * 4);

        halo::math::bit_vector_or(row, (int16_t)cluster_count, out_bitmask, out_bitmask);
    }
}

/**
 * For a given player/object filter, walks the custom waypoint table and copies the positions and slot indices
 * of all matching entries into the caller-supplied output arrays.
 *
 * Original register convention: stack -> candidate, out_positions, out_slots, max_count.
 *
 * @address 0x462190
 */
int16_t SpawnLocations::collect_matching_waypoints(int32_t candidate, float *out_positions, uint8_t *out_slots, int16_t max_count)
{
    int16_t written = 0;
    int16_t slot;

    if (current_game_engine != 0 && game_engine_variant.objective_indicator == 0 && candidate != -1) {
        player *reference_player = (player *)((uint8_t *)player_data->data + (candidate & 0xffff) * 0x200);

        for (slot = 0; slot < k_maximum_custom_waypoints; slot++) {
            if (custom_waypoint_matches_filter(candidate, reference_player, slot) != 0 &&
                written < max_count) {
                out_slots[written] = (uint8_t)slot;
                out_positions[written * 2] = custom_waypoints[slot].position.x;
                out_positions[written * 2 + 1] = custom_waypoints[slot].position.y;
                written = written + 1;
            }
        }
    }
    return written;
}

/**
 * Finds the nearest (or, if no reference point is given, the first) unused type-4 scenario starting location,
 * excluding indices already claimed.
 *
 * Original register convention: stack -> excluded_indices, EDI -> excluded_count, EBX -> reference_point.
 *
 * @address 0x46d520
 */
int32_t SpawnLocations::find_nearest_unused_type4_location(int32_t *excluded_indices, int32_t excluded_count, real_point3d *reference_point)
{
    int32_t flag_count = (int32_t)global_scenario->netgame_flags.count;
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;
    float best_distance = 1e+06f;
    int32_t best_index = -1;
    int32_t i;

    for (i = 0; i < flag_count; i++) {
        int32_t j;
        uint8_t excluded = 0;

        if (flags[i].type != 4) {
            continue;
        }
        for (j = 0; j < excluded_count; j++) {
            if (excluded_indices[j] == i) {
                excluded = 1;
                break;
            }
        }
        if (excluded) {
            continue;
        }
        if (reference_point == (real_point3d *)0) {
            return i;
        }
        {
            float dx = flags[i].position.x - reference_point->x;
            float dy = flags[i].position.y - reference_point->y;
            float dz = flags[i].position.z - reference_point->z;
            float dist = dy * dy + dz * dz + dx * dx;
            if (dist < best_distance) {
                best_distance = dist;
                best_index = i;
            }
        }
    }
    return best_index;
}

/**
 * Finds one valid starting location near an origin within the given horizontal and height limits.
 *
 * Original register convention: ECX -> type, EDX -> team, unaff_EBX -> origin, stack -> max_horizontal_dist,
 * max_height_delta.
 *
 * @address 0x461180
 */
int32_t SpawnLocations::find_one_valid_starting_location(int16_t type, int16_t team, real_point3d *origin, float max_horizontal_dist, float max_height_delta)
{
    int32_t result = -1;
    game_engine_find_valid_starting_locations(origin, max_horizontal_dist, max_height_delta,
        team, type, 1, &result);
    return result;
}

/**
 * Collects the valid starting locations of a team and type near an origin.
 *
 * Original register convention: unaff_EBX -> origin, stack -> max_horizontal_dist, max_height_delta, team,
 * type,.
 *
 * @address 0x461080
 */
int SpawnLocations::find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results)
{
    int32_t found = 0;
    int16_t i;
    ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;

    for (i = 0; i < (int32_t)global_scenario->netgame_flags.count; i++) {
        ScenarioNetgameFlags *f = &flags[i];

        if ((team == -1 || team == (int16_t)f->type) &&
            (type == -1 || type == (int16_t)f->usage_id) &&
            (origin == 0 ||
             ((max_horizontal_dist < 0.0f ||
               (origin->y - f->position.y) * (origin->y - f->position.y) +
               (origin->z - f->position.z) * (origin->z - f->position.z) +
               (origin->x - f->position.x) * (origin->x - f->position.x) <=
               max_horizontal_dist * max_horizontal_dist) &&
              (max_height_delta <= 0.0f ||
               (((f->position.z - origin->z) < 0.0f) ? -(f->position.z - origin->z)
                : (f->position.z - origin->z)) <= max_height_delta)))) {
            if (found < max_results) {
                results[found] = i;
                found = found + 1;
            }
        }
    }
    return found;
}

/**
 * Returns whether a vehicle blocks the given spawn point.
 *
 * Original register convention: EDX -> point.
 *
 * @address 0x461e60
 */
uint8_t SpawnLocations::location_blocked_by_vehicle(real_point3d *point)
{
    struct { int32_t leaf_index; int16_t cluster_index; } location;
    datum_index candidates[16];
    int16_t count;
    int16_t i;

    location.leaf_index = bsp3d_node_find_leaf(0, global_collision_bsp, point);
    if (location.leaf_index == -1) {
        location.cluster_index = -1;
    } else {
        location.cluster_index = (int16_t)((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[location.leaf_index & 0x7fffffff].cluster;
    }

    count = object_find_in_sphere(0, 0x11f, &location, point, 0.1f, candidates, 0x10);

    for (i = 0; i < count; i = i + 1) {
        object *obj = ((object_header *)object_data->data)[candidates[i] & 0xffff].data;

        if (obj != 0 && obj->type == 1) {
            return 1;
        }
    }
    return 0;
}

}
