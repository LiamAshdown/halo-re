// collision_bsp_query_segment_node_recursive  (Ghidra: FUN_00502140, still unnamed there;
// phase-2 proposed physics_shape_pill_node_test_recursive, but the struct this function walks
// (collision_bsp_segment_query, confirmed field-by-field in
// out/phase4/physics_types_notes.md) is the SEGMENT query, not the pill/swept-sphere one, so
// this rewrite uses the more accurate name to match collision_bsp_query_sphere_node_recursive.)
// address 0x502140, size 787 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: collision_bsp_segment_query/…_result field offsets (flags 0x00, bsp 0x04, origin
//   0x10, delta 0x14, result 0x18, last_leaf 0x1c, last_leaf_type 0x20, crossing_plane 0x24)
//   match every word offset this function reads or writes; collision_bsp_segment_flags bit
//   values (0x01 test_front_face, 0x02 test_back_face, 0x04 test_double_sided) match the
//   post-leaf gating logic exactly.
// register convention: none -- all four arguments are Ghidra-recognized stack parameters.
//   // blam-cc: stack -> query, node_index, t_min, t_max
// UNSURE, significantly: the leaf-side gating logic (which of last_leaf_type/this leaf_type/the
// three test_* flag bits decide whether to test for a crossing surface at all, and whether to
// prefer the previous leaf over this one) is preserved exactly as decompiled, including the
// nested if/else/goto shape, rather than simplified, since a reordering here could silently
// change which contact surface the engine reports. See collision_bsp_surface_test_point_leaf.c
// for the crossing_point parameter this function must supply -- reconstructed as
// origin + delta * t at the plane crossing, which Ghidra's decompile of this function does not
// show being computed, consistent with the same missing computation that function's header
// documents.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern int32_t collision_bsp_surface_test_point_leaf(
    ModelCollisionGeometryBSP *bsp, int32_t leaf_index, int16_t breakable_surface_count,
    uint32_t *breakable_surfaces, uint32_t plane_index, real_point3d *crossing_point,
    uint8_t two_sided); // 0x502460, this batch

