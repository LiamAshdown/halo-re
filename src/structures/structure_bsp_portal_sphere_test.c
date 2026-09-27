// structure_bsp_portal_sphere_test  (Ghidra: FUN_00554b00, still unnamed there)
// address 0x554b00, size 420 bytes
// name confidence: 0.55 -- matches the phase4 summary ("Tests whether a sphere intersects a
//   specific portal polygon by combining a plane-distance check with a 2D point-in-polygon test").
// rewrite confidence: 0.85 (VERIFIED against objdump 0x554b00..0x554ca3 (plane distance vs tolerance, centroid sphere, major-axis projection through k_projection_axes, portal vertices to 2D, polygon2d_point_inside_tolerance); FIXED the 2D tolerance: the binary takes sqrt(tol*tol - distance * projected.x) ([esp+0x14] at 0x554c6a), not distance * distance) -- 3 of its 4 inputs are register-passed with no visible call-site
//   arguments in Ghidra's decompile; resolved from the caller (cluster_flood_fill_within_radius.c,
//   this batch: EAX -> structure_bsp, ECX -> point, DX -> portal_index) and from the already-
//   written math module (vector3d_major_axis_index, polygon2d_point_inside_tolerance,
//   k_projection_axes).
// evidence: types/tags.h ScenarioStructureBSPClusterPortal (centroid, bounding_radius, vertices)
//   and ModelCollisionGeometryBSPPlane; src/math/polygon2d_point_inside_tolerance.c and
//   src/effects/decal_place.c's existing vector3d_major_axis_index extern.
// register convention: in_EAX -> structure_bsp, in_ECX -> point (real_point3d *),
//   in_DX -> portal_index. Stack: param_1 -> tolerance.
//   // blam-cc: EAX -> structure_bsp, ECX -> point, DX -> portal_index, stack -> tolerance
// The second read of the plane's normal goes through global_collision_bsp (0x00746f90) rather
//   than structure_bsp->collision_bsp.pointer (used for the first read). The bsp switch stores
//   ScenarioStructureBSP +0xb4 into 0x00746f90 (0x53ef78), so both name the same data; the two
//   separate reads are kept as disassembled.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

// 0x00746f90 is a ModelCollisionGeometryBSP pointer, proved twice in this module: it is the ECX
// argument of bsp3d_node_find_leaf at 0x553e4a / 0x549a6a, and 0x554b9f reads its +0x10 as
// planes.pointer and indexes it with plane_index * 0x10. It is global_collision_bsp
// (types/scenario.h): ScenarioStructureBSP +0xb4, stored together with 0x00746f98 on every bsp
// switch (0x53ef68..0x53ef78), so 0x00746f98 always holds the same pointer.
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90 (types/scenario.h)
extern int16_t vector3d_major_axis_index(real_vector3d *v); // 0x44d820, math module
extern const projection_axis_pair k_projection_axes[6];     // 0x0065c29c, math module
extern uint8_t polygon2d_point_inside_tolerance(real_point2d *vertices, int16_t count,
                                                 real_point2d *point, real tolerance);
extern double sqrt(double x); // FSQRT, per src/math/quaternion_from_matrix3x3.c's convention

// blam-cc: EAX -> structure_bsp, ECX -> point, DX -> portal_index, stack -> tolerance
uint8_t structure_bsp_portal_sphere_test(ScenarioStructureBSP *structure_bsp, real_point3d *point,
                                          int16_t portal_index, float tolerance)
{
    ScenarioStructureBSPClusterPortal *portal =
        &((ScenarioStructureBSPClusterPortal *)structure_bsp->cluster_portals.pointer)[portal_index];
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)structure_bsp->collision_bsp.pointer;
    ModelCollisionGeometryBSPPlane *plane =
        &((ModelCollisionGeometryBSPPlane *)collision_bsp->planes.pointer)[portal->plane_index];

    float distance = point->x * plane->plane.vector.i + point->y * plane->plane.vector.j +
                      point->z * plane->plane.vector.k - plane->plane.w;
    if ((distance < 0.0f ? -distance : distance) >= tolerance) {
        return 0;
    }

    float dx = portal->centroid.x - point->x;
    float dy = portal->centroid.y - point->y;
    float dz = portal->centroid.z - point->z;
    float expanded_radius = tolerance + portal->bounding_radius;
    if (dy * dy + dx * dx + dz * dz >= expanded_radius * expanded_radius) {
        return 0;
    }

    // This second normal read goes through global_collision_bsp rather than
    // structure_bsp->collision_bsp.pointer used above (same pointer) -- see the file header.
    Vector3D *normal_raw = &((ModelCollisionGeometryBSPPlane *)global_collision_bsp->planes
                                  .pointer)[portal->plane_index]
                                 .plane.vector;
    real_vector3d *normal = (real_vector3d *)normal_raw;
    int16_t axis = vector3d_major_axis_index(normal);
    int32_t table_index = ((0.0f < ((float *)normal)[axis]) ? 1 : 0) + axis * 2;
    int16_t axis_i = k_projection_axes[table_index].i;
    int16_t axis_j = k_projection_axes[table_index].j;

    real_point3d projected;
    float neg_distance = -distance;
    projected.x = neg_distance * plane->plane.vector.i + point->x;
    projected.y = neg_distance * plane->plane.vector.j + point->y;
    projected.z = neg_distance * plane->plane.vector.k + point->z;
    real_point2d point_2d;
    point_2d.x = ((float *)&projected)[axis_i];
    point_2d.y = ((float *)&projected)[axis_j];

    real_point2d polygon_2d[0x100];
    ScenarioStructureBSPClusterPortalVertex *vertices =
        (ScenarioStructureBSPClusterPortalVertex *)portal->vertices.pointer;
    for (int32_t i = 0; i < (int32_t)portal->vertices.count; i++) {
        polygon_2d[i].x = ((float *)&vertices[i])[axis_i];
        polygon_2d[i].y = ((float *)&vertices[i])[axis_j];
    }

    // 0x554c51..0x554c72: fld tol; fmul tol; fld [esp+0x10] (the signed plane distance); fmul [esp+0x14] --
    // with four registers pushed [esp+0x14] is projected.x (stored at 0x554bf5), not the distance again. The
    // original really multiplies the distance by the projected point's x; kept for fidelity.
    float remaining = tolerance * tolerance - distance * projected.x;
    float radius_2d = (real)sqrt((double)remaining);
    return polygon2d_point_inside_tolerance(polygon_2d, (int16_t)portal->vertices.count, &point_2d,
                                             radius_2d);
}

