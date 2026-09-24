// Phase 4 syntax gate for types/scenario.h. The host gcc is 64-bit, so a struct that holds a
// pointer would be 4 bytes larger per pointer than in halo.exe; scenario.h has no pointer members,
// so every check below fires either way.
#include "tags.h"
#include "memory.h"
#include "scenario.h"

#define CHECK(name, expr) typedef char name[(expr) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// the module records
CHECK(check_sky_fog_size, sizeof(scenario_sky_fog_state) == 0x2c);
CHECK(check_sky_fog_position, OFF(scenario_sky_fog_state, camera_position) == 0x04);
CHECK(check_sky_fog_start, OFF(scenario_sky_fog_state, start_distance) == 0x10);
CHECK(check_sky_fog_opaque, OFF(scenario_sky_fog_state, opaque_distance) == 0x14);
CHECK(check_sky_fog_density, OFF(scenario_sky_fog_state, maximum_density) == 0x18);
CHECK(check_sky_fog_color, OFF(scenario_sky_fog_state, color) == 0x1c);
CHECK(check_sky_fog_blend, OFF(scenario_sky_fog_state, fog_screen_blend) == 0x28);
CHECK(check_game_globals_size, sizeof(scenario_game_globals) == 0x7c);
CHECK(check_game_globals_size_enum, sizeof(scenario_game_globals) == k_scenario_game_globals_size);
CHECK(check_game_globals_fog, OFF(scenario_game_globals, sky_fog) == 0x04);
CHECK(check_game_globals_latch, OFF(scenario_game_globals, sound_environment_is_water) == 0x30);
CHECK(check_game_globals_env, OFF(scenario_game_globals, sound_environment) == 0x34);
CHECK(check_box_size, sizeof(scenario_trigger_volume_box) == 0x18);
CHECK(check_box_fixed, OFF(scenario_trigger_volume_fixed_box, y_bounds) == 0x08
                       && OFF(scenario_trigger_volume_fixed_box, z_bounds) == 0x10);
CHECK(check_box_rot, OFF(scenario_trigger_volume_rotational_box, extents) == 0x0c);
CHECK(check_proc_tables, 0x0069e8dc + k_structure_bsp_activate_procedure_count * 4 == 0x0069e910);
CHECK(check_default_material, 0x006e3208 + OFF(GlobalsMaterial, melee_hit_sound) + 0x0c == 0x006e3578);

// the tag layouts every offset in scenario.h rests on
CHECK(check_scnr_size, sizeof(Scenario) == 0x5b0);
CHECK(check_scnr_skies, OFF(Scenario, skies) == 0x30);
CHECK(check_scnr_names, OFF(Scenario, object_names) == 0x204);
CHECK(check_scnr_tv, OFF(Scenario, trigger_volumes) == 0x360);
CHECK(check_scnr_neteq, OFF(Scenario, netgame_equipment) == 0x384);
CHECK(check_scnr_bsps, OFF(Scenario, structure_bsps) == 0x5a4);
CHECK(check_scnr_bsp_entry, sizeof(ScenarioBSP) == 0x20 && OFF(ScenarioBSP, structure_bsp) + 0x0c == 0x1c);
CHECK(check_objname, sizeof(ScenarioObjectName) == 0x24);
CHECK(check_neteq, sizeof(ScenarioNetgameEquipment) == 0x90 && OFF(ScenarioNetgameEquipment, unknown_ffffffff) == 0x10);
CHECK(check_tv_size, sizeof(ScenarioTriggerVolume) == 0x60);
CHECK(check_tv_fwd, OFF(ScenarioTriggerVolume, rotation_vector_forward) == 0x30
                    && OFF(ScenarioTriggerVolume, rotation_vector_up) == 0x3c);
CHECK(check_tv_box, OFF(ScenarioTriggerVolume, starting_corner) == 0x48
                    && OFF(ScenarioTriggerVolume, ending_corner_offset) == 0x54);
CHECK(check_sbsp_cbsp, OFF(ScenarioStructureBSP, collision_bsp) + 4 == 0xb4);
CHECK(check_sbsp_leaves, OFF(ScenarioStructureBSP, leaves) + 4 == 0xe4);
CHECK(check_sbsp_clusters, OFF(ScenarioStructureBSP, clusters) == 0x134);
CHECK(check_sbsp_pvs, OFF(ScenarioStructureBSP, cluster_data) + 0x0c == 0x14c);
CHECK(check_sbsp_fogplanes, OFF(ScenarioStructureBSP, fog_planes) + 4 == 0x17c);
CHECK(check_sbsp_fogregions, OFF(ScenarioStructureBSP, fog_regions) + 4 == 0x188);
CHECK(check_sbsp_fogpalette, OFF(ScenarioStructureBSP, fog_palette) + 4 == 0x194);
CHECK(check_sbsp_bgsound, OFF(ScenarioStructureBSP, background_sound_palette) == 0x1fc);
CHECK(check_cluster, sizeof(ScenarioStructureBSPCluster) == 0x68 && OFF(ScenarioStructureBSPCluster, weather) == 0x08);
CHECK(check_leaf_cluster, OFF(ScenarioStructureBSPLeaf, cluster) == 0x08);
CHECK(check_fogregion, OFF(ScenarioStructureBSPFogRegion, weather_palette) == 0x26);
CHECK(check_fogpal, OFF(ScenarioStructureBSPFogPalette, fog) + 0x0c == 0x2c);
CHECK(check_bgpal, OFF(ScenarioStructureBSPBackgroundSoundPalette, background_sound) + 0x0c == 0x2c);
CHECK(check_matg_materials, OFF(Globals, materials) == 0x194 && sizeof(GlobalsMaterial) == 0x374);
CHECK(check_sky_outdoor, OFF(Sky, outdoor_fog_color) == 0x58 && OFF(Sky, outdoor_fog_maximum_density) == 0x6c
                         && OFF(Sky, outdoor_fog_start_distance) == 0x70 && OFF(Sky, outdoor_fog_opaque_distance) == 0x74);
CHECK(check_sky_indoor, OFF(Sky, indoor_fog_color) == 0x78 && OFF(Sky, indoor_fog_screen) + 0x0c == 0xa4);
CHECK(check_fog_water, OFF(Fog, distance_to_water_plane) == 0x74);
CHECK(check_soundenv, sizeof(SoundEnvironment) == 0x48);
CHECK(check_sky_fog_block, sizeof(sky_fog_block) == 0x20 && OFF(sky_fog_block, maximum_density) == 0x14
                          && OFF(sky_fog_block, start_distance) == 0x18 && OFF(sky_fog_block, opaque_distance) == 0x1c
                          && OFF(Sky, indoor_fog_color) - OFF(Sky, outdoor_fog_color) == 0x20);

int main(void) {
  scenario_game_globals globals;
  scenario_trigger_volume_box box;
  structure_bsp_procedure procedure = 0;
  datum_index scenario_index = (datum_index)k_datum_index_none;
  globals.structure_bsp_index = k_structure_bsp_index_none;
  box.fixed.x_bounds[0] = 0.0f;
  return (int)(sizeof(globals) + sizeof(box) + (procedure != 0) + (scenario_index != 0)
               + k_cluster_fog_index_mask + k_fog_flag_is_water + k_scenario_location_nudge_attempts);
}
