// include/halo/math/math_types.hpp -- the math module's types for C++ code (namespace halo::math).
//
// There is exactly ONE definition of every math type: the C-compatible typedefs in types/math.h, which the
// unconverted (C-style) modules, the C-compiled engine globals in standalone/data/*.c and the tools keep including
// as before. This header includes that file, brings the names into halo::math, pins every layout with
// static_asserts, and adds the C++-only parts: scoped mirrors of the two function-selector enums and constexpr,
// operator and helper functions for NEW code.
//
// Rules for these types (docs/CPP_CONVENTIONS.md, "Structs"): they stay standard-layout, trivially copyable
// aggregates -- no constructors, no default member initialisers, no virtuals, no members of any kind -- because
// other modules brace-initialise them ({0}), memcpy them, and they live inside tag data, saved games and network
// packets. Every operation is a free function in halo::math.
#pragma once

#include "math.h"   // types/math.h (found through the `types` include directory, ahead of the CRT's <math.h>)

#include <cstddef>
#include <type_traits>

namespace halo::math {

// ---- the types, by their engine names --------------------------------------------------------------------------
using ::real;
using ::random_seed;
using ::large_integer;
using ::real_vector2d;
using ::real_point2d;
using ::real_vector3d;
using ::real_point3d;
using ::real_plane2d;
using ::real_plane3d;
using ::bounded_ramp_profile;
using ::real_quaternion;
using ::real_euler_angles2d;
using ::real_euler_angles3d;
using ::real_bounds;
using ::real_rectangle3d;
using ::real_matrix3x3;
using ::real_matrix4x3;
using ::sphere_mesh;
using ::sphere_mesh_edge_cache;
using ::sphere_mesh_face_cache;
using ::periodic_function_table;
using ::projection_axis_pair;
using ::periodic_function_t;
using ::transition_function_t;

// ---- layout: size, standard layout, trivially copyable, and the offset of every field ---------------------------
template <typename T>
inline constexpr bool is_engine_pod_v = std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T> &&
                                        std::is_trivially_default_constructible_v<T> && std::is_aggregate_v<T>;

#define HALO_MATH_LAYOUT(T, size) \
    static_assert(sizeof(T) == (size), #T " size changed"); \
    static_assert(is_engine_pod_v<T>, #T " must stay a standard-layout trivially copyable aggregate")

static_assert(sizeof(real) == 4 && std::is_same_v<real, float>, "real is a 32-bit float");
static_assert(sizeof(random_seed) == 4 && std::is_unsigned_v<random_seed>, "random_seed is a 32-bit unsigned LCG state");
HALO_MATH_LAYOUT(real_vector2d, 0x08);
HALO_MATH_LAYOUT(real_point2d, 0x08);
HALO_MATH_LAYOUT(real_vector3d, 0x0c);
HALO_MATH_LAYOUT(real_point3d, 0x0c);
HALO_MATH_LAYOUT(real_plane2d, 0x0c);
HALO_MATH_LAYOUT(real_plane3d, 0x10);
HALO_MATH_LAYOUT(bounded_ramp_profile, 0x20);
HALO_MATH_LAYOUT(real_quaternion, 0x10);
HALO_MATH_LAYOUT(real_euler_angles2d, 0x08);
HALO_MATH_LAYOUT(real_euler_angles3d, 0x0c);
HALO_MATH_LAYOUT(real_bounds, 0x08);
HALO_MATH_LAYOUT(real_rectangle3d, 0x18);
HALO_MATH_LAYOUT(real_matrix3x3, 0x24);
HALO_MATH_LAYOUT(real_matrix4x3, 0x34);
HALO_MATH_LAYOUT(sphere_mesh, 0x14);
HALO_MATH_LAYOUT(sphere_mesh_edge_cache, 0x80);
HALO_MATH_LAYOUT(sphere_mesh_face_cache, 0x02);
HALO_MATH_LAYOUT(periodic_function_table, 0x400);
HALO_MATH_LAYOUT(projection_axis_pair, 0x04);
static_assert(sizeof(large_integer) == 0x08 && std::is_standard_layout_v<large_integer> &&
              std::is_trivially_copyable_v<large_integer>, "large_integer layout changed");
static_assert(sizeof(periodic_function_t) == 2 && sizeof(transition_function_t) == 2, "selector width changed");
#undef HALO_MATH_LAYOUT

static_assert(offsetof(large_integer, parts.low_part) == 0x00 && offsetof(large_integer, parts.high_part) == 0x04);
static_assert(offsetof(real_vector2d, i) == 0x00 && offsetof(real_vector2d, j) == 0x04);
static_assert(offsetof(real_point2d, x) == 0x00 && offsetof(real_point2d, y) == 0x04);
static_assert(offsetof(real_vector3d, i) == 0x00 && offsetof(real_vector3d, j) == 0x04 && offsetof(real_vector3d, k) == 0x08);
static_assert(offsetof(real_point3d, x) == 0x00 && offsetof(real_point3d, y) == 0x04 && offsetof(real_point3d, z) == 0x08);
static_assert(offsetof(real_plane2d, normal) == 0x00 && offsetof(real_plane2d, d) == 0x08);
static_assert(offsetof(real_plane3d, normal) == 0x00 && offsetof(real_plane3d, d) == 0x0c);
static_assert(offsetof(bounded_ramp_profile, within_dead_zone) == 0x00 && offsetof(bounded_ramp_profile, unknown_01) == 0x01 &&
              offsetof(bounded_ramp_profile, start_position) == 0x04 && offsetof(bounded_ramp_profile, start_velocity) == 0x08 &&
              offsetof(bounded_ramp_profile, phase1_acceleration) == 0x0c && offsetof(bounded_ramp_profile, phase1_duration) == 0x10 &&
              offsetof(bounded_ramp_profile, phase2_duration) == 0x14 && offsetof(bounded_ramp_profile, phase3_acceleration) == 0x18 &&
              offsetof(bounded_ramp_profile, phase3_duration) == 0x1c);
static_assert(offsetof(real_quaternion, i) == 0x00 && offsetof(real_quaternion, j) == 0x04 &&
              offsetof(real_quaternion, k) == 0x08 && offsetof(real_quaternion, w) == 0x0c);
static_assert(offsetof(real_euler_angles2d, yaw) == 0x00 && offsetof(real_euler_angles2d, pitch) == 0x04);
static_assert(offsetof(real_euler_angles3d, yaw) == 0x00 && offsetof(real_euler_angles3d, pitch) == 0x04 &&
              offsetof(real_euler_angles3d, roll) == 0x08);
static_assert(offsetof(real_bounds, lower) == 0x00 && offsetof(real_bounds, upper) == 0x04);
static_assert(offsetof(real_rectangle3d, x) == 0x00 && offsetof(real_rectangle3d, y) == 0x08 && offsetof(real_rectangle3d, z) == 0x10);
static_assert(offsetof(real_matrix3x3, forward) == 0x00 && offsetof(real_matrix3x3, left) == 0x0c && offsetof(real_matrix3x3, up) == 0x18);
static_assert(offsetof(real_matrix4x3, scale) == 0x00 && offsetof(real_matrix4x3, forward) == 0x04 &&
              offsetof(real_matrix4x3, left) == 0x10 && offsetof(real_matrix4x3, up) == 0x1c &&
              offsetof(real_matrix4x3, position) == 0x28);
static_assert(offsetof(sphere_mesh, subdivisions) == 0x00 && offsetof(sphere_mesh, unknown_02) == 0x02 &&
              offsetof(sphere_mesh, points) == 0x04 && offsetof(sphere_mesh, indices) == 0x08 &&
              offsetof(sphere_mesh, point_count) == 0x0c && offsetof(sphere_mesh, triangle_count) == 0x0e &&
              offsetof(sphere_mesh, strip_count) == 0x10 && offsetof(sphere_mesh, unknown_12) == 0x12);
static_assert(offsetof(periodic_function_table, samples) == 0x00);
static_assert(offsetof(projection_axis_pair, i) == 0x00 && offsetof(projection_axis_pair, j) == 0x02);

// ---- scoped selectors ------------------------------------------------------------------------------------------
// The C enums periodic_function / transition_function (types/math.h) stay unscoped: other modules use their
// enumerators as plain integers (e.g. src/effects, src/shaders) and store the selector as an int16_t
// (periodic_function_t / transition_function_t). These are the C++ API's scoped mirrors; the values are pinned to
// the C enumerators, and the C wrappers convert with static_cast.
enum class periodic_function_type : int16_t {
    one = _periodic_function_one,
    zero = _periodic_function_zero,
    cosine = _periodic_function_cosine,
    cosine_variable_period = _periodic_function_cosine_variable_period,
    diagonal_wave = _periodic_function_diagonal_wave,
    diagonal_wave_variable_period = _periodic_function_diagonal_wave_variable_period,
    slide = _periodic_function_slide,
    slide_variable_period = _periodic_function_slide_variable_period,
    noise = _periodic_function_noise,
    jitter = _periodic_function_jitter,
    wander = _periodic_function_wander,
    spark = _periodic_function_spark,
};
inline constexpr int16_t k_periodic_function_type_count = k_periodic_function_count;

enum class transition_function_type : int16_t {
    linear = _transition_function_linear,
    early = _transition_function_early,
    very_early = _transition_function_very_early,
    late = _transition_function_late,
    very_late = _transition_function_very_late,
    cosine = _transition_function_cosine,
};
inline constexpr int16_t k_transition_function_type_count = k_transition_function_count;

static_assert(sizeof(periodic_function_type) == sizeof(periodic_function_t));
static_assert(sizeof(transition_function_type) == sizeof(transition_function_t));
static_assert(static_cast<int>(periodic_function_type::spark) + 1 == k_periodic_function_count);
static_assert(static_cast<int>(transition_function_type::cosine) + 1 == k_transition_function_count);

// ---- helpers for NEW code ----------------------------------------------------------------------------------------
// Each helper below fixes ONE evaluation order, written next to it. The converted engine functions do NOT use
// them: almost every one of those has its own term order (the order the original x87 code summed in), and float
// addition is not associative. Replacing an engine expression with one of these helpers is a behaviour change that
// needs the module's difftest (tests/math) to pass first. For the same reason there is no operator* for dot/cross.

constexpr real_vector3d operator+(const real_vector3d &a, const real_vector3d &b) { return {a.i + b.i, a.j + b.j, a.k + b.k}; }
constexpr real_vector3d operator-(const real_vector3d &a, const real_vector3d &b) { return {a.i - b.i, a.j - b.j, a.k - b.k}; }
constexpr real_vector3d operator-(const real_vector3d &v) { return {-v.i, -v.j, -v.k}; }
constexpr real_vector3d operator*(const real_vector3d &v, real s) { return {v.i * s, v.j * s, v.k * s}; }
constexpr real_vector3d operator*(real s, const real_vector3d &v) { return {s * v.i, s * v.j, s * v.k}; }
constexpr real_vector3d &operator+=(real_vector3d &a, const real_vector3d &b) { a.i += b.i; a.j += b.j; a.k += b.k; return a; }
constexpr real_vector3d &operator-=(real_vector3d &a, const real_vector3d &b) { a.i -= b.i; a.j -= b.j; a.k -= b.k; return a; }
constexpr real_vector3d &operator*=(real_vector3d &v, real s) { v.i *= s; v.j *= s; v.k *= s; return v; }

constexpr real_point3d operator+(const real_point3d &p, const real_vector3d &v) { return {p.x + v.i, p.y + v.j, p.z + v.k}; }
constexpr real_point3d operator-(const real_point3d &p, const real_vector3d &v) { return {p.x - v.i, p.y - v.j, p.z - v.k}; }
constexpr real_vector3d operator-(const real_point3d &a, const real_point3d &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
constexpr real_point3d &operator+=(real_point3d &p, const real_vector3d &v) { p.x += v.i; p.y += v.j; p.z += v.k; return p; }

constexpr real_vector2d operator+(const real_vector2d &a, const real_vector2d &b) { return {a.i + b.i, a.j + b.j}; }
constexpr real_vector2d operator-(const real_vector2d &a, const real_vector2d &b) { return {a.i - b.i, a.j - b.j}; }
constexpr real_vector2d operator*(const real_vector2d &v, real s) { return {v.i * s, v.j * s}; }
constexpr real_vector2d operator-(const real_point2d &a, const real_point2d &b) { return {a.x - b.x, a.y - b.y}; }

// (a.i*b.i + a.j*b.j) + a.k*b.k, left to right
constexpr real dot_product(const real_vector3d &a, const real_vector3d &b) { return a.i * b.i + a.j * b.j + a.k * b.k; }
// a.i*b.i + a.j*b.j
constexpr real dot_product(const real_vector2d &a, const real_vector2d &b) { return a.i * b.i + a.j * b.j; }
// (j1*k2 - k1*j2, k1*i2 - i1*k2, i1*j2 - j1*i2), each component one product minus another
constexpr real_vector3d cross_product(const real_vector3d &a, const real_vector3d &b)
{
    return {a.j * b.k - a.k * b.j, a.k * b.i - a.i * b.k, a.i * b.j - a.j * b.i};
}
// dot_product(v, v), same order
constexpr real magnitude_squared(const real_vector3d &v) { return dot_product(v, v); }
// a.i*b.i + a.j*b.j + a.k*b.k on the normal, then minus d: the signed distance of a point from a plane
constexpr real plane_distance(const real_plane3d &plane, const real_point3d &p)
{
    return plane.normal.i * p.x + plane.normal.j * p.y + plane.normal.k * p.z - plane.d;
}
constexpr real_vector3d as_vector(const real_point3d &p) { return {p.x, p.y, p.z}; }
constexpr real_point3d as_point(const real_vector3d &v) { return {v.i, v.j, v.k}; }

}  // namespace halo::math
