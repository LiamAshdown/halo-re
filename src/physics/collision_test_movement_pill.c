// collision_test_movement_pill  (Ghidra: FUN_00506040; renamed -- structure-BSP-only sibling of
//   collision_test_movement_segment, sweeping a sphere/pill instead of a point)
// address 0x506040, size 371 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/physics.h collision_bsp_pill_result (0x420: t@0x00, plane_i/j/k/d@0x04-0x13,
//   surface_index@0x14, material_index@0x1a, leaf_count@0x1c, leaves@0x20) matches the seven
//   Ghidra locals local_420..local_404[] byte for byte, the same way
//   collision_test_movement_segment's local_418..local_404[] matched collision_bsp_segment_result;
//   types/physics.h "what this module adds to collision_result" for the collision_result fields
//   this writes. out/phase4/physics_functions.md's own one-line summary ("queries the local
//   physics-model shapes rather than raw BSP surfaces directly") does not match what this
//   function's callee (collision_bsp_query_pill_init, the pill-query constructor, per its own description
//   "Initializes a swept-sphere/segment collision query record") actually is; this rewrite
//   follows the struct evidence over that low-confidence (0.3) summary.
// register convention: EAX (unrecognized by Ghidra as in_EAX, but the byte parameter has no
//   other source) -> flags, stack -> origin; unaff_ESI -> result (collision_result *, out
//   parameter), unaff_EDI -> delta (real_vector3d *). No radius is visible anywhere in this
//   function's own body.
//   // blam-cc: EAX -> flags, ESI -> result, EDI -> delta, stack -> origin
// UNSURE: collision_bsp_query_pill_init is called here with only ONE visible argument (origin). A pill query
//   needs at least bsp, delta, radius, flags and a result pointer besides; the same "argument
//   already live in a register from an earlier instruction, so the compiler never reloads it"
//   pattern collision_test_movement_segment documents for FUN_005013a0's zero-argument calls
//   plausibly extends to delta (already in EDI here) and possibly more, but Ghidra recovered
//   none of it and this rewrite does not invent values for what it cannot recover.
// UNSURE: unlike collision_test_movement_segment, this function copies the pill query's raw
//   material_index straight into both result->material_type and result->unknown_4e, with no
//   collision_materials lookup. Preserved as decompiled; not investigated further here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "physics.h"

extern ModelCollisionGeometryBSP *structure_collision_bsp; // 0x00746f98
extern ScenarioStructureBSP *structure_bsp_tag_data; // 0x00746f9c

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point); // 0x5013a0, this module (lower half)
// blam-cc: UNSURE which registers besides the visible origin are actually read; see file header
extern uint8_t collision_bsp_query_pill_init(real_point3d *origin, collision_bsp_pill_result *out_result); // 0x502730

// Sweeps a sphere/pill from origin along delta (delta comes in via the still-live EDI register,
// not a visible parameter) through the structure BSP only -- no water-plane test, no nearby-object
// walk, and no post-hit unstick loop, unlike collision_test_movement_segment. When
// _collision_test_flag_structure_bsp (flags bit 0x20) is set and the pill query found a surface,
// applies it to *result; either way resolves and stores the touched leaves' clusters, then the
// final resting point's own leaf/cluster.
uint8_t collision_test_movement_pill(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    collision_result *result)
{
    uint8_t hit = 0;
    collision_bsp_pill_result pill_result;
    uint8_t found_surface;
    bsp_leaf_reference *last_leaf_ref = (bsp_leaf_reference *)&result->leaf;
    int32_t final_leaf;

    result->type = -1;
    result->t = 0x7f7fffff; // FLT_MAX

    found_surface = collision_bsp_query_pill_init(origin, &pill_result);
    result->t = pill_result.t;
    if (found_surface && (flags & 0x20) != 0) {
        result->normal.i = pill_result.plane_i;
        result->normal.j = pill_result.plane_j;
        result->normal.k = pill_result.plane_k;
        result->unknown_30 = pill_result.plane_d;
        result->surface_flags = 0;
        result->unknown_4d = 0;
        result->type = 2;
        result->material_type = pill_result.material_index;
        result->unknown_48 = pill_result.surface_index;
        result->unknown_4e = pill_result.material_index;
        hit = 1;
    }

    if (pill_result.leaf_count > 0) {
        int32_t first_leaf = pill_result.leaves[0];
        int32_t last_leaf = pill_result.leaves[pill_result.leaf_count - 1];

        result->leaf.leaf_index = first_leaf;
        result->leaf.cluster_index = (first_leaf == -1) ? -1 :
            ((ScenarioStructureBSPLeaf *)structure_bsp_tag_data->leaves.pointer)[first_leaf].cluster;

        last_leaf_ref->leaf_index = last_leaf;
        last_leaf_ref->cluster_index = (last_leaf == -1) ? -1 :
            ((ScenarioStructureBSPLeaf *)structure_bsp_tag_data->leaves.pointer)[last_leaf].cluster;
    }

    if (hit == 0) {
        result->t = 1.0f;
    }
    result->point.x = result->t * delta->i + origin->x;
    result->point.y = result->t * delta->j + origin->y;
    result->point.z = result->t * delta->k + origin->z;

    final_leaf = bsp3d_node_find_leaf(0, structure_collision_bsp, &result->point);
    last_leaf_ref->leaf_index = final_leaf;
    if (final_leaf == -1) {
        last_leaf_ref->cluster_index = -1;
        return hit;
    }
    last_leaf_ref->cluster_index =
        ((ScenarioStructureBSPLeaf *)structure_bsp_tag_data->leaves.pointer)[final_leaf].cluster;
    return hit;
}

