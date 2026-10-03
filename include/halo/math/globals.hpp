/**
 * @file include/halo/math/globals.hpp
 * The math module's engine globals as one service object. The variables live at fixed addresses in the data
 * image (standalone/data) under their original link names; Globals holds a reference to each, so no other file
 * declares them.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * References to the math module's engine variables.
 *
 * random_seed_global is the deterministic LCG stream (0x00719cd0), effect_random_seed the non-deterministic one
 * for effects (0x00719cd4). The periodic and transition tables are indexed by function id; sphere_point_table
 * holds the 1026 quasi-uniform unit directions. matrix4x3_multiply_procedure is the CPU-selected multiply. The
 * global_*3d_pointer members point at the constant axes (1,0,0), (0,1,0), (0,0,1).
 */
struct Globals {
    random_seed &random_seed_global;
    random_seed &effect_random_seed;
    periodic_function_table *(&periodic_function_tables)[12];
    periodic_function_table *(&transition_function_tables)[6];
    uint8_t &periodic_functions_initialized;
    real_point3d *&sphere_point_table;
    int16_t &sphere_point_table_count;
    void (*&matrix4x3_multiply_procedure)(const real_matrix4x3 *a, const real_matrix4x3 *b, real_matrix4x3 *out);
    real_point3d (&k_octahedron_vertices)[6];
    int16_t (&k_octahedron_faces)[8][3];
    projection_axis_pair (&k_projection_axes)[6];
    int16_t (&k_quaternion_next_index_matrix3x3)[3];
    int16_t (&k_quaternion_next_index_matrix4x3)[3];
    real_vector3d *&global_forward3d_pointer;
    real_vector3d *&global_left3d_pointer;
    real_vector3d *&global_up3d_pointer;
    real_point3d &global_origin3d;
};

extern const Globals math_globals;

/** The math service object (single instance, constant-initialised). */
inline const Globals &globals() { return math_globals; }

}  // namespace halo::math
