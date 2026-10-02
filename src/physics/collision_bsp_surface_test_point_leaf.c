// collision_bsp_surface_test_point_leaf  (Ghidra: FUN_00502460, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x502460, size 407 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: types/tags.h ModelCollisionGeometryBSPLeaf/…BSP2DReference match the leaf and
//   reference tables walked here; the dominant-axis selection is the same ABS-comparison idiom
//   used throughout the module (e.g. collision_bsp_query_sphere_node_recursive); the trailing
//   call to collision_bsp_surface_test_point_2d (0x502600) matches its parameter order exactly.
// register convention: unaff_EBX -> bsp (ModelCollisionGeometryBSP *), in_ECX -> leaf_index.
//   param_1..param_5 are Ghidra-recognized stack parameters.
//   // blam-cc: EBX -> bsp, ECX -> leaf_index, stack -> breakable_surface_count,
//   //           breakable_surfaces, plane_index, crossing_point, two_sided
// UNSURE, significantly: this function calls bsp2d_node_find_leaf (0x501340) with no visible
// arguments in Ghidra's decompile, then treats its result as a 64-bit pair, using the low 32
// bits as a leaf/surface index (matches bsp2d_node_find_leaf's real return) and the high 32
// bits as a `float *` point argument to collision_bsp_surface_test_point_2d. Since
// bsp2d_node_find_leaf only ever returns one 32-bit value, the "high half" is almost certainly
// EDX's incoming value read back after the call (i.e. never actually written by the callee),
// which is consistent with EDX being bsp2d_node_find_leaf's own `point` input parameter and
// never being clobbered by it. This rewrite reconstructs that shared point as the segment's
// crossing point at the parent plane, added here as an explicit parameter
// (`crossing_point`) rather than a register this file cannot otherwise recover, and projects it
// through the same dominant axis computed for the plane normal below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t bsp2d_node_find_leaf(int32_t node_index, TagReflexive *bsp2d_nodes,
                                     real_point2d *point); // 0x501340, this batch
extern uint8_t collision_bsp_surface_test_point_2d(
    ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count,
    uint32_t *breakable_surfaces, int32_t surface_index, int16_t axis, uint8_t sign,
    real_point2d *point); // 0x502600, this batch
extern double fabs(double x);
extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)

// blam-cc: EBX -> bsp, ECX -> leaf_index, stack -> breakable_surface_count, breakable_surfaces,
//          plane_index, crossing_point, two_sided
int32_t collision_bsp_surface_test_point_leaf(ModelCollisionGeometryBSP *bsp, int32_t leaf_index,
                                               int16_t breakable_surface_count,
                                               uint32_t *breakable_surfaces, uint32_t plane_index,
                                               real_point3d *crossing_point, uint8_t two_sided)
{
    ModelCollisionGeometryBSPLeaf *leaf =
        &((ModelCollisionGeometryBSPLeaf *)bsp->leaves.pointer)[leaf_index];
    int32_t reference_index = (int32_t)leaf->first_bsp2d_reference;
    int32_t reference_end = reference_index + leaf->bsp2d_reference_count;

    if (reference_index < reference_end) {
        ModelCollisionGeometryBSP2DReference *references =
            (ModelCollisionGeometryBSP2DReference *)bsp->bsp2d_references.pointer;
        ModelCollisionGeometryBSPPlane *planes =
            (ModelCollisionGeometryBSPPlane *)bsp->planes.pointer;
        do {
            ModelCollisionGeometryBSP2DReference *reference = &references[reference_index];
            if ((reference->plane & 0x7fffffffu) == plane_index) {
                Plane3D *plane = &planes[plane_index].plane;
                int16_t dominant_axis;
                float axis_component;

                if (((float)fabs((double)plane->vector.k) < (float)fabs((double)plane->vector.j)) ||
                    ((float)fabs((double)plane->vector.k) < (float)fabs((double)plane->vector.i))) {
                    dominant_axis =
                        ((float)fabs((double)plane->vector.j) < (float)fabs((double)plane->vector.i))
                            ? 0
                            : 1;
                } else {
                    dominant_axis = 2;
                }
                axis_component = ((float *)&plane->vector)[dominant_axis];

                {
                    uint8_t sign = (0.0f < axis_component) != ((reference->plane & 0x80000000u) != 0);
                    projection_axis_pair proj = k_projection_axes[dominant_axis * 2 + sign];
                    real_point2d point2d;
                    point2d.x = ((float *)crossing_point)[proj.i];
                    point2d.y = ((float *)crossing_point)[proj.j];

                    int32_t found_surface =
                        bsp2d_node_find_leaf((int32_t)reference->bsp2d_node,
                                              &bsp->bsp2d_nodes, &point2d);
                    if ((two_sided == 0) ||
                        collision_bsp_surface_test_point_2d(bsp, breakable_surface_count,
                                                             breakable_surfaces, found_surface,
                                                             dominant_axis, sign, &point2d)) {
                        return found_surface;
                    }
                }
            }
            reference_index = reference_index + 1;
        } while (reference_index < reference_end);
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x502460):

undefined4
FUN_00502460(undefined4 param_1,undefined4 param_2,uint param_3,undefined4 param_4,char param_5)

{
  float fVar1;
  uint uVar2;
  short sVar3;
  char cVar4;
  int iVar5;
  int in_ECX;
  int iVar6;
  int unaff_EBX;
  int iVar7;
  undefined8 uVar8;
  uint *local_24;

  iVar5 = *(int *)(unaff_EBX + 0x1c) + in_ECX * 8;
  iVar7 = *(int *)(iVar5 + 4);
  iVar5 = *(short *)(iVar5 + 2) + iVar7;
  if (iVar7 < iVar5) {
    local_24 = (uint *)(*(int *)(unaff_EBX + 0x28) + iVar7 * 8);
    do {
      uVar2 = *local_24;
      if ((uVar2 & 0x7fffffff) == param_3) {
        iVar6 = param_3 * 0x10 + *(int *)(unaff_EBX + 0x10);
        fVar1 = ABS(*(float *)(param_3 * 0x10 + *(int *)(unaff_EBX + 0x10)));
        if ((ABS(*(float *)(iVar6 + 8)) < ABS(*(float *)(iVar6 + 4))) ||
           (ABS(*(float *)(iVar6 + 8)) < fVar1)) {
          sVar3 = 1;
          if (ABS(*(float *)(iVar6 + 4)) < fVar1) {
            sVar3 = 0;
          }
        }
        else {
          sVar3 = 2;
        }
        fVar1 = *(float *)(iVar6 + sVar3 * 4);
        uVar8 = FUN_00501340();
        if ((param_5 == '\0') ||
           (cVar4 = FUN_00502600(param_1,param_2,(int)uVar8,sVar3,
                                 0.0 < fVar1 != ((uVar2 & 0x80000000) != 0),
                                 (int)((ulonglong)uVar8 >> 0x20)), cVar4 != '\0')) {
          return (int)uVar8;
        }
      }
      iVar7 = iVar7 + 1;
      local_24 = local_24 + 2;
    } while (iVar7 < iVar5);
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
