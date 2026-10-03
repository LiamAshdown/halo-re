#pragma once

#include "halo/effects/types.hpp"

namespace halo::effects {

namespace {

/**
 * One weather instance slot addressed by its index.
 */
class weather_instance_ref {
public:
    explicit weather_instance_ref(int16_t value) : slot(value) {}

    void activate(datum_index definition_index, real intensity);
    void adjust_count(int16_t type_index, real target_value);
    void build_render_geometry();
    void deactivate();
    void update();

    int16_t slot;
};

/**
 * A weather particle addressed by its datum index.
 */
class weather_particle_ref {
public:
    explicit weather_particle_ref(datum_index value) : datum(value) {}

    static datum_index create(int16_t instance_index, int16_t type_index);
    void update(int16_t type_index, int16_t instance_index);

    datum_index datum;
};

/**
 * Module-wide weather passes.
 */
class weather_system {
public:
    static void update();
    static void update_local_player();
};

}
}
