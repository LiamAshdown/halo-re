/**
 * @file standalone/data/link/math.hpp
 * Link names of the engine variables the math module binds in halo::math::Globals (src/math/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern random_seed random_seed_global;
extern random_seed effect_random_seed;
extern periodic_function_table *periodic_function_tables[12];
extern periodic_function_table *transition_function_tables[6];
extern uint8_t periodic_functions_initialized;
extern real_point3d *sphere_point_table;
extern int16_t sphere_point_table_count;
extern void (*matrix4x3_multiply_procedure)(const real_matrix4x3 *a, const real_matrix4x3 *b, real_matrix4x3 *out);
extern real_point3d k_octahedron_vertices[6];
extern int16_t k_octahedron_faces[8][3];
extern projection_axis_pair k_projection_axes[6];
extern int16_t k_quaternion_next_index_matrix3x3[3];
extern int16_t k_quaternion_next_index_matrix4x3[3];
extern real_vector3d *global_forward3d_pointer;
extern real_vector3d *global_left3d_pointer;
extern real_vector3d *global_up3d_pointer;
extern real_point3d global_origin3d;
extern int32_t safe_mode;
}
