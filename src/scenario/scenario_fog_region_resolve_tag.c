// scenario_fog_region_resolve_tag  (Ghidra: FUN_0053ed10, still unnamed there; named for this
// rewrite -- it walks the resident structure BSP's fog_regions -> fog_palette chain to resolve a
// region index down to the Fog tag it names.)
// address 0x53ed10, size 66 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase4/scenario_types_notes.md ("ScenarioStructureBSP: ... fog_regions +0x188,
//   stride 0x28, with fog +0x24 ... fog_palette +0x194, stride 0x88, fog.tag_id +0x2c") and its
//   register table ("0x53ed10: AX = fog region. Returns a fog tag handle or -1."); the sibling
//   src/structures/structure_bsp_resolve_fog_tag.c walks the same two tables (with an explicit
//   structure_bsp parameter instead of the module global) and confirms both strides and both
//   field offsets independently. Confirmed against objdump -d -M intel
//   --start-address=0x53ed10 --stop-address=0x53ed60 bin/halo.exe.
// register convention: AX -> fog_region (int16_t); no stack parameters.
//   // blam-cc: AX -> fog_region

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "scenario.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

// blam-cc: AX -> fog_region
// Resolves a fog region index to the tag id of the Fog it names, by way of the region's
// fog_palette entry, or -1 if the region is invalid, has no palette entry, or the palette entry
// has no fog assigned.
uint32_t scenario_fog_region_resolve_tag(int16_t fog_region)
{
    ScenarioStructureBSPFogRegion *region;
    int16_t palette_index;
    ScenarioStructureBSPFogPalette *palette;

    if (fog_region == -1) {
        return 0xffffffff;
    }
    region = &((ScenarioStructureBSPFogRegion *)global_structure_bsp->fog_regions.pointer)
                 [fog_region];
    palette_index = (int16_t)region->fog;
    if (palette_index == -1) {
        return 0xffffffff;
    }
    palette = &((ScenarioStructureBSPFogPalette *)global_structure_bsp->fog_palette.pointer)
                  [palette_index];
    if (*(uint32_t *)&palette->fog.tag_id == 0xffffffff) {
        return 0xffffffff;
    }
    return *(uint32_t *)&palette->fog.tag_id;
}

#if 0
Original Ghidra decompilation (0x53ed10):

int FUN_0053ed10(void)

{
  short sVar1;
  short in_AX;
  int iVar2;

  if (((in_AX == -1) ||
      (sVar1 = *(short *)(*(int *)(DAT_00746f9c + 0x188) + in_AX * 0x28 + 0x24), sVar1 == -1)) ||
     (iVar2 = *(int *)(sVar1 * 0x88 + *(int *)(DAT_00746f9c + 0x194) + 0x2c), iVar2 == -1)) {
    iVar2 = -1;
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
