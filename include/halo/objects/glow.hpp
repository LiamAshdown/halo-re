/**
 * @file include/halo/objects/glow.hpp
 * Object system API: glow.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Glow widgets: datum table management, rendering and particle allocation.
 */
class GlowSystem {
public:
    /**
     * Creates the glow and glow particle data arrays.
     *
     * Original register convention: none.
     *
     * @address 0x004fcbb0
     */
    static void initialize();

    /**
     * Disposes the glow data arrays.
     *
     * Original register convention: none.
     *
     * @address 0x004fcc00
     */
    static void dispose();

    /**
     * Clears the disposing flag of the glow data arrays after a dispose pass.
     *
     * Original register convention: none.
     *
     * @address 0x004fcc30
     */
    static void clear_disposing_flag();

    /**
     * Allocates a glow datum for a Glow tag, spawns its particles and returns its handle.
     *
     * Original register convention: stack -> glow_tag (cdecl).
     *
     * @address 0x004fcc50
     */
    static datum_index create(datum_index glow_tag);

    /**
     * Frees a glow datum and its particles.
     *
     * Original register convention: stack -> glow_index (cdecl).
     *
     * @address 0x004fcd40
     */
    static void destroy(datum_index glow_index);

    /**
     * Widget render hook for a glow: resolves the owning object and forwards to the glow renderer.
     *
     * @address 0x004fcdb0
     */
    static void render_dispatch(uint32_t object_index, datum_index glow_handle);

    /**
     * Allocates a glow particle datum from the particle data array.
     *
     * Original register convention: none (no parameters); return value only.
     *
     * @address 0x004fdde0
     */
    static glow_particle * particle_datum_new();

    /**
     * Renders the glow identified by the handle.
     *
     * Original register convention: `in_ECX` unresolved by Ghidra; ECX -> glow_handle per the call site.
     *
     * @address 0x004fe570
     */
    static void render(datum_index glow_handle);
};

/**
 * One glow widget instance: a ring of particles around an object marker.
 */
class GlowView {
public:
    explicit GlowView(glow *self) : self(self) {}

    /**
     * Per-tick update of a glow instance attached to an object.
     *
     * Original register convention: object_index is this function's one Ghidra-recognized parameter; the glow
     * instance pointer arrives in EDI, confirmed by disassembling glow_render_dispatch.c's call site (`mov edi,eax;
     * call 0x4fce80`).
     *
     * @address 0x004fce80
     */
    void update(uint32_t object_index);

    /**
     * Advances one particle's animation phase by the given rate and respawns or repositions it as needed.
     *
     * @address 0x004fd650
     */
    void particle_advance_time(uint32_t object_index, glow_particle *particle, float rate);

    /**
     * Links the particles of a glow instance into its render chain.
     *
     * Original register convention: Ghidra shows a single `int in_EAX` with no other implicit inputs; by analogy with
     * glow_particle_compute_fade.c's EAX=entry convention, EAX is the glow entry.
     *
     * @address 0x004fd830
     */
    void chain_build();

    /**
     * Creates the particle at the given index of a glow with randomised parameters.
     *
     * Original register convention: Ghidra shows a clean (short param_1, short param_2) plus one unresolved
     * `unaff_EDI`; by the same reasoning as every other function in this file group, EDI is the glow entry.
     *
     * @address 0x004fd8e0
     */
    glow_particle * particle_new(int16_t index, int16_t count);

    /**
     * Allocates and initialises a fresh particle for a glow instance.
     *
     * Original register convention: Ghidra shows a single unresolved `unaff_EBX`; by the same +0x224 anchor as every
     * sibling function, EBX is the glow entry.
     *
     * @address 0x004fdb20
     */
    glow_particle * particle_spawn();

    /**
     * Moves a particle to a new randomised location around its marker, advancing its phase by phase_rate.
     *
     * @address 0x004fde40
     */
    void particle_reposition(glow_particle *particle, float phase_rate);

private:
    glow *self;
};

/**
 * One glow particle.
 */
class GlowParticleView {
public:
    explicit GlowParticleView(glow_particle *self) : self(self) {}

    /**
     * Computes the fade factor of a glow particle from its phase.
     *
     * @address 0x004fd3a0
     */
    void compute_fade(glow *entry);

    /**
     * Computes the colour of a glow particle from the glow's tag settings and its current phase.
     *
     * Original register convention: same EAX=entry/ECX=particle shape as glow_particle_compute_fade.c (0x4fd3a0),
     * which this function is the direct color-fade companion of.
     *
     * @address 0x004fd420
     */
    void compute_color(glow *entry);

    /**
     * Computes the world position of a glow particle from the owning object's marker and the particle's offset.
     *
     * @address 0x004fd4a0
     */
    void compute_position(uint32_t object_index, glow *entry);

private:
    glow_particle *self;
};

}  // namespace halo::objects
