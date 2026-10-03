// tests/math/math_test_stubs.h -- what the math module needs from the rest of the engine, for the difftest.
// The definitions (math_test_stubs.cpp) are trivial and are linked, unchanged, into BOTH test executables: the one
// built from the original src/math/*.c and the one built from the converted src/math/*.cpp. Only the math module
// differs between the two, so any difference in the output is a difference in the math module.
#pragma once

#include "math.h"   // types/math.h

extern "C" {
// engine globals the math module reads or writes (standalone/data / globals.asm in the real build)
extern char **shell_argv;
extern int32_t shell_argc;
extern int32_t safe_mode;
extern projection_axis_pair k_projection_axes[6];
extern const real_vector3d *global_forward3d_pointer;
extern const real_vector3d *global_left3d_pointer;
extern const real_vector3d *global_up3d_pointer;
extern real_point3d global_origin3d;
extern float k_physics_gravity;
extern int16_t k_octahedron_faces[8][3];
extern real_point3d k_octahedron_vertices[6];
extern int16_t k_quaternion_next_index_matrix3x3[3];
extern int16_t k_quaternion_next_index_matrix4x3[3];
extern int16_t sphere_point_table_count;
extern real_point3d *sphere_point_table;
extern int64_t performance_frequency;
extern periodic_function_table *periodic_function_tables[12];
extern periodic_function_table *transition_function_tables[6];
extern uint8_t periodic_functions_initialized;
extern random_seed effect_random_seed;
extern random_seed random_seed_global;
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);

// other modules' functions the math module calls
int cpu_get_type(int feature);
uint8_t real_approximately_equal(real a, real b);
uint8_t vector3d_is_unit_length(real_vector3d *v);
void vector3d_clamp_length(real_vector3d *v, real max_length);
}

// test control for cpu_get_type: bit n set -> cpu_get_type(n) returns 1
extern uint32_t g_test_cpu_features_low;    // features 0..31
