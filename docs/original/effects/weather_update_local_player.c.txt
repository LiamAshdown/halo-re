// weather_update_local_player  (Ghidra: FUN_00458a90, still unnamed there; named directly by
//   types/effects.h: "weather_update_local_player 0x458a90 owns the render origin, cluster and
//   sky flag")
// address 0x458a90, size 192 bytes
// name confidence: 0.6   rewrite confidence: 0.3 (see UNSURE)
// evidence: types/effects.h weather_instance (unknown_10 "copied from 0x007c3344", unknown_14
//   "copied from 0x007c3348", cluster_index +0x18 "scenario_location_get_water_and_weather output", in_sky +0x1a
//   "scenario_location_get_water_and_weather return"); global 0x00687350 weather_enabled (types/effects.h globals list).
// register convention: __cdecl, no arguments.
// UNSURE: the per-cluster "weather row" table read at `structure_bsp+0x1b8`, stride 0xf0, +0x2c
//   is outside this module's slice (a structures/BSP runtime cluster array, not the 0x68 byte
//   ScenarioStructureBSPCluster tag struct), so it is kept as a raw offset read rather than a
//   named field; scenario_location_get_water_and_weather's own signature (a BSP leaf/cluster probe) is likewise not
//   established here and is declared opaque.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t weather_enabled;               // 0x00687350
extern int16_t current_local_player_index;     // 0x007c3108, UNSURE: foreign module (render globals)
extern uint32_t render_leaf_index;  // 0x007c3344, UNSURE: foreign module
extern int16_t render_cluster_index; // 0x007c3348, UNSURE: foreign module
extern ScenarioStructureBSP *global_structure_bsp;
                                    // row table this function reads, UNSURE (foreign/BSP module)
extern weather_instance weather_instances[1]; // 0x006b0ae4

extern real_point3d render_camera_global; // 0x007c3114
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out); // 0x53ed60, EBX point, stack (leaf, weather_index_out)
extern void weather_instance_deactivate(int16_t instance_index); // 0x457f00, this module
extern void weather_instance_activate(datum_index definition_index, int16_t instance_index,
    real intensity); // 0x457e20, this module
extern void weather_instance_build_render_geometry(int16_t instance_index); // 0x458bf0, this module

// Per-tick weather driver for the local player: re-probes the render sample point's BSP cluster,
// looks up that cluster's weather row, and activates/deactivates the local player's weather
// instance when the row changes, then rebuilds its render geometry while active.
void weather_update_local_player(void)
{
    if (weather_enabled != 0 && current_local_player_index != -1) {
        int16_t instance_index = current_local_player_index;
        weather_instance *instance = &weather_instances[instance_index];
        int16_t cluster_index;
        int32_t new_definition_index = -1;

        instance->render_cluster_index = render_cluster_index;
        instance->render_leaf_index = render_leaf_index;
        // 0x458acf..0x458ae6: EBX = &render_camera_global (0x7c3114), push &instance->unknown_10 (the leaf),
        // push &instance->cluster_index -- the callee writes the index straight into the instance
        instance->in_sky = scenario_location_get_water_and_weather(&render_camera_global,
            (bsp_leaf_reference *)&instance->render_leaf_index, &instance->cluster_index);
        cluster_index = instance->cluster_index;

        if (cluster_index != -1) {
            new_definition_index = *(int32_t *)((uint8_t *)global_structure_bsp->weather_palette.pointer +
                (uint32_t)cluster_index * 0xf0 + 0x2c); // UNSURE, see file header
        }

        if ((int32_t)instance->definition_index != new_definition_index) {
            if (instance->definition_index != (datum_index)0xffffffff) {
                weather_instance_deactivate(instance_index);
            }
            if (new_definition_index != -1) {
                weather_instance_activate((datum_index)new_definition_index, instance_index, 1.0f);
            }
        }

        if (instance->definition_index != (datum_index)0xffffffff) {
            weather_instance_build_render_geometry(instance_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x458a90):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00458a90(void)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  undefined1 uVar4;
  int iVar5;
  int iVar6;

  uVar3 = DAT_007c3344;
  if ((DAT_00687350 != '\0') && ((short)_DAT_007c3108 != -1)) {
    iVar6 = (short)_DAT_007c3108 * 0x9c;
    *(undefined2 *)(&DAT_006b0af8 + iVar6) = DAT_007c3348;
    iVar5 = -1;
    *(undefined4 *)(&DAT_006b0af4 + iVar6) = uVar3;
    uVar4 = FUN_0053ed60(&DAT_006b0af4 + iVar6,&DAT_006b0afc + iVar6);
    sVar1 = *(short *)(&DAT_006b0afc + iVar6);
    (&DAT_006b0afe)[iVar6] = uVar4;
    if (sVar1 != -1) {
      iVar5 = *(int *)(sVar1 * 0xf0 + 0x2c + *(int *)(DAT_00746f9c + 0x1b8));
    }
    iVar2 = *(int *)(&DAT_006b0ae4 + iVar6);
    if (iVar2 != iVar5) {
      if (iVar2 != -1) {
        FUN_00457f00();
      }
      if (iVar5 != -1) {
        FUN_00457e20(0x3f800000);
      }
    }
    if (*(int *)(&DAT_006b0ae4 + iVar6) != -1) {
      FUN_00458bf0();
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
