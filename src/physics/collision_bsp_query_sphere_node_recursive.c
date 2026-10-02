// collision_bsp_query_sphere_node_recursive  (Ghidra: FUN_00501a10, still unnamed there; name
// from out/phase2/results/physics_00.json)
// address 0x501a10, size 632 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (step 1: checked against objdump -d 0x501a10..0x501c87; NaN branches now exact)
// evidence: out/phase4/physics_types_notes.md section 2 derives collision_bsp_sphere_query and
//   collision_bsp_sphere_result from this function directly (the plane stack at query+0x1c,
//   the leaf list at result+0xc0c, projection_axis/projection_sign/projected_center_i/j at
//   query+0x21c..0x224). types/tags.h ModelCollisionGeometryBSP3DNode, …BSPPlane, …BSPLeaf and
//   …BSP2DReference match every field this function reads.
// register convention: none -- both arguments are Ghidra-recognized stack parameters. Every
//   other value this function needs comes from the query struct itself.
// UNSURE: the field-order push/pop around each recursive call (park a plane on query->planes,
//   recurse, pop, push the flipped encoding, recurse, pop) is preserved exactly as decompiled
//   rather than simplified, since it is easy to get an off-by-one wrong here. A BSP3D node
//   child value with its sign bit set encodes "this is a leaf, not another node"; multiplying
//   or shifting that raw (still sign-bit-set) value by a power of two (0x8, 0x10) discards the
//   sign bit from the low 32 bits of the result identically to masking it off first, which is
//   why Ghidra's decompile never shows an explicit "& 0x7fffffff" before those multiplies, and
//   why this rewrite performs them as plain uint32_t arithmetic rather than reproducing that
//   coincidence with an explicit mask.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern const projection_axis_pair k_projection_axes[6]; // 0x0065c29c, types/math.h (const to
                                                        // match src/math/*.c own declaration)
extern double fabs(double x);

extern void collision_bsp_query_sphere_leaf_edge_recursive(collision_bsp_sphere_query *query,
                                                             uint32_t bsp2d_node_index); // 0x501c90, this batch

