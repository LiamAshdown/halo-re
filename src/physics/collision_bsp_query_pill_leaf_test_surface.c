// collision_bsp_query_pill_leaf_test_surface  (Ghidra: FUN_00502e70, still unnamed there; name
// from out/phase2/results/physics_00.json)
// address 0x502e70, size 474 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: same ModelCollisionGeometryBSP surfaces/edges/vertices layout as
//   collision_bsp_surface_get_vertices; collision_bsp_pill_query/…_result field offsets
//   (origin 0x004, delta 0x008, radius 0x00c, result 0x010) confirmed in
//   out/phase4/physics_types_notes.md. Walks a surface's edge loop testing the swept sphere
//   against each edge via physics_shape_pill_sweep_test_point, keeping the closest valid
//   contact.
// register convention: none -- both arguments are Ghidra-recognized stack parameters.
//   // blam-cc: stack -> query, surface_index
// UNSURE: the call to physics_shape_pill_sweep_test_point shows only the radius argument in
// Ghidra's decompile; near_vertex/edge_dir are reconstructed from the edge just walked,
// origin/delta from the query, matching that function's own established signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction
extern double fabs(double x);
extern uint8_t physics_shape_pill_sweep_test_point(real_point3d *near_vertex, real_vector3d *delta,
                                                    real_point3d *origin, real_vector3d *edge_dir,
                                                    float radius, float *out_t,
                                                    float *out_edge_fraction); // 0x503050, this batch

