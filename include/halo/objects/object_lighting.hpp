/**
 * @file include/halo/objects/object_lighting.hpp
 * Object system API: object lighting.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Lighting of objects: ambient sampling, light attachment enumeration and effect parameter blocks.
 */
class ObjectLighting {
public:
    explicit ObjectLighting(uint32_t handle) : handle(handle) {}

    /**
     * Returns the summed luminance of the lights attached to an object.
     *
     * Original register convention: stack -> object_index.
     *
     * @address 0x004f1b30
     */
    real sum_attached_light_luminance();

    /**
     * Samples the total lighting colour at a point in a BSP location.
     *
     * Original register convention: plain cdecl. // blam-cc: stack -> (point, location, color).
     *
     * @address 0x004f1c20
     */
    static void sample_total_lighting_at_point(real_point3d *point, bsp_leaf_reference *location,
    real_vector3d *color);

    /**
     * Samples the lightmap colour and base map colour at a point.
     *
     * Original register convention: plain cdecl, four stack parameters. // blam-cc: stack -> (point, lightmap_color,
     * base_map_color, wait_for_textures).
     *
     * @address 0x004f1e60
     */
    static void sample_ambient_lightmap_point(real_point3d *point, real_vector3d *lightmap_color,
    real_vector3d *base_map_color, uint8_t wait_for_textures);

    /**
     * Samples the ambient lighting around an object into the sample array.
     *
     * Original register convention: EAX -> object_index, stack -> sample.
     *
     * @address 0x004f20b0
     */
    void sample_ambient_lighting(float *sample);

    /**
     * Collects the lights that affect an object into the output list.
     *
     * @address 0x004f2430
     */
    void gather_light_list(uint8_t *out);

    /**
     * Fills a render_lighting record from a lightmap sample, shading normal and base map colour.
     *
     * Original register convention: stack -> flags, shading_normal, intensity; ECX -> lightmap_color, EAX ->
     * lightmap_normal, EDX -> base_map_color, ESI -> lighting.
     *
     * @address 0x004f2ff0
     */
    static void build_effect_parameter_block(uint8_t flags, real_vector3d *shading_normal, float intensity,
    ColorRGB *lightmap_color, real_vector3d *lightmap_normal, ColorRGB *base_map_color, render_lighting *lighting);

    /**
     * Scales a colour so that it does not exceed the given intensity.
     *
     * @address 0x004f3410
     */
    static void color_clamp_to_intensity(float intensity, ColorRGB *color);

    /**
     * Visits each light attachment of an object, optionally registering it in the table and invoking the callback.
     *
     * @address 0x004f9a20
     */
    void for_each_light_attachment(int32_t register_in_table, int32_t invoke_callback);

private:
    uint32_t handle;
};

}  // namespace halo::objects
