/**
 * @file include/halo/objects/antenna.hpp
 * Object system API: antenna.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Antenna widgets: datum table management and per-tick update.
 */
class AntennaSystem {
public:
    /**
     * Creates the antenna data array with room for twelve antennas.
     *
     * Original register convention: no parameters.
     *
     * @address 0x004faa20
     */
    static void initialize();

    /**
     * Disposes the antenna data array.
     *
     * Original register convention: no parameters.
     *
     * @address 0x004faa40
     */
    static void dispose();

    /**
     * Clears the disposing flag of the antenna data array after a dispose pass.
     *
     * Original register convention: no parameters.
     *
     * @address 0x004faa60
     */
    static void clear_disposing_flag();

    /**
     * Re-reads the antenna data array pointer from the game state after a state restore.
     *
     * Original register convention: no parameters.
     *
     * @address 0x004faa70
     */
    static void reset_data_pointer();

    /**
     * Allocates an antenna datum for an Antenna tag, initialising its vertex chain, and returns its handle.
     *
     * Original register convention: antenna tag as the sole parameter, per Ghidra's own "antenna_new(uint param_1)"
     * with no in_REG markers.
     *
     * @address 0x004faa90
     */
    static datum_index create(datum_index antenna_tag);

    /**
     * Frees an antenna datum from the antenna data array.
     *
     * Original register convention: stack -> antenna_index (cdecl).
     *
     * @address 0x004fac80
     */
    static void destroy(datum_index antenna_index);

    /**
     * Widget render hook for an antenna: looks up the owning object and the antenna datum and submits its geometry.
     *
     * Original register convention: stack -> object_index, antenna_index (cdecl).
     *
     * @address 0x004fac90
     */
    static void render_callback(datum_index object_index, datum_index antenna_index);

    /**
     * Runs the per-tick update of every live antenna.
     *
     * Original register convention: dt is the sole stack parameter (Ghidra's own "antennas_update(float param_1)").
     *
     * @address 0x004fad20
     */
    static void update(float dt);

    /**
     * Adds a randomised displacement scaled by the amplitude to the antenna tip position, transformed by the supplied
     * matrix.
     *
     * @address 0x004fef40
     */
    static void tip_jitter(real_vector3d *amplitude, real_point3d *position, real_matrix4x3 *m);
};

/**
 * One antenna widget instance: a chain of damped vertices driven by the attached marker.
 */
class AntennaView {
public:
    explicit AntennaView(antenna *self) : self(self) {}

    /**
     * Advances the damped spring simulation of the antenna vertices by dt seconds against the supplied Antenna tag.
     *
     * @address 0x004fae10
     */
    void update_physics(Antenna *antenna_tag, float dt);

    /**
     * Applies the movement of the antenna's attached marker since the last tick to the antenna chain, producing the
     * forward vector and base position the vertex simulation starts from.
     *
     * @address 0x004fb1c0
     */
    void apply_marker_delta(real_vector3d *out_forward, real_point3d *out_position, Antenna *antenna_tag,
    bsp_leaf_reference *node_ref);

    /**
     * Builds the camera-facing sprite geometry for the antenna chain from its tag vertices and current simulated
     * positions.
     *
     * Original register convention: EDI -> antenna_tag, stack -> ant.
     *
     * @address 0x004fb340
     */
    void render_geometry(Antenna *antenna_tag);

    /**
     * Builds the wire segments between antenna vertices as sprites. Unreferenced in the retail binary and kept as a
     * literal transliteration; its signature is a best-effort reconstruction.
     *
     * Original register convention: UNSURE, see file header; this signature is a best-effort reconstruction with no.
     *
     * @address 0x004fb3e0
     */
    void render_wire(uint32_t widget_flags, float scale, Antenna *antenna_tag);

private:
    antenna *self;
};

}  // namespace halo::objects
