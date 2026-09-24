// bsp3d_node_query_recursive  (Ghidra: bsp3d_node_query_recursive, already named via cea-pdb match)
// address 0x553f10, size 428 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: full stack-frame reconstruction from disassembly (objdump -d -M intel bin/halo.exe,
//   0x553f10..0x5540bb). Ghidra recovered the 11 stack parameters but mis-modelled three of them;
//   all three are pinned here by the frame arithmetic (prologue `sub esp,0x20` then four pushes,
//   so arg_n sits at esp+0x30+4n once the prologue is done):
//     - arg2 is the ECX argument of bsp3d_node_bounds_decompress (0x553380), i.e. the *parent's*
//       bounds rectangle each level decompresses its own byte-packed bounds against -- the classic
//       progressive-refinement scheme. The recursion passes its own freshly decompressed box down
//       as the child's arg2 (`lea ecx,[esp+0x3c]` == &node_bounds at 0x55404a).
//     - arg3 is the shared visited-surface bitset, forwarded untouched; arg4 is the output int32
//       array and arg5 its capacity. The original advances the output by `lea edx,[ebx+ecx*4]`
//       with ecx = movsx(bp) (0x554042..0x554048), i.e. POINTER arithmetic on int32 elements, and
//       shrinks the capacity by `sub edx,ebp`. Ghidra rendered the first of those as the scalar
//       `iVar3 + (short)iVar11 * 4`, which an earlier rewrite of this file copied literally and
//       thereby dropped the element stride.
//     - arg8/arg9/arg10 (an earlier rewrite's "query_extra_a/b/c") are resolved: arg8 is the query
//       box handed to aabb_overlap_classify in ECX (`mov ecx,[esp+0x50]` at 0x553f4f), and arg10 /
//       arg9 are the clip-plane array and plane count handed to frustum_planes_classify_box in
//       EBX / EDI (`mov ebx,[esp+0x58]` / `mov edi,[esp+0x54]` at 0x553f61, matching that
//       function's own EAX/EBX/DI convention and the identical register setup at its other two
//       call sites 0x553cc9 and 0x554107). arg9 being the plane COUNT is what makes the
//       `mov [esp+0x54],ebp` at 0x553f76 meaningful: once the box is fully inside the frustum the
//       plane count is set to 0 so the whole subtree skips plane testing.
//   The caller 0x553d80 (structure_bsp_query_surfaces, this module) confirms the order end to end:
//   it pushes 0, tag+0xc8 (&world_bounds_x), its local visited bitset, out_surfaces, max_count,
//   the query point, the radius, the query box, plane_count, planes, 1.
//   The two child-visit conditions are also corrected here. The original sets both bytes to 1 and
//   then clears byte 0 when `radius <= distance` and byte 1 when `distance <= -radius`; an earlier
//   rewrite had the two conditions swapped and one boundary inverted.
// register convention: none -- all 11 parameters are on the stack.
// UNSURE: the 32-bit `add ebp,eax` that accumulates each child's result while both this function
//   and structure_bsp_leaf_query return through `mov ax,bp` (leaving EAX's high half untouched).
//   The counts involved are bounded by k_maximum_visible_surfaces (0x4000) so the high half is
//   always zero in practice; modelled here as an int16_t return widened by the caller.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)

// blam-cc: ECX -> parent_bounds, EDX -> compressed_bounds, ESI -> out
extern void bsp3d_node_bounds_decompress(real_rectangle3d *parent_bounds,
    uint8_t *compressed_bounds, real_rectangle3d *out); // 0x553380, this module
// blam-cc: ECX -> box_a, EDX -> box_b; returns _contained only when box_a encloses box_b, which is
// why the query box is box_a here (a node fully inside the query needs no further testing).
extern structure_bsp_overlap aabb_overlap_classify(real_rectangle3d *box_a,
    real_rectangle3d *box_b); // 0x5541b0, this module
// blam-cc: EAX -> box, EBX -> planes, DI -> plane_count
extern structure_bsp_overlap frustum_planes_classify_box(real_rectangle3d *box,
    real_plane3d *planes, int16_t plane_count); // 0x554260, this module
// blam-cc: EAX -> raw_child, ECX -> inherited_classification, stack -> the rest
extern int16_t structure_bsp_leaf_query(int32_t raw_child, int16_t inherited_classification,
    real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array,
    int32_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes);
    // 0x5540c0, this module

