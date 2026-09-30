// collision_bsp_query_sphere_leaf_edge_recursive  (Ghidra: FUN_00501c90, still unnamed there;
// name from out/phase2/results/physics_00.json)
// address 0x501c90, size 136 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: types/tags.h ModelCollisionGeometryBSP.bsp2d_nodes (+0x34, stride 0x14) and
//   ModelCollisionGeometryBSP2DNode {Plane2D plane; left_child; right_child} match every offset
//   read here; query->projected_center_i/j (0x220/0x224) and query->radius (0x10) confirmed in
//   out/phase4/physics_types_notes.md section 2.
// register convention: in_EAX -> query (collision_bsp_sphere_query *). param_2 is the
//   Ghidra-recognized stack parameter (bsp2d_node_index).
//   // blam-cc: EAX -> query, stack -> bsp2d_node_index
// UNSURE: Ghidra types bsp2d_node_index as float because it is read back out of a node's
//   left_child/right_child fields through a float-typed alias; every value it holds is really
//   an int32_t (a node index, or a node index with the sign bit set meaning "not a node, a
//   terminal geometry reference -- clear the sign bit to get its argument"), so this rewrite
//   carries it as int32_t and clears the sign bit with a mask rather than negating it (the same
//   convention documented for bsp2d_node_find_leaf and bsp3d_node_find_leaf).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "fn_physics.h"


// blam-cc: EAX -> query, stack -> bsp2d_node_index
void collision_bsp_query_sphere_leaf_edge_recursive(collision_bsp_sphere_query *query,
                                                      int32_t bsp2d_node_index)
{
    ModelCollisionGeometryBSP *bsp = (ModelCollisionGeometryBSP *)query->bsp;
    ModelCollisionGeometryBSP2DNode *nodes =
        (ModelCollisionGeometryBSP2DNode *)bsp->bsp2d_nodes.pointer;

    for (;;) {
        if (bsp2d_node_index < 0) {
            collision_bsp_query_sphere_collect_geometry(
                query, (int32_t)((uint32_t)bsp2d_node_index & 0x7fffffffu));
            return;
        }
        {
            ModelCollisionGeometryBSP2DNode *node = &nodes[bsp2d_node_index];
            float side = query->projected_center_i * node->plane.vector.i +
                         query->projected_center_j * node->plane.vector.j - node->plane.w;

            // equivalent to (side <= query->radius); see the identical XOR idiom in
            // breakable_surface_apply_damage.c
            if (side <= query->radius) {
                collision_bsp_query_sphere_leaf_edge_recursive(query, (int32_t)node->left_child);
            }
            if (side < -query->radius) {
                break;
            }
            bsp2d_node_index = (int32_t)node->right_child;
        }
    }
}

#if 0
Original Ghidra decompilation (0x501c90):

void FUN_00501c90(int *param_1,float param_2)

{
  float *pfVar1;
  float fVar2;
  float fVar3;

  while( true ) {
    if ((int)param_2 < 0) {
      FUN_00501d20(ABS(param_2));
      return;
    }
    pfVar1 = (float *)(*(int *)(*param_1 + 0x34) + (int)param_2 * 0x14);
    fVar3 = ((float)param_1[0x88] * *pfVar1 +
            *(float *)(*(int *)(*param_1 + 0x34) + 4 + (int)param_2 * 0x14) * (float)param_1[0x89])
            - pfVar1[2];
    fVar2 = (float)param_1[4];
    if (fVar3 < (float)param_1[4] != (fVar3 == (float)param_1[4])) {
      FUN_00501c90(param_1,pfVar1[3]);
    }
    if (fVar3 < -fVar2) break;
    param_2 = pfVar1[4];
  }
  return;
}
#endif
