#pragma once

#include "halo/effects/types.hpp"

namespace halo::effects {

namespace {

/**
 * A view over a particle system record: per-type spawning and physics.
 */
class particle_system_view {
public:
    explicit particle_system_view(particle_system *value) : record(value) {}

    void creation_physics_default(int32_t type_index, particle_system_particle *particle, object_marker *marker);
    void creation_physics_explosion(int32_t type_index, particle_system_particle *particle, object_marker *marker);
    void creation_physics_jet(int32_t type_index, particle_system_particle *particle, object_marker *marker);
    void advance_type_state(particle_system_type_state *state, ParticleSystemType *type);
    void spawn(int32_t type_index, float dt);
    void update_physics_default(real dt);
    void update_physics_explosion(real dt);
    void update_physics_default(int16_t type_index, real dt, particle_system_particle *particle);

    particle_system *record;
};

/**
 * A particle system addressed by its datum index: creation, update, render and the module-wide passes.
 */
class particle_system_ref {
public:
    explicit particle_system_ref(datum_index value) : datum(value) {}

    static void advance_particle_state(particle_system_particle *particle, ParticleSystemType *type);
    void destroy();
    static datum_index new_at_point(uint32_t definition_index, real_point3d *position, real_vector3d *velocity, ColorARGB *color, float scale);
    static datum_index new_on_marker(uint32_t definition_index, uint32_t object_index, int16_t attachment_index);
    uint8_t new_type_states();
    void render();
    static void resolve_local_players();
    static void roll_particle_state(int16_t index, ParticleSystemTypeParticleState *states, particle_state_values *out);
    void update(float delta_time);
    static void delete_all();
    static void render_all();
    static void update_all(float delta_time);

    datum_index datum;
};

/**
 * A particle addressed by its datum index: animation, motion and impact handling.
 */
class particle_ref {
public:
    explicit particle_ref(datum_index value) : datum(value) {}

    uint8_t advance_animation(real delta_time);
    uint8_t advance_frame();
    real current_radius();
    void impact();
    static void impact_response_dispatch(particle *self, tag_group fourcc, datum_index definition_index, real intensity);
    static void create(particle_creation_data *creation_data);
    uint8_t next_sequence();
    uint8_t update_motion(real delta_time);
    static void delete_by_first_person_weapon(uint8_t first_person_weapon_index);
    static void refresh_structure_locations();
    static void update(real delta_time);

    datum_index datum;
};

}
}
