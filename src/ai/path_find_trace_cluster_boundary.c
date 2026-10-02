// path_find_trace_cluster_boundary  (Ghidra: path_find_trace_cluster_boundary, renamed)
// address 0x43d4b0, size 716 bytes
// name confidence: 0.35  rewrite confidence: 0.8
// REWRITTEN from objdump 0x43d4b0..0x43d782. ECX = the path find map, EAX = a boundary edge; stack: origin
//   (2D), radius, side, ignore_permission, out_point. Walks the boundary between passable and blocked surfaces
//   (passable = map flag table +0x1e8 bit 0x40 and, unless ignore_permission, not intact glass) from the edge:
//   each boundary edge is oriented by whether its first surface (+0x10) is passable; with n its left normal
//   (normalised when longer than 0.0001), the origin offset by +/-radius along n is compared with the edge's first
//   vertex (a "front" test on +radius when the dot sign matches the side, and a cross test on -radius). The
//   pivot vertex (edge[0] or edge[1], from those tests, the first-surface passability and the side) is then
//   rotated around through its edges until one whose surface on that side has passability == side, which becomes
//   the next boundary edge. Returns 1 with the pivot's x/y in out_point when the same pivot comes up twice in a
//   row; 0 when the walk returns to its first pivot or a rotation finds no such edge.
// blam-cc: ECX -> map, EAX -> edge_index, stack -> origin, radius, side, ignore_permission, out_point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *breakable_surface_state;   // 0x006b8d78
extern int16_t global_structure_bsp_index; // 0x0069e8d8
extern double sqrt(double x); // FSQRT
extern double fabs(double x); // FABS

#define EDGE(bsp, i) ((int32_t *)(*(uint8_t **)((bsp) + 0x4c) + (i) * 0x18))
#define VERTEX(bsp, i) ((float *)(*(uint8_t **)((bsp) + 0x58) + (i) * 16))

static uint8_t path_find_surface_passable(uint8_t *bsp, uint8_t *walkable, uint32_t *broken, int32_t surface,
    uint8_t ignore_permission)
{
    uint8_t flags = walkable[surface];
    uint8_t passable = (uint8_t)((flags >> 6) & 1);

    if (!ignore_permission && passable && (flags & 0x80) != 0) {
        uint32_t bit = (*(uint8_t **)(bsp + 0x40))[surface * 12 + 9];

        passable = (uint8_t)((broken[bit >> 5] & (1u << (bit & 0x1f))) != 0);
    }
    return passable;
}

