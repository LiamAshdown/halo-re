/**
 * @file src/models/globals.cpp
 * Binds halo::models::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/models/models.hpp"
#include "halo/models/flags.hpp"
#include "halo/cache/api.hpp"
#include "halo/math/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/rasterizer/globals.hpp"
#include "halo/render/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/crt.hpp"
#include "halo/models/globals.hpp"
#include "link/models.hpp"
#include "halo/models/api.hpp"

namespace halo::models {

Globals &Service::instance()
{
    static Globals state{
        ::animation_compressed_data_enabled,
        ::global_identity_quaternion_pointer,
        ::model_render_first_person,
        ::model_render_default_region_permutations,
        ::model_render_default_effect,
        ::model_render_default_change_colors,
        ::model_render_default_function_values,
        ::console_model_lod_override,
        ::rasterizer_caps_flag_689,
        ::console_debug_toggle_6893f2,
        ::rasterizer_object_shadow_model_context,
        ::rasterizer_object_shadow_model_active,
    };
    return state;
}

}  // namespace halo::models
