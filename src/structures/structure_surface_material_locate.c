// structure_surface_material_locate  (Ghidra: FUN_00552110; named here)
// address 0x552110, size 243 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: sole caller structure_bsp_leaf_find_material_surface (0x554fa0, this module) passes
//   DAT_00746f9c (the resident ScenarioStructureBSP) and a surface index read out of a
//   ScenarioStructureBSPSurfaceReference.surface field; disassembly (objdump -d -M intel
//   bin/halo.exe, 0x554fa0..0x55502c) shows the fourth argument arrives in EAX
//   (`mov eax,[esp+0x50]` immediately before `call 0x552110`), which Ghidra's decompilation of
//   0x552110 shows only as the never-declared "in_EAX" it writes through at every step of the
//   first loop -- resolved here as out_lightmap_index. Field offsets +0x104/+0x108 match
//   types/tags.h ScenarioStructureBSP.lightmaps (already documented in types/structures.h);
//   +0x14/+0x18 inside a lightmap match ScenarioStructureBSPLightmap.materials; +0x14/+0x18
//   inside a material match ScenarioStructureBSPMaterial.surfaces/surface_count. The out/phase4
//   one-line summary ("locates the grid cell... two-level X/Y spatial index") is a misread -- there
//   is no coordinate here, only a surface index and two monotonic-range binary searches (lightmap,
//   then material within it), corrected in this rewrite.
// register convention: EAX -> out_lightmap_index; stack -> structure_bsp, surface_index,
//   out_material_index.
// UNSURE: the outer search bounds its high end at lightmaps.count - 1 while the inner search
//   bounds its high end at materials.count with no "- 1"; preserved exactly as this asymmetry,
//   not harmonized.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

// Given a global surface index, finds which lightmap and which material within it owns that
// surface (materials own a contiguous, monotonically increasing run of surface indices via
// their `surfaces` / `surface_count` fields) via two nested binary searches: lightmaps first,
// then materials within the winning lightmap.
void structure_surface_material_locate(ScenarioStructureBSP *structure_bsp, int32_t surface_index,
    int16_t *out_material_index, int16_t *out_lightmap_index)
    // blam-cc: EAX -> out_lightmap_index
{
    int16_t low = 0;
    int16_t high = (int16_t)(structure_bsp->lightmaps.count - 1);

    *out_lightmap_index = 0;
    if (high > 0) {
        do {
            int16_t mid = (int16_t)(((int32_t)high - (int32_t)low) / 2 + low);
            ScenarioStructureBSPLightmap *lightmap =
                (ScenarioStructureBSPLightmap *)structure_bsp->lightmaps.pointer + mid;
            ScenarioStructureBSPMaterial *materials =
                (ScenarioStructureBSPMaterial *)lightmap->materials.pointer;

            *out_lightmap_index = mid;
            if (surface_index < materials[0].surfaces) {
                high = mid - 1;
                *out_lightmap_index = high;
            } else {
                int32_t last = lightmap->materials.count;
                if (surface_index < materials[last - 1].surfaces + materials[last - 1].surface_count) {
                    break;
                }
                low = mid + 1;
                *out_lightmap_index = low;
            }
        } while (low < high);
    }

    {
        ScenarioStructureBSPLightmap *lightmap =
            (ScenarioStructureBSPLightmap *)structure_bsp->lightmaps.pointer + *out_lightmap_index;
        ScenarioStructureBSPMaterial *materials =
            (ScenarioStructureBSPMaterial *)lightmap->materials.pointer;
        int16_t low2 = 0;
        int16_t high2 = (int16_t)lightmap->materials.count;

        *out_material_index = 0;
        if (high2 > 0) {
            do {
                int16_t mid2 = (int16_t)(((int32_t)high2 - (int32_t)low2) / 2 + low2);
                ScenarioStructureBSPMaterial *material = &materials[mid2];

                *out_material_index = mid2;
                if (surface_index < material->surfaces) {
                    high2 = mid2 - 1;
                    *out_material_index = high2;
                } else {
                    if (surface_index < material->surfaces + material->surface_count) {
                        return;
                    }
                    low2 = mid2 + 1;
                    *out_material_index = low2;
                }
            } while (low2 < high2);
        }
    }
}

#if 0
Original Ghidra decompilation (0x552110):

void FUN_00552110(int param_1,int param_2,short *param_3)

{
  int iVar1;
  short *in_EAX;
  int iVar2;
  int iVar3;
  ushort uVar4;
  int iVar6;
  int iVar7;
  uint uVar5;

  iVar7 = 0;
  *in_EAX = 0;
  uVar4 = *(short *)(param_1 + 0x104) - 1;
  uVar5 = (uint)uVar4;
  if (0 < (short)uVar4) {
    do {
      iVar6 = ((int)(short)uVar5 - (int)(short)iVar7) / 2 + iVar7;
      *in_EAX = (short)iVar6;
      iVar2 = (short)iVar6 * 0x20 + *(int *)(param_1 + 0x108);
      if (param_2 < *(int *)(*(int *)(iVar2 + 0x18) + 0x14)) {
        uVar5 = iVar6 - 1;
        *in_EAX = (short)uVar5;
      }
      else {
        iVar7 = *(int *)(iVar2 + 0x14) * 0x100;
        if (param_2 < *(int *)(*(int *)(iVar2 + 0x18) + iVar7 + -0xec) +
                      *(int *)(*(int *)(iVar2 + 0x18) + -0xe8 + iVar7)) break;
        iVar7 = iVar6 + 1;
        *in_EAX = (short)iVar7;
      }
    } while ((short)iVar7 < (short)uVar5);
  }
  iVar7 = 0;
  iVar6 = *in_EAX * 0x20 + *(int *)(param_1 + 0x108);
  *param_3 = 0;
  uVar4 = *(ushort *)(iVar6 + 0x14);
  uVar5 = (uint)uVar4;
  if (0 < (short)uVar4) {
    do {
      iVar2 = ((int)(short)uVar5 - (int)(short)iVar7) / 2 + iVar7;
      *param_3 = (short)iVar2;
      iVar3 = (short)iVar2 * 0x100 + *(int *)(iVar6 + 0x18);
      iVar1 = *(int *)(iVar3 + 0x14);
      if (param_2 < iVar1) {
        uVar5 = iVar2 - 1;
        *param_3 = (short)uVar5;
      }
      else {
        if (param_2 < *(int *)(iVar3 + 0x18) + iVar1) {
          return;
        }
        iVar7 = iVar2 + 1;
        *param_3 = (short)iVar7;
      }
    } while ((short)iVar7 < (short)uVar5);
  }
  return;
}
#endif
