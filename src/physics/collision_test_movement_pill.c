// collision_test_movement_pill  (Ghidra: FUN_00506040; renamed -- structure-BSP-only sibling of
//   collision_test_movement_segment, sweeping a sphere/pill instead of a point)
// address 0x506040, size 371 bytes
// name confidence: 0.35   rewrite confidence: 0.9
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
//   // blam-cc: stack -> flags, origin, radius; EDI -> delta; ESI -> result
// UNSURE: collision_bsp_query_pill_init is called here with only ONE visible argument (origin). A pill query
//   needs at least bsp, delta, radius, flags and a result pointer besides; the same "argument
//   already live in a register from an earlier instruction, so the compiler never reloads it"
//   pattern collision_test_movement_segment documents for FUN_005013a0's zero-argument calls
//   plausibly extends to delta (already in EDI here) and possibly more, but Ghidra recovered
//   none of it and this rewrite does not invent values for what it cannot recover.
// UNSURE: unlike collision_test_movement_segment, this function copies the pill query's raw
//   material_index straight into both result->material_type and result->collision_material_index, with no
//   collision_materials lookup. Preserved as decompiled; not investigated further here.
// reconciled: R23 collision_result: normal -> plane.normal, unknown_30 -> plane.d, unknown_04 -> first_leaf/first_cluster, unknown_3c -> region_index, marker_index -> node_index, unknown_40 -> permutation_index (int16), unknown_48 -> plane_index, unknown_4d -> breakable_surface_index, unknown_4e -> collision_material_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "physics.h"
#include "fn_physics.h"

extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point); // 0x5013a0, EAX, ECX, EDX

extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90

static int16_t pill_leaf_cluster(int32_t leaf)
{
    if (leaf == -1) {
        return -1;
    }
    return (int16_t)((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf & 0x7fffffff].cluster;
}

// REWRITTEN from objdump 0x506040..0x5061b2. Stack: (flags, origin, radius); EDI: delta; ESI: result. Sweeps a pill
//   of the radius from origin along delta through the structure BSP (0x502730, max fraction FLT_MAX). With flags
//   0x20 a contact becomes a BSP hit (type 2): plane +0x24, material +0x34 / +0x4e, surface +0x44, plane index -1.
//   The first / last touched leaves (and clusters) go to +0x4 / +0xc; t is 1 without a hit; the end point (+0x18)
//   and its leaf (+0xc, cluster +0x10) follow. The draft had no radius, called the query with 2 of 6 operands and
//   stored the last leaf over the first.
// blam-cc: stack -> flags, origin, radius; EDI -> delta; ESI -> result
uint8_t collision_test_movement_pill(uint32_t flags, real_point3d *origin, float radius, real_vector3d *delta,
    collision_result *result)
{
    collision_result *r = result;
    collision_bsp_pill_result pill;     // [esp+0x8]
    uint8_t hit = 0;                    // bl
    int32_t leaf;
    uint32_t flt_max_bits = 0x7f7fffff;

    r->type = -1;
    *(uint32_t *)&r->t = 0x7f7fffff; // FLT_MAX, stored as its bits
    if (collision_bsp_query_pill_init(global_structure_collision_bsp, &pill, origin, delta, radius,
            *(float *)&flt_max_bits)) {
        r->t = pill.t;
        if (flags & 0x20) {
            r->plane.normal.i = pill.plane_i;
            r->plane.normal.j = pill.plane_j;
            r->plane.normal.k = pill.plane_k;
            r->plane.d = pill.plane_d;
            r->surface_flags = 0;
            r->breakable_surface_index = 0;
            r->type = 2;
            r->material_type = pill.material_index;
            r->surface_index = pill.surface_index;
            r->plane_index = -1;
            r->collision_material_index = pill.material_index;
            hit = 1;
        }
    }
    if (pill.leaf_count > 0) {
        r->first_leaf = pill.leaves[0];
        r->first_cluster = pill_leaf_cluster(pill.leaves[0]);
        leaf = pill.leaves[pill.leaf_count - 1];
        r->leaf.leaf_index = leaf;
        r->leaf.cluster_index = pill_leaf_cluster(leaf);
    }
    if (!hit) {
        r->t = 1.0f;
    }
    {
        float t = r->t;
        real_point3d *point = &r->point;

        point->x = t * delta->i + origin->x;
        point->y = t * delta->j + origin->y;
        point->z = t * delta->k + origin->z;
        leaf = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, point);
    }
    r->leaf.leaf_index = leaf;
    r->leaf.cluster_index = pill_leaf_cluster(leaf);
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
