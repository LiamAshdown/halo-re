/**
 * @file include/halo/math/math_globals.h
 * The math module's engine globals, under their original C symbol names. They are defined at their fixed addresses
 * in the data image (standalone/data/*.c, globals.asm), never by the math module, so these are declarations only and
 * keep C linkage.
 */
#pragma once

#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The deterministic LCG stream (0x00719cd0); periodic_function_tables_init seeds it with 0x20f3f660. */
extern random_seed random_seed_global;
/** The non-deterministic LCG stream for effects (0x00719cd4); seeded by sphere_point_table_init. */
extern random_seed effect_random_seed;
/** The twelve periodic function tables, indexed by periodic_function (0x006b7aa8). */
extern periodic_function_table *periodic_function_tables[12];
/** The six transition function tables, indexed by transition_function (0x006b7ad8). */
extern periodic_function_table *transition_function_tables[6];
/** Nonzero while the periodic and transition tables exist (0x006b7af0). */
extern uint8_t periodic_functions_initialized;
/** The 1026 quasi-uniform unit directions built by sphere_point_table_init (0x006b7af4). */
extern real_point3d *sphere_point_table;
/** Number of entries in sphere_point_table (0x006b7af8). */
extern int16_t sphere_point_table_count;
/** The matrix4x3_multiply variant math_initialize selected for the CPU (0x00696664). */
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);
/** The six octahedron vertices the sphere mesh subdivides (0x0065c190). */
extern real_point3d k_octahedron_vertices[6];
/** The eight octahedron faces as vertex index triples (0x0065c1d8). */
extern int16_t k_octahedron_faces[8][3];
/** Surviving axis pairs indexed [dominant_axis*2 + (component > 0)] (0x0065c29c). */
extern projection_axis_pair k_projection_axes[6];
/** {1, 2, 0}, the next-axis table of quaternion_from_matrix3x3 (0x00696668). */
extern int16_t k_quaternion_next_index_matrix3x3[3];
/** {1, 2, 0}, the next-axis table of quaternion_from_matrix4x3 (0x0069665c). */
extern int16_t k_quaternion_next_index_matrix4x3[3];
/** Pointer to the constant (1, 0, 0) (0x00696718). */
extern const real_vector3d *global_forward3d_pointer;
/** Pointer to the constant (0, 1, 0) (0x0069671c). */
extern const real_vector3d *global_left3d_pointer;
/** Pointer to the constant (0, 0, 1) (0x00696720). */
extern const real_vector3d *global_up3d_pointer;
/** The constant origin (0, 0, 0) (0x0065c230). */
extern real_point3d global_origin3d;

#ifdef __cplusplus
}
#endif
