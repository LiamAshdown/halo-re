/**
 * @file include/halo/objects/light_volume.hpp
 * Object system API: light volume.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Light volume widgets: datum table management and the render hooks.
 */
class LightVolumeSystem {
public:
    /**
     * Creates the light volume data array.
     *
     * Original register convention: none.
     *
     * @address 0x004fe680
     */
    static void initialize();

    /**
     * Disposes the light volume data array.
     *
     * Original register convention: none.
     *
     * @address 0x004fe6a0
     */
    static void dispose();

    /**
     * Clears the disposing flag of the light volume data array after a dispose pass.
     *
     * Original register convention: none.
     *
     * @address 0x004fe6c0
     */
    static void clear_disposing_flag();

    /**
     * Allocates a light volume datum for a LightVolume tag and returns its handle.
     *
     * Original register convention: stack -> definition_tag (cdecl).
     *
     * @address 0x004fe6d0
     */
    static datum_index create(datum_index definition_tag);

    /**
     * Frees a light volume datum.
     *
     * Original register convention: stack -> light_volume_index (cdecl).
     *
     * @address 0x004fe720
     */
    static void destroy(datum_index light_volume_index);

    /**
     * Widget render hook for a light volume.
     *
     * Original register convention: Ghidra shows a clean (param_1, param_2, param_3, param_4); param_3 is never read
     * anywhere in the body. Follows the same handle-validation shape as glow_render_dispatch.c's param_2.
     *
     * @address 0x004fe900
     */
    static void render(uint32_t object_index, datum_index light_volume_handle, uint32_t unused,
    uint8_t *function_context);

    /**
     * Builds and submits the sprite strip for a light volume, interpolating colours along its length.
     *
     * Original register convention: stack -> object_index, light_volume_handle.
     *
     * @address 0x004fea80
     */
    static void render_procedure(uint32_t object_index, datum_index light_volume_handle);
};

/**
 * Lightning widgets: datum table management and the render hook.
 */
class LightningSystem {
public:
    /**
     * Creates the lightning data array.
     *
     * Original register convention: none.
     *
     * @address 0x004fee80
     */
    static void initialize();

    /**
     * Disposes the lightning data array.
     *
     * Original register convention: none.
     *
     * @address 0x004feea0
     */
    static void dispose();

    /**
     * Clears the disposing flag of the lightning data array after a dispose pass.
     *
     * Original register convention: none.
     *
     * @address 0x004feec0
     */
    static void clear_disposing_flag();

    /**
     * Allocates a lightning datum for a Lightning tag and returns its handle.
     *
     * Original register convention: stack -> definition_tag (cdecl).
     *
     * @address 0x004feed0
     */
    static datum_index create(datum_index definition_tag);

    /**
     * Frees a lightning datum.
     *
     * Original register convention: stack -> lightning_index (cdecl).
     *
     * @address 0x004fef20
     */
    static void destroy(datum_index lightning_index);

    /**
     * Builds and submits the jittered lightning segments for a lightning widget.
     *
     * Original register convention: Ghidra shows a clean (param_1, param_2, param_3, param_4); param_3 is never read.
     * No implicit register inputs.
     *
     * @address 0x004ff010
     */
    static void render(uint32_t object_index, datum_index lightning_handle, uint32_t unused,
    int32_t *function_context);
};

}  // namespace halo::objects
