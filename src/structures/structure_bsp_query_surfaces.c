// structure_bsp_query_surfaces  (Ghidra: FUN_00553d80, still unnamed there)
// address 0x553d80, size 387 bytes
// name confidence: 0.6 -- matches the phase4 summary ("Entry point that resolves the BSP surfaces
//   overlapping a sphere or box query, choosing between a fast cluster-radius path and a general
//   recursive BSP-tree query depending on the query size").
// rewrite confidence: 0.5 -- Ghidra's parameter recovery (7 stack params typed mostly
//   `undefined4`) is positionally correct but gives no roles; objdump disassembly of the whole
//   function (a clean EBP-frame function, unlike most of its neighbours) pins every parameter and
//   every downstream call's argument mapping precisely.
// evidence: objdump -M intel disassembly of 0x553d80..0x553f10; the three callees this function
//   dispatches to are all in this batch (structure_bsp_collect_surfaces_in_clusters,
//   structure_bsp_cluster_flood_seed, bsp3d_node_query_recursive) and their own files independently
//   confirm the argument order used here.
// register convention: cdecl, 7 stack parameters; 2 register-passed: in_ECX -> query_box (NULL to
//   build one from point+radius), in_EDX -> query_point.
//   // blam-cc: ECX -> query_box (nullable), EDX -> query_point, stack -> the rest
// UNSURE: none left; every branch (pre-supplied cluster list, resolved-leaf flood-fill, and the
//   bsp3d recursive fallback used both for a small radius and whenever the faster paths fail to
//   resolve a leaf/cluster) is confirmed against the disassembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *structure_bsp;         // 0x00746f9c
extern float k_cluster_query_radius_threshold;      // 0x0069fa4c
// 0x00746f90 is a ModelCollisionGeometryBSP pointer, proved twice in this module: it is the ECX
// argument of bsp3d_node_find_leaf at 0x553e4a / 0x549a6a, and 0x554b9f reads its +0x10 as
// planes.pointer and indexes it with plane_index * 0x10. Seven files elsewhere in the repo
// (src/effects, src/objects, src/hs) declare the same address as `void *global_globals`; the name is
// kept for cross-module agreement and only the type is refined. It is a DIFFERENT global from
// `global_globals` at 0x00746f98, whose +0x40 surfaces.pointer src/effects and src/items
// already pin -- do not merge the two.
extern ModelCollisionGeometryBSP *global_globals; // 0x00746f90

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
                                      real_point3d *point); // types/physics.h, blam-cc: EAX/ECX/EDX

extern int32_t structure_bsp_collect_surfaces_in_clusters(
    int32_t *out_surfaces, int16_t max_count, real_rectangle3d *query_box, int16_t plane_count,
    real_plane3d *planes, uint32_t *visited_bits, int16_t cluster_count,
    int16_t *cluster_indices); // 0x553c40, this module

extern int32_t structure_bsp_cluster_flood_seed(real_point3d *point, float radius,
                                                 int16_t start_cluster, int16_t *output,
                                                 int16_t max_count);

extern int16_t bsp3d_node_query_recursive(int32_t node_index, real_rectangle3d *parent_bounds,
                                           uint32_t *visited_bits, int32_t *output_array,
                                           int32_t max_count, real_point3d *point,
                                           float radius, real_rectangle3d *query_box,
                                           int16_t plane_count, real_plane3d *planes,
                                           int16_t inherited_classification); // 0x553f10

