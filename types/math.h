#ifndef HALO_TYPES_MATH_H   /* guarded: <process.h> includes <math.h>, which finds this file on the include path */
#define HALO_TYPES_MATH_H

// Blam math module (halo.exe 1.0.10 retail, 0x401050..0x4cf7a0, 99 functions).
// Layouts recovered from the decompiled module plus direct reads of the .rdata
// constant tables in bin/halo.exe. Offsets in comments are byte offsets from the
// struct base.
//
// The geometric primitives below are the engine-side (snake_case, Blam) spelling
// of the same layouts that types/tags.h exports under the invader tag names:
//   real_vector2d == Vector2D      real_point2d        == Point2D
//   real_vector3d == Vector3D      real_point3d        == Point3D
//   real_plane2d  == Plane2D       real_plane3d        == Plane3D
//   real_quaternion == Quaternion  real_euler_angles3d == Euler3D
//   real_matrix3x3  == Matrix
// They are declared separately (rather than aliased) so this header parses on its
// own; the names do not collide with the tag names when both headers are loaded.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;
// 64-bit integers: only random_seed_generate @0x4cd070 needs them, to fold the MSVC 7.1
// __allmul/__alldiv helpers back into plain arithmetic over the QueryPerformanceCounter
// reading and the frequency at 0x006ac8f8.
typedef long long int64_t; typedef unsigned long long uint64_t;

// LARGE_INTEGER as the Win32 ABI lays it out; declared here because no windows.h is
// available to the CParser. random_seed_generate passes one to QueryPerformanceCounter.
typedef union large_integer {
    struct { uint32_t low_part; int32_t high_part; } parts;
    int64_t quad_part;
} large_integer;                // size 0x08

// ---------------------------------------------------------------------------
// scalar and vector primitives
// ---------------------------------------------------------------------------
typedef float real;

// Tolerance the whole module compares against: 0.0001f, emitted by the compiler
// as the .rdata literal 9.999999747378752e-05. Used by
// vector2d_normalize_with_length @0x4018e0, vector3d_normalize_with_length
// @0x401990, polygon2d_convex_hull_build @0x4caae0, plane3d_intersect_three
// @0x4cf040, segment3d_distance_squared_to_segment @0x4cdef0 and others.
// A second, much smaller guard of 1e-06f appears in
// vector3d_random_point_in_cone @0x4cf530.

typedef struct real_vector2d {
    float i;                   // 0x00
    float j;                   // 0x04
#ifdef __cplusplus   /* C++ only: member functions and hidden-friend operators; no data, layout unchanged */
    /** Dot product, i*o.i + j*o.j. */
    constexpr real dot(const real_vector2d &o) const { return i * o.i + j * o.j; }
    /** Normalises in place (vector2d_normalize_with_length); returns the old length, or 0 for a near-zero vector. */
    real normalize();
    /** Componentwise arithmetic, one float operation per component as written. */
    friend constexpr real_vector2d operator+(const real_vector2d &a, const real_vector2d &b) { return {a.i + b.i, a.j + b.j}; }
    friend constexpr real_vector2d operator-(const real_vector2d &a, const real_vector2d &b) { return {a.i - b.i, a.j - b.j}; }
    friend constexpr real_vector2d operator-(const real_vector2d &v) { return {-v.i, -v.j}; }
    friend constexpr real_vector2d operator*(const real_vector2d &v, real s) { return {v.i * s, v.j * s}; }
    friend constexpr real_vector2d operator*(real s, const real_vector2d &v) { return {s * v.i, s * v.j}; }
#endif
} real_vector2d;               // size 0x08

typedef struct real_point2d {
    float x;                   // 0x00
    float y;                   // 0x04
#ifdef __cplusplus   /* C++ only: member functions and hidden-friend operators; no data, layout unchanged */
    /** Componentwise arithmetic, one float operation per component as written. */
    friend constexpr real_vector2d operator-(const real_point2d &a, const real_point2d &b) { return {a.x - b.x, a.y - b.y}; }
    friend constexpr real_point2d operator+(const real_point2d &p, const real_vector2d &v) { return {p.x + v.i, p.y + v.j}; }
#endif
} real_point2d;                // size 0x08