void collision_bsp_query_sphere_node_recursive(collision_bsp_sphere_query *query,
                                                uint32_t node_index)
{
    ModelCollisionGeometryBSP *bsp = (ModelCollisionGeometryBSP *)query->bsp;

    if (-1 < (int32_t)node_index) {
        ModelCollisionGeometryBSPPlane *planes =
            (ModelCollisionGeometryBSPPlane *)bsp->planes.pointer;
        ModelCollisionGeometryBSP3DNode *nodes =
            (ModelCollisionGeometryBSP3DNode *)bsp->bsp3d_nodes.pointer;
        real_point3d *center = (real_point3d *)query->center;

        do {
            ModelCollisionGeometryBSP3DNode *node = &nodes[node_index];
            Plane3D *plane = &planes[node->plane].plane;
            float side = plane->vector.i * center->x + plane->vector.j * center->y +
                         plane->vector.k * center->z - plane->w;
            int front_side;

            // 0x501a6c: fcomp -radius / test ah,0x41 -- at or behind -radius, or NaN, goes back
            if (!(side > -query->radius)) {
                front_side = 0;
            } else {
                front_side = 1;
                if (side < query->radius) {
                    // the sphere straddles this plane: descend both children, tagging the
                    // pushed plane with which side each recursive call is bounded by
                    query->planes[query->plane_count] = (int32_t)(node->plane | 0x80000000u);
                    query->plane_count += 1;
                    collision_bsp_query_sphere_node_recursive(query, node->back_child);
                    query->plane_count -= 1;
                    query->planes[query->plane_count] = (int32_t)(node->plane & 0x7fffffffu);
                    query->plane_count += 1;
                    collision_bsp_query_sphere_node_recursive(query, node->front_child);
                    query->plane_count -= 1;
                    return;
                }
            }
            node_index = front_side ? node->front_child : node->back_child;
        } while (-1 < (int32_t)node_index);
    }

    if (node_index != 0xffffffff) {
        ModelCollisionGeometryBSPPlane *planes =
            (ModelCollisionGeometryBSPPlane *)bsp->planes.pointer;
        ModelCollisionGeometryBSPLeaf *leaf =
            &((ModelCollisionGeometryBSPLeaf *)bsp->leaves.pointer)[node_index & 0x7fffffffu];
        collision_bsp_sphere_result *result = (collision_bsp_sphere_result *)query->result;
        real_point3d *center = (real_point3d *)query->center;
        int32_t reference_index = (int32_t)leaf->first_bsp2d_reference;
        int32_t reference_count = (int16_t)leaf->bsp2d_reference_count; // movsx at 0x501acb

        if (result->leaf_count < 0x100) {
            result->leaves[result->leaf_count] = (int32_t)(node_index & 0x7fffffffu);
            result->leaf_count += 1;
        }

        if (0 < reference_count) {
            int32_t reference_end = reference_index + reference_count;
            do {
                ModelCollisionGeometryBSP2DReference *reference =
                    &((ModelCollisionGeometryBSP2DReference *)bsp->bsp2d_references.pointer)
                        [reference_index];

                if (0 < query->plane_count) {
                    int32_t i = 0;
                    do {
                        if (query->planes[i] == (int32_t)reference->plane) {
                            Plane3D *ref_plane = &planes[reference->plane & 0x7fffffffu].plane;
                            float ref_side = -(ref_plane->vector.i * center->x +
                                               ref_plane->vector.j * center->y +
                                               ref_plane->vector.k * center->z - ref_plane->w);
                            float projected[3];
                            int16_t dominant_axis;
                            uint8_t sign;

                            projected[0] = ref_side * ref_plane->vector.i + center->x;
                            projected[1] = ref_side * ref_plane->vector.j + center->y;
                            projected[2] = ref_side * ref_plane->vector.k + center->z;

                            // 0x501bc0..0x501bf1: each test is `test ah,1` (C0), so unordered counts as "less"
                            if (!((float)fabs((double)ref_plane->vector.k) >= (float)fabs((double)ref_plane->vector.j)) ||
                                !((float)fabs((double)ref_plane->vector.k) >= (float)fabs((double)ref_plane->vector.i))) {
                                dominant_axis =
                                    !((float)fabs((double)ref_plane->vector.j) >= (float)fabs((double)ref_plane->vector.i)) ? 0 : 1;
                            } else {
                                dominant_axis = 2;
                            }
                            query->projection_axis = dominant_axis;

                            sign = (0.0f < ((float *)&ref_plane->vector)[dominant_axis]) !=
                                   ((reference->plane & 0x80000000u) != 0);
                            query->projection_sign = sign;

                            projection_axis_pair proj =
                                k_projection_axes[dominant_axis * 2 + sign];
                            query->projected_center_i = projected[proj.i];
                            query->projected_center_j = projected[proj.j];

                            collision_bsp_query_sphere_leaf_edge_recursive(query,
                                                                            reference->bsp2d_node);
                            break;
                        }
                        i = i + 1;
                    } while (i < query->plane_count);
                }
                reference_index = reference_index + 1;
            } while (reference_index < reference_end);
        }
    }
}

#if 0
Original Ghidra decompilation (0x501a10):

void FUN_00501a10(int *param_1,uint param_2)

