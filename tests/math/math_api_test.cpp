/**
 * @file tests/math/math_api_test.cpp
 * Checks the C++ API of the converted math module against its C ABI: every value-type member and operator, the
 * glm conversions and random_stream must produce exactly the bits of the C function they wrap. Linked with the
 * converted objects only; exit code 1 and a list of mismatches on any difference.
 */
#include "halo/math/math.hpp"
#include "halo/math/math_c_api.h"
#include "halo/math/glm_interop.hpp"
#include "math_test_stubs.h"

#include <cstdio>
#include <cstring>

namespace {

uint32_t g_rng = 0x9e3779b9u;
int g_failures = 0;
int g_checks = 0;

float rf()
{
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 17;
    g_rng ^= g_rng << 5;
    return (float)(g_rng & 0xffffff) / 16777216.0f * 8.0f - 4.0f;
}

template <typename T>
void same(const char *what, const T &a, const T &b)
{
    ++g_checks;
    if (std::memcmp(&a, &b, sizeof(T)) != 0) {
        if (++g_failures < 20) std::printf("MISMATCH %s\n", what);
    }
}

}  // namespace

int main()
{
    using namespace halo::math;
    for (int it = 0; it < 2000; ++it) {
        real_vector3d v = {rf(), rf(), rf()}, w = {rf(), rf(), rf()};
        real_point3d p = {rf(), rf(), rf()}, q = {rf(), rf(), rf()};
        real_quaternion qa = {rf(), rf(), rf(), rf()}, qb = {rf(), rf(), rf(), rf()};
        real_matrix4x3 a = real_matrix4x3::from_euler_angles(rf(), rf(), rf()), b = real_matrix4x3::from_euler_angles(rf(), rf(), rf());
        a.scale = (it & 1) ? 1.0f : rf();
        a.position = p;
        real_plane3d plane = {{rf(), rf(), rf()}, rf()};

        { real_vector3d t = v; same("magnitude", t.magnitude(), vector3d_length(&t)); }
        same("magnitude_squared", v.magnitude_squared(), vector3d_magnitude_squared(&v));
        { real_vector3d x = v, y = v; real lx = x.normalize(); real ly = vector3d_normalize_with_length(&y); same("normalize", lx, ly); same("normalize_v", x, y); }
        { real_point3d x = p, y = q; same("distance_to", x.distance_to(y), vector3d_distance(&x, &y)); same("distance_squared_to", x.distance_squared_to(y), vector3d_distance_squared(&x, &y)); }
        { real_plane3d o; plane3d_negate(&o, &plane); same("negated", plane.negated(), o); }
        { real_vector3d o; quaternion_rotate_vector(&qa, &v, &o); same("rotate", qa.rotate(v), o); }
        { real_matrix4x3 o; matrix4x3_from_quaternion(&qa, &o); same("to_matrix", qa.to_matrix(), o); }
        { real_quaternion o; quaternion_multiply(&qb, &qa, &o); same("quaternion*", qa * qb, o); }
        { real_quaternion x = qa, y = qa; x.normalize(); quaternion_normalize(&y); same("q.normalize", x, y); }
        { real_point3d o; matrix4x3_transform_point(&o, &p, &a); same("transform_point", a.transform_point(p), o); }
        { real_vector3d o; matrix4x3_transform_vector(&o, &v, &a); same("transform_vector", a.transform_vector(v), o); }
        { real_vector3d o; matrix4x3_transform_normal(&o, &v, &a); same("transform_normal", a.transform_normal(v), o); }
        { real_plane3d o; matrix4x3_transform_plane(&o, &a, &plane); same("transform_plane", a.transform_plane(plane), o); }
        { real_point3d o; matrix4x3_inverse_transform_point(&a, &o, &p); same("inverse_transform_point", a.inverse_transform_point(p), o); }
        { real_vector3d o; matrix4x3_inverse_transform_vector(&o, &v, &a); same("inverse_transform_vector", a.inverse_transform_vector(v), o); }
        { real_vector3d o; matrix4x3_inverse_transform_normal(&o, &v, &a); same("inverse_transform_normal", a.inverse_transform_normal(v), o); }
        { real_matrix4x3 o; matrix4x3_inverse(&o, &a); same("inverse", a.inverse(), o); }
        { real_matrix4x3 o; matrix4x3_multiply(&a, &b, &o); same("matrix4x3*", a * b, o); }
        { real_quaternion o; quaternion_from_matrix4x3(&a, &o); same("to_quaternion", a.to_quaternion(), o); }
        { real_matrix3x3 x = {a.forward, a.left, a.up}, y = {b.forward, b.left, b.up}, o;
          matrix3x3_multiply(&o, &x, &y); same("matrix3x3*", x * y, o);
          matrix3x3_transpose(&o, &x); same("transposed", x.transposed(), o);
          same("glm round trip 3x3", matrix3x3_from_glm(to_glm(x)), x); }
        { real_matrix4x3 o; real_vector3d up = a.up, fw = a.forward; matrix4x3_from_forward_up_position(&up, &fw, &p, &o);
          same("from_forward_up", real_matrix4x3::from_forward_up(fw, up, p), o); }
        { real_matrix4x3 o; float s = rf() / 4.0f, c = rf() / 4.0f; matrix4x3_from_axis_angle(&o, &v, s, c); same("from_axis_angle", real_matrix4x3::from_axis_angle(v, s, c), o); }
        same("glm vector", vector_from_glm(to_glm(v)), v);
        same("glm point", point_from_glm(to_glm(p)), p);
        same("glm quaternion", quaternion_from_glm(to_glm(qa)), qa);
        same("dot", v.dot(w), glm::dot(to_glm(v), to_glm(w)));
        same("cross", v.cross(w), vector_from_glm(glm::cross(to_glm(v), to_glm(w))));
        same("operator+", v + w, vector_from_glm(to_glm(v) + to_glm(w)));
        same("point-point", p - q, vector_from_glm(to_glm(p) - to_glm(q)));
        { random_seed s1 = g_rng, s2 = g_rng; real lo = rf(), hi = rf();
          same("random_stream", random_stream(s1).next_real(lo, hi), random_real_range_seeded(&s2, lo, hi)); same("random_stream seed", s1, s2); }
    }
    { random_seed_global = 1234; random_seed s = 1234; random_stream r(s); real x = ::random_real(); same("simulation_random", r.next_real(), x); }
    std::printf("%d checks, %d mismatches\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
