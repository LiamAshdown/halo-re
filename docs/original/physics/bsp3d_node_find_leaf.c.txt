// bsp3d_node_find_leaf  (Ghidra: FUN_005013a0, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x5013a0, size 92 bytes
// name confidence: 0.45   rewrite confidence: 0.7
// evidence: types/tags.h ModelCollisionGeometryBSP.bsp3d_nodes (+0x04, stride 0x0c) and .planes
//   (+0x10, stride 0x10) match ECX+4 / ECX+0x10 exactly; ModelCollisionGeometryBSP3DNode
//   {plane, back_child, front_child} and ModelCollisionGeometryBSPPlane{Plane3D} match the node
//   and plane records read here field for field. 36 callers, consistent with a hot low-level
//   BSP utility used throughout the module.
// register convention: in_EAX -> node_index, in_ECX -> bsp (ModelCollisionGeometryBSP *),
//   in_EDX -> point (real_point3d *). No stack parameters.
//   // blam-cc: EAX -> node_index, ECX -> bsp, EDX -> point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

// blam-cc: EAX -> node_index, ECX -> bsp, EDX -> point
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
                               real_point3d *point)
{
    ModelCollisionGeometryBSP3DNode *nodes =
        (ModelCollisionGeometryBSP3DNode *)bsp->bsp3d_nodes.pointer;
    ModelCollisionGeometryBSPPlane *planes =
        (ModelCollisionGeometryBSPPlane *)bsp->planes.pointer;

    do {
        ModelCollisionGeometryBSP3DNode *node = &nodes[node_index];
        Plane3D *plane = &planes[node->plane].plane;
        float side = plane->vector.i * point->x + plane->vector.j * point->y +
                     plane->vector.k * point->z - plane->w;
        node_index = (0.0f <= side) ? (int32_t)node->front_child : (int32_t)node->back_child;
    } while (-1 < node_index);

    if ((uint32_t)node_index != 0xffffffff) {
        return (uint32_t)node_index & 0x7fffffff;
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x5013a0):

uint FUN_005013a0(void)

{
  int *piVar1;
  uint in_EAX;
  int iVar2;
  float *pfVar3;
  int in_ECX;
  float *in_EDX;

  do {
    piVar1 = (int *)(*(int *)(in_ECX + 4) + in_EAX * 0xc);
    iVar2 = *piVar1 * 0x10;
    pfVar3 = (float *)(iVar2 + *(int *)(in_ECX + 0x10));
    in_EAX = piVar1[(0.0 <= (*pfVar3 * *in_EDX +
                            pfVar3[1] * in_EDX[1] +
                            *(float *)(iVar2 + 8 + *(int *)(in_ECX + 0x10)) * in_EDX[2]) - pfVar3[3]
                    ) + 1];
  } while (-1 < (int)in_EAX);
  if (in_EAX != 0xffffffff) {
    return in_EAX & 0x7fffffff;
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
