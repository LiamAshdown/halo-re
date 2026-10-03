/**
 * @file include/halo/math/glm_interop.hpp
 * glm (third_party/glm, 1.0.1, header-only, pure scalar build) for halo::math, and the conversions between the
 * engine's stored types and glm's. The stored types keep their layout; the conversions copy (std::bit_cast where the
 * memory layout is identical, so the compiler can elide them).
 *
 * Include this header only from .cpp files: glm pulls in <cmath>, whose float overloads (sqrtf, fabsf, ...) then
 * compete with the double CRT functions the engine code calls. Every CRT call in a file that includes it must pass
 * an explicit double argument so the original function is still the one called.
 */
#pragma once

// types/ shadows <math.h> on the include path, so the CRT declarations <cmath> expects must already be present.
#include <corecrt_math.h>

#ifndef GLM_FORCE_PURE
#define GLM_FORCE_PURE
#endif
#ifndef GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_XYZW_ONLY
#endif
#ifndef GLM_FORCE_SILENT_WARNINGS
#define GLM_FORCE_SILENT_WARNINGS
#endif
// glm's index asserts compile to _wassert calls carrying absolute source paths in a build without NDEBUG (this
// one): parse glm with assert disabled, then restore assert for the including file.
#pragma push_macro("NDEBUG")
#ifndef NDEBUG
#define NDEBUG
#endif
#include <assert.h>
#include "../../../third_party/glm/glm/glm.hpp"
#include "../../../third_party/glm/glm/gtc/quaternion.hpp"
#pragma pop_macro("NDEBUG")
#include <assert.h>

#include <bit>

#include "halo/math/math_types.hpp"

namespace halo::math {

static_assert(sizeof(glm::vec3) == sizeof(real_vector3d) && sizeof(glm::vec2) == sizeof(real_vector2d));
static_assert(sizeof(glm::mat3) == sizeof(real_matrix3x3) && std::is_trivially_copyable_v<glm::mat3>);

/** Converts a vector to glm (same three floats, i/j/k as x/y/z). */
inline glm::vec3 to_glm(const real_vector3d &v) { return std::bit_cast<glm::vec3>(v); }

/** Converts a point to a glm position vector (x/y/z). */
inline glm::vec3 to_glm(const real_point3d &p) { return std::bit_cast<glm::vec3>(p); }

/** Converts a 2D vector to glm. */
inline glm::vec2 to_glm(const real_vector2d &v) { return std::bit_cast<glm::vec2>(v); }

/**
 * Converts a 3x3 basis to a glm matrix whose columns are forward, left and up. real_matrix3x3 stores the three basis
 * vectors one after another, which is exactly glm's column-major layout, so m * v is forward*i + left*j + up*k.
 */
inline glm::mat3 to_glm(const real_matrix3x3 &m) { return std::bit_cast<glm::mat3>(m); }

/** The rotation of a matrix4x3 as a glm matrix with columns forward, left and up (scale and position not included). */
inline glm::mat3 rotation_to_glm(const real_matrix4x3 &m) { return glm::mat3(to_glm(m.forward), to_glm(m.left), to_glm(m.up)); }

/** The full transform of a matrix4x3 as an affine glm::mat4: rotation scaled by `scale`, translation `position`. */
inline glm::mat4 to_glm_affine(const real_matrix4x3 &m)
{
    glm::mat4 r(rotation_to_glm(m) * m.scale);
    r[3] = glm::vec4(to_glm(m.position), 1.0f);
    return r;
}

/** Converts a quaternion to glm (glm::quat stores w separately; values are copied, not reinterpreted). */
inline glm::quat to_glm(const real_quaternion &q) { return glm::quat::wxyz(q.w, q.i, q.j, q.k); }

/** Converts a glm vector back to an engine vector. */
inline real_vector3d vector_from_glm(const glm::vec3 &v) { return std::bit_cast<real_vector3d>(v); }

/** Converts a glm vector back to an engine point. */
inline real_point3d point_from_glm(const glm::vec3 &v) { return std::bit_cast<real_point3d>(v); }

/** Converts a glm 2D vector back to an engine vector. */
inline real_vector2d vector_from_glm(const glm::vec2 &v) { return std::bit_cast<real_vector2d>(v); }

/** Converts a glm matrix (columns forward, left, up) back to a 3x3 basis. */
inline real_matrix3x3 matrix3x3_from_glm(const glm::mat3 &m) { return std::bit_cast<real_matrix3x3>(m); }

/** Builds a matrix4x3 from a glm rotation (columns forward, left, up), a uniform scale and a position. */
inline real_matrix4x3 matrix4x3_from_glm(const glm::mat3 &rotation, real scale, const glm::vec3 &position)
{
    return {scale, vector_from_glm(rotation[0]), vector_from_glm(rotation[1]), vector_from_glm(rotation[2]),
            point_from_glm(position)};
}

/** Converts a glm quaternion back to the engine layout (i, j, k, w). */
inline real_quaternion quaternion_from_glm(const glm::quat &q) { return {q.x, q.y, q.z, q.w}; }

}  // namespace halo::math