typedef struct real_vector3d {
    float i;                   // 0x00
    float j;                   // 0x04
    float k;                   // 0x08
#ifdef __cplusplus   /* C++ only: member functions and hidden-friend operators; no data, layout unchanged */
    /** Dot product, (i*o.i + j*o.j) + k*o.k. */
    constexpr real dot(const real_vector3d &o) const { return i * o.i + j * o.j + k * o.k; }
    /** Cross product this x o, each component one product minus another. */
    constexpr real_vector3d cross(const real_vector3d &o) const { return {j * o.k - k * o.j, k * o.i - i * o.k, i * o.j - j * o.i}; }
    /** Squared length (vector3d_magnitude_squared, the same sum as dot(*this)). */
    constexpr real magnitude_squared() const { return dot(*this); }
    /** Length (vector3d_length). */
    real magnitude() const;
    /** Normalises in place (vector3d_normalize_with_length); returns the old length, or 0 for a near-zero vector. */
    real normalize();
    /** Componentwise arithmetic, one float operation per component as written. */
    friend constexpr real_vector3d operator+(const real_vector3d &a, const real_vector3d &b) { return {a.i + b.i, a.j + b.j, a.k + b.k}; }
    friend constexpr real_vector3d operator-(const real_vector3d &a, const real_vector3d &b) { return {a.i - b.i, a.j - b.j, a.k - b.k}; }
    friend constexpr real_vector3d operator-(const real_vector3d &v) { return {-v.i, -v.j, -v.k}; }
    friend constexpr real_vector3d operator*(const real_vector3d &v, real s) { return {v.i * s, v.j * s, v.k * s}; }
    friend constexpr real_vector3d operator*(real s, const real_vector3d &v) { return {s * v.i, s * v.j, s * v.k}; }
    friend constexpr real_vector3d &operator+=(real_vector3d &a, const real_vector3d &b) { a.i += b.i; a.j += b.j; a.k += b.k; return a; }
    friend constexpr real_vector3d &operator-=(real_vector3d &a, const real_vector3d &b) { a.i -= b.i; a.j -= b.j; a.k -= b.k; return a; }
    friend constexpr real_vector3d &operator*=(real_vector3d &v, real s) { v.i *= s; v.j *= s; v.k *= s; return v; }
#endif
} real_vector3d;               // size 0x0c

