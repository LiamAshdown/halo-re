/**
 * @file src/math/globals.cpp
 * Binds halo::math::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/math/math.hpp"
#include "crt.h"
#include "tags.h"
#include "halo/shell/api.hpp"
#include "halo/core/crt.hpp"
#include "halo/math/globals.hpp"
#include "link/math.hpp"

namespace halo::math {

const Globals &Service::instance()
{
    static const Globals state{
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
        ::safe_mode,
    };
    return state;
}

}  // namespace halo::math