// Recursively descends the structure BSP's ModelCollisionGeometryBSP3DNode tree, decompressing
// each node's bounds against its parent's, classifying that box against the query box and the
// clip planes, and descending into whichever children the query sphere (point plus radius) can
// still reach. Every leaf it reaches is handed to structure_bsp_leaf_query, which appends that
// leaf's surface indices to the output array. Once the classification reaches
// _structure_bsp_overlap_contained the box tests are skipped for the whole subtree, and once the
// frustum test reports "contained" the plane count is dropped to 0 for the subtree as well.
// Returns the number of surface indices written.
int16_t bsp3d_node_query_recursive(int32_t node_index, real_rectangle3d *parent_bounds,
    uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_point3d *point,
    float radius, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes,
    int16_t inherited_classification)
{
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;   // tag +0xb4
    ScenarioStructureBSPNode *compressed_bounds =
        (ScenarioStructureBSPNode *)global_structure_bsp->nodes.pointer + node_index; // tag +0xc0, stride 6
    real_rectangle3d node_bounds;
    int16_t classification = inherited_classification;
    int32_t written = 0;

    bsp3d_node_bounds_decompress(parent_bounds, (uint8_t *)compressed_bounds, &node_bounds);

    if (inherited_classification != _structure_bsp_overlap_contained) {
        structure_bsp_overlap box_result = aabb_overlap_classify(query_box, &node_bounds);
        classification = (int16_t)box_result;
        if (box_result != _structure_bsp_overlap_none) {
            structure_bsp_overlap plane_result =
                frustum_planes_classify_box(&node_bounds, planes, plane_count);
            if (plane_result == _structure_bsp_overlap_contained) {
                plane_count = 0;   // `mov [esp+0x54],ebp` -- no plane left to clip against
            }
            classification = (int16_t)((plane_result < box_result) ? plane_result : box_result);
        }
    }

    if (classification != _structure_bsp_overlap_none) {
        ModelCollisionGeometryBSP3DNode *node =
            (ModelCollisionGeometryBSP3DNode *)collision_bsp->bsp3d_nodes.pointer + node_index;
        ModelCollisionGeometryBSPPlane *plane =
            (ModelCollisionGeometryBSPPlane *)collision_bsp->planes.pointer + node->plane;
        float distance = point->x * plane->plane.vector.i + point->y * plane->plane.vector.j +
            point->z * plane->plane.vector.k - plane->plane.w;
        // The original builds a two-byte array and indexes it with the child ordinal, so child 0
        // (back_child) is visited while the sphere reaches behind the plane and child 1
        // (front_child) while it reaches in front. Both keep the original's exact boundary:
        // `fcom radius` / jnp clears byte 0 when radius <= distance, and `fcompp` against -radius
        // clears byte 1 when distance <= -radius.
        uint8_t visit[2];
        uint32_t *children = &node->back_child;   // [0] back_child, [1] front_child
        int32_t i;

        visit[0] = (uint8_t)!(radius <= distance);
        visit[1] = (uint8_t)!(distance <= -radius);

        for (i = 0; i < 2; i = i + 1) {
            if (visit[i]) {
                int32_t child = (int32_t)children[i];
                int16_t added;
                if (child < 0) {
                    if (child == -1) {
                        continue;             // no child on this side
                    }
                    added = structure_bsp_leaf_query(child, classification, &node_bounds,
                        visited_bits, output_array + written, max_count - written, query_box,
                        plane_count, planes);
                } else {
                    added = bsp3d_node_query_recursive(child, &node_bounds, visited_bits,
                        output_array + written, max_count - written, point, radius, query_box,
                        plane_count, planes, classification);
                }
                written = written + added;
            }
        }
    }

    return (int16_t)written;
}

#if 0
Original Ghidra decompilation (0x553f10):

undefined4
bsp3d_node_query_recursive
          (int param_1,undefined4 param_2,undefined4 param_3,int param_4,int param_5,float *param_6,
          float param_7,undefined4 param_8,undefined4 param_9,undefined4 param_10,char *param_11)

{
  int iVar1;
  float fVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined2 uVar8;
  float *pfVar6;
  int iVar7;
  int *piVar9;
  int iVar10;
  int iVar11;
  undefined4 uVar12;
  int local_1c;
  undefined1 local_18 [24];

  iVar10 = param_1;
  iVar7 = *(int *)(DAT_00746f9c + 0xb4);
  iVar11 = 0;
  uVar4 = FUN_00553380();
  uVar12 = param_11;
  if ((short)param_11 != 2) {
    uVar5 = aabb_overlap_classify();
    uVar4 = uVar5;
    if ((short)uVar5 != 0) {
      uVar4 = frustum_planes_classify_box();
      if ((short)uVar4 == 2) {
        param_9 = 0;
      }
      iVar10 = param_1;
      uVar12 = uVar4;
      if ((short)uVar4 < (short)uVar5) goto LAB_00553f87;
    }
    uVar12 = uVar5;
  }
LAB_00553f87:
  iVar3 = param_4;
  uVar5 = param_3;
  uVar8 = (undefined2)((uint)uVar4 >> 0x10);
  if ((short)uVar12 != 0) {
    iVar1 = *(int *)(iVar7 + 0x10);
    piVar9 = (int *)(*(int *)(iVar7 + 4) + iVar10 * 0xc);
    pfVar6 = (float *)(*piVar9 * 0x10 + iVar1);
    param_1 = CONCAT31(param_1._1_3_,1);
    fVar2 = (*param_6 * *pfVar6 +
            pfVar6[1] * param_6[1] + *(float *)(*piVar9 * 0x10 + 8 + iVar1) * param_6[2]) -
            pfVar6[3];
    if (param_7 <= fVar2) {
      param_1 = (uint)param_1._1_3_ << 8;
    }
    param_1._0_2_ = CONCAT11(1,(byte)param_1);
    if (fVar2 <= -param_7) {
      param_1._0_2_ = (ushort)(byte)param_1;
    }
    param_11 = (char *)&param_1;
    local_1c = 2;
    do {
      piVar9 = piVar9 + 1;
      if (*param_11 != '\0') {
        iVar7 = *piVar9;
        if (iVar7 < 0) {
          if (iVar7 == -1) goto LAB_0055408e;
          iVar7 = FUN_005540c0(local_18,uVar5,iVar3 + (short)iVar11 * 4,param_5 - iVar11,param_8,
                               param_9,param_10);
        }
        else {
          iVar7 = bsp3d_node_query_recursive
                            (iVar7,local_18,uVar5,iVar3 + (short)iVar11 * 4,param_5 - iVar11,param_6
                             ,param_7,param_8,param_9,param_10,uVar12);
        }
        iVar11 = iVar11 + iVar7;
      }
LAB_0055408e:
      param_11 = param_11 + 1;
      local_1c = local_1c + -1;
      uVar8 = 0;
    } while (local_1c != 0);
  }
  return CONCAT22(uVar8,(short)iVar11);
}
#endif