// blam-cc: ECX -> query_box (nullable), EDX -> query_point, stack -> the rest
// The return is the number of surface indices written: every caller in this module reads it
// (`movsx esi,ax`), so it is not void.
int16_t structure_bsp_query_surfaces(real_rectangle3d *query_box, real_point3d *query_point,
                                   int32_t *out_surfaces, int32_t max_count, float radius,
                                   int16_t plane_count, real_plane3d *planes, int16_t cluster_count,
                                   int16_t *cluster_indices)
{
    uint32_t visited_bits[k_maximum_visible_surface_bits];
    int32_t visited_dwords = (structure_bsp->surfaces.count + 0x1f) >> 5;
    for (int32_t i = 0; i < visited_dwords; i++) {
        visited_bits[i] = 0;
    }

    real_rectangle3d built_box;
    if (query_box == 0) {
        built_box.x.lower = query_point->x - radius; built_box.x.upper = query_point->x + radius;
        built_box.y.lower = query_point->y - radius; built_box.y.upper = query_point->y + radius;
        built_box.z.lower = query_point->z - radius; built_box.z.upper = query_point->z + radius;
        query_box = &built_box;
    }

    if (radius >= k_cluster_query_radius_threshold) {
        if (cluster_indices != 0) {
            return (int16_t)structure_bsp_collect_surfaces_in_clusters(out_surfaces,
                        (int16_t)max_count, query_box, plane_count, planes, visited_bits,
                        cluster_count, cluster_indices);
        }

        int32_t leaf = bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_globals,
                                             query_point);
        if (leaf != -1) {
            int32_t leaf_index = leaf & 0x7fffffff;
            uint16_t leaf_cluster =
                ((ScenarioStructureBSPLeaf *)structure_bsp->leaves.pointer)[leaf_index].cluster;
            if (leaf_cluster != 0xffff) {
                int16_t flood_clusters[0x200];
                int32_t flood_count = structure_bsp_cluster_flood_seed(
                    query_point, radius, (int16_t)leaf_cluster, flood_clusters, 0x200);
                return (int16_t)structure_bsp_collect_surfaces_in_clusters(
                    out_surfaces, (int16_t)max_count, query_box, plane_count, planes, visited_bits,
                    (int16_t)flood_count, flood_clusters);
            }
        }
        // fall through: neither a resolved leaf nor a valid cluster -- use the general descent.
    }

    return bsp3d_node_query_recursive(0, (real_rectangle3d *)&structure_bsp->world_bounds_x,
                                visited_bits, out_surfaces, max_count, query_point, radius,
                                query_box, plane_count, planes, _structure_bsp_overlap_partial);
}

#if 0
Original Ghidra decompilation (0x553d80):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00553d80(undefined4 param_1,undefined4 param_2,float param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6,int param_7)

{
  undefined4 uVar1;
  float *in_ECX;
  uint uVar2;
  int iVar3;
  float *in_EDX;
  undefined4 *puVar4;
  undefined8 uVar5;
  float local_4428;
  float local_4424;
  float local_4420;
  float local_441c;
  float local_4418;
  float local_4414;
  short local_440c;
  undefined1 local_4408 [1024];
  undefined4 local_4008 [4095];
  undefined4 uStack_c;

  uStack_c = 0x553d90;
  puVar4 = local_4008;
  for (uVar2 = *(int *)(DAT_00746f9c + 0xf8) + 0x1f >> 5 & 0x3fffffff; uVar2 != 0; uVar2 = uVar2 - 1
      ) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
    *(undefined1 *)puVar4 = 0;
    puVar4 = (undefined4 *)((int)puVar4 + 1);
  }
  if (in_ECX == (float *)0x0) {
    in_ECX = &local_4428;
    local_4428 = *in_EDX - param_3;
    local_4424 = param_3 + *in_EDX;
    local_4420 = in_EDX[1] - param_3;
    local_441c = param_3 + in_EDX[1];
    local_4418 = in_EDX[2] - param_3;
    local_4414 = param_3 + in_EDX[2];
  }
  if (_DAT_0069fa4c <= param_3) {
    if (param_7 != 0) {
      FUN_00553c40(param_1,param_2,in_ECX,param_4,param_5,local_4008,param_6,param_7);
      return;
    }
    uVar5 = FUN_005013a0();
    in_EDX = (float *)((ulonglong)uVar5 >> 0x20);
    if (((int)uVar5 != -1) &&
       (local_440c = *(short *)((int)uVar5 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4)),
       local_440c != -1)) {
      uVar1 = FUN_00554cb0(in_EDX,param_3);
      FUN_00553c40(param_1,param_2,in_ECX,param_4,param_5,local_4008,uVar1,local_4408);
      return;
    }
  }
  bsp3d_node_query_recursive
            (0,DAT_00746f9c + 200,local_4008,param_1,param_2,in_EDX,param_3,in_ECX,param_4,param_5,1
            );
  return;
}
#endif
