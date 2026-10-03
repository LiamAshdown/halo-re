/**
 * @file src/scenario/structure_bsp_switch.cpp
 * Switching the resident structure bsp and the activate/deactivate callback tables.
 * The original author notes and decompiles are in docs/original/scenario/.
 */

#include "halo/scenario/scenario.hpp"
#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"

extern "C" {
extern structure_bsp_procedure structure_bsp_activate_procedures[k_structure_bsp_activate_procedure_count];
extern structure_bsp_procedure structure_bsp_deactivate_procedures[k_structure_bsp_deactivate_procedure_count];
extern int16_t global_structure_bsp_index;
extern Scenario *global_scenario;
extern uint8_t unknown_00719769;
extern uint8_t unknown_0071976a;
extern scenario_game_globals *global_scenario_game_globals;
extern ScenarioStructureBSP *global_structure_bsp;
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
}

namespace halo::scenario {

void structure_bsp_switcher::activate_callbacks(void)
{
    int32_t i;

    for (i = 0; i < k_structure_bsp_activate_procedure_count; i++) {
        structure_bsp_activate_procedures[i]();
    }
}

void structure_bsp_switcher::deactivate_callbacks(void)
{
    int32_t i;

    for (i = 0; i < k_structure_bsp_deactivate_procedure_count; i++) {
        structure_bsp_deactivate_procedures[i]();
    }
}

uint8_t structure_bsp_switcher::switch_to(int16_t structure_bsp_index)
{
    ScenarioBSP *new_entry;
    uint8_t old_bsp_was_active;
    uint32_t tag_index;

    if (structure_bsp_index == global_structure_bsp_index || structure_bsp_index < 0) {
        return 0;
    }
    if ((int32_t)global_scenario->structure_bsps.count <= (int32_t)structure_bsp_index) {
        return 0;
    }

    new_entry = &((ScenarioBSP *)global_scenario->structure_bsps.pointer)[structure_bsp_index];
    old_bsp_was_active = (global_structure_bsp_index != -1);

    unknown_00719769 = 0;
    unknown_0071976a = 0;

    if (old_bsp_was_active) {
        ScenarioBSP *old_entry;

        structure_bsp_switcher::deactivate_callbacks();
        old_entry = &((ScenarioBSP *)global_scenario->structure_bsps.pointer)
                        [global_structure_bsp_index];
        halo::cache::structure_bsp_dispose(old_entry);
        global_scenario_game_globals->structure_bsp_index = -1;
        global_structure_bsp_index = -1;
    }

    if (!halo::cache::structure_bsp_load(new_entry)) {
        unknown_0071976a = 1;
        return 0;
    }

    tag_index = new_entry->structure_bsp.tag_id.index;
    global_structure_bsp = (ScenarioStructureBSP *)halo::cache::globals().tag_instances[tag_index].data;
    global_structure_collision_bsp =
        (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    halo::physics::globals().collision_bsp = (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    global_scenario_game_globals->structure_bsp_index = structure_bsp_index;
    global_structure_bsp_index = structure_bsp_index;

    if (old_bsp_was_active) {
        structure_bsp_switcher::activate_callbacks();
    }

    unknown_0071976a = 1;
    return 1;
}

void structure_bsp_switcher::switch_after_load(void)
{
    ScenarioBSP *old_entry;
    int32_t tag_index;
    int16_t requested_index;

    if (global_scenario_game_globals->structure_bsp_index == global_structure_bsp_index) {
        return;
    }

    old_entry = &((ScenarioBSP *)global_scenario->structure_bsps.pointer)
                    [global_structure_bsp_index];
    halo::cache::structure_bsp_dispose_material_vertex_buffers(
        (ScenarioStructureBSPCompiledHeader *)halo::cache::globals().structure_bsp_data);

    tag_index = (int16_t)old_entry->structure_bsp.tag_id.index;

    halo::cache::globals().tag_instances[tag_index].data = 0;
    halo::cache::globals().structure_bsp_data = 0;

    requested_index = global_scenario_game_globals->structure_bsp_index;
    global_structure_bsp_index = -1;
    structure_bsp_switcher::switch_to(requested_index);
}

uint8_t structure_bsp_switcher::locate_point_nudge_up(real_point3d *point)
{
    int16_t attempts;
    uint32_t leaf;

    leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, point);
    attempts = 0;
    while (leaf == halo::k_dword_none && attempts < k_scenario_location_nudge_attempts) {
        attempts++;
        point->z = point->z + 0.05f;
        leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, point);
    }
    return attempts == 0;
}

}  // namespace halo::scenario
