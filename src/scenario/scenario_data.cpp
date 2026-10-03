/**
 * @file src/scenario/scenario_data.cpp
 * Scenario loading, object name lookup and the globals material table.
 * The original author notes and decompiles are in docs/original/scenario/.
 */

#include "halo/scenario/scenario.hpp"

extern "C" {
extern datum_index cache_file_load(char *path);
extern datum_index tag_lookup(tag_group group, char *path);
extern tag_instance *tag_instances;
extern datum_index global_scenario_index;
extern Scenario *global_scenario;
extern Globals *global_globals;
extern char k_empty_string[1];
extern uint8_t material_table_warning_issued;
extern GlobalsMaterial material_table_fallback;
}

namespace halo::scenario {

uint8_t scenario_loader::load(char *path)
{
    char *scan;
    char *next_newline;
    int32_t i;
    uint8_t result;

    global_scenario_index = cache_file_load(path);
    if (global_scenario_index == (datum_index)0xffffffff) {
        result = 0;
        scan = k_empty_string;
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

    global_scenario = (Scenario *)tag_instances[global_scenario_index & 0xffff].data;
    if ((int32_t)global_scenario->structure_bsps.count <= 0) {
        return 0;
    }

    global_globals = (Globals *)tag_instances[
        tag_lookup(0x6d617467 , (char *)"globals\\globals") & 0xffff].data;

    if (structure_bsp_switcher::switch_to(0) == 0) {
        return 0;
    }

    if (0 < (int32_t)global_scenario->netgame_equipment.count) {
        ScenarioNetgameEquipment *equipment =
            (ScenarioNetgameEquipment *)global_scenario->netgame_equipment.pointer;
        for (i = 0; i < (int32_t)global_scenario->netgame_equipment.count; i++) {
            equipment[i].unknown_ffffffff = 0xffffffff;
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

    if (0 <= material_index && (int32_t)material_index < (int32_t)global_globals->materials.count) {
        materials = (GlobalsMaterial *)global_globals->materials.pointer;
        return &materials[material_index];
    }

    if (material_table_warning_issued == 0) {
        *(uint32_t *)&material_table_fallback.melee_hit_sound.tag_id = 0xffffffff;
        material_table_warning_issued = 1;
    }
    return &material_table_fallback;
}

}  // namespace halo::scenario