// blam-cc: stack -> query, node_index, t_min, t_max
uint8_t collision_bsp_query_segment_node_recursive(collision_bsp_segment_query *query,
                                                    uint32_t node_index, float t_min,
                                                    float t_max)
{
    ModelCollisionGeometryBSP *bsp = (ModelCollisionGeometryBSP *)query->bsp;

    if (-1 < (int32_t)node_index) {
        ModelCollisionGeometryBSP3DNode *node =
            &((ModelCollisionGeometryBSP3DNode *)bsp->bsp3d_nodes.pointer)[node_index];
        ModelCollisionGeometryBSPPlane *planes =
            (ModelCollisionGeometryBSPPlane *)bsp->planes.pointer;
        Plane3D *plane = &planes[node->plane].plane;
        real_point3d *origin = (real_point3d *)query->origin;
        real_vector3d *delta = (real_vector3d *)query->delta;
        float side_at_origin = plane->vector.i * origin->x + plane->vector.j * origin->y +
                                plane->vector.k * origin->z - plane->w;
        float dot_delta_normal =
            plane->vector.i * delta->i + plane->vector.j * delta->j + plane->vector.k * delta->k;
        float side_at_min = dot_delta_normal * t_min + side_at_origin;
        float side_at_max = dot_delta_normal * t_max + side_at_origin;
        int any_negative = (side_at_min < 0.0f) || (side_at_max < 0.0f);
        int any_nonneg = (0.0f <= side_at_min) || (0.0f <= side_at_max);
        uint32_t far_child;
        uint8_t hit;

        if (any_negative && any_nonneg) {
            // the segment straddles this plane within [t_min, t_max]: recurse into the near
            // side up to the crossing, then (if nothing closer was found) the far side from it
            float t_cross = -(side_at_origin / dot_delta_normal);
            // FIXED (0x502226..0x502239: bl = dot > 0; sete cl; child [node + 4 + cl*4]): the near side is the FRONT
            // child when dot <= 0 and the BACK child otherwise -- the opposite of the far child below. The draft had
            // it inverted, so for dot > 0 it searched the front child twice and never the back one (in game: bullets
            // passed through walls and NPCs).
            uint32_t near_child =
                (dot_delta_normal <= 0.0f) ? node->front_child : node->back_child;

            hit = collision_bsp_query_segment_node_recursive(query, near_child, t_min, t_cross);
            if (hit) {
                return 1;
            }
            // equivalent to (result->t <= t_cross); see the XOR idiom in
            // breakable_surface_apply_damage.c
            collision_bsp_segment_result *result = (collision_bsp_segment_result *)query->result;
            if (result->t <= t_cross) {
                return 0;
            }
            query->crossing_plane = (int32_t)node->plane;
            far_child = (0.0f < dot_delta_normal) ? node->front_child : node->back_child;
            t_min = t_cross;
        } else {
            far_child = any_nonneg ? node->front_child : node->back_child;
        }
        return collision_bsp_query_segment_node_recursive(query, far_child, t_min, t_max) ? 1 : 0;
    }

    {
        collision_bsp_leaf_type leaf_type = _collision_bsp_leaf_type_none;
        int32_t leaf_index = -1;
        collision_bsp_segment_result *result = (collision_bsp_segment_result *)query->result;
        int32_t candidate_leaf;
        uint8_t two_sided = 0;

        if (node_index != 0xffffffff) {
            ModelCollisionGeometryBSPLeaf *leaf =
                &((ModelCollisionGeometryBSPLeaf *)bsp->leaves.pointer)[node_index & 0x7fffffffu];
            leaf_index = (int32_t)(node_index & 0x7fffffffu);
            // FIXED (0x5022d7: test [leaf],1; setne al; inc al): flag bit 0 set gives type 2, clear gives 1. The
            // draft had the mapping inverted (in game: wall hits resolved through the wrong branch, so the surface
            // and material were wrong and no impact effect played). NOTE: by the value the original assigns, the
            // names _double_sided (1) and _normal (2) in types/physics.h look swapped; kept numerically faithful.
            leaf_type = ((leaf->flags & 1) != 0) ? _collision_bsp_leaf_type_normal
                                                  : _collision_bsp_leaf_type_double_sided;
        }

        candidate_leaf = leaf_index;
        if ((query->flags & _collision_bsp_test_front_face) == 0 ||
            ((query->last_leaf_type != _collision_bsp_leaf_type_double_sided &&
              query->last_leaf_type != _collision_bsp_leaf_type_normal) ||
             leaf_type != _collision_bsp_leaf_type_none)) {
            if ((query->flags & _collision_bsp_test_back_face) == 0 ||
                query->last_leaf_type != _collision_bsp_leaf_type_none ||
                (leaf_type != _collision_bsp_leaf_type_double_sided &&
                 leaf_type != _collision_bsp_leaf_type_normal)) {
                if ((query->flags & _collision_bsp_test_double_sided) != 0 ||
                    query->last_leaf_type != _collision_bsp_leaf_type_normal ||
                    leaf_type != _collision_bsp_leaf_type_normal) {
                    goto test_candidate;
                }
                if ((query->flags & _collision_bsp_test_front_face) != 0) {
                    candidate_leaf = query->last_leaf;
                }
                two_sided = 1;
            }
        } else {
            candidate_leaf = query->last_leaf;
        }

        if (candidate_leaf != -1) {
            ModelCollisionGeometryBSPPlane *planes =
                (ModelCollisionGeometryBSPPlane *)bsp->planes.pointer;
            real_point3d crossing_point;
            crossing_point.x = ((real_point3d *)query->origin)->x +
                                ((real_vector3d *)query->delta)->i * t_min;
            crossing_point.y = ((real_point3d *)query->origin)->y +
                                ((real_vector3d *)query->delta)->j * t_min;
            crossing_point.z = ((real_point3d *)query->origin)->z +
                                ((real_vector3d *)query->delta)->k * t_min;

            int32_t surface_index = collision_bsp_surface_test_point_leaf(
                bsp, candidate_leaf, (int16_t)query->breakable_surface_count,
                query->breakable_surfaces, (uint32_t)query->crossing_plane, &crossing_point,
                two_sided);
            if (surface_index != -1) {
                ModelCollisionGeometryBSPSurface *surface =
                    &((ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer)[surface_index];
                if (((surface->flags & 0x02) == 0 || (query->flags & 0x08) == 0) &&
                    ((surface->flags & 0x08) == 0 || (query->flags & 0x10) == 0)) {
                    result->t = t_min;
                    result->plane = &planes[query->crossing_plane].plane;
                    result->surface_index = surface_index;
                    result->plane_index = (int32_t)surface->plane;
                    result->surface_flags = surface->flags;
                    result->breakable_surface_index = surface->breakable_surface;
                    result->material_index = (int16_t)surface->material;
                    return 1;
                }
            }
        }
    test_candidate:
        if (leaf_index != -1) {
            if (result->leaf_count < 0x100) {
                result->leaves[result->leaf_count] = leaf_index;
                result->leaf_count += 1;
                query->last_leaf_type = leaf_type;
                query->last_leaf = leaf_index;
                return 0;
            }
            result->leaves[255] = leaf_index; // overflow: overwrite the last slot
        }
        query->last_leaf_type = leaf_type;
        query->last_leaf = leaf_index;
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x502140):

undefined4 FUN_00502140(uint *param_1,uint param_2,float param_3,float param_4)

{
  uint *puVar1;
  undefined4 *puVar2;
  float *pfVar3;
  float *pfVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  bool bVar9;
  bool bVar10;
  char cVar11;
  float *pfVar12;
  int iVar13;
  uint uVar14;
  uint uVar15;
  uint local_c;
  undefined4 local_8;

  if (-1 < (int)param_2) {
    puVar1 = (uint *)(*(int *)(param_1[1] + 4) + param_2 * 0xc);
    iVar13 = *(int *)(param_1[1] + 0x10);
    pfVar3 = (float *)param_1[4];
    pfVar12 = (float *)(*puVar1 * 0x10 + iVar13);
    pfVar4 = (float *)param_1[5];
    fVar5 = (*pfVar12 * *pfVar3 +
            pfVar3[1] * pfVar12[1] + pfVar3[2] * *(float *)(*puVar1 * 0x10 + 8 + iVar13)) -
            pfVar12[3];
    fVar6 = *pfVar12 * *pfVar4 + pfVar4[1] * pfVar12[1] + pfVar4[2] * pfVar12[2];
    fVar7 = fVar6 * param_3 + fVar5;
    fVar8 = fVar6 * param_4 + fVar5;
    if ((fVar7 < 0.0) || (fVar8 < 0.0)) {
      bVar10 = true;
    }
    else {
      bVar10 = false;
    }
    if ((0.0 <= fVar7) || (0.0 <= fVar8)) {
      bVar9 = true;
    }
    else {
      bVar9 = false;
    }
    if ((bVar10) && (bVar9)) {
      fVar5 = -(fVar5 / fVar6);
      cVar11 = FUN_00502140(param_1,puVar1[(fVar6 <= 0.0) + 1],param_3,fVar5);
      if (cVar11 != '\0') {
        return 1;
      }
      if (*(float *)param_1[6] < fVar5 != (*(float *)param_1[6] == fVar5)) {
        return 0;
      }
      param_1[9] = *puVar1;
      uVar15 = puVar1[(0.0 < fVar6) + 1];
      param_3 = fVar5;
    }
    else {
      uVar15 = puVar1[bVar9 + 1];
    }
    cVar11 = FUN_00502140(param_1,uVar15,param_3,param_4);
    if (cVar11 == '\0') {
      return 0;
    }
    return 1;
  }
  cVar11 = '\x03';
  local_c = 0xffffffff;
  local_8 = 0;
  if (param_2 != 0xffffffff) {
    local_c = param_2 & 0x7fffffff;
    cVar11 = ((*(byte *)(*(int *)(param_1[1] + 0x1c) + param_2 * 8) & 1) != 0) + '\x01';
  }
  uVar15 = *param_1;
  if (((uVar15 & 1) == 0) ||
     ((((char)param_1[8] != '\x01' && ((char)param_1[8] != '\x02')) || (cVar11 != '\x03')))) {
    uVar14 = local_c;
    if ((((uVar15 & 2) == 0) || ((char)param_1[8] != '\x03')) ||
       ((cVar11 != '\x01' && (cVar11 != '\x02')))) {
      if ((((uVar15 & 4) != 0) || ((char)param_1[8] != '\x02')) || (cVar11 != '\x02'))
      goto LAB_0050240c;
      if ((uVar15 & 1) != 0) {
        uVar14 = param_1[7];
      }
      local_8 = 1;
    }
  }
  else {
    uVar14 = param_1[7];
  }
  if (uVar14 != 0xffffffff) {
    uVar14 = param_1[1];
    iVar13 = FUN_00502460((short)param_1[2],param_1[3],param_1[9],param_3,local_8);
    if (iVar13 != -1) {
      puVar2 = (undefined4 *)(*(int *)(uVar14 + 0x40) + iVar13 * 0xc);
      if ((((*(byte *)(puVar2 + 2) & 2) == 0) || ((uVar15 & 8) == 0)) &&
         (((*(byte *)(puVar2 + 2) & 8) == 0 || ((uVar15 & 0x10) == 0)))) {
        *(float *)param_1[6] = param_3;
        *(uint *)(param_1[6] + 4) = param_1[9] * 0x10 + *(int *)(param_1[1] + 0x10);
        *(int *)(param_1[6] + 8) = iVar13;
        *(undefined4 *)(param_1[6] + 0xc) = *puVar2;
        *(undefined1 *)(param_1[6] + 0x10) = *(undefined1 *)(puVar2 + 2);
        *(undefined1 *)(param_1[6] + 0x11) = *(undefined1 *)((int)puVar2 + 9);
        *(undefined2 *)(param_1[6] + 0x12) = *(undefined2 *)((int)puVar2 + 10);
        return 1;
      }
    }
  }
LAB_0050240c:
  if (local_c != 0xffffffff) {
    uVar15 = param_1[6];
    if (*(int *)(uVar15 + 0x14) < 0x100) {
      *(uint *)(uVar15 + 0x18 + *(int *)(uVar15 + 0x14) * 4) = local_c;
      *(int *)(param_1[6] + 0x14) = *(int *)(param_1[6] + 0x14) + 1;
      *(char *)(param_1 + 8) = cVar11;
      param_1[7] = local_c;
      return 0;
    }
    *(uint *)(uVar15 + 0x414) = local_c;
  }
  param_1[7] = local_c;
  *(char *)(param_1 + 8) = cVar11;
  return 0;
}
#endif
