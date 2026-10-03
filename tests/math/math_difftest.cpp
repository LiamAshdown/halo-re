/**
 * @file tests/math/math_difftest.cpp
 * Behaviour-equivalence driver for the math module. Calls every exported C function of the module through its C
 * symbol (halo/math/math_c_api.h) with fixed-seed pseudo-random inputs plus edge cases (signed zeros, denormals,
 * values at the 0.0001 tolerance, aliased in/out buffers) and writes every output as raw 32-bit hex. The same object
 * is linked once against the original src/math objects and once against the converted ones (run_difftest.py); the
 * two output files must be byte-for-byte identical.
 */
#include "halo/math/math_c_api.h"
#include "math_test_stubs.h"

#include <cstdio>
#include <cstring>

extern "C" double sqrt(double x);
extern "C" void srand(unsigned int seed);
extern "C" int __stdcall GlobalFree(void *memory);

namespace {

FILE *g_out = nullptr;
uint32_t g_rng = 0x2545f491u;
int g_calls = 0;

uint32_t rnd()
{
    uint32_t x = g_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return g_rng = x;
}

float from_bits(uint32_t u)
{
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

float unit_random() { return (float)(rnd() & 0xffffff) / 16777216.0f; }

/** A float from a mix of ranges and edge cases; never NaN or infinite. */
float rf()
{
    uint32_t r = rnd() % 100;
    if (r < 55) return unit_random() * 16.0f - 8.0f;
    if (r < 67) return unit_random() * 2.0f - 1.0f;
    if (r < 72) return (unit_random() - 0.5f) * 2e-3f;
    if (r < 76) return (rnd() & 1) ? 0.0f : -0.0f;
    if (r < 80) return (rnd() & 1) ? 1.0f : -1.0f;
    if (r < 84) return from_bits((rnd() & 0x007fffffu) | (rnd() & 0x80000000u));
    if (r < 88) return ((rnd() & 1) ? 1.0f : -1.0f) * (float)(rnd() % 1000000);
    if (r < 92) return ((rnd() & 1) ? 1e-4f : -1e-4f) * (1.0f + (float)((int)(rnd() % 3) - 1) * 1e-6f);
    return (float)(int)(rnd() % 17) - 8.0f;
}

float rf_range(float lo, float hi) { return lo + (hi - lo) * unit_random(); }

real_vector3d rv()
{
    real_vector3d v = {rf(), rf(), rf()};
    if (rnd() % 10 == 0) v = {0.0f, 0.0f, 0.0f};
    return v;
}

real_vector3d unit_v()
{
    real_vector3d v = {rf_range(-1, 1), rf_range(-1, 1), rf_range(-1, 1)};
    float l = (float)sqrt((double)(v.i * v.i + v.j * v.j + v.k * v.k));
    if (l < 1e-3f) return {0.0f, 0.0f, 1.0f};
    return {v.i / l, v.j / l, v.k / l};
}

real_point3d rp()
{
    real_point3d p = {rf(), rf(), rf()};
    return p;
}

real_vector2d rv2() { return {rf(), rf()}; }
real_point2d rp2() { return {rf(), rf()}; }
real_point2d rp2_small() { return {rf_range(-4, 4), rf_range(-4, 4)}; }
real_quaternion rq() { return {rf_range(-1, 1), rf_range(-1, 1), rf_range(-1, 1), rf_range(-1, 1)}; }
real_plane3d rplane()
{
    real_vector3d n = (rnd() % 4) ? unit_v() : rv();
    return {n, rf()};
}

real_matrix4x3 rm4()
{
    real_matrix4x3 m;
    uint32_t kind = rnd() % 5;
    if (kind < 3) {
        matrix4x3_from_euler_angles(&m, rf_range(-4, 4), rf_range(-2, 2), rf_range(-4, 4));
        m.scale = kind == 0 ? 1.0f : rf_range(0.1f, 3.0f);
        m.position = rp();
    } else {
        float *f = (float *)&m;
        for (int i = 0; i < 13; i++) f[i] = rf();
        if (kind == 4) m.scale = 0.0f;
    }
    return m;
}

real_matrix3x3 rm3()
{
    real_matrix4x3 m = rm4();
    real_matrix3x3 r = {m.forward, m.left, m.up};
    return r;
}

void begin(const char *name, int it)
{
    std::fprintf(g_out, "%s %d:", name, it);
    ++g_calls;
}

void h(float f)
{
    uint32_t u;
    std::memcpy(&u, &f, 4);
    std::fprintf(g_out, " %08x", u);
}

void hu(uint32_t v) { std::fprintf(g_out, " %08x", v); }

template <typename T>
void hs(const T &s)
{
    static_assert(sizeof(T) % 4 == 0);
    uint32_t w[sizeof(T) / 4];
    std::memcpy(w, &s, sizeof(T));
    for (uint32_t x : w) hu(x);
}

void hb(const void *p, size_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    std::fputc(' ', g_out);
    for (size_t i = 0; i < n; i++) std::fprintf(g_out, "%02x", b[i]);
}

void end()
{
    std::fputc('\n', g_out);
    std::fflush(g_out);
}

template <typename T>
void poison(T &s) { std::memset(&s, 0xcd, sizeof(T)); }

constexpr int N = 400;

void test_vectors()
{
    for (int it = 0; it < N; ++it) {
        real_vector3d a = rv(), b = rv(), c = rv(), out;
        real_point3d p = rp(), q = rp();
        real_vector2d v2 = rv2(), w2 = rv2();
        float s = rf();

        begin("vector3d_magnitude_squared", it); h(vector3d_magnitude_squared(&a)); end();
        begin("vector3d_distance_squared", it); h(vector3d_distance_squared(&p, &q)); end();
        begin("vector3d_distance", it); h(vector3d_distance(&p, &q)); end();
        begin("vector3d_length", it); h(vector3d_length(&a)); end();
        { real_vector3d t = a; begin("vector3d_normalize_with_length", it); h(vector3d_normalize_with_length(&t)); hs(t); end(); }
        { real_vector3d t = a; begin("vector3d_normalize", it); vector3d_normalize(&t); hs(t); end(); }
        { real_vector2d t = v2; begin("vector2d_normalize_with_length", it); h(vector2d_normalize_with_length(&t)); hs(t); end(); }
        { real_vector2d t = v2; begin("vector2d_normalize", it); vector2d_normalize(&t); hs(t); end(); }
        begin("vector2d_angle_between", it); h(vector2d_angle_between(&v2, &w2)); end();
        { real_vector2d x = {rf_range(-1, 1), rf_range(-1, 1)}, y = {rf_range(-1, 1), rf_range(-1, 1)};
          begin("vector2d_angle_between_unit", it); vector2d_normalize(&x); vector2d_normalize(&y); h(vector2d_angle_between(&x, &y)); end(); }
        { real_point3d o; poison(o); begin("point3d_add_scaled", it); point3d_add_scaled(&o, &a, &p, s); hs(o); end(); }
        { real_point3d o = p; begin("point3d_add_scaled_alias", it); point3d_add_scaled(&o, &a, &o, s); hs(o); end(); }
        poison(out); begin("vector3d_cross_product", it); vector3d_cross_product(&out, &a, &b); hs(out); end();
        { real_vector3d t = a; begin("vector3d_cross_product_alias", it); vector3d_cross_product(&t, &t, &b); hs(t); end(); }
        begin("vector3d_cross_product_length", it); h(vector3d_cross_product_length(&a, &b)); end();
        begin("vector3d_scalar_triple_product", it); h(vector3d_scalar_triple_product(&a, &b, &c)); end();
        begin("vector3d_major_axis_index", it); hu((uint32_t)(uint16_t)vector3d_major_axis_index(&a)); end();
        { float period = rf(); if (period == 0.0f) period = 2.5f;
          poison(out); begin("vector3d_positive_modulo", it); vector3d_positive_modulo(&a, &out, period); hs(out); end(); }
        { real_vector3d ua = unit_v(), ub = unit_v();
          begin("vector3d_angle_between_4cd4f0", it); h(vector3d_angle_between_4cd4f0(&a, &b)); h(vector3d_angle_between_4cd4f0(&ua, &ub)); end();
          begin("vector3d_angle_between_4cd5e0", it); h(vector3d_angle_between_4cd5e0(&ua, &ub)); h(vector3d_angle_between_4cd5e0(&ua, &ua)); end(); }
        poison(out); begin("vector3d_build_perpendicular", it); vector3d_build_perpendicular(&out, &a); hs(out); end();
        { real_vector3d par, perp; poison(par); poison(perp); real_vector3d ax = unit_v();
          begin("vector3d_project_onto_unit_axis", it); vector3d_project_onto_unit_axis(&par, &ax, &a, &perp); hs(par); hs(perp);
          vector3d_project_onto_unit_axis(nullptr, &ax, &a, &perp); hs(perp); vector3d_project_onto_unit_axis(&par, &ax, &a, nullptr); hs(par); end();
          poison(par); poison(perp);
          begin("vector3d_project_onto_axis", it); vector3d_project_onto_axis(&par, &b, &a, &perp); hs(par); hs(perp); end(); }
        { real_point3d o; poison(o); real_vector3d d = rv();
          begin("point3d_project_onto_line", it); point3d_project_onto_line(&p, &d, &q, &o); hs(o);
          real_point3d o2 = p; point3d_project_onto_line(&o2, &d, &q, &o2); hs(o2); end(); }
        { real_point3d origin = rp(), target = rp(); real_vector3d delta; poison(delta);
          begin("vector3d_delta_toward_gravity_biased_clamp_length", it);
          vector3d_delta_toward_gravity_biased_clamp_length(&origin, &target, &delta, rf_range(0, 5), rf_range(0, 5)); hs(delta); end(); }
        poison(out); begin("vector3d_lerp", it); vector3d_lerp(&out, &a, &b, rf_range(-0.5f, 1.5f)); hs(out); end();
        { real_vector3d t = a; begin("vector3d_lerp_alias", it); vector3d_lerp(&t, &t, &b, s); hs(t); end(); }
        poison(out); begin("vector3d_barycentric_interpolate", it); vector3d_barycentric_interpolate(&out, &a, &b, &c, rf_range(0, 1), rf_range(0, 1)); hs(out); end();
        { real_vector3d p0 = rv(), p1 = rv(), p2 = rv(), p3 = rv();
          float t0 = 0.0f, t1 = rf_range(0.1f, 1), t2 = t1 + rf_range(0.1f, 1), t3 = t2 + rf_range(0.1f, 1);
          poison(out); begin("vector3d_cubic_interpolate", it); vector3d_cubic_interpolate(&out, &p0, &p1, &p2, &p3, t0, t1, t2, t3, rf_range(0, 3)); hs(out); end();
          begin("cubic_interpolate_divided_difference", it); h(cubic_interpolate_divided_difference(rf(), rf(), rf(), rf(), t0, t1, t2, t3, rf_range(-1, 4))); end(); }
    }
}

void test_rotation()
{
    for (int it = 0; it < N; ++it) {
        real_vector3d a = rv(), ax = unit_v(), u = unit_v(), w = unit_v();
        float angle = rf_range(-3.2f, 3.2f), sn = (float)sqrt(1.0), cs;
        sn = rf_range(-1, 1); cs = (float)sqrt((double)(1.0f - sn * sn));
        { real_vector3d t = a; begin("vector3d_rotate_about_axis", it); vector3d_rotate_about_axis(&t, &ax, sn, cs); hs(t); end(); }
        { real_vector3d t = a; begin("vector3d_rotate_about_axis_perpendicular", it); vector3d_rotate_about_axis_perpendicular(&t, &ax, sn, cs); hs(t); end(); }
        { real_vector3d x = u, y = w; begin("vector3d_rotate_pair_in_plane", it); vector3d_rotate_pair_in_plane(&x, &y, sn, cs); hs(x); hs(y); end(); }
        { real_vector3d o; poison(o); begin("vector3d_rotate_toward", it); hu(vector3d_rotate_toward(&u, &w, &o, sn, cs)); hs(o); end(); }
        { real_vector3d dir = u, vel = (rnd() & 1) ? rv() : real_vector3d{0, 0, 0};
          begin("vector3d_rotate_toward_with_acceleration", it);
          for (int k = 0; k < 4; k++) {
              vector3d_rotate_toward_with_acceleration(&dir, &w, &vel, rf_range(-0.1f, 0.5f), rf_range(-0.05f, 0.2f));
              hs(dir); hs(vel);
          }
          end(); }
        { real_vector3d cur = u, vel = (rnd() & 1) ? real_vector3d{rf_range(-0.2f, 0.2f), rf_range(-0.2f, 0.2f), rf_range(-0.2f, 0.2f)} : real_vector3d{0, 0, 0};
          float bounds[4] = {rf_range(-3.2f, 0), rf_range(0, 3.2f), rf_range(-1.5f, 0), rf_range(0, 1.5f)};
          if (rnd() % 4 == 0) { bounds[0] = -3.1415927f; bounds[1] = 3.1415927f; }
          real_matrix4x3 m; real_matrix4x3 *tm = nullptr;
          if (rnd() & 1) { matrix4x3_from_euler_angles(&m, rf_range(-3, 3), rf_range(-1, 1), rf_range(-3, 3)); tm = &m; }
          begin("vector3d_rotate_toward_bounded", it);
          for (int k = 0; k < 4; k++) {
              vector3d_rotate_toward_bounded(&cur, &vel, bounds, rf_range(0.01f, 0.5f), rf_range(0.001f, 0.1f), &w, tm);
              hs(cur); hs(vel);
          }
          end(); }
        (void)angle;
    }
}

void test_matrices()
{
    for (int it = 0; it < N; ++it) {
        real_matrix4x3 a = rm4(), b = rm4(), o;
        real_matrix3x3 x = rm3(), y = rm3(), o3;
        real_vector3d v = rv(), up = unit_v(), fw = unit_v();
        real_point3d p = rp();
        real_plane3d pl = rplane(), plo;
        real_quaternion q = rq(), qo;

        poison(o); begin("matrix4x3_inverse", it); matrix4x3_inverse(&o, &a); hs(o); { real_matrix4x3 t = a; matrix4x3_inverse(&t, &t); hs(t); } end();
        { float sn = rf_range(-1, 1), cs = (float)sqrt((double)(1.0f - sn * sn)); real_vector3d ax = (rnd() & 3) ? unit_v() : v;
          poison(o); begin("matrix4x3_from_axis_angle", it); matrix4x3_from_axis_angle(&o, &ax, sn, cs); hs(o); end(); }
        poison(o); begin("matrix4x3_from_forward_up", it); matrix4x3_from_forward_up(&up, &fw, &o); hs(o); end();
        poison(o); begin("matrix4x3_from_euler_angles", it); matrix4x3_from_euler_angles(&o, rf(), rf(), rf()); hs(o); end();
        { real_quaternion qq = (rnd() % 8) ? q : real_quaternion{0, 0, 0, 0};
          poison(o); begin("matrix4x3_from_quaternion", it); matrix4x3_from_quaternion(&qq, &o); hs(o); end(); }
        poison(qo); begin("quaternion_from_matrix4x3", it); quaternion_from_matrix4x3(&a, &qo); hs(qo); end();
        poison(o); begin("matrix4x3_from_forward_up_position", it); matrix4x3_from_forward_up_position(&up, &fw, &p, &o); hs(o); end();
        { real_vector3d ou, of; real_point3d op; poison(ou); poison(of); poison(op);
          begin("matrix4x3_extract_forward_up_position", it); matrix4x3_extract_forward_up_position(&ou, &of, &a, &op); hs(ou); hs(of); hs(op); end(); }
        { real_point3d op; poison(op); begin("matrix4x3_transform_point", it); matrix4x3_transform_point(&op, &p, &a); hs(op);
          real_point3d t = p; matrix4x3_transform_point(&t, &t, &a); hs(t); end(); }
        { real_vector3d ov; poison(ov); begin("matrix4x3_transform_vector", it); matrix4x3_transform_vector(&ov, &v, &a); hs(ov);
          real_vector3d t = v; matrix4x3_transform_vector(&t, &t, &a); hs(t); end(); }
        { real_vector3d ov; poison(ov); begin("matrix4x3_transform_normal", it); matrix4x3_transform_normal(&ov, &v, &a); hs(ov);
          real_vector3d t = v; matrix4x3_transform_normal(&t, &t, &a); hs(t); end(); }
        poison(plo); begin("matrix4x3_transform_plane", it); matrix4x3_transform_plane(&plo, &a, &pl); hs(plo); { real_plane3d t = pl; matrix4x3_transform_plane(&t, &a, &t); hs(t); } end();
        { real_point3d op; poison(op); begin("matrix4x3_inverse_transform_point", it); matrix4x3_inverse_transform_point(&a, &op, &p); hs(op);
          real_point3d t = p; matrix4x3_inverse_transform_point(&a, &t, &t); hs(t); end(); }
        { real_vector3d ov; poison(ov); begin("matrix4x3_inverse_transform_vector", it); matrix4x3_inverse_transform_vector(&ov, &v, &a); hs(ov);
          real_vector3d t = v; matrix4x3_inverse_transform_vector(&t, &t, &a); hs(t); end(); }
        { real_vector3d ov; poison(ov); begin("matrix4x3_inverse_transform_normal", it); matrix4x3_inverse_transform_normal(&ov, &v, &a); hs(ov);
          real_vector3d t = v; matrix4x3_inverse_transform_normal(&t, &t, &a); hs(t); end(); }
        poison(o); begin("matrix4x3_multiply", it); matrix4x3_multiply(&a, &b, &o); hs(o);
        { real_matrix4x3 t = a; matrix4x3_multiply(&t, &b, &t); hs(t); t = b; matrix4x3_multiply(&a, &t, &t); hs(t); t = a; matrix4x3_multiply(&t, &t, &t); hs(t); } end();
        poison(o); begin("matrix4x3_multiply_sse", it); matrix4x3_multiply_sse(&a, &b, &o); hs(o); { real_matrix4x3 t = a; matrix4x3_multiply_sse(&t, &b, &t); hs(t); } end();
        poison(o); begin("matrix4x3_multiply_3dnow", it); matrix4x3_multiply_3dnow(&a, &b, &o); hs(o);
        { real_matrix4x3 t = a; matrix4x3_multiply_3dnow(&t, &b, &t); hs(t); t = b; matrix4x3_multiply_3dnow(&a, &t, &t); hs(t); } end();
        poison(o3); begin("matrix3x3_transpose", it); matrix3x3_transpose(&o3, &x); hs(o3); { real_matrix3x3 t = x; matrix3x3_transpose(&t, &t); hs(t); } end();
        poison(o3); begin("matrix3x3_from_forward_up", it); matrix3x3_from_forward_up(&up, &fw, &o3); hs(o3); end();
        poison(o3); begin("matrix3x3_multiply", it); matrix3x3_multiply(&o3, &x, &y); hs(o3);
        { real_matrix3x3 t = x; matrix3x3_multiply(&t, &t, &y); hs(t); t = y; matrix3x3_multiply(&t, &x, &t); hs(t); } end();
        { real_vector3d ov; poison(ov); begin("matrix3x3_inverse_transform_vector", it); matrix3x3_inverse_transform_vector(&ov, &v, &x); hs(ov);
          real_vector3d t = v; matrix3x3_inverse_transform_vector(&t, &t, &x); hs(t); end(); }
        poison(qo); begin("quaternion_from_matrix3x3", it); { real_quaternion *r = quaternion_from_matrix3x3(&x, &qo); hu(r == &qo); hs(qo); } end();
        { real_euler_angles3d e = {rf(), rf(), rf()}; real_vector3d ou, of; poison(ou); poison(of);
          begin("euler_angles_to_basis_vectors", it); euler_angles_to_basis_vectors(&e, &ou, &of); hs(ou); hs(of); end(); }
        { real_vector3d f = (rnd() % 4) ? unit_v() : v, l = (rnd() % 4) ? unit_v() : rv(), u = (rnd() % 4) ? unit_v() : rv();
          if (rnd() % 3 == 0) { f = a.forward; l = a.left; u = a.up; }
          begin("real_matrix4x3_rotation_is_orthonormal", it); hu(real_matrix4x3_rotation_is_orthonormal(&f, &l, &u)); end();
          { real_vector3d f2 = f, l2 = l, u2 = u; begin("real_matrix4x3_rotation_rebuild_orthonormal", it);
            real_matrix4x3_rotation_rebuild_orthonormal(&f2, &l2, &u2); hs(f2); hs(l2); hs(u2); end(); }
          { real_vector3d f2 = (rnd() % 5) ? f : real_vector3d{0, 0, 1}, l2, u2; poison(l2); poison(u2);
            begin("real_matrix4x3_rotation_from_forward", it); real_matrix4x3_rotation_from_forward(&f2, &l2, &u2); hs(f2); hs(l2); hs(u2); end(); } }
    }
}

void test_quaternions()
{
    for (int it = 0; it < N; ++it) {
        real_quaternion a = rq(), b = rq(), o;
        if (rnd() % 10 == 0) a = {0, 0, 0, 0};
        real_vector3d v = rv(), ov;
        { real_quaternion t = a; begin("quaternion_normalize", it); quaternion_normalize(&t); hs(t); end(); }
        { real_quaternion t = a; quaternion_normalize(&t); real_vector3d axis; float ang; poison(axis); poison(ang);
          begin("quaternion_to_axis_angle", it); quaternion_to_axis_angle(&t, &axis, &ang); hs(axis); h(ang); end(); }
        poison(o); begin("quaternion_multiply", it); quaternion_multiply(&a, &b, &o); hs(o); { real_quaternion t = a; quaternion_multiply(&t, &b, &t); hs(t); } end();
        poison(o); begin("quaternion_lerp", it); quaternion_lerp(&a, &b, &o, rf_range(-0.2f, 1.2f)); hs(o); end();
        { real_quaternion t = a; quaternion_normalize(&t); poison(ov);
          begin("quaternion_rotate_vector", it); quaternion_rotate_vector(&t, &v, &ov); hs(ov); real_vector3d w = v; quaternion_rotate_vector(&t, &w, &w); hs(w); end(); }
    }
}

void test_geometry()
{
    for (int it = 0; it < N; ++it) {
        real_point3d p = rp(), q = rp(), r = rp();
        real_vector3d d = rv(), e = rv(), ud = unit_v();
        float radius = rf_range(0, 6);

        { real_point3d to = rp(), ref = {rf_range(-1, 1), rf_range(-1, 1), 0};
          begin("point3d_within_horizontal_cone", it); hu(point3d_within_horizontal_cone(&to, &ref, rf_range(-1, 1))); end(); }
        { real_point3d o; poison(o); real_point3d end_p = (rnd() % 8) ? q : p;
          begin("path_find_closest_point_on_segment", it); path_find_closest_point_on_segment(&r, &p, &end_p, &o); hs(o); end(); }
        begin("point3d_within_radius", it); hu((uint32_t)point3d_within_radius(&p, &q, radius)); end();
        { real_vector2d dir = {rf_range(-1, 1), rf_range(-1, 1)}; vector2d_normalize(&dir); real_point2d o2 = rp2_small(), c2 = rp2_small(); float dist; poison(dist);
          begin("ray2d_intersect_circle_distance", it); hu(ray2d_intersect_circle_distance(&dir, &o2, &c2, &dist, radius)); h(dist); end(); }
        { real_vector2d dir = {rf_range(-1, 1), rf_range(-1, 1)}, ep, en; float adj; poison(ep); poison(en); poison(adj);
          begin("vector2d_tangent_edge_directions", it); vector2d_tangent_edge_directions(&dir, &ep, &en, (rnd() % 8) ? rf() : 0.0f, rf(), &adj); hs(ep); hs(en); h(adj); end(); }
        { real_point3d o; poison(o); real_plane3d pl = rplane(); real_point2d known = rp2();
          uint32_t sign = (rnd() & 0xffffff00u) | (rnd() & 1); int32_t axis = (int32_t)((rnd() & 0xffff0000u) | (rnd() % 3));
          begin("decal_plane_solve_third_axis", it); real_point3d *res = decal_plane_solve_third_axis(&o, sign, axis, &pl, &known); hu(res == &o); hs(o); end(); }
        { real_plane2d pl; poison(pl); real_point2d a2 = rp2(), b2 = (rnd() % 6) ? rp2() : a2;
          begin("plane2d_from_points", it); real_plane2d *res = plane2d_from_points(&pl, &a2, &b2); hu(res == &pl); hu(res == nullptr); hs(pl); end(); }
        { real_plane3d pl; poison(pl); begin("plane3d_from_point_and_normal", it); plane3d_from_point_and_normal(&pl, &ud, &p); hs(pl); end(); }
        { real_plane3d pl = rplane(), o; poison(o); begin("plane3d_negate", it); plane3d_negate(&o, &pl); hs(o); plane3d_negate(&pl, &pl); hs(pl); end(); }
        begin("point3d_distance_squared_to_segment", it); h(point3d_distance_squared_to_segment(&p, &d, &q)); end();
        { real_vector3d e2 = (rnd() % 6) ? e : d;
          begin("segment3d_distance_squared_to_segment", it); h(segment3d_distance_squared_to_segment(&p, &q, &d, &e2)); end();
          begin("segment3d_within_radius_of_segment", it); hu((uint32_t)segment3d_within_radius_of_segment(&p, &q, &d, &e2, radius)); end(); }
        { real_vector3d n; float t; poison(n); poison(t);
          begin("ray_intersects_sphere", it); hu(ray_intersects_sphere(&p, &n, &d, &t, &q, radius)); hs(n); h(t); end(); }
        { real_vector3d hit; float t; poison(hit); poison(t);
          begin("ray_intersects_cylinder", it); hu(ray_intersects_cylinder(rf_range(0, 5), radius, &hit, &t, &q, &p, &d)); hs(hit); h(t); end(); }
        begin("ray_intersects_sphere_test", it); hu(ray_intersects_sphere_test(&q, &p, &d, radius)); end();
        begin("ray_intersect_sphere_distance", it); h(ray_intersect_sphere_distance(&p, &q, &d, radius)); end();
        { real_point3d a3 = rp(), b3 = rp(), c3 = rp(); real_point3d pt;
          if (rnd() & 1) { float u = rf_range(-0.2f, 1), w = rf_range(-0.2f, 1);
              pt = {a3.x + u * (b3.x - a3.x) + w * (c3.x - a3.x), a3.y + u * (b3.y - a3.y) + w * (c3.y - a3.y), a3.z + u * (b3.z - a3.z) + w * (c3.z - a3.z)}; }
          else pt = rp();
          float ou, ov; poison(ou); poison(ov);
          begin("triangle_point_barycentric_2d", it); hu(triangle_point_barycentric_2d(&a3, &b3, &c3, &pt, &ou, &ov)); h(ou); h(ov); end(); }
        begin("vector3d_projection_band_test", it);
        hu(vector3d_projection_band_test(&ud, &p, &q, radius, rf_range(0, 10), rf_range(0, 1), rf_range(0, 1))); end();
        { real_plane3d p1 = rplane(), p2 = rplane(), p3 = rplane(); real_point3d o; poison(o);
          if (rnd() % 8 == 0) p3 = p2;
          begin("plane3d_intersect_three", it); hu(plane3d_intersect_three(&p1, &p2, &p3, &o)); hs(o); end();
          real_vector3d dir; poison(dir); poison(o);
          begin("plane3d_intersect_pair_to_line", it); hu(plane3d_intersect_pair_to_line(&dir, &p2, &p1, &o)); hs(dir); hs(o); end(); }
    }
}

void test_polygons()
{
    for (int it = 0; it < N; ++it) {
        real_point2d pts[32], out[128];
        std::memset(pts, 0xcd, sizeof(pts));
        int16_t n = (int16_t)(rnd() % 12);
        for (int i = 0; i < n; i++) pts[i] = rp2_small();
        if (rnd() % 6 == 0) for (int i = 0; i < n; i++) pts[i] = {pts[0].x + (float)i, pts[0].y + 2.0f * (float)i};
        if (rnd() % 8 == 0) for (int i = 0; i < n; i++) pts[i] = pts[0];
        begin("polygon2d_points_classify", it); hu((uint32_t)(uint16_t)polygon2d_points_classify(pts, n)); end();
        { int16_t hull[64]; std::memset(hull, 0xcd, sizeof(hull));
          begin("polygon2d_convex_hull_build", it); int16_t hn = polygon2d_convex_hull_build(pts, n, hull); hu((uint32_t)(uint16_t)hn); hb(hull, sizeof(hull)); end(); }
        real_point2d point = rp2_small();
        begin("polygon2d_point_inside_tolerance", it); hu(polygon2d_point_inside_tolerance(pts, n, &point, rf_range(0, 1))); end();
        begin("polygon2d_point_inside_margin", it); hu(polygon2d_point_inside_margin(pts, n, &point, rf_range(0, 1))); end();
        { real_point2d clip[8]; int16_t cn = (int16_t)(rnd() % 6); for (int i = 0; i < cn; i++) clip[i] = rp2_small();
          if (cn > 1 && rnd() % 5 == 0) clip[1] = clip[0];
          std::memset(out, 0xcd, sizeof(out));
          int16_t maxc = (int16_t)((rnd() % 4) ? 64 : (rnd() % 6));
          begin("polygon2d_clip_to_planes", it); int16_t rn = polygon2d_clip_to_planes(n, pts, cn, clip, maxc, out, rf_range(0, 0.01f));
          hu((uint32_t)(uint16_t)rn); hb(out, sizeof(real_point2d) * 32); end(); }
        { real_plane2d pl; real_point2d a2 = rp2_small(), b2 = rp2_small(); if (!plane2d_from_points(&pl, &a2, &b2)) pl = {{1, 0}, 0};
          uint32_t mask = rnd(); uint8_t flag = 0xcd; std::memset(out, 0xcd, sizeof(out));
          int16_t maxc = (int16_t)((rnd() % 4) ? 64 : (rnd() % 6)); float eps = rf_range(0, 0.01f);
          begin("polygon2d_clip_to_plane", it); int16_t rn = polygon2d_clip_to_plane(out, n, pts, &pl, maxc, &mask, &flag, eps);
          hu((uint32_t)(uint16_t)rn); hu(mask); hu(flag); hb(out, sizeof(real_point2d) * 32);
          real_point2d inplace[64]; std::memset(inplace, 0xcd, sizeof(inplace)); std::memcpy(inplace, pts, sizeof(pts));
          rn = polygon2d_clip_to_plane(inplace, n, inplace, &pl, maxc, nullptr, nullptr, eps); hu((uint32_t)(uint16_t)rn); hb(inplace, sizeof(real_point2d) * 32); end(); }
        { real_point3d p3[32], o3[64]; std::memset(p3, 0xcd, sizeof(p3)); for (int i = 0; i < n; i++) p3[i] = {pts[i].x, pts[i].y, rf_range(-1, 1)};
          real_plane3d pl = rplane(); if (rnd() % 5 == 0) { pl = {{0, 0, 1}, 0}; for (int i = 0; i < n; i++) p3[i].z = 0; }
          uint8_t flag = 0xcd; std::memset(o3, 0xcd, sizeof(o3)); int16_t maxc = (int16_t)((rnd() % 4) ? 64 : (rnd() % 6));
          float eps = rf_range(0, 0.01f); char keep = (char)(rnd() & 1);
          begin("polygon3d_clip_to_plane", it); int16_t rn = polygon3d_clip_to_plane(n, p3, &pl, maxc, o3, &flag, eps, keep);
          hu((uint32_t)(uint16_t)rn); hu(flag); hb(o3, sizeof(real_point3d) * 32);
          real_point3d inplace[64]; std::memset(inplace, 0xcd, sizeof(inplace)); std::memcpy(inplace, p3, sizeof(p3));
          rn = polygon3d_clip_to_plane(n, inplace, &pl, maxc, inplace, nullptr, eps, keep); hu((uint32_t)(uint16_t)rn); hb(inplace, sizeof(real_point3d) * 32); end(); }
    }
}

void test_scalars()
{
    for (int it = 0; it < N; ++it) {
        float a = rf(), b = rf(), t = rf_range(-0.5f, 1.5f);
        { float o; poison(o); begin("real_lerp_clamped", it); real_lerp_clamped(&o, a, b, t); h(o); end(); }
        begin("real_inverse_lerp_clamped", it); h(real_inverse_lerp_clamped(rf(), rf(), (rnd() % 8) ? rf() : a)); end();
        { float lo = rf_range(-5, 5), hi = lo + ((rnd() % 8) ? rf_range(0.01f, 10) : 0.0f);
          begin("lerp_find_threshold_byte", it); hu(lerp_find_threshold_byte(lo, hi, rf_range(lo - 1, hi + 1))); end(); }
        { float vel = rf_range(-1, 1), val = rf_range(-3, 3); float lo = rf_range(-4, 0), hi = rf_range(0.1f, 4); int wrap = (int)(rnd() & 1);
          begin("real_seek_toward_clamped", it);
          for (int k = 0; k < 4; k++) { hu(real_seek_toward_clamped(wrap, &vel, &val, rf_range(-4, 4), rf_range(0.001f, 0.5f), rf_range(0.01f, 1), lo, hi)); h(vel); h(val); }
          end(); }
        { bounded_ramp_profile pa, pb; poison(pa); poison(pb);
          float maxa = rf_range(0.01f, 1);
          begin("bounded_ramp_profile_build", it);
          bounded_ramp_profile_build((rnd() % 8) ? rf_range(-5, 5) : 0.0f, (rnd() % 8) ? rf_range(-1, 1) : 0.0f, (rnd() % 4) ? rf_range(0.01f, 2) : 0.0f, maxa, &pa);
          bounded_ramp_profile_build(rf_range(-5, 5), rf_range(-1, 1), rf_range(-0.5f, 2), maxa, &pb);
          hs(pa); hs(pb); end();
          begin("bounded_ramp_profile_synchronize", it); bounded_ramp_profile_synchronize(&pa, &pb, maxa); hs(pa); hs(pb); end();
          float pos, vel; poison(pos); poison(vel);
          begin("bounded_ramp_profile_evaluate", it);
          hu(bounded_ramp_profile_evaluate(&pa, rf_range(-1, 20), rf(), &pos, rf(), &vel)); h(pos); h(vel);
          hu(bounded_ramp_profile_evaluate(&pb, rf_range(0, 2), rf(), &pos, rf(), &vel)); h(pos); h(vel); end(); }
        begin("uint32_log2_floor", it); { uint32_t x = rnd() >> (rnd() % 32); if (it < 3) x = (uint32_t)it; hu((uint32_t)uint32_log2_floor(x)); } end();
        { uint32_t va[8], vb[8], dst[8]; for (int i = 0; i < 8; i++) { va[i] = rnd(); vb[i] = rnd(); dst[i] = 0xcdcdcdcdu; }
          begin("bit_vector_or", it); bit_vector_or(va, (int16_t)(rnd() % 257), vb, dst); hb(dst, sizeof(dst)); end(); }
        { float x[2] = {rf(), 0}, y[2] = {rf(), 0}; if (rnd() % 5 == 0) y[0] = x[0];
          begin("float_compare_ascending", it); hu((uint32_t)float_compare_ascending(x, y)); end(); }
        { uint8_t ra[12], rb[12]; std::memset(ra, 0, 12); std::memset(rb, 0, 12);
          float da = rf(), db = (rnd() % 5) ? rf() : da; std::memcpy(ra + 4, &da, 4); std::memcpy(rb + 4, &db, 4);
          ra[8] = (uint8_t)((rnd() % 3) ? 0 : rnd()); rb[8] = (uint8_t)((rnd() % 3) ? 0 : rnd());
          begin("object_sort_by_flag_then_distance", it); hu((uint32_t)object_sort_by_flag_then_distance(ra, rb)); end(); }
        { float rgb[3] = {rf_range(-0.2f, 1.2f), rf_range(0, 1), (rnd() % 6) ? rf_range(0, 1) : rf()};
          begin("color_real_to_argb_pack", it); hu(color_real_to_argb_pack(rf_range(0, 1), rgb)); end(); }
    }
}

void test_random()
{
    random_seed_global = 0x89abcdefu;
    effect_random_seed = 0x1234567u;
    for (int it = 0; it < N; ++it) {
        begin("random_real", it); h(random_real()); hu(random_seed_global); end();
        begin("random_real_range", it); h(random_real_range(rf(), rf())); hu(random_seed_global); end();
        begin("random_range_real", it); h(random_range_real(rf(), rf())); hu(effect_random_seed); end();
        { int16_t lo = (int16_t)(rnd() % 2000) - 1000, hi = (int16_t)(lo + (int16_t)(rnd() % 3000) - 500);
          if (it < 4) { lo = (int16_t)-32768; hi = 32767; }
          begin("random_int_range", it); hu((uint32_t)random_int_range(lo, hi)); hu(random_seed_global); end(); }
        { random_seed seed = rnd(); begin("random_real_range_seeded", it); h(random_real_range_seeded(&seed, rf(), rf())); hu(seed); end(); }
    }
    srand(12345u);
    begin("random_seed_generate", 0); for (int k = 0; k < 8; k++) hu(random_seed_generate()); end();
}

void test_sphere_mesh()
{
    for (int16_t n = 1; n <= 8; n++) {
        sphere_mesh *mesh = sphere_mesh_generate(n);
        begin("sphere_mesh_generate", n);
        hu(mesh != nullptr);
        if (mesh) {
            hu((uint32_t)(uint16_t)mesh->subdivisions); hu((uint32_t)(uint16_t)mesh->point_count);
            hu((uint32_t)(uint16_t)mesh->triangle_count); hu((uint32_t)(uint16_t)mesh->strip_count);
            hb(mesh->points, sizeof(real_point3d) * (size_t)mesh->point_count);
            size_t used = 0;
            for (int16_t s = 0; s < mesh->strip_count && used < (size_t)mesh->triangle_count * 4; s++)
                used += 1 + (size_t)(uint16_t)mesh->indices[used];
            hu((uint32_t)used);
            hb(mesh->indices, used * 2);
            GlobalFree(mesh->points); GlobalFree(mesh->indices); GlobalFree(mesh);
        }
        end();
    }
    srand(777u);
    sphere_point_table_init();
    begin("sphere_point_table_init", 0); hu(effect_random_seed); hu((uint32_t)(uint16_t)sphere_point_table_count);
    hb(sphere_point_table, sizeof(real_point3d) * (size_t)sphere_point_table_count); end();
    for (int it = 0; it < N; ++it) {
        real_point3d dir = {0, 0, 1}; real_vector3d u = unit_v(); dir = {u.i, u.j, u.k};
        real_vector3d o; poison(o); random_seed seed = rnd();
        begin("vector3d_randomize_direction", it);
        real_vector3d *r = vector3d_randomize_direction(&dir, &o, &seed, rf_range(0, 1), rf_range(1, 3)); hu(r == &o); hs(o); hu(seed); end();
    }
}

void test_periodic()
{
    begin("periodic_function_evaluate_uninitialized", 0); h(periodic_function_evaluate(0, 1.0)); h(periodic_function_evaluate(3, 1.0));
    h(transition_function_evaluate(2, 0.5f)); end();
    {
        float noise[1024];
        random_seed_global = 0x20f3f660u;
        begin("periodic_function_build_noise_table", 0); periodic_function_build_noise_table(noise); hb(noise, sizeof(noise)); hu(random_seed_global); end();
    }
    for (int16_t type = 0; type <= 12; type++) {
        uint8_t table[1024];
        std::memset(table, 0xcd, sizeof(table));
        random_seed_global = 0x20f3f660u + (uint32_t)type;
        begin("periodic_function_build_table", type); periodic_function_build_table(type, table); hb(table, sizeof(table)); hu(random_seed_global); end();
    }
    for (int16_t type = 0; type <= 7; type++) {
        uint8_t table[1024];
        std::memset(table, 0xcd, sizeof(table));
        begin("periodic_function_build_transition_table", type); periodic_function_build_transition_table(type, table); hb(table, sizeof(table)); end();
    }
    periodic_function_tables_init();
    begin("periodic_function_tables_init", 0); hu(periodic_functions_initialized); hu(random_seed_global);
    for (int i = 0; i < 12; i++) hb(periodic_function_tables[i], 1024);
    for (int i = 0; i < 6; i++) hb(transition_function_tables[i], 1024);
    end();
    for (int it = 0; it < N; ++it) {
        begin("periodic_function_evaluate", it);
        double time = (double)rf_range(-50, 50);
        if (it % 7 == 0) time = (double)rf() * 3.0;
        for (int16_t type = 0; type < 12; type++) h(periodic_function_evaluate(type, time));
        end();
        begin("transition_function_evaluate", it);
        float phase = (it % 9 == 0) ? (float)(it % 2) : rf_range(0, 1);
        for (int16_t type = 0; type < 6; type++) h(transition_function_evaluate(type, phase));
        end();
    }
    periodic_function_tables_free();
    begin("periodic_function_tables_free", 0); hu(periodic_functions_initialized); h(periodic_function_evaluate(2, 0.3)); end();
    periodic_function_tables_free();
}

void test_math_initialize()
{
    static char no_sse[] = "-NOsse";
    static char other[] = "-window";
    const struct { int argc; char *arg1; int32_t safe; uint32_t features; } cases[] = {
        {1, nullptr, 0, 0}, {1, nullptr, 0, 1u << 0x1d}, {1, nullptr, 0, 1u << 0x1a}, {1, nullptr, 0, (1u << 0x1a) | (1u << 0x1d)},
        {1, nullptr, 1, 1u << 0x1d}, {2, no_sse, 0, 1u << 0x1d}, {2, other, 0, 1u << 0x1a},
    };
    int k = 0;
    for (const auto &c : cases) {
        shell_argc = c.argc;
        shell_argv[1] = c.arg1;
        safe_mode = c.safe;
        g_test_cpu_features_low = c.features;
        matrix4x3_multiply_procedure = nullptr;
        srand(99u);
        math_initialize();
        uint32_t which = matrix4x3_multiply_procedure == matrix4x3_multiply ? 1 :
                         matrix4x3_multiply_procedure == matrix4x3_multiply_sse ? 2 :
                         matrix4x3_multiply_procedure == matrix4x3_multiply_3dnow ? 3 : 0;
        begin("math_initialize", k++); hu(which); hu(periodic_functions_initialized); hu((uint32_t)(uint16_t)sphere_point_table_count);
        hb(periodic_function_tables[5], 1024); hb(sphere_point_table, 64); end();
    }
    shell_argc = 1;
    shell_argv[1] = nullptr;
    safe_mode = 0;
}

}  // namespace

int main(int argc, char **argv)
{
    g_out = std::fopen(argc > 1 ? argv[1] : "math_difftest.txt", "wb");
    if (!g_out) return 2;
    test_vectors();
    test_rotation();
    test_matrices();
    test_quaternions();
    test_geometry();
    test_polygons();
    test_scalars();
    test_random();
    test_sphere_mesh();
    test_periodic();
    test_math_initialize();
    std::fprintf(g_out, "calls %d\n", g_calls);
    std::fclose(g_out);
    std::printf("%d calls\n", g_calls);
    return 0;
}