{
  uint *puVar1;
  int iVar2;
  float *pfVar3;
  float fVar4;
  float *pfVar5;
  int iVar6;
  short sVar7;
  int iVar8;
  bool bVar9;
  float local_c [3];

  if (-1 < (int)param_2) {
    iVar2 = *(int *)(*param_1 + 0x10);
    pfVar3 = (float *)param_1[3];
    do {
      puVar1 = (uint *)(*(int *)(*param_1 + 4) + param_2 * 0xc);
      pfVar5 = (float *)(*puVar1 * 0x10 + iVar2);
      fVar4 = (*pfVar5 * *pfVar3 +
              pfVar5[1] * pfVar3[1] + *(float *)(*puVar1 * 0x10 + 8 + iVar2) * pfVar3[2]) -
              pfVar5[3];
      if (fVar4 <= -(float)param_1[4]) {
        iVar8 = 0;
      }
      else {
        iVar8 = 1;
        if (fVar4 < (float)param_1[4]) {
          param_1[param_1[6] + 7] = *puVar1 | 0x80000000;
          param_1[6] = param_1[6] + 1;
          FUN_00501a10(param_1,puVar1[1]);
          iVar2 = param_1[6];
          param_1[6] = iVar2 + -1;
          param_1[iVar2 + 6] = *puVar1 & 0x7fffffff;
          param_1[6] = param_1[6] + 1;
          FUN_00501a10(param_1,puVar1[2]);
          param_1[6] = param_1[6] + -1;
          return;
        }
      }
      param_2 = puVar1[iVar8 + 1];
    } while (-1 < (int)param_2);
  }
  if (param_2 != 0xffffffff) {
    iVar2 = *(int *)(*param_1 + 0x1c) + param_2 * 8;
    iVar8 = *(int *)(param_1[5] + 0xc0c);
    if (iVar8 < 0x100) {
      *(uint *)(param_1[5] + 0xc10 + iVar8 * 4) = param_2 & 0x7fffffff;
      *(int *)(param_1[5] + 0xc0c) = *(int *)(param_1[5] + 0xc0c) + 1;
    }
    iVar8 = *(int *)(iVar2 + 4);
    if (iVar8 < *(short *)(iVar2 + 2) + iVar8) {
      do {
        sVar7 = 0;
        puVar1 = (uint *)(*(int *)(*param_1 + 0x28) + iVar8 * 8);
        if (0 < param_1[6]) {
          iVar6 = 0;
          do {
            if (param_1[iVar6 + 7] == *puVar1) {
              pfVar5 = (float *)(*puVar1 * 0x10 + *(int *)(*param_1 + 0x10));
              pfVar3 = (float *)param_1[3];
              fVar4 = -((*pfVar5 * *pfVar3 + pfVar3[1] * pfVar5[1] + pfVar3[2] * pfVar5[2]) -
                       pfVar5[3]);
              local_c[0] = fVar4 * *pfVar5 + *pfVar3;
              local_c[1] = fVar4 * pfVar5[1] + pfVar3[1];
              local_c[2] = fVar4 * pfVar5[2] + pfVar3[2];
              if ((ABS(pfVar5[2]) < ABS(pfVar5[1])) || (ABS(pfVar5[2]) < ABS(*pfVar5))) {
                if (ABS(pfVar5[1]) < ABS(*pfVar5)) {
                  sVar7 = 0;
                }
                else {
                  sVar7 = 1;
                }
              }
              else {
                sVar7 = 2;
              }
              *(short *)(param_1 + 0x87) = sVar7;
              bVar9 = 0.0 < pfVar5[sVar7] != ((*puVar1 & 0x80000000) != 0);
              *(bool *)((int)param_1 + 0x21e) = bVar9;
              iVar6 = ((uint)bVar9 + sVar7 * 2) * 4;
              fVar4 = local_c[*(short *)(&DAT_0065c29c + iVar6)];
              param_1[0x89] = (int)local_c[*(short *)(&DAT_0065c29e + iVar6)];
              param_1[0x88] = (int)fVar4;
              FUN_00501c90(param_1,puVar1[1]);
              break;
            }
            sVar7 = sVar7 + 1;
            iVar6 = (int)sVar7;
          } while (iVar6 < param_1[6]);
        }
        iVar8 = iVar8 + 1;
      } while (iVar8 < (int)*(short *)(iVar2 + 2) + *(int *)(iVar2 + 4));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
