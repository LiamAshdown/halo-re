// structure_bsp_resolve_fog_tag  (Ghidra: FUN_00555270, still unnamed there)
// address 0x555270, size 181 bytes
// name confidence: 0.6 -- the phase4 summary ("Retrieves the background sound environment tag
//   associated with a cluster...") is wrong: every offset this function touches
//   (fog_planes/fog_regions/fog_palette, and the Sky tag's indoor_fog_screen) resolves a Fog tag,
//   not a sound environment. types/structures.h's own detailed writeup for this address ("The
//   lookup chain 0x00555270 implements...") already documents it correctly under this
//   understanding; named here to match that.
// rewrite confidence: 0.6 -- clean decompile, all register roles resolved via
//   types/structures.h's own already-written analysis of this exact function.
// evidence: types/structures.h's "structure_fog_environment" section, which walks this exact
//   lookup chain field by field; types/tags.h ScenarioStructureBSPFogPlane/FogRegion/FogPalette,
//   Scenario.skies, ScenarioSky, Sky.indoor_fog_screen.
// register convention: in_CX -> cluster_index, in_EDX -> structure_bsp, unaff_BL -> use_sky.
//   No stack parameters.
//   // blam-cc: CX -> cluster_index, EDX -> structure_bsp, BL -> use_sky
// UNSURE: none left in this function's own body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "structures.h"
#include "fn_structures.h"

extern Scenario *global_scenario;     // 0x00746f8c (types/game.h)
extern tag_instance *tag_instances;   // 0x0087bc14

// blam-cc: CX -> cluster_index, EDX -> structure_bsp, BL -> use_sky
uint32_t structure_bsp_resolve_fog_tag(int16_t cluster_index, ScenarioStructureBSP *structure_bsp,
                                        uint8_t use_sky)
{
    if (cluster_index == -1) {
        return 0xffffffff;
    }

    if (use_sky) {
        Scenario *scenario = global_scenario;
        uint32_t sky_tag_id = 0xffffffff;
        if (scenario->skies.count > 0) {
            sky_tag_id = ((ScenarioSky *)scenario->skies.pointer)[0].sky.tag_id.index |
                         ((uint32_t)((ScenarioSky *)scenario->skies.pointer)[0].sky.tag_id.id
                          << 16);
        }
        if (sky_tag_id != 0xffffffff) {
            Sky *sky = (Sky *)tag_instances[sky_tag_id & 0xffff].data;
            if (sky != 0) {
                return *(uint32_t *)&sky->indoor_fog_screen.tag_id;
            }
        }
        return 0xffffffff;
    }

    ScenarioStructureBSPCluster *cluster =
        &((ScenarioStructureBSPCluster *)structure_bsp->clusters.pointer)[cluster_index];
    uint16_t fog = cluster->fog;
    if (fog == 0xffff) {
        return 0xffffffff;
    }

    uint16_t region_index;
    if ((int16_t)fog < 0) {
        ScenarioStructureBSPFogPlane *plane =
            &((ScenarioStructureBSPFogPlane *)structure_bsp->fog_planes.pointer)[fog & 0x7fff];
        region_index = plane->front_region;
    } else {
        region_index = fog & 0x7fff;
    }
    if (region_index == 0xffff) {
        return 0xffffffff;
    }

    int16_t palette_index =
        ((ScenarioStructureBSPFogRegion *)structure_bsp->fog_regions.pointer)[region_index].fog;
    if (palette_index == -1) {
        return 0xffffffff;
    }
    ScenarioStructureBSPFogPalette *palette =
        &((ScenarioStructureBSPFogPalette *)structure_bsp->fog_palette.pointer)[palette_index];
    return *(uint32_t *)&palette->fog.tag_id;
}

#if 0
Original Ghidra decompilation (0x555270):

undefined4 FUN_00555270(void)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  short in_CX;
  ushort uVar4;
  uint uVar5;
  int in_EDX;
  char unaff_BL;

  uVar3 = 0xffffffff;
  if (in_CX != -1) {
    if (unaff_BL == '\0') {
      uVar4 = *(ushort *)(in_CX * 0x68 + *(int *)(in_EDX + 0x138) + 2);
      if (uVar4 != 0xffff) {
        if ((short)uVar4 < 0) {
          uVar4 = *(ushort *)(((int)(short)uVar4 & 0x7fffU) * 0x20 + *(int *)(in_EDX + 0x17c));
        }
        else {
          uVar4 = uVar4 & 0x7fff;
        }
        if ((uVar4 != 0xffff) &&
           (sVar1 = *(short *)(*(int *)(in_EDX + 0x188) + (short)uVar4 * 0x28 + 0x24), sVar1 != -1))
        {
          uVar3 = *(undefined4 *)(sVar1 * 0x88 + 0x2c + *(int *)(in_EDX + 0x194));
        }
      }
    }
    else {
      uVar5 = 0xffffffff;
      if (0 < *(int *)(global_scenario + 0x30)) {
        uVar5 = *(uint *)(*(int *)(global_scenario + 0x34) + 0xc);
      }
      if ((uVar5 != 0xffffffff) &&
         (iVar2 = *(int *)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), iVar2 != 0)) {
        return *(undefined4 *)(iVar2 + 0xa4);
      }
    }
  }
  return 0xffffffff;
}
#endif
