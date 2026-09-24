// collision_bsp_query_pill_leaf_recursive  (Ghidra: FUN_00502d60, still unnamed there; name
// from out/phase2/results/physics_00.json)
// address 0x502d60, size 261 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: types/tags.h ModelCollisionGeometryBSP.bsp2d_nodes (+0x34) and
//   ModelCollisionGeometryBSP2DNode {plane; left_child; right_child} match every offset read
//   here; collision_bsp_pill_query.projected_origin_i/j / projected_delta_i/j (0x21c..0x228)
//   and .radius (0x00c) confirmed in out/phase4/physics_types_notes.md.
// register convention: none -- both arguments are Ghidra-recognized stack parameters.
//   // blam-cc: stack -> query, bsp2d_node_index
// UNSURE: bsp2d_node_index is typed as uint in Ghidra's decompile (unlike the equivalent sphere-
// query helper, which shows it as float); carried here as int32_t directly, same convention as
// collision_bsp_query_sphere_leaf_edge_recursive.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern uint8_t collision_bsp_query_pill_leaf_test_surface(collision_bsp_pill_query *query,
                                                            int32_t surface_index); // 0x502e70, this batch

// blam-cc: stack -> query, bsp2d_node_index
uint8_t collision_bsp_query_pill_leaf_recursive(collision_bsp_pill_query *query,
                                                 int32_t bsp2d_node_index)
{
    if (bsp2d_node_index < 0) {
        return collision_bsp_query_pill_leaf_test_surface(
            query, (int32_t)((uint32_t)bsp2d_node_index & 0x7fffffffu));
    }
    {
        ModelCollisionGeometryBSP *bsp = (ModelCollisionGeometryBSP *)query->bsp;
        ModelCollisionGeometryBSP2DNode *node =
            &((ModelCollisionGeometryBSP2DNode *)bsp->bsp2d_nodes.pointer)[bsp2d_node_index];
        float side_origin = node->plane.vector.i * query->projected_origin_i +
                             query->projected_origin_j * node->plane.vector.j - node->plane.w;
        float side_end = query->projected_delta_i * node->plane.vector.i +
                          query->projected_delta_j * node->plane.vector.j + side_origin;
        float margin = query->radius + 0.00012207031f;
        // equivalent to !((side_origin > margin) && (side_end > margin))
        int test_left = !((side_origin > margin) && (side_end > margin));
        int test_right = (-query->radius - 0.00012207031f <= side_origin) ||
                          (-query->radius - 0.00012207031f <= side_end);
        int left_hit = 0, right_hit = 0;

        if (test_left) {
            left_hit = collision_bsp_query_pill_leaf_recursive(query, (int32_t)node->left_child);
        }
        if (!test_left || !left_hit) {
            if (test_right) {
                right_hit =
                    collision_bsp_query_pill_leaf_recursive(query, (int32_t)node->right_child);
            }
            if (!test_right || !right_hit) {
                return 0;
            }
        }
        return 1;
    }
}

#if 0
Original Ghidra decompilation (0x502d60):

undefined4 FUN_00502d60(int *param_1,uint param_2)

{
  float *pfVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  bool bVar6;
  bool bVar7;
  char cVar8;

  if ((int)param_2 < 0) {
    cVar8 = FUN_00502e70(param_1,param_2 & 0x7fffffff);
    if (cVar8 == '\0') {
      return 0;
    }
    return 1;
  }
  iVar2 = *(int *)(*param_1 + 0x34);
  pfVar1 = (float *)(iVar2 + param_2 * 0x14);
  fVar3 = (*(float *)(iVar2 + param_2 * 0x14) * (float)param_1[0x87] +
          (float)param_1[0x88] * *(float *)(iVar2 + 4 + param_2 * 0x14)) - pfVar1[2];
  fVar4 = (float)param_1[0x89] * *pfVar1 + (float)param_1[0x8a] * pfVar1[1] + fVar3;
  fVar5 = (float)param_1[3] + 0.00012207031;
  if ((fVar3 < fVar5 == (fVar3 == fVar5)) && (fVar4 < fVar5 == (fVar4 == fVar5))) {
    bVar6 = false;
  }
  else {
    bVar6 = true;
  }
  if ((-(float)param_1[3] - 0.00012207031 <= fVar3) || (-(float)param_1[3] - 0.00012207031 <= fVar4)
     ) {
    bVar7 = true;
  }
  else {
    bVar7 = false;
  }
  if (((!bVar6) || (cVar8 = FUN_00502d60(param_1,pfVar1[3]), cVar8 == '\0')) &&
     ((!bVar7 || (cVar8 = FUN_00502d60(param_1,pfVar1[4]), cVar8 == '\0')))) {
    return 0;
  }
  return 1;
}
#endif
