/**
 * @file src/scenario/scenario_data.cpp
 * Scenario loading, object name lookup and the globals material table.
 */

#include "halo/scenario/scenario.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"


namespace halo::scenario {

uint8_t scenario_loader::load(char *path)
{
    char *scan;
    char *next_newline;
    int32_t i;
    uint8_t result;

    globals().scenario_index = halo::cache::cache_file_load(path);
    if (globals().scenario_index == (datum_index)k_datum_index_none) {
        result = 0;
        scan = globals().k_empty_string;
        do {
            next_newline = strchr(scan, '\n');
            result = 0;
            if (next_newline == (char *)0) {
                break;
            }
            scan = next_newline + 1;
            *next_newline = '\n';
            result = 0;
        } while (scan != (char *)0);
        return result;
    }

    globals().scenario = (Scenario *)halo::cache::globals().tag_instances[halo::datum_slot(globals().scenario_index)].data;
    if ((int32_t)globals().scenario->structure_bsps.count <= 0) {
        return 0;
    }

    globals().global_globals = (::Globals *)halo::cache::globals().tag_instances[
        halo::datum_slot(halo::cache::tag_lookup(halo::groups::globals, "globals\\globals"))].data;

    if (structure_bsp_switcher::switch_to(0) == 0) {
        return 0;
    }

    if (0 < (int32_t)globals().scenario->netgame_equipment.count) {
        ScenarioNetgameEquipment *equipment =
            (ScenarioNetgameEquipment *)globals().scenario->netgame_equipment.pointer;
        for (i = 0; i < (int32_t)globals().scenario->netgame_equipment.count; i++) {
            equipment[i].spawned_item = halo::k_dword_none;
        }
    }
    return 1;
}

int16_t scenario_view::object_name_find_index(char *name)
{
    ScenarioObjectName *names = (ScenarioObjectName *)self->object_names.pointer;
    int16_t i;

    for (i = 0; i < (int32_t)self->object_names.count; i++) {
        if (strcmp(names[i].name.string, name) == 0) {
            return i;
        }
    }
    return -1;
}

GlobalsMaterial * scenario_loader::globals_material_get(int16_t material_index)
{
    GlobalsMaterial *materials;

    if (0 <= material_index && (int32_t)material_index < (int32_t)globals().global_globals->materials.count) {
        materials = (GlobalsMaterial *)globals().global_globals->materials.pointer;
        return &materials[material_index];
    }

    if (globals().material_table_warning_issued == 0) {
        *(uint32_t *)&globals().material_table_fallback.melee_hit_sound.tag_id = halo::k_dword_none;
        globals().material_table_warning_issued = 1;
    }
    return &globals().material_table_fallback;
}

}  // namespace halo::scenario
