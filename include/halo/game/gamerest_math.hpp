#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"

namespace halo::game {

/**
 * Stateless scalar and vector helpers used by the control, camera and AI layers. All
 * float arithmetic is kept exactly as in the original so results stay bit-identical.
 */
class ScalarMath {
public:
    ScalarMath() = delete;

    static float angle_delta_wrapped(float from, float to);
    static real control_axis_sign(real value);
    static real distance_falloff_fraction(real value, real max_range);
    static void closest_point_on_segment(datum_index unit_index, real_vector3d *aux_vector, real_point3d *reference_point, real_point3d *out_closest);
};

/**
 * Non-owning view of a real_vector3d for in-place operations.
 */
class Vector3dView {
public:
    real_vector3d * v;

    explicit constexpr Vector3dView(real_vector3d * v_) : v(v_) {}

    void clamp_length(real max_length);
};

/**
 * Non-owning view of a float that is stepped toward a target value.
 */
class FloatValue {
public:
    float * value;

    explicit constexpr FloatValue(float * value_) : value(value_) {}

    void step_toward_target(float target, float max_step);
};

/**
 * Non-owning view of an array of 3D points.
 */
class Point3dArray {
public:
    real_point3d * source;

    explicit constexpr Point3dArray(real_point3d * source_) : source(source_) {}

    void project_to_xy_plane(Point2D *destination, int32_t count);
};

/**
 * Piecewise response-curve evaluation over a table of (x, y) pairs.
 */
class ResponseCurve {
public:
    ResponseCurve() = delete;

    static real evaluate(int16_t table_count, real x, real *table);
};

/**
 * Zoom field-of-view lookups for weapon zoom levels.
 */
class WeaponZoom {
public:
    WeaponZoom() = delete;

    static real get_zoom_fov(int16_t zoom_table_index, int16_t magnification);
    static real get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index);
};

/**
 * Random draws driven by tag reflexives and the global random table.
 */
class RandomTable {
public:
    RandomTable() = delete;

    static int32_t advance_draws(TagReflexive *reflexive);
    static void get_table_point(real_point3d *out);
    static int32_t pick_weighted_random_index(datum_index tag_id);
};

/**
 * Non-owning view of an object_placement_data record being prepared for object creation.
 */
class PlacementData {
public:
    object_placement_data * placement;

    explicit constexpr PlacementData(object_placement_data * placement_) : placement(placement_) {}

    void set_change_colors(real *color);
};

}  // namespace halo::game