typedef struct real_point3d {
    float x;                   // 0x00
    float y;                   // 0x04
    float z;                   // 0x08
#ifdef __cplusplus   /* C++ only: member functions and hidden-friend operators; no data, layout unchanged */
    /** Distance to another point (vector3d_distance). */
    real distance_to(const real_point3d &o) const;
    /** Squared distance to another point (vector3d_distance_squared). */
    real distance_squared_to(const real_point3d &o) const;
    /** Componentwise arithmetic, one float operation per component as written. */
    friend constexpr real_point3d operator+(const real_point3d &p, const real_vector3d &v) { return {p.x + v.i, p.y + v.j, p.z + v.k}; }
    friend constexpr real_point3d operator-(const real_point3d &p, const real_vector3d &v) { return {p.x - v.i, p.y - v.j, p.z - v.k}; }
    friend constexpr real_vector3d operator-(const real_point3d &a, const real_point3d &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
    friend constexpr real_point3d &operator+=(real_point3d &p, const real_vector3d &v) { p.x += v.i; p.y += v.j; p.z += v.k; return p; }
#endif
} real_point3d;                // size 0x0c

// polygon2d_clip_to_plane @0x4caff0 evaluates (p.x*pl[0] + p.y*pl[1]) - pl[2],
// so the plane distance is subtracted, not added: the plane is i*x + j*y == d.
typedef struct real_plane2d {
    real_vector2d normal;      // 0x00
    float d;                   // 0x08 distance along the normal
} real_plane2d;                // size 0x0c
// Built out of two points by plane2d_from_points @0x44d950 (out of this module,
// still FUN_0044d950 in symbols/functions.txt; out plane in ECX, point a in EAX,
// point b in EDX): normal = (a.y - b.y, b.x - a.x), normalized, d = normal . b,
// returning NULL when the two points are closer together than 0.0001. Both
// polygon2d_points_classify @0x4caa40 and polygon2d_clip_to_planes @0x4caee0
// build every plane they use this way, which is why neither of them ever takes
// an array of real_plane2d -- they take the POINTS and pair them up.

// polygon3d_clip_to_plane @0x4cb380: (p.x*pl[0] + p.y*pl[1] + p.z*pl[2]) - pl[3].
// plane3d_intersect_three @0x4cf040 and plane3d_intersect_pair_to_line @0x4cf1e0
// read the same four floats.
typedef struct real_plane3d {
    real_vector3d normal;      // 0x00
    float d;                   // 0x0c distance along the normal
#ifdef __cplusplus   /* C++ only: member functions and hidden-friend operators; no data, layout unchanged */
    /** Signed distance of a point from the plane: normal.dot(point) - d, summed x, y, z. */
    constexpr real distance_to(const real_point3d &p) const { return normal.i * p.x + normal.j * p.y + normal.k * p.z - d; }
    /** The plane facing the other way (plane3d_negate). */
    real_plane3d negated() const;
#endif
} real_plane3d;                // size 0x10

// bounded_ramp_profile: a three-phase (accelerate / coast / decelerate) motion profile bounded by
// a maximum velocity and acceleration. bounded_ramp_profile_build (0x564580) fills it,
// bounded_ramp_profile_evaluate (0x564990) samples it, bounded_ramp_profile_synchronize
// (0x564840) stretches two of them to the same duration. Field names are inferred from that
// arithmetic (see src/math/bounded_ramp_profile_build.c).
typedef struct bounded_ramp_profile {
    uint8_t within_dead_zone;           // 0x00 error and velocity already below 0.001: no ramp
    uint8_t unknown_01[3];              // 0x01
    real start_position;                // 0x04 the position error
    real start_velocity;                // 0x08
    real phase1_acceleration;           // 0x0c
    real phase1_duration;               // 0x10
    real phase2_duration;               // 0x14 constant-velocity coast
    real phase3_acceleration;           // 0x18
    real phase3_duration;               // 0x1c
} bounded_ramp_profile;                 // size 0x20

// quaternion_normalize @0x4cdb20 writes {0,0,0,1} for a degenerate quaternion,
// which fixes element 3 as the scalar part.
typedef struct real_quaternion {
    float i;                   // 0x00
    float j;                   // 0x04
    float k;                   // 0x08
    float w;                   // 0x0c scalar part
#ifdef __cplusplus   /* C++ only: member functions and hidden-friend operators; no data, layout unchanged */
    /** Normalises in place; a zero quaternion becomes the identity (quaternion_normalize). */
    void normalize();
    /** Rotates a vector by this quaternion (quaternion_rotate_vector). */
    real_vector3d rotate(const real_vector3d &v) const;
    /** The rotation as a matrix4x3 with scale 1 and no translation (matrix4x3_from_quaternion). */
    struct real_matrix4x3 to_matrix() const;
    /** The identity rotation (0, 0, 0, 1). */
    static constexpr real_quaternion identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    /** Hamilton product a*b (quaternion_multiply(b, a)). */
    friend real_quaternion operator*(const real_quaternion &a, const real_quaternion &b);
#endif
} real_quaternion;             // size 0x10

typedef struct real_euler_angles2d {
    float yaw;                 // 0x00
    float pitch;               // 0x04
} real_euler_angles2d;         // size 0x08

// matrix4x3_from_euler_angles @0x4cba10 takes (yaw, pitch, roll) in that
// argument order; FUN_004cdde0 @0x4cdde0 feeds it *p, p[1], p[2] from one
// 3-float object, which is what fixes the field order.
typedef struct real_euler_angles3d {
    float yaw;                 // 0x00
    float pitch;               // 0x04
    float roll;                // 0x08
} real_euler_angles3d;         // size 0x0c

// FUN_004cf360 @0x4cf360 clamps (and optionally wraps) a tracked value into
// [param_4, param_5]; 0x0065c284 is a rectangle built out of three of these.
typedef struct real_bounds {
    float lower;               // 0x00
    float upper;               // 0x04
} real_bounds;                 // size 0x08

typedef struct real_rectangle3d {
    real_bounds x;             // 0x00
    real_bounds y;             // 0x08
    real_bounds z;             // 0x10
} real_rectangle3d;            // size 0x18

// ---------------------------------------------------------------------------
// real_matrix3x3  (matrix3x3_transpose @0x4cc500, matrix3x3_multiply @0x4cc5f0,
// matrix3x3_inverse_transform_vector @0x4cc710, quaternion_from_matrix3x3
// @0x4cc780, FUN_004cc560 @0x4cc560)
// Nine floats: matrix3x3_transpose and matrix3x3_multiply copy exactly 9, and
// quaternion_from_matrix3x3 indexes m[i*3 + j] with the diagonal at 0, 4, 8.
// The rows are the basis vectors: matrix3x3_inverse_transform_vector computes
// out.i = m[0]*v.i + m[3]*v.j + m[6]*v.k, i.e. the transpose of the forward /
// left / up rows, which is the inverse for an orthonormal basis.
// ---------------------------------------------------------------------------
typedef struct real_matrix3x3 {
    real_vector3d forward;     // 0x00
    real_vector3d left;        // 0x0c
    real_vector3d up;          // 0x18
#ifdef __cplusplus   /* C++ only: member functions and hidden-friend operators; no data, layout unchanged */
    /** The transposed matrix (matrix3x3_transpose). */
    real_matrix3x3 transposed() const;
    /** The identity basis. */
    static constexpr real_matrix3x3 identity() { return {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}; }
    /** Product a*b (matrix3x3_multiply). */
    friend real_matrix3x3 operator*(const real_matrix3x3 &a, const real_matrix3x3 &b);
#endif
} real_matrix3x3;              // size 0x24

// ---------------------------------------------------------------------------
// real_matrix4x3  (matrix4x3_inverse @0x4cb7a0, matrix4x3_multiply @0x4cc0d0,
// matrix4x3_transform_point @0x4cbde0, matrix4x3_from_quaternion @0x4cbad0,
// quaternion_from_matrix4x3 @0x4cbc00, FUN_004cbd60 @0x4cbd60,
// FUN_004cbd90 @0x4cbd90)
// Thirteen floats: matrix4x3_inverse zero-fills 13 on a degenerate matrix and
// matrix4x3_multiply spills 13 when the destination aliases an operand.
// The uniform scale is separate from the rotation, so the rotation rows stay
// orthonormal: matrix4x3_transform_point pre-multiplies the point by scale only
// when scale != 1, then does out.x = p.i*m[1] + p.j*m[4] + p.k*m[7] + m[10],
// which makes m[1..3] / m[4..6] / m[7..9] the forward / left / up rows and
// m[10..12] the translation. FUN_004cbd60 writes the translation at +0x28,
// +0x2c, +0x30 and FUN_004cbd90 reads forward at +0x04 and up at +0x1c.
// ---------------------------------------------------------------------------
typedef struct real_matrix4x3 {
    float scale;               // 0x00 uniform scale; 0 marks the matrix invalid
    real_vector3d forward;     // 0x04
    real_vector3d left;        // 0x10
    real_vector3d up;          // 0x1c
    real_point3d position;     // 0x28
#ifdef __cplusplus   /* C++ only: member functions and hidden-friend operators; no data, layout unchanged */
    /** Scale, rotate and translate a point (matrix4x3_transform_point). */
    real_point3d transform_point(const real_point3d &p) const;
    /** Scale and rotate a vector (matrix4x3_transform_vector). */
    real_vector3d transform_vector(const real_vector3d &v) const;
    /** Rotate a direction (matrix4x3_transform_normal). */
    real_vector3d transform_normal(const real_vector3d &n) const;
    /** Transform a plane (matrix4x3_transform_plane). */
    real_plane3d transform_plane(const real_plane3d &p) const;
    /** World point to local space (matrix4x3_inverse_transform_point). */
    real_point3d inverse_transform_point(const real_point3d &p) const;
    /** World vector to local space (matrix4x3_inverse_transform_vector). */
    real_vector3d inverse_transform_vector(const real_vector3d &v) const;
    /** World direction to local space (matrix4x3_inverse_transform_normal). */
    real_vector3d inverse_transform_normal(const real_vector3d &n) const;
    /** The inverse transform (matrix4x3_inverse); all zeros when scale is 0. */
    real_matrix4x3 inverse() const;
    /** The rotation as a quaternion (quaternion_from_matrix4x3). */
    real_quaternion to_quaternion() const;
    /** Scale 1, identity rotation, no translation. */
    static constexpr real_matrix4x3 identity()
    {
        return {1.0f, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
    }
    /** Rotation from yaw, pitch and roll (matrix4x3_from_euler_angles). */
    static real_matrix4x3 from_euler_angles(real yaw, real pitch, real roll);
    /** Rotation about a unit axis by the angle with the given sine and cosine (matrix4x3_from_axis_angle). */
    static real_matrix4x3 from_axis_angle(const real_vector3d &axis, real sin_angle, real cos_angle);
    /** Basis from forward and up, left = cross(up, forward), at `position` (matrix4x3_from_forward_up_position). */
    static real_matrix4x3 from_forward_up(const real_vector3d &forward, const real_vector3d &up, const real_point3d &position);
    /** Composition a*b: b applied first (matrix4x3_multiply). */
    friend real_matrix4x3 operator*(const real_matrix4x3 &a, const real_matrix4x3 &b);
#endif
} real_matrix4x3;              // size 0x34

// ---------------------------------------------------------------------------
// shared read-only constants in .rdata, reached through the pointer table in
// .data at 0x006966d0..0x0069674c so the compiler cannot constant-fold them.
// Only 0x00696714 (the zero vector) is dereferenced from inside this module,
// by vector3d_random_point_in_cone @0x4cf530.
// ---------------------------------------------------------------------------
// global 0x0065c208: real_matrix4x3 global_identity_matrix4x3   scale 1, rotation I, position 0
// global 0x0065c20c: real_matrix3x3 global_identity_matrix3x3   aliases the rotation part above
// global 0x0065c20c: real_vector3d global_forward3d    ( 1,  0,  0)
// global 0x0065c218: real_vector3d global_left3d       ( 0,  1,  0)
// global 0x0065c224: real_vector3d global_up3d         ( 0,  0,  1)
// global 0x0065c230: real_point3d  global_origin3d     ( 0,  0,  0)
// global 0x0065c240: real_matrix4x3 global_negative_identity_matrix4x3  scale 1, rotation -I, position 0
// global 0x0065c244: real_vector3d global_backward3d   (-1,  0,  0)
// global 0x0065c250: real_vector3d global_right3d      ( 0, -1,  0)
// global 0x0065c25c: real_vector3d global_down3d       ( 0,  0, -1)
// global 0x0065c274: real_quaternion global_identity_quaternion  (0, 0, 0, 1)
// global 0x0065c284: real_rectangle3d global_null_rectangle3d    each axis {+FLT_MAX, -FLT_MAX}
// global 0x006966d0: const void *global_math_constant_pointers[31]  0x006966d0..0x0069674c
// global 0x00696714: const real_point3d *global_origin3d_pointer     == 0x0065c230
// global 0x00696718: const real_vector3d *global_forward3d_pointer   == 0x0065c20c
// global 0x0069671c: const real_vector3d *global_left3d_pointer      == 0x0065c218
// global 0x00696720: const real_vector3d *global_up3d_pointer        == 0x0065c224

// ---------------------------------------------------------------------------
// pseudo-random numbers  (random_real @0x4019f0, random_real_range @0x401050,
// random_real_range_seeded @0x4cd170, random_seed_generate @0x4cd070,
// random_int_range @0x405320)
// One 32-bit linear congruential generator, seed = seed*0x0019660d + 0x3c6ef35f,
// with the value taken from the high 16 bits scaled by 1/65536 (1.5259022e-05),
// so random_real returns [0,1). random_real_range_seeded takes the state by
// pointer instead of using a global. random_int_range returns
// min + (uint32_t)((max - min) * (seed >> 16)) >> 16 (unsigned multiply and
// logical shift), i.e. [min, max) for max > min.
// ---------------------------------------------------------------------------
typedef uint32_t random_seed;

typedef enum random_constants {
    k_random_multiplier = 0x0019660d,
    k_random_increment  = 0x3c6ef35f,
    k_random_value_shift = 16          // value = (seed >> 16) * (1.0f / 65536.0f)
} random_constants;

// global 0x00719cd0: random_seed random_seed_global
//   deterministic stream; periodic_function_tables_init @0x4cc8d0 forces it to
//   0x20f3f660 before building the wave tables so the tables are reproducible.
// global 0x00719cd4: random_seed local_random_seed
//   non-deterministic stream; sphere_point_table_init @0x4cd0e0 seeds it from
//   random_seed_generate @0x4cd070 (rand() xor two QueryPerformanceCounter
//   readings scaled by the performance frequency at 0x006ac8f8).

// ---------------------------------------------------------------------------
// sphere_mesh  (sphere_mesh_generate @0x4ca4b0, sphere_mesh_build_face @0x4ca5f0,
// sphere_mesh_get_face_point @0x4ca7c0, sphere_mesh_get_edge_point @0x4ca8c0,
// sphere_mesh_interpolate_vertex @0x4ca9a0)
// A subdivided octahedron. sphere_mesh_generate does GlobalAlloc(0, 0x14) for
// the header, which fixes the size exactly, then allocates point_count*0x0c
// bytes of points and triangle_count*8 bytes of indices. The index buffer is a
// run list: each row of a face emits a run length (3, 5, 7, ... = 2*row+1)
// followed by that many point indices, and strip_count counts the runs.
// sphere_point_table_init @0x4cd0e0 calls it with subdivisions = 16
// (mov eax, 0x10 at 0x4cd0ec), giving 1026 points and 2048 triangles.
// ---------------------------------------------------------------------------
typedef struct sphere_mesh {
    int16_t subdivisions;      // 0x00 n; the caller passes it in AX
    int16_t unknown_02;        // 0x02 never written (alignment for the pointer below)
    real_point3d *points;      // 0x04 point_count unit vectors
    int16_t *indices;          // 0x08 run-length encoded triangle strips
    int16_t point_count;       // 0x0c 8*(n-2)*(n-1)/2 - 6 + 12*n
    int16_t triangle_count;    // 0x0e 8*n*n
    int16_t strip_count;       // 0x10 runs written into indices, 8*n
    int16_t unknown_12;        // 0x12 never written (tail padding)
} sphere_mesh;                 // size 0x14

// Scratch tables used while subdividing; both are filled with 0xffff, meaning
// not yet created, and freed before sphere_mesh_generate returns.
// sphere_mesh_get_edge_point indexes edge_cache[lo*8 + hi] over the 6 base
// vertices, and sphere_mesh_generate allocates it as GlobalAlloc(0, 0x80).
typedef struct sphere_mesh_edge_cache {
    int16_t point_index[8][8]; // 0x00 keyed by the two base vertex indices
} sphere_mesh_edge_cache;      // size 0x80

// sphere_mesh_build_face allocates (n+1)*(n+1) int16 and
// sphere_mesh_get_face_point indexes it as [(n+1)*row + column].
typedef struct sphere_mesh_face_cache {
    int16_t point_index[1];    // 0x00 (subdivisions+1)*(subdivisions+1) entries
} sphere_mesh_face_cache;      // size (subdivisions+1)*(subdivisions+1)*2

typedef enum sphere_mesh_constants {
    k_sphere_mesh_base_vertex_count = 6,
    k_sphere_mesh_base_face_count = 8,
    k_sphere_point_table_subdivisions = 16,  // sphere_point_table_init
    k_sphere_point_table_count = 1026        // point_count for n == 16
} sphere_mesh_constants;

// global 0x0065c190: real_point3d k_octahedron_vertices[6]
//   (0,0,1) (0,1,0) (1,0,0) (0,-1,0) (-1,0,0) (0,0,-1); read directly from .rdata
// global 0x0065c1d8: int16_t k_octahedron_faces[8][3]
//   {0,1,2} {0,2,3} {0,3,4} {0,4,1} {5,1,4} {5,4,3} {5,3,2} {5,2,1}
//   sphere_mesh_generate walks it as 8 triples starting one short before
//   0x0065c1da, and it ends exactly where global_identity_matrix4x3 begins.

// The quasi-uniform direction table seeded from the sphere mesh.
// global 0x006b7af4: real_point3d *sphere_point_table   1026 unit vectors
// global 0x006b7af8: int16_t sphere_point_table_count   1026
//   FUN_004cd1b0 @0x4cd1b0 picks an entry with
//   index = ((seed >> 16) * count) >> 16 and uses it as a rotation axis.

// ---------------------------------------------------------------------------
// periodic (wave) functions  (periodic_function_tables_init @0x4cc8d0,
// periodic_function_tables_free @0x4cc960, periodic_function_evaluate @0x4cc9b0,
// periodic_function_build_table @0x4ccdb0, FUN_004ccbb0 @0x4ccbb0,
// FUN_004cccb0 @0x4cccb0, FUN_004ccac0 @0x4ccac0)
// Twelve byte tables of 0x400 entries each, one per periodic_function, plus six
// tables of the same shape for the transition_function easing curves. A table
// entry is value*255, and periodic_function_evaluate masks the sample index with
// 0x3ff and lerps between neighbours.
// The twelve cases of the switch in periodic_function_build_table line up one
// for one with the WaveFunction enum already in types/tags.h (case 2 is a plain
// cosine, case 8 is raw LCG noise, case 0x0b squares its sample), and the six
// cases in FUN_004cccb0 line up with FunctionType (case 5 is the cosine easing
// sin(t*pi - pi/2)); that tag-side definition is the evidence for both orders.
// ---------------------------------------------------------------------------
typedef enum periodic_function {
    _periodic_function_one = 0,
    _periodic_function_zero = 1,
    _periodic_function_cosine = 2,
    _periodic_function_cosine_variable_period = 3,
    _periodic_function_diagonal_wave = 4,
    _periodic_function_diagonal_wave_variable_period = 5,
    _periodic_function_slide = 6,
    _periodic_function_slide_variable_period = 7,
    _periodic_function_noise = 8,
    _periodic_function_jitter = 9,
    _periodic_function_wander = 10,
    _periodic_function_spark = 11,
    k_periodic_function_count = 12
} periodic_function;
typedef int16_t periodic_function_t;

typedef enum transition_function {
    _transition_function_linear = 0,
    _transition_function_early = 1,
    _transition_function_very_early = 2,
    _transition_function_late = 3,
    _transition_function_very_late = 4,
    _transition_function_cosine = 5,
    k_transition_function_count = 6
} transition_function;
typedef int16_t transition_function_t;

// Both periodic_function_evaluate @0x4cc9b0 and periodic_function_build_table
// @0x4ccdb0 test (1 << type) & 0xc0, i.e. slide and slide_variable_period.
// The evaluator wraps their interpolation modulo 1 because those two are
// sawtooths rather than continuous waves, and the builder uses the same mask to
// exempt them from the min/max renormalization described below.
typedef enum periodic_function_flags {
    k_periodic_function_wrapping_mask = 0x00c0
} periodic_function_flags;

typedef enum periodic_function_constants {
    k_periodic_function_table_size = 0x400,  // GlobalAlloc(0, 0x400) per table
    k_periodic_function_table_mask = 0x3ff   // sample index mask
    // a table entry is value * 255; the evaluators scale it by 0.003921569
    // (1/255), and periodic_function_evaluate maps time to an index with t*25.6
} periodic_function_constants;

// How a table entry is actually produced (read off the disassembly at 0x4ccdb0
// and 0x4cccb0, which Ghidra drops):
//   periodic tables: the builder samples the wave at 1024 phases keeping a
//     running minimum and maximum, then emits
//       (value - minimum) / (maximum - minimum) * 255
//     except for slide / slide_variable_period, which are already in [0,1) and
//     are emitted as value * 255. A range of exactly 0 (the constant waves)
//     skips the division, so a constant-1.0 table is all 255 and a constant-0.0
//     table is all 0. A table is therefore NOT a raw sample of the waveform.
//   transition tables: no normalization; entry i is curve(i / 1023) * 255, and
//     transition_function_evaluate samples them at phase*1023 -- 1023, not 1024,
//     so the last entry is the endpoint and the evaluator special-cases index
//     0x3ff rather than reading samples[1024].
//   Both quantizers finish with __ftol (0x6391b4) then clamp to 0..255.
typedef struct periodic_function_table {
    uint8_t samples[1024];     // 0x00 value*255 at 1024 evenly spaced phases
} periodic_function_table;     // size 0x400

// global 0x006b7aa8: periodic_function_table *periodic_function_tables[12]   indexed by periodic_function
// global 0x006b7ad8: periodic_function_table *transition_function_tables[6]  indexed by transition_function
// global 0x006b7af0: uint8_t periodic_functions_initialized
//   set to 1 at the top of periodic_function_tables_init and cleared if any
//   GlobalAlloc fails; every evaluator returns 0 while it is clear.

// ---------------------------------------------------------------------------
// projection axis table  (FUN_004ce8c0 @0x4ce8c0)
// To turn a 3D triangle/point problem into a 2D one, the component of the plane
// normal with the largest magnitude is dropped and the remaining two are used.
// The pair is chosen by dominant axis and by the sign of that component so the
// winding of the projected triangle is preserved. FUN_004ce8c0 indexes the
// table with ((component > 0) + axis*2)*4, which makes the entry 4 bytes wide.
// Read straight out of .rdata: {2,1} {1,2} {0,2} {2,0} {1,0} {0,1}. The table
// runs 0x0065c29c..0x0065c2b4, ending exactly where the bit_stream mask table
// bit_mask_clear (types/memory.h) begins.
// ---------------------------------------------------------------------------
typedef struct projection_axis_pair {
    int16_t i;                 // 0x00 first surviving axis index, 0=x 1=y 2=z
    int16_t j;                 // 0x02 second surviving axis index
} projection_axis_pair;        // size 0x04

// global 0x0065c29c: projection_axis_pair k_projection_axes[6]  [dominant_axis*2 + (component > 0)]

// ---------------------------------------------------------------------------
// quaternion extraction index table  (quaternion_from_matrix4x3 @0x4cbc00,
// quaternion_from_matrix3x3 @0x4cc780)
// When the matrix trace is non-positive both functions pick the largest diagonal
// element i and then walk i -> j -> k with next[i]. Both tables read {1, 2, 0}
// in .data; they are duplicated because each function got its own copy.
// ---------------------------------------------------------------------------
// global 0x0069665c: int16_t k_quaternion_next_index_matrix4x3[3]  {1, 2, 0}, 0x00696662 is padding
// global 0x00696668: int16_t k_quaternion_next_index_matrix3x3[3]  {1, 2, 0}, 0x0069666e is padding

// ---------------------------------------------------------------------------
// matrix multiply dispatch  (math_initialize @0x4cd3f0)
// math_initialize installs the scalar matrix4x3_multiply, then upgrades it to a
// SIMD build unless the command line contains -noSSE and unless safe_mode at
// 0x007196f4 is set (a shell command-line BOOL, see types/shell.h). cpu_get_type(0x1d) selects the routine at 0x004cc250
// (MOVSS / MOVHPS / SHUFPS, i.e. SSE) and cpu_get_type(0x1a) selects the one at
// 0x004cc3a0 (21 0f 0f escapes, i.e. AMD 3DNow!).
// ---------------------------------------------------------------------------
// global 0x00696664: void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
//   default 0x004cc0d0 (scalar), SSE 0x004cc250, 3DNow! 0x004cc3a0

#pragma pack(pop)

#endif
