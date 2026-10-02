// structure_leaf_portal_vertex_count_debug  (Ghidra: FUN_005520b0; named here)
// address 0x5520b0, size 85 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: sole caller structure_picked_polygon_refresh (0x5527f0, this batch) at
//   `mov eax,[0x69fa40]` (picked_leaf_map_leaf) / `lea ecx,[esi+0x26c]` (&structure_bsp_leaf_map)
//   / `call 0x5520b0` -- confirmed by disassembly (objdump -d -M intel bin/halo.exe,
//   0x5527f0..0x552838). types/structures.h's structure_bsp_leaf_map already documents ECX+8 as
//   leaves.pointer; the field this function actually walks per leaf is portal_indices (+0xc),
//   not the leaf's own faces (+0x00) -- out/phase4/structures_functions.md's one-line summary
//   ("a per-face count value") is imprecise, corrected here.
// register convention: EAX -> leaf_index, ECX -> leaf_map.
// UNSURE: the inner "count up from 2 to iVar4" loop computes max(iVar4, 2) one increment at a
//   time and never stores the result anywhere -- a pure debug/no-op busy-loop (its cost scales
//   with each portal's vertex count). Preserved exactly rather than dropped, since a debug build
//   may have relied on it for timing or a breakpoint target.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Walks every portal referenced by leaf `leaf_index`, and for each one, busy-loops counting up to
// its vertex count (result discarded). A debug/validation pass with no observable effect on
// program state; every read is bounds-implicit on the tag's own reflexive counts.
void structure_leaf_portal_vertex_count_debug(int32_t leaf_index, structure_bsp_leaf_map *leaf_map)
    // blam-cc: EAX -> leaf_index, ECX -> leaf_map
{
    ScenarioStructureBSPGlobalMapLeaf *leaf =
        (ScenarioStructureBSPGlobalMapLeaf *)leaf_map->leaves.pointer + (leaf_index & 0x7fffffff); // VERIFIED 0x5520b7: and eax,0x7fffffff
    int32_t portal_ref_count = leaf->portal_indices.count;
    int32_t *portal_indices = (int32_t *)leaf->portal_indices.pointer;
    ScenarioStructureBSPGlobalLeafPortal *portals =
        (ScenarioStructureBSPGlobalLeafPortal *)leaf_map->portals.pointer;
    int16_t discarded_count;
    int32_t i;

    for (i = 0; i < portal_ref_count; i = i + 1) {
        int32_t portal_index = portal_indices[i] & 0x7fffffff; // VERIFIED 0x5520d8: and eax,0x7fffffff
        int32_t vertex_count = portals[portal_index].vertices.count;

        discarded_count = 2;
        while (discarded_count < vertex_count) {
            discarded_count = discarded_count + 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x5520b0):

void FUN_005520b0(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint in_EAX;
  int iVar4;
  short sVar5;
  int in_ECX;
  short sVar6;

  iVar1 = *(int *)(in_ECX + 8) + ((in_EAX & 0x7fffffff) + in_EAX * 2) * 8;
  iVar2 = *(int *)(iVar1 + 0xc);
  sVar6 = 0;
  if (0 < iVar2) {
    iVar4 = 0;
    do {
      uVar3 = *(uint *)(*(int *)(iVar1 + 0x10) + iVar4 * 4);
      iVar4 = *(int *)(*(int *)(in_ECX + 0x14) + ((uVar3 & 0x7fffffff) + uVar3 * 2) * 8 + 0xc);
      sVar5 = 2;
      if (2 < iVar4) {
        do {
          sVar5 = sVar5 + 1;
        } while (sVar5 < iVar4);
      }
      sVar6 = sVar6 + 1;
      iVar4 = (int)sVar6;
    } while (iVar4 < iVar2);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
