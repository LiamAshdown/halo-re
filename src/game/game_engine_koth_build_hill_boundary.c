// game_engine_koth_build_hill_boundary  (Ghidra: FUN_0046a240; named per this rewrite)
// address 0x46a240, size 709 bytes
// name confidence: 0.45   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Builds a 2D convex-hull boundary around a set of
//   valid starting locations and computes its bounding-box center and vertical extent");
//   game_engine_find_valid_starting_locations (0x461080, already committed) called with origin
//   NULL (EBX=0, verified in the disassembly) and its established 7-parameter order;
//   point3d_array_project_to_xy_plane (0x46a130, this batch, CORRECTED name); polygon2d_convex_
//   hull_build (0x4caae0, math/geometry module, not yet rewritten -- declared locally).
//   types/objects.h/game.h global 0x00746f8c global_scenario (+0x37c netgame_flags).
// register convention: no parameters.
// UNSURE: the 24-dword `local_60` side array (12 dword pairs, one per starting-location result)
//   is copied in lockstep with the position/index reshuffle but this function never itself
//   writes it before reading it back -- it must be a second output the disassembly does not
//   show being filled (possibly a hidden extra result of game_engine_find_valid_starting_
//   locations, or scratch left by an inlined helper); kept as an opaque uint32_t pair array
//   with a raw copy, not given a semantic type.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern Scenario *global_scenario; // 0x00746f8c
extern int32_t king_starting_location_type; // 0x006b1064, UNSURE exact identity (see PLAN.md
    // team/type parameter-swap note on game_engine_find_valid_starting_locations)
extern int32_t king_starting_location_count;  // 0x006b0f50
extern real_point3d king_hill_boundary_points[12]; // 0x006b0f54, UNSURE exact declared size
extern uint32_t king_hill_boundary_extra[12][2];   // 0x006b0fe4, UNSURE identity, see header
extern float king_hill_boundary_min_z;   // 0x006b1060 (stored as -0.1 below the true min)
extern float king_hill_boundary_max_z;   // 0x006b105c (stored as +0.8 above the true max)
extern real_point3d king_hill_boundary_center; // 0x006b1044 (x,y,z = 0x1044/0x1048/0x104c)

extern int32_t game_engine_find_valid_starting_locations(real_point3d *origin,
    float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type,
    int32_t max_results, int32_t *results); // 0x461080
extern void point3d_array_project_to_xy_plane(real_point3d *source, Point2D *destination,
    int32_t count); // 0x46a130, this batch (CORRECTED name)
extern int16_t polygon2d_convex_hull_build(int32_t count, Point2D *points); // 0x4caae0,
    // math/geometry module, not yet rewritten; UNSURE exact signature -- writes the hull vertex
    // order back into `points` as int16 indices (Ghidra reads it that way at the caller)

