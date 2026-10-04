/**
 * @file standalone/data/link/scenario.hpp
 * Link names of the engine variables the scenario module binds in halo::scenario::Globals (src/scenario/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern Scenario *global_scenario;
extern datum_index global_scenario_index;
extern ScenarioStructureBSP *global_structure_bsp;
extern int16_t global_structure_bsp_index;
extern scenario_game_globals *global_scenario_game_globals;
extern Globals *global_globals;
extern char k_empty_string[1];
extern uint8_t material_table_warning_issued;
extern GlobalsMaterial material_table_fallback;
extern structure_bsp_procedure structure_bsp_activate_procedures[k_structure_bsp_activate_procedure_count];
extern structure_bsp_procedure structure_bsp_deactivate_procedures[k_structure_bsp_deactivate_procedure_count];
extern uint8_t time_is_running;
extern uint8_t reset_frame_timers;
}