uint8_t path_find_trace_cluster_boundary(void *map, int32_t edge_index, real_point2d *origin, float radius,
    uint8_t side, uint8_t ignore_permission, real_point2d *out_point)
{
    uint8_t *bsp = *(uint8_t **)((uint8_t *)map + 0xb4);
    uint8_t *walkable = *(uint8_t **)((uint8_t *)map + 0x1e8);
    uint32_t *broken = (uint32_t *)(breakable_surface_state + 1 + global_structure_bsp_index * 32);
    int32_t first_pivot = -1;
    int32_t previous = -1;
    int32_t current = edge_index;
    int32_t *edge = EDGE(bsp, current);

    for (;;) {
        uint8_t first_passable = path_find_surface_passable(bsp, walkable, broken, edge[4], ignore_permission);
        float *v1 = VERTEX(bsp, edge[first_passable]);
        float *v2 = VERTEX(bsp, edge[!first_passable]);
        float ex = v2[0] - v1[0];
        float ey = v2[1] - v1[1];
        float length = (float)sqrt(ey * ey + ex * ex);
        float nx = ey;
        float ny = -ex;
        float w1x;
        float w1y;
        float w2x;
        float w2y;
        uint8_t turn = 0;
        int32_t pivot;
        int32_t rotation_start;

        if (!((float)fabs(length) < 0.0001f)) {
            float scale = 1.0f / length;

            nx = ey * scale;
            ny = -ex * scale;
        }
        w1x = v1[0] - (nx * radius + origin->x);
        w1y = v1[1] - (ny * radius + origin->y);
        w2x = v1[0] - (-radius * nx + origin->x);
        w2y = v1[1] - (ny * -radius + origin->y);
        if ((uint8_t)(w1y * ey + w1x * ex < 0.0f) == side && w1x * ey - w1y * ex < 0.0f) {
            turn = 1;
        }
        if (w2x * ey - w2y * ex < 0.0f) {
            turn = 1;
        }
        if (first_pivot == -1) {
            turn = 1;
        }
        pivot = ((uint8_t)(turn != first_passable) != side) ? edge[0] : edge[1];
        if (pivot == previous) {
            out_point->x = VERTEX(bsp, pivot)[0];
            out_point->y = VERTEX(bsp, pivot)[1];
            return 1;
        }
        if (pivot == first_pivot) {
            return 0;
        }
        if (first_pivot == -1) {
            first_pivot = pivot;
        }

        // 0x43d6d0: rotate around the pivot for the next boundary edge
        rotation_start = current;
        for (;;) {
            int32_t index = (pivot == edge[1]) ? 0 : 1;

            if (path_find_surface_passable(bsp, walkable, broken, edge[4 + index], ignore_permission) == side) {
                break;
            }
            current = edge[2 + index];
            edge = EDGE(bsp, current);
            if (current == rotation_start) {
                return 0;
            }
        }
        previous = pivot;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d4b0 @ 0x43d4b0) ----
ulonglong FUN_0043d4b0(float *param_1,float param_2,byte param_3,char param_4,undefined4 *param_5)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  int iVar12;
  int in_EAX;
  int iVar13;
  bool bVar14;
  int in_ECX;
  uint uVar15;
  float *pfVar16;
  int iVar17;
  bool bVar18;
  int iVar19;
  int iVar20;
  int local_40;
  int local_34;
  int local_30;
  float local_18;

  iVar3 = *(int *)(in_ECX + 0xb4);
  iVar1 = DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78;
  local_40 = -1;
  local_30 = -1;
  iVar4 = *(int *)(iVar3 + 0x4c);
  iVar19 = in_EAX * 0x18 + iVar4;
  iVar5 = *(int *)(iVar3 + 0x58);
  local_34 = in_EAX;
  while( true ) {
    iVar12 = local_34;
    bVar2 = *(byte *)(*(int *)(iVar19 + 0x10) + *(int *)(in_ECX + 0x1e8));
    bVar14 = (bool)(bVar2 >> 6 & 1);
    if (((param_4 == '\0') && (bVar14 != false)) && ((char)bVar2 < '\0')) {
      bVar2 = *(byte *)(*(int *)(iVar3 + 0x40) + *(int *)(iVar19 + 0x10) * 0xc + 9);
      bVar14 = (*(uint *)(iVar1 + (uint)(bVar2 >> 5) * 4) & 1 << (bVar2 & 0x1f)) != 0;
    }
    pfVar16 = (float *)(*(int *)(iVar19 + (uint)bVar14 * 4) * 0x10 + iVar5);
    bVar18 = false;
    iVar20 = *(int *)(iVar19 + (uint)(bVar14 == false) * 4) * 0x10;
    fVar7 = *(float *)(iVar20 + iVar5) - *pfVar16;
    fVar10 = *(float *)(iVar20 + iVar5 + 4) - pfVar16[1];
    fVar9 = -fVar7;
    fVar8 = SQRT(fVar10 * fVar10 + fVar9 * fVar9);
    local_18 = fVar10;
    if (0.0001 <= ABS(fVar8)) {
      fVar8 = 1.0 / fVar8;
      fVar9 = fVar8 * fVar9;
      local_18 = fVar8 * fVar10;
    }
    fVar8 = *pfVar16 - (local_18 * param_2 + *param_1);
    fVar11 = pfVar16[1] - (fVar9 * param_2 + param_1[1]);
    if ((fVar8 * fVar7 + fVar11 * fVar10 < 0.0 == (bool)param_3) &&
       (fVar8 * fVar10 - fVar11 * fVar7 < 0.0)) {
      bVar18 = true;
    }
    if ((*pfVar16 - (-param_2 * local_18 + *param_1)) * fVar10 -
        (pfVar16[1] - (fVar9 * -param_2 + param_1[1])) * fVar7 < 0.0) {
      bVar18 = true;
    }
    if (local_40 == -1) {
      bVar18 = true;
    }
    uVar15 = (uint)((bVar18 != bVar14) != (bool)param_3);
    iVar20 = *(int *)(iVar19 + 4 + uVar15 * -4);
    iVar17 = 1 - uVar15;
    if (iVar20 == local_30) {
      uVar6 = *(undefined4 *)(iVar5 + iVar20 * 0x10);
      *param_5 = uVar6;
      param_5[1] = *(undefined4 *)(iVar5 + 4 + iVar20 * 0x10);
      return CONCAT44(uVar6,CONCAT31((int3)((uint)param_5 >> 8),1));
    }
    iVar13 = local_40;
    if (iVar20 == local_40) break;
    if (local_40 == -1) {
      local_40 = iVar20;
    }
    while( true ) {
      uVar15 = (uint)(iVar20 != *(int *)(iVar19 + 4));
      iVar17 = *(int *)(iVar19 + 0x10 + uVar15 * 4);
      bVar2 = *(byte *)(iVar17 + *(int *)(in_ECX + 0x1e8));
      bVar14 = (bool)(bVar2 >> 6 & 1);
      if (((param_4 == '\0') && (bVar14 != false)) && ((char)bVar2 < '\0')) {
        bVar2 = *(byte *)(*(int *)(iVar3 + 0x40) + iVar17 * 0xc + 9);
        bVar14 = (*(uint *)(iVar1 + (uint)(bVar2 >> 5) * 4) & 1 << (bVar2 & 0x1f)) != 0;
      }
      local_30 = iVar20;
      if (bVar14 == (bool)param_3) break;
      local_34 = *(int *)(iVar19 + 8 + uVar15 * 4);
      iVar17 = local_34 * 3;
      iVar19 = iVar4 + local_34 * 0x18;
      iVar13 = iVar4;
      if (local_34 == iVar12) goto LAB_0043d74b;
    }
  }
LAB_0043d74b:
  return CONCAT44(iVar17,iVar13) & 0xffffffffffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
