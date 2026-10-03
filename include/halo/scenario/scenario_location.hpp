/**
 * @file include/halo/scenario/scenario_location.hpp
 * Locations (structure bsp leaf plus cluster) and the water, fog and weather questions asked of them.
 */
#pragma once

#include "halo/scenario/scenario_types.hpp"

namespace halo::scenario {

/**
 * Non-owning view of a bsp_leaf_reference, the (leaf, cluster) pair that identifies where a point is in the resident
 * structure bsp.
 */
class location_view {
public:
    explicit location_view(bsp_leaf_reference *p) : self(p) {}

    /**
     * Resolves a point to a structure-bsp leaf and its cluster and writes both into the wrapped location. The cluster
     * index is -1 alongside a -1 leaf index when the point is outside all geometry.
     *
     * @address 0x53e780
     */
    void from_point(real_point3d *point);

    /**
     * Resolves the fog region that applies at the location. The cluster's fog word is -1 (no fog), a region index, or with
     * the top bit set the index of a fog plane whose front region applies only when the optional point is on the right
     * side of the plane.
     *
     * @address 0x53ec30
     */
    int16_t fog_region(real_point3d *point);

    /**
     * Signed distance from the point to the water surface plane of the location's fog, biased by the fog's own
     * distance_to_water_plane. Returns -FLT_MAX when the location has no water fog and +FLT_MAX when the cluster names a
     * region directly.
     *
     * @address 0x53ee00
     */
    float water_surface_distance(real_point3d *point);

    /**
     * True when the background sound of the location's cluster is a sound_looping tag with the deafening_to_ais flag set.
     * False when the cluster has no background sound, the index is out of range or the tag id is unset.
     *
     * @address 0x53e810
     */
    uint8_t background_sound_is_deafening_to_ais();

private:
    bsp_leaf_reference *self;
};

/**
 * Stateless queries over the resident scenario and structure bsp: cluster visibility, fog regions, weather and trigger
 * volumes. All state is read from the engine's fixed globals.
 */
struct scenario_query {
    /**
     * Resolves whether a location is in water and which weather palette entry applies there. The weather comes from the
     * location's fog region when it has a weather palette entry, otherwise from the cluster's own weather field;
     * weather_index_out may be NULL.
     *
     * @address 0x53ed60
     */
    static uint8_t location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out);

    /**
     * Resolves a fog region index to the tag id of the fog it names through the region's fog palette entry, or -1 when the
     * region is invalid or has no fog assigned.
     *
     * @address 0x53ed10
     */
    static uint32_t fog_region_resolve_tag(int16_t fog_region);

    /**
     * Tests whether column_cluster is visible from row_cluster in the resident structure bsp's cluster-by-cluster bit
     * matrix (one row of ceil(cluster count / 32) dwords per cluster).
     *
     * @address 0x53eb60
     */
    static uint8_t cluster_visibility_test(int16_t row_cluster, int16_t column_cluster);

    /**
     * Tests whether the point lies inside a scenario trigger volume: axis-aligned bounds for the fixed type, an
     * inverse-transformed extents test for the rotational type, false for any other type. Every comparison is written as a
     * negated strict test so NaN coordinates are rejected.
     *
     * @address 0x53f020
     */
    static uint8_t trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point);

    /**
     * Updates the per-local-player sky fog blend state and writes the render fog from it. The state snaps to the target
     * sky fog when it was not valid yet, the camera jumped 15 or more world units, or an opaque distance is 0; otherwise
     * every field blends toward the target at a rate scaled by the distance moved.
     *
     * @address 0x53e8c0
     */
    static void sky_fog_state_update(int16_t sky_index, int16_t local_player_index, real_point3d *camera_position, render_fog *out);

};

}  // namespace halo::scenario
