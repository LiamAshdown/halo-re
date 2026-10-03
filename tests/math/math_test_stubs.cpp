/**
 * @file tests/math/math_test_stubs.cpp
 * Trivial definitions of the engine globals and other-module functions the math module references, linked unchanged
 * into both difftest executables. Table values are the retail .rdata contents documented in types/math.h;
 * performance_frequency is INT64_MAX so random_seed_generate reduces to rand() and is reproducible.
 */
#include "math_test_stubs.h"

static const real_vector3d k_test_forward = {1.0f, 0.0f, 0.0f};
static const real_vector3d k_test_left = {0.0f, 1.0f, 0.0f};
static const real_vector3d k_test_up = {0.0f, 0.0f, 1.0f};
static char k_test_arg0[] = "halo.exe";
static char *k_test_argv[4] = {k_test_arg0, nullptr, nullptr, nullptr};

uint32_t g_test_cpu_features_low = 0;

extern "C" {
char **shell_argv = k_test_argv;
int32_t shell_argc = 1;
int32_t safe_mode = 0;
projection_axis_pair k_projection_axes[6] = {{2, 1}, {1, 2}, {0, 2}, {2, 0}, {1, 0}, {0, 1}};
const real_vector3d *global_forward3d_pointer = &k_test_forward;
const real_vector3d *global_left3d_pointer = &k_test_left;
const real_vector3d *global_up3d_pointer = &k_test_up;
real_point3d global_origin3d = {0.0f, 0.0f, 0.0f};
float k_physics_gravity = 0.0035651792f;
int16_t k_octahedron_faces[8][3] = {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1}, {5, 1, 4}, {5, 4, 3}, {5, 3, 2}, {5, 2, 1}};
real_point3d k_octahedron_vertices[6] = {{0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {0, -1, 0}, {-1, 0, 0}, {0, 0, -1}};
int16_t k_quaternion_next_index_matrix3x3[3] = {1, 2, 0};
int16_t k_quaternion_next_index_matrix4x3[3] = {1, 2, 0};
int16_t sphere_point_table_count = 0;
real_point3d *sphere_point_table = nullptr;
int64_t performance_frequency = 0x7fffffffffffffffLL;
periodic_function_table *periodic_function_tables[12] = {};
periodic_function_table *transition_function_tables[6] = {};
uint8_t periodic_functions_initialized = 0;
random_seed effect_random_seed = 0x1234567u;
random_seed random_seed_global = 0x89abcdefu;
void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out) = nullptr;

int cpu_get_type(int feature)
{
    return (feature >= 0 && feature < 32) ? (int)((g_test_cpu_features_low >> feature) & 1) : 0;
}

uint8_t real_approximately_equal(real a, real b)
{
    real d = a - b;
    return (uint8_t)(d < 0.001f && d > -0.001f);
}

uint8_t vector3d_is_unit_length(real_vector3d *v)
{
    real d = v->i * v->i + v->j * v->j + v->k * v->k - 1.0f;
    return (uint8_t)(d < 0.001f && d > -0.001f);
}

void vector3d_clamp_length(real_vector3d *v, real max_length)
{
    real s = v->i * v->i + v->j * v->j + v->k * v->k;
    if (s > max_length * max_length && s > 0.0f) {
        real f = max_length * max_length / s;
        v->i = v->i * f;
        v->j = v->j * f;
        v->k = v->k * f;
    }
}
}