#if 0
Original Ghidra decompilation (0x506040):

undefined4 FUN_00506040(byte param_1,float *param_2)

{
  float fVar1;
  char cVar2;
  undefined2 uVar3;
  int iVar4;
  char cVar5;
  undefined2 *unaff_ESI;
  float *unaff_EDI;
  undefined4 local_420;
  undefined4 local_41c;
  undefined4 local_418;
  undefined4 local_414;
  undefined4 local_410;
  undefined4 local_40c;
  undefined2 local_406;
  int local_404 [257];

  cVar5 = '\0';
  *unaff_ESI = 0xffff;
  *(undefined4 *)(unaff_ESI + 10) = 0x7f7fffff;
  cVar2 = FUN_00502730(param_2);
  if ((cVar2 != '\0') && (*(undefined4 *)(unaff_ESI + 10) = local_420, (param_1 & 0x20) != 0)) {
    *(undefined4 *)(unaff_ESI + 0x12) = local_41c;
    *(undefined4 *)(unaff_ESI + 0x14) = local_418;
    *(undefined4 *)(unaff_ESI + 0x16) = local_414;
    *(undefined4 *)(unaff_ESI + 0x18) = local_410;
    *(undefined1 *)(unaff_ESI + 0x26) = 0;
    *(undefined1 *)((int)unaff_ESI + 0x4d) = 0;
    *unaff_ESI = 2;
    unaff_ESI[0x1a] = local_406;
    *(undefined4 *)(unaff_ESI + 0x22) = local_40c;
    *(undefined4 *)(unaff_ESI + 0x24) = 0xffffffff;
    unaff_ESI[0x27] = local_406;
    cVar5 = '\x01';
  }
  if (0 < local_404[0]) {
    *(int *)(unaff_ESI + 2) = local_404[1];
    if (local_404[1] == -1) {
      uVar3 = 0xffff;
    }
    else {
      uVar3 = *(undefined2 *)(local_404[1] * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
    }
    unaff_ESI[4] = uVar3;
    iVar4 = local_404[local_404[0]];
    *(int *)(unaff_ESI + 6) = iVar4;
    if (iVar4 == -1) {
      uVar3 = 0xffff;
    }
    else {
      uVar3 = *(undefined2 *)(iVar4 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
    }
    unaff_ESI[8] = uVar3;
  }
  if (cVar5 == '\0') {
    *(undefined4 *)(unaff_ESI + 10) = 0x3f800000;
  }
  fVar1 = *(float *)(unaff_ESI + 10);
  *(float *)(unaff_ESI + 0xc) = fVar1 * *unaff_EDI + *param_2;
  *(float *)(unaff_ESI + 0xe) = fVar1 * unaff_EDI[1] + param_2[1];
  *(float *)(unaff_ESI + 0x10) = fVar1 * unaff_EDI[2] + param_2[2];
  iVar4 = FUN_005013a0();
  *(int *)(unaff_ESI + 6) = iVar4;
  if (iVar4 == -1) {
    unaff_ESI[8] = 0xffff;
    return CONCAT31(0xffffff,cVar5);
  }
  uVar3 = *(undefined2 *)(iVar4 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  unaff_ESI[8] = uVar3;
  return CONCAT31((int3)(char)((ushort)uVar3 >> 8),cVar5);
}
#endif