#if 0
Original Ghidra decompilation (0x554b00):

undefined4 FUN_00554b00(float param_1)

{
  int iVar1;
  short sVar2;
  undefined4 uVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  int iVar9;
  char cVar10;
  short sVar11;
  int in_EAX;
  float *pfVar12;
  float *in_ECX;
  int iVar13;
  short in_DX;
  int iVar14;
  short sVar15;
  float local_414 [4];
  float local_404;
  undefined4 local_400 [256];

  iVar14 = in_DX * 0x40 + *(int *)(in_EAX + 0x158);
  iVar13 = *(int *)(iVar14 + 4) * 0x10;
  pfVar12 = (float *)(*(int *)(*(int *)(in_EAX + 0xb4) + 0x10) + iVar13);
  fVar4 = (*in_ECX * *pfVar12 + pfVar12[2] * in_ECX[2] + pfVar12[1] * in_ECX[1]) - pfVar12[3];
  if ((ABS(fVar4) < param_1) &&
     (fVar5 = *(float *)(iVar14 + 8) - *in_ECX, fVar7 = *(float *)(iVar14 + 0xc) - in_ECX[1],
     fVar8 = *(float *)(iVar14 + 0x10) - in_ECX[2], fVar6 = param_1 + *(float *)(iVar14 + 0x14),
     fVar7 * fVar7 + fVar5 * fVar5 + fVar8 * fVar8 < fVar6 * fVar6)) {
    pfVar12 = (float *)(*(int *)(DAT_00746f90 + 0x10) + iVar13);
    sVar11 = vector3d_major_axis_index();
    fVar5 = -fVar4;
    iVar13 = ((uint)(0.0 < pfVar12[sVar11]) + sVar11 * 2) * 4;
    sVar11 = *(short *)(&DAT_0065c29c + iVar13);
    sVar2 = *(short *)(&DAT_0065c29e + iVar13);
    local_414[0] = fVar5 * *pfVar12 + *in_ECX;
    local_414[1] = fVar5 * pfVar12[1] + in_ECX[1];
    sVar15 = 0;
    local_414[2] = fVar5 * pfVar12[2] + in_ECX[2];
    local_414[3] = local_414[sVar11];
    local_404 = local_414[sVar2];
    if (0 < *(int *)(iVar14 + 0x34)) {
      iVar13 = 0;
      do {
        iVar9 = iVar13 * 2;
        iVar1 = *(int *)(iVar14 + 0x38) + iVar13 * 0xc;
        uVar3 = *(undefined4 *)(sVar11 * 4 + iVar1);
        sVar15 = sVar15 + 1;
        local_400[iVar13 * 2 + 1] = *(undefined4 *)(sVar2 * 4 + iVar1);
        iVar13 = (int)sVar15;
        local_400[iVar9] = uVar3;
      } while (iVar13 < *(int *)(iVar14 + 0x34));
    }
    cVar10 = polygon2d_point_inside_tolerance
                       (*(undefined2 *)(iVar14 + 0x34),local_414 + 3,
                        SQRT(param_1 * param_1 - fVar4 * fVar4));
    if (cVar10 != '\0') {
      return 1;
    }
  }
  return 0;
}
#endif