// blam-cc: stack -> query, surface_index
uint8_t collision_bsp_query_pill_leaf_test_surface(collision_bsp_pill_query *query,
                                                    int32_t surface_index)
{
    ModelCollisionGeometryBSP *bsp = (ModelCollisionGeometryBSP *)query->bsp;
    ModelCollisionGeometryBSPSurface *surfaces =
        (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
    ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
    float *vertex_floats = (float *)bsp->vertices.pointer;
    ModelCollisionGeometryBSPSurface *surface = &surfaces[surface_index];
    collision_bsp_pill_result *result = (collision_bsp_pill_result *)query->result;
    real_point3d *origin = (real_point3d *)query->origin;
    real_vector3d *delta = (real_vector3d *)query->delta;
    int32_t start_edge = (int32_t)surface->first_edge;
    int32_t edge_index = start_edge;
    uint8_t any_hit = 0;

    do {
        ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
        int owns_right_side = ((int32_t)edge->right_surface == surface_index);
        uint32_t near_vertex_index = owns_right_side ? edge->end_vertex : edge->start_vertex;
        uint32_t far_vertex_index = owns_right_side ? edge->start_vertex : edge->end_vertex;
        real_point3d near_vertex;
        real_vector3d edge_dir;
        float t, edge_fraction;

        near_vertex.x = vertex_floats[near_vertex_index * 4 + 0];
        near_vertex.y = vertex_floats[near_vertex_index * 4 + 1];
        near_vertex.z = vertex_floats[near_vertex_index * 4 + 2];
        edge_dir.i = vertex_floats[far_vertex_index * 4 + 0] - near_vertex.x;
        edge_dir.j = vertex_floats[far_vertex_index * 4 + 1] - near_vertex.y;
        edge_dir.k = vertex_floats[far_vertex_index * 4 + 2] - near_vertex.z;

        if (physics_shape_pill_sweep_test_point(&near_vertex, delta, origin, &edge_dir,
                                                 query->radius, &t, &edge_fraction) &&
            (t < result->t)) {
            real_vector3d contact_dir;
            float length;

            result->t = t;
            contact_dir.i = (t * delta->i + origin->x) - (edge_dir.i * edge_fraction + near_vertex.x);
            contact_dir.j = (t * delta->j + origin->y) - (edge_dir.j * edge_fraction + near_vertex.y);
            contact_dir.k = (t * delta->k + origin->z) - (edge_dir.k * edge_fraction + near_vertex.z);
            length = (float)sqrt((double)(contact_dir.k * contact_dir.k +
                                           contact_dir.j * contact_dir.j +
                                           contact_dir.i * contact_dir.i));
            if (0.0001 <= (float)fabs((double)length)) {
                float inv_length = 1.0f / length;
                contact_dir.i *= inv_length;
                contact_dir.j *= inv_length;
                contact_dir.k *= inv_length;
            }
            result->plane_i = contact_dir.i;
            result->plane_j = contact_dir.j;
            result->plane_k = contact_dir.k;
            result->plane_d = 3.4028235e+38f; // FLT_MAX: marks this contact as an edge, not a face
            result->surface_index = surface_index;
            result->material_index = (int16_t)surface->material;
            any_hit = 1;
        }
        edge_index = (int32_t)(owns_right_side ? edge->reverse_edge : edge->forward_edge);
    } while (edge_index != start_edge);

    return any_hit;
}

#if 0
Original Ghidra decompilation (0x502e70):

undefined4 FUN_00502e70(int *param_1,int param_2)

{
  float fVar1;
  int iVar2;
  float *pfVar3;
  float *pfVar4;
  undefined1 uVar5;
  char cVar6;
  int iVar7;
  int iVar8;
  float *pfVar9;
  bool bVar10;
  float local_38;
  float local_34;
  int local_30;
  int local_2c;
  uint local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_14;
  float local_10;
  float local_4;

  local_30 = *(int *)(*param_1 + 0x40) + param_2 * 0xc;
  iVar7 = *(int *)(local_30 + 4);
  uVar5 = 0;
  do {
    iVar2 = *(int *)(*param_1 + 0x58);
    local_2c = *(int *)(*param_1 + 0x4c) + iVar7 * 0x18;
    bVar10 = *(int *)(local_2c + 0x14) == param_2;
    local_28 = (uint)bVar10;
    pfVar9 = (float *)(*(int *)(local_2c + local_28 * 4) * 0x10 + iVar2);
    iVar7 = *(int *)(local_2c + (uint)!bVar10 * 4) * 0x10;
    iVar8 = iVar7 + iVar2;
    local_24 = *(float *)(iVar7 + iVar2) - *pfVar9;
    local_20 = *(float *)(iVar8 + 4) - pfVar9[1];
    local_1c = *(float *)(iVar8 + 8) - pfVar9[2];
    cVar6 = FUN_00503050(param_1[3],&local_38,&local_34);
    if ((cVar6 != '\0') && (local_38 < *(float *)param_1[4])) {
      *(float *)param_1[4] = local_38;
      pfVar3 = (float *)param_1[2];
      pfVar4 = (float *)param_1[1];
      fVar1 = pfVar9[1];
      local_4 = local_1c * local_34 + pfVar9[2];
      local_14 = local_38 * pfVar3[1] + pfVar4[1];
      iVar7 = param_1[4];
      local_10 = local_38 * pfVar3[2] + pfVar4[2];
      *(float *)(iVar7 + 4) = (local_38 * *pfVar3 + *pfVar4) - (local_24 * local_34 + *pfVar9);
      *(float *)(iVar7 + 8) = local_14 - (local_20 * local_34 + fVar1);
      *(float *)(iVar7 + 0xc) = local_10 - local_4;
      iVar7 = param_1[4];
      pfVar9 = (float *)(iVar7 + 4);
      fVar1 = SQRT(*(float *)(iVar7 + 0xc) * *(float *)(iVar7 + 0xc) +
                   *(float *)(iVar7 + 8) * *(float *)(iVar7 + 8) + *pfVar9 * *pfVar9);
      if (0.0001 <= ABS(fVar1)) {
        fVar1 = 1.0 / fVar1;
        *pfVar9 = fVar1 * *pfVar9;
        *(float *)(iVar7 + 8) = fVar1 * *(float *)(iVar7 + 8);
        *(float *)(iVar7 + 0xc) = fVar1 * *(float *)(iVar7 + 0xc);
      }
      *(undefined4 *)(param_1[4] + 0x10) = 0x7f7fffff;
      *(int *)(param_1[4] + 0x14) = param_2;
      *(undefined2 *)(param_1[4] + 0x1a) = *(undefined2 *)(local_30 + 10);
      uVar5 = 1;
    }
    iVar7 = *(int *)(local_2c + 8 + local_28 * 4);
  } while (iVar7 != *(int *)(local_30 + 4));
  return CONCAT31((int3)((uint)iVar7 >> 8),uVar5);
}
#endif
