// render_camera_update_leaf_and_cluster  (Ghidra: FUN_00553490; named here)
// address 0x553490, size 197 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: types/structures.h's own globals list documents this exact function as the writer of
//   render_leaf_index/render_cluster_index/render_cluster_has_sky/render_cluster_sky_index
//   (0x007c3344/48/4d/4e) and names its leaf probe callee "bsp3d_node_find_leaf" (kept declared
//   as bsp3d_node_find_leaf here for consistency with every other module's extern declaration of it,
//   e.g. src/objects/object_light_recompute_transform.c). Field offsets: ScenarioStructureBSP
//   leaves (+0xe0/+0xe4), clusters (+0x134/+0x138); types/tags.h ScenarioStructureBSPLeaf.cluster
//   (+0x08), ScenarioStructureBSPCluster.sky (+0x00), ScenarioSky (a plain `sky` TagDependency,
//   stride 0x10), Sky.model (+0x00, tag_id at +0x0c).
// register convention: none (void).
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)
// FIXED (register inputs, objdump): EDX carries the camera position pointer (read at 0x55349f,
//   the `call 0x5013a0` to bsp3d_node_find_leaf -- ECX and EAX are set locally in this function,
//   0x553497/0x55349d, but EDX never is). render_camera_position (0x7c3114) is not referenced
//   anywhere in this function's disassembly; the sole caller (0x50ba9d) passes the address of a
//   local point it just computed (edi = ebp+4), so the old `&render_camera_position` argument
//   was fabricated. blam-cc: EDX -> camera_position
//   // blam-cc: EDX -> camera_position

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "structures.h"
#include "fn_structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90, physics.h/objects.h (read, not owned)
extern Scenario *global_scenario; // 0x00746f8c, game.h/hs.h (read, not owned)
extern tag_instance *tag_instances; // 0x0087bc14, cache.h

extern int32_t render_leaf_index;      // 0x007c3344, this module
extern int32_t render_cluster_index;   // 0x007c3348, this module
extern uint8_t render_cluster_has_sky; // 0x007c334d, this module
extern int16_t render_cluster_sky_index; // 0x007c334e, this module

extern int32_t bsp3d_node_find_leaf(int32_t node_index, void *bsp, real_point3d *point); // 0x5013a0,
    // blam-cc: EAX -> node_index, ECX -> bsp, EDX -> point

// Resolves the render camera's current BSP leaf and cluster (falling back to the last-known leaf
// when the point probe fails but that leaf is still in range), then caches whether that cluster
// has a sky whose model dependency is actually set.
// blam-cc: EDX -> camera_position
void render_camera_update_leaf_and_cluster(real_point3d *camera_position)
{
    // 0x553491..0x55349f: the structure BSP's own collision BSP (its pointer at +0xb4), from node 0
    int32_t leaf = bsp3d_node_find_leaf(0, *(void **)((uint8_t *)global_structure_bsp + 0xb4), camera_position);

    if (leaf == -1 && render_leaf_index < global_structure_bsp->leaves.count) {
        leaf = render_leaf_index;
    }
    render_leaf_index = leaf;
    render_cluster_index = -1;
    render_cluster_sky_index = -1;
    render_cluster_has_sky = 0;

    if (render_leaf_index != -1) {
        ScenarioStructureBSPLeaf *leaves = (ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer;
        ScenarioStructureBSPCluster *clusters = (ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer;
        TagID sky_tag_id;
        int have_sky_tag_id = 0;

        render_cluster_index = leaves[render_leaf_index & 0x7fffffff].cluster; // 0x5534e3: and eax,0x7fffffff
        render_cluster_sky_index = clusters[render_cluster_index].sky;

        if (render_cluster_sky_index > -1 && render_cluster_sky_index < global_scenario->skies.count) {
            ScenarioSky *skies = (ScenarioSky *)global_scenario->skies.pointer;
            if (skies[render_cluster_sky_index].sky.tag_id.index != 0xffff) {
                sky_tag_id = skies[render_cluster_sky_index].sky.tag_id;
                have_sky_tag_id = 1;
            }
        }

        if (have_sky_tag_id) {
            Sky *sky = (Sky *)tag_instances[sky_tag_id.index].data;
            if (sky != 0 && sky->model.tag_id.index != 0xffff) {
                render_cluster_has_sky = 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x553490):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00553490(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;

  iVar1 = DAT_00746f9c;
  iVar2 = FUN_005013a0();
  if ((iVar2 == -1) && (iVar2 = DAT_007c3344, *(int *)(iVar1 + 0xe0) <= DAT_007c3344)) {
    iVar2 = -1;
  }
  DAT_007c3344 = iVar2;
  _DAT_007c3348 = -1;
  DAT_007c334e = -1;
  DAT_007c334d = 0;
  if (DAT_007c3344 != -1) {
    _DAT_007c3348 = (int)*(short *)(DAT_007c3344 * 0x10 + 8 + *(int *)(iVar1 + 0xe4));
    DAT_007c334e = *(short *)(_DAT_007c3348 * 0x68 + *(int *)(iVar1 + 0x138));
    uVar3 = 0xffffffff;
    if ((-1 < DAT_007c334e) && ((int)DAT_007c334e < *(int *)(global_scenario + 0x30))) {
      uVar3 = *(uint *)(DAT_007c334e * 0x10 + 0xc + *(int *)(global_scenario + 0x34));
    }
    if (((uVar3 != 0xffffffff) &&
        (iVar1 = *(int *)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), iVar1 != 0)) &&
       (*(int *)(iVar1 + 0xc) != -1)) {
      DAT_007c334d = 1;
    }
  }
  return;
}
#endif
