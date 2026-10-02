// bsp2d_node_find_leaf  (Ghidra: FUN_00501340, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x501340, size 78 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: types/tags.h ModelCollisionGeometryBSP2DNode {Plane2D plane; left_child;
//   right_child} (stride 0x14) matches the node record exactly; types/physics.h documents this
//   function as being handed the address of a BSP's bsp2d_nodes TagReflexive directly (ECX+4 is
//   the reflexive's .pointer field). Ghidra types the loop/return value as float because the
//   node's child-index words are read through a float* alias, but every value that flows
//   through it is really an int32_t node/leaf index; the final "ABS" is bit-identical to
//   clearing the sign bit of that int32_t, which is what the rewrite does directly.
// register convention: in_EAX -> node_index, in_ECX -> bsp2d_nodes (TagReflexive *), in_EDX ->
//   point (real_point2d *). No stack parameters.
//   // blam-cc: EAX -> node_index, ECX -> bsp2d_nodes, EDX -> point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX -> node_index, ECX -> bsp2d_nodes, EDX -> point
int32_t bsp2d_node_find_leaf(int32_t node_index, TagReflexive *bsp2d_nodes, real_point2d *point)
{
    ModelCollisionGeometryBSP2DNode *nodes =
        (ModelCollisionGeometryBSP2DNode *)bsp2d_nodes->pointer;

    while (-1 < node_index) {
        ModelCollisionGeometryBSP2DNode *node = &nodes[node_index];
        float side = node->plane.vector.i * point->x + node->plane.vector.j * point->y -
                     node->plane.w;
        node_index = (int32_t)((0.0f <= side) ? node->right_child : node->left_child);
    }
    if (node_index != -1) {
        return node_index & 0x7fffffff; // clear the sign bit; bit-identical to Ghidra's
                                          // float ABS() on the same bit pattern
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x501340):

float FUN_00501340(void)

{
  float *pfVar1;
  float in_EAX;
  int in_ECX;
  float *in_EDX;

  if (-1 < (int)in_EAX) {
    do {
      pfVar1 = (float *)(*(int *)(in_ECX + 4) + (int)in_EAX * 0x14);
      in_EAX = pfVar1[(0.0 <= (*pfVar1 * *in_EDX +
                              *(float *)(*(int *)(in_ECX + 4) + 4 + (int)in_EAX * 0x14) * in_EDX[1])
                              - pfVar1[2]) + 3];
    } while (-1 < (int)in_EAX);
  }
  if (in_EAX != -NAN) {
    return ABS(in_EAX);
  }
  return -NAN;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
