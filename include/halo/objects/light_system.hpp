/**
 * @file include/halo/objects/light_system.hpp
 * Object system API: light system.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * The light data array, attached and positioned lights, the transient light table and per-cluster light lists.
 */
class LightSystem {
public:
    /**
     * Creates the light data array, the cluster tables and the light reference arrays.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004f0a20
     */
    static void initialize();

    /**
     * Disposes the light data array and the per-cluster light tables.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004f0aa0
     */
    static void dispose_all();

    /**
     * Creates a light attached to an object marker and returns its handle.
     *
     * Original register convention: datum_index light_tag in EAX (param_1); datum_index owner_object on the.
     *
     * @address 0x004f0af0
     */
    static datum_index new_attached(datum_index light_tag, datum_index owner_object, int16_t marker_index,
    int16_t marker_index_secondary, int16_t change_color_index);

    /**
     * Removes a light from its cluster lists and frees its datum.
     *
     * Original register convention: light handle in ESI.
     *
     * @address 0x004f0bd0
     */
    static void destroy(datum_index light_handle);

    /**
     * Creates a light at a fixed position and direction and returns its handle.
     *
     * Original register convention: datum_index light_tag on the stack (param_1); int32_t marker_index (-1.
     *
     * @address 0x004f0c10
     */
    static datum_index new_positioned(datum_index light_tag, int32_t marker_index, int16_t marker_sub_index,
    real_point3d *position, uint32_t param_5, real_vector3d *direction);

    /**
     * Per-frame update of all lights: lifetimes, owner transforms and cluster membership.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004f0cf0
     */
    static void update_all();

    /**
     * Adds a one-frame light with the given colour, position and intensity to the transient light table; refuses when
     * it is full.
     *
     * Original register convention: datum_index light_tag in EAX (in_EAX); real_vector3d *color in EDX.
     *
     * @address 0x004f1600
     */
    static void transient_add(datum_index light_tag, real_vector3d *color, real_point3d *position, uint32_t direction,
    uint32_t param_3, float intensity);

    /**
     * Copies the handles of the objects a light references into out_buffer, up to max_count, and returns the count.
     *
     * Original register convention: uint32_t light_handle in ECX (in_ECX); int16_t max_count in SI.
     *
     * @address 0x004f1700
     */
    static int16_t collect_object_references(uint32_t light_handle, int16_t max_count, int16_t *out_buffer);

    /**
     * Applies spot cone falloff to the active light list.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004f1780
     */
    static void apply_spot_falloff();

    /**
     * Applies spot cone falloff to the active light list for the specular pass.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004f1950
     */
    static void apply_spot_falloff_specular();

    /**
     * Clears the dirty flag of an attached light.
     *
     * Original register convention: light index in EAX (in_EAX).
     *
     * @address 0x004f29c0
     */
    static void clear_dirty_flag(uint32_t light_index);

    /**
     * Recomputes the world transform of an attached light from its owner's marker.
     *
     * Original register convention: light index as the single stack argument ([esp+0x8c] after the prologue's sub
     * esp,0x84 and three pushes).
     *
     * @address 0x004f2a00
     */
    static void recompute_transform(uint32_t light_index);

    /**
     * Removes every light from the structure BSP cluster lists.
     *
     * Original register convention: no arguments.
     *
     * @address 0x004f2cb0
     */
    static void detach_from_structure_bsp();

    /**
     * Refreshes the transforms of every attached light.
     *
     * Original register convention: none (void).
     *
     * @address 0x004f2d50
     */
    static void refresh_transforms();

    /**
     * Gathers the nearest lights to a probe point in a cluster with their intensities and falloffs.
     *
     * Original register convention: cluster/hash index in AX (in_AX), then the eight stack parameters shown by Ghidra
     * unchanged.
     *
     * @address 0x004f2df0
     */
    static void gather_nearest(int16_t cluster_index, uint32_t self_object_index, real_point3d *probe_point,
    float search_margin, uint32_t *out_indices, float *out_intensities, uint32_t out_falloffs, int16_t *count,
    int16_t max_count);

    /**
     * Starts iterating the lights registered in a BSP cluster and returns the first light handle.
     *
     * Original register convention: stack -> cursor, cluster_index (cdecl).
     *
     * @address 0x004f34c0
     */
    static datum_index cluster_iterate_begin(datum_index *cursor, int16_t cluster_index);

    /**
     * Advances the light cluster cursor and returns the next light handle.
     *
     * Original register convention: stack -> cursor (cdecl).
     *
     * @address 0x004f3500
     */
    static datum_index cluster_iterate_next(datum_index *cursor);

    /**
     * Returns the centre and radius of the bounding sphere used when rendering a light.
     *
     * Original register convention: stack -> handle, center_out, radius_out (cdecl).
     *
     * @address 0x004f3530
     */
    static void get_render_bounds(datum_index handle, real_point3d *center_out, float *radius_out);

    /**
     * Returns nonzero when the light has not been marked this frame.
     *
     * Original register convention: stack -> handle (cdecl); returns AL.
     *
     * @address 0x004f3620
     */
    static uint8_t not_marked_this_frame(datum_index handle);

    /**
     * Marks a light as seen this frame and returns its previous mark state.
     *
     * Original register convention: stack -> handle (cdecl); returns AL.
     *
     * @address 0x004f3650
     */
    static uint8_t mark_this_frame(datum_index handle);
};

}  // namespace halo::objects
