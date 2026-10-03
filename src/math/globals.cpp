/**
 * @file src/math/globals.cpp
 * Binds halo::math::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/math/globals.hpp"

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
}

namespace halo::math {

const Globals math_globals{
    ::random_seed_global,
    ::effect_random_seed,
    ::periodic_function_tables,
    ::transition_function_tables,
    ::periodic_functions_initialized,
    ::sphere_point_table,
    ::sphere_point_table_count,
    ::matrix4x3_multiply_procedure,
    ::k_octahedron_vertices,
    ::k_octahedron_faces,
    ::k_projection_axes,
    ::k_quaternion_next_index_matrix3x3,
    ::k_quaternion_next_index_matrix4x3,
    ::global_forward3d_pointer,
    ::global_left3d_pointer,
    ::global_up3d_pointer,
    ::global_origin3d,
};

}  // namespace halo::math
