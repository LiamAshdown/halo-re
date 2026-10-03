#pragma once

#include "halo/effects/types.hpp"

namespace halo::effects {

namespace {

/**
 * A decal addressed by its datum index; static members cover placement and the module-wide passes.
 */
class decal_ref {
public:
    explicit decal_ref(datum_index value) : datum(value) {}

    static void build_projection(real_matrix4x3 *placement, real *box, decal_projection *out);
    static void clear_flags(uint8_t clear_object_attached);
    void destroy();
    static void evict_object_decals(int16_t cluster_index);
    static void flood_surfaces(decal_projection *projection, decal_flood_accumulator *accumulator, int32_t surface_index, uint8_t is_first_surface, real radius, int16_t decal_type, int32_t *surface_queue, uint16_t *surface_queue_count, int32_t *fallback_queue, uint16_t *fallback_queue_count);
    void link(int16_t cluster_index, int16_t layer);
    static datum_index create(datum_index requested_handle, int16_t cluster_index, int16_t layer, datum_index insert_before, uint8_t object_attached);
    static void place(datum_index decal_tag_index, collision_result *placement, real_vector3d *direction, real radius_scale, uint8_t object_attached, int16_t requested_sequence_index);
    static void rehash_object_decals();
    static void spawn_for_response(datum_index response_tag_index, uint8_t deterministic, real_point3d *origin, real_vector3d *direction, real radius, int32_t marker_index);
    void update_fade();
    static void detach_from_structure_bsp();
    static void initialize();
    static void update_fade_all();

    datum_index datum;
};

}
}