// Builds the 2D convex-hull boundary of up to 12 type-8/king starting locations (or a synthetic
// 1x1 square around the single location when there is exactly one), projects them to the XY
// ground plane, computes the hull, reorders the boundary points and their opaque per-point data
// into king_hill_boundary_points/_extra, then derives the boundary's vertical extent
// (king_hill_boundary_min_z/_max_z) and its horizontal+vertical center.
void game_engine_koth_build_hill_boundary(void)
{
    real_point3d points[12];
    int32_t indices[12];
    Point2D hull_points[12];
    uint32_t extra[12][2]; // UNSURE, see header
    int32_t count;
    int16_t hull_count;
    int32_t i;

    count = game_engine_find_valid_starting_locations(
        (real_point3d *)0, 0.0f, 0.0f, 8, (int16_t)king_starting_location_type, 0xc, indices);
    king_starting_location_count = count;
    if (count == 0) {
        return;
    }

    for (i = 0; i < count; i++) {
        ScenarioNetgameFlags *loc =
            &((ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer)[indices[i]];
        points[i].x = loc->position.x;
        points[i].y = loc->position.y;
        points[i].z = loc->position.z;
    }

    if (count == 1) {
        // Synthesizes a 1x1 square centered on the single point when there is nothing to hull.
        points[1].x = points[0].x + 1.0f;
        points[1].y = points[0].y - 1.0f;
        points[1].z = points[0].z;
        points[2].x = points[0].x + 1.0f;
        points[2].y = points[0].y + 1.0f;
        points[2].z = points[0].z;
        points[3].x = points[0].x - 1.0f;
        points[3].y = points[0].y + 1.0f;
        points[3].z = points[0].z;
        points[0].y = points[0].y - 1.0f;
        points[0].x = points[0].x - 1.0f;
        count = 4;
    }

    point3d_array_project_to_xy_plane(points, hull_points, count);
    hull_count = polygon2d_convex_hull_build(count, hull_points);
    king_starting_location_count = hull_count;

    if (hull_count > 0) {
        for (i = 0; i < hull_count; i++) {
            int16_t src = ((int16_t *)hull_points)[i];
            king_hill_boundary_points[i] = points[src];
            extra[i][0] = ((uint32_t *)hull_points)[src * 2];     // UNSURE, see header
            extra[i][1] = ((uint32_t *)hull_points)[src * 2 + 1]; // UNSURE, see header
        }
        for (i = 0; i < hull_count; i++) {
            king_hill_boundary_extra[i][0] = extra[i][0];
            king_hill_boundary_extra[i][1] = extra[i][1];
        }
    }

    king_hill_boundary_center.x = king_hill_boundary_points[0].x;
    king_hill_boundary_center.y = king_hill_boundary_points[0].y;
    king_hill_boundary_center.z = king_hill_boundary_points[0].z;
    {
        real_point3d minv = king_hill_boundary_points[0];
        real_point3d maxv = king_hill_boundary_points[0];

        for (i = 0; i < hull_count; i++) {
            real_point3d *p = &king_hill_boundary_points[i];
            if (p->x < minv.x) minv.x = p->x;
            if (p->y < minv.y) minv.y = p->y;
            if (p->z < minv.z) minv.z = p->z;
            if (p->x >= maxv.x) maxv.x = p->x;
            if (p->y >= maxv.y) maxv.y = p->y;
            if (p->z >= maxv.z) maxv.z = p->z;
        }

        king_hill_boundary_min_z = minv.z - 0.1f;
        king_hill_boundary_max_z = maxv.z + 0.8f;
        king_hill_boundary_center.x = (maxv.x + minv.x) * 0.5f;
        king_hill_boundary_center.y = (maxv.y + minv.y) * 0.5f;
        king_hill_boundary_center.z = (maxv.z + minv.z) * 0.5f;
    }
}

#if 0
Original Ghidra decompilation (0x46a240), from tools/pack.py 0x46a240:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0046a240(void)

{
  float fVar1;
  undefined4 uVar2;
  short sVar3;
  int iVar4;
  float *pfVar5;
  undefined4 *puVar6;
  int iVar7;
  float *pfVar8;
  float afStackY_600f0 [98280];
  float local_130;
  float local_12c;
  float local_128;
  float *local_124;
  float local_120 [12];
  float local_f0 [4];
  float local_e0;
  undefined4 local_dc;
  float local_d8;
  float local_d4;
  undefined4 local_d0;
  float local_cc;
  float local_c8;
  undefined4 local_c4;
  undefined4 local_60 [24];

  iVar7 = DAT_00746f8c;
  DAT_006b0f50 = game_engine_find_valid_starting_locations
                           (0.0,0.0,8,(short)DAT_006b1064,0xc,(int *)local_120);
  if (DAT_006b0f50 != 0) {
    iVar4 = 0;
    if (0 < DAT_006b0f50) {
      iVar7 = *(int *)(iVar7 + 0x37c);
      pfVar5 = local_f0;
      do {
        pfVar8 = (float *)((int)local_120[iVar4] * 0x94 + iVar7);
        *pfVar5 = *pfVar8;
        fVar1 = pfVar8[2];
        iVar4 = iVar4 + 1;
        pfVar5[1] = pfVar8[1];
        pfVar5[2] = fVar1;
        pfVar5 = pfVar5 + 3;
      } while (iVar4 < DAT_006b0f50);
    }
    iVar7 = DAT_006b0f50;
    if (DAT_006b0f50 == 1) {
      local_dc = local_f0[2];
      local_d0 = local_f0[2];
      local_f0[3] = local_f0[0] + 1.0;
      local_c4 = local_f0[2];
      iVar7 = 4;
      local_e0 = local_f0[1] - 1.0;
      local_d8 = local_f0[0] - 1.0;
      local_d4 = local_f0[1] + 1.0;
      local_cc = local_f0[0] + 1.0;
      local_c8 = local_f0[1] + 1.0;
      local_f0[0] = local_f0[0] - 1.0;
      local_f0[1] = local_f0[1] - 1.0;
    }
    point3d_array_extract_xz_pairs(local_f0);
    sVar3 = polygon2d_convex_hull_build(iVar7,local_120);
    DAT_006b0f50 = (int)sVar3;
    iVar7 = 0;
    if (0 < DAT_006b0f50) {
      puVar6 = &DAT_006b0fe4;
      local_124 = &DAT_006b0f54;
      do {
        iVar4 = (int)*(short *)((int)local_120 + iVar7 * 2);
        *local_124 = local_f0[iVar4 * 3];
        fVar1 = local_f0[iVar4 * 3 + 2];
        local_124[1] = local_f0[iVar4 * 3 + 1];
        local_124[2] = fVar1;
        uVar2 = local_60[iVar4 * 2 + 1];
        *puVar6 = local_60[iVar4 * 2];
        puVar6[1] = uVar2;
        iVar7 = iVar7 + 1;
        local_124 = local_124 + 3;
        puVar6 = puVar6 + 2;
      } while (iVar7 < DAT_006b0f50);
    }
    local_120[0] = DAT_006b0f54;
    local_120[1] = DAT_006b0f58;
    local_120[2] = DAT_006b0f5c;
    local_130 = DAT_006b0f54;
    local_12c = DAT_006b0f58;
    local_128 = DAT_006b0f5c;
    if (0 < DAT_006b0f50) {
      pfVar5 = &DAT_006b0f58;
      iVar7 = DAT_006b0f50;
      do {
        if (pfVar5[-1] < local_120[0]) {
          local_120[0] = pfVar5[-1];
        }
        if (*pfVar5 < local_120[1]) {
          local_120[1] = *pfVar5;
        }
        if (pfVar5[1] < local_120[2]) {
          local_120[2] = pfVar5[1];
        }
        if (local_130 <= pfVar5[-1]) {
          local_130 = pfVar5[-1];
        }
        if (local_12c <= *pfVar5) {
          local_12c = *pfVar5;
        }
        if (local_128 <= pfVar5[1]) {
          local_128 = pfVar5[1];
        }
        pfVar5 = pfVar5 + 3;
        iVar7 = iVar7 + -1;
      } while (iVar7 != 0);
    }
    _DAT_006b1060 = local_120[2] - 0.1;
    _DAT_006b105c = local_128 + 0.8;
    DAT_006b1044 = (local_130 + local_120[0]) * 0.5;
    DAT_006b1048 = (local_12c + local_120[1]) * 0.5;
    DAT_006b104c = (local_128 + local_120[2]) * 0.5;
  }
  return;
}
#endif
