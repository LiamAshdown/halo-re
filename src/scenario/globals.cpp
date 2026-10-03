/**
 * @file src/scenario/globals.cpp
 * Binds halo::scenario::Globals to the engine variables the data image defines under their original link names.
 */

#include <stdarg.h>
#include "halo/scenario/scenario.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/cache/api.hpp"
#include "halo/math/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/render/api.hpp"
#include "halo/core/crt.hpp"
#include "halo/scenario/api.hpp"
#include "link/scenario.hpp"

static_assert(k_structure_bsp_activate_procedure_count == 13 && k_structure_bsp_deactivate_procedure_count == 10);

namespace halo::scenario {

Globals &Service::instance()
{
    static Globals state{
        ::global_scenario,
        ::global_scenario_index,
        ::global_structure_bsp,
        ::global_structure_bsp_index,
        ::global_scenario_game_globals,
        ::global_globals,
        ::k_empty_string,
        ::material_table_warning_issued,
        ::material_table_fallback,
        ::structure_bsp_activate_procedures,
        ::structure_bsp_deactivate_procedures,
        ::unknown_00719769,
        ::unknown_0071976a,
    };
    return state;
}

}  // namespace halo::scenario
