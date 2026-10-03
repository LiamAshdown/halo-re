/**
 * @file include/halo/objects/object_update.hpp
 * Object system API: object update.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Per-tick update of one object: bounding radius, exported functions, change colours and region permutations.
 */
class ObjectUpdater {
public:
    explicit ObjectUpdater(uint32_t handle) : handle(handle) {}

    /**
     * Locks or unlocks the object's region permutations.
     *
     * Original register convention: uint32_t object_index in EAX (in_EAX); a bool lock flag in the low byte.
     *
     * @address 0x004f03e0
     */
    void regions_reset_permutation_lock(int8_t unlock);

    /**
     * Selects a named permutation for the object's matching regions.
     *
     * @address 0x004f6c60
     */
    void set_permutation_by_name(char *name, int16_t region_filter, char use_matched_index);

    /**
     * Per-tick update of one object; returns nonzero when the object is still alive.
     *
     * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
     * "object_update(uint param_1)"), reused as EAX/EBX at various callee call sites within the body per each
     * callee's own established convention.
     *
     * @address 0x004f7ef0
     */
    uint8_t update();

    /**
     * Evaluates and stores the object's exported function values.
     *
     * Original register convention: stack -> object_index (cdecl).
     *
     * @address 0x004f80d0
     */
    void update_export_functions();

    /**
     * Evaluates one object function input selector, with the input angle and the object's tag data.
     *
     * @address 0x004f8207
     */
    static void function_evaluate_input(float initial_angle_input, float initial_st0, int16_t *selectors,
    float *out_values, object *object_record, int32_t object_index_scaled, int32_t remaining_count);

    /**
     * Recomputes the bounding radius of an object and its children.
     *
     * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
     * "object_recalculate_bounding_radius_recursive(uint param_1)").
     *
     * @address 0x004f82b0
     */
    void recalculate_bounding_radius_recursive();

    /**
     * Recomputes the object's bounding centre and radius from its model nodes.
     *
     * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
     * "object_recalculate_bounding_radius(uint param_1)").
     *
     * @address 0x004f8310
     */
    void recalculate_bounding_radius();

    /**
     * Initialises the object's change colours from the tag, clamping and blending as configured.
     *
     * Original register convention: EAX -> object_index, stack -> colors (cdecl).
     *
     * @address 0x004f8b70
     */
    void initialize_change_colors(ColorRGB *colors);

    /**
     * Finds the permutation group of a model region matching a group number.
     *
     * @address 0x004f8d80
     */
    static int16_t permutation_find_matching_group(ModelRegion *region, int16_t group, int16_t *out);

    /**
     * Chooses a permutation for each model region of the object from a probability group.
     *
     * @address 0x004f8dd0
     */
    uint8_t regions_initialize_permutations(int16_t group, GBXModel *model);

    /**
     * Returns the first permutation probability group of a model's regions.
     *
     * @address 0x004f8ef0
     */
    int16_t get_first_region_probability_group(GBXModel *model);

    /**
     * Re-applies the object's region permutations.
     *
     * Original register convention: object index in EBX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f8f5f
     * mov eax,ebx at entry with no stack access at all. // blam-cc: EBX -> object_index.
     *
     * @address 0x004f8f50
     */
    void refresh_region_permutations();

    /**
     * Updates the object's change colours for the tick.
     *
     * @address 0x004f9110
     */
    void update_change_colors();

    /**
     * Evaluates the object's function inputs and updates its node function blocks.
     *
     * Original register convention: object index in EAX. Consistent with every other single-register accessor in this
     * module and with this function's own Ghidra signature ("in_EAX" only). // blam-cc: EAX -> object_index.
     *
     * @address 0x004f92f0
     */
    void update_functions();

    /**
     * Raises value to exponent with the engine's input conventions and returns the result; used when evaluating
     * object function curves.
     *
     * Original register convention: two float stack parameters (Ghidra shows them cleanly).
     *
     * @address 0x004fea50
     */
    static float curve_apply_exponent(float value, float exponent);

private:
    uint32_t handle;
};

}  // namespace halo::objects
