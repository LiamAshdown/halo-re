#pragma once

#include "halo/effects/types.hpp"

namespace halo::effects {

namespace {

/**
 * A contrail addressed by its datum index; static members cover the module-wide passes.
 */
class contrail_ref {
public:
    explicit contrail_ref(datum_index value) : datum(value) {}

    void advance(uint8_t detach, real delta_time);
    void age_points(real delta_time);
    void destroy();
    void generate_points(int16_t point_count, uint8_t force);
    static datum_index create(int16_t attachment_index, datum_index object_index, datum_index definition_index);
    static void next_sequence(contrail *self);
    int16_t points_due(real elapsed_time);
    static void refresh_lightmap();
    static void update(real delta_time);
    static void initialize();

    datum_index datum;
};

}
}
