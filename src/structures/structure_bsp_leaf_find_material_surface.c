// structure_bsp_leaf_find_material_surface  (Ghidra: FUN_00554fa0, still unnamed there)
// address 0x554fa0, size 485 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: full disassembly (objdump -d -M intel bin/halo.exe, 0x554fa0..0x55518e) rather than
//   the decompile, which elided both the callee argument lists this function depends on:
//     - 0x552110 is structure_surface_material_locate (this module). The call site pushes
//       (global_structure_bsp, surface_index, out_material_index) and loads EAX with
//       out_lightmap_index -- `mov eax,[esp+0x50]` at 0x555024 -- which matches that function's
//       own EAX convention exactly. So this function's third stack parameter is the LIGHTMAP
//       index slot and its fourth is the MATERIAL index slot, not the "outer cell / inner cell"
//       pair the phase4 one-line summary of 0x552110 suggested; the very next instructions
//       confirm it (`lightmaps[*arg3].materials.pointer + *arg4 * 0x100`, strides 0x20 and 0x100).
//     - 0x4ce8c0 takes three real_point3d in EAX/EDX/ECX (`lea eax,[esp+0x20]` /
//       `lea edx,[esp+0x2c]` / `lea ecx,[esp+0x38]`, which are three 12-byte locals), the query
//       point in ESI (this function's first stack parameter), and two further stack arguments.
//       An earlier rewrite of this file dropped the whole triangle-vertex gather that fills those
//       three locals, and with it the fact that the vertex source depends on the material's
//       rendered_vertices_type: type 1 reads compressed_vertices.pointer with stride 0x20, types
//       0 and 0xc read uncompressed_vertices.pointer with stride 0x38. Both are restored here.
//   Also confirmed by disassembly: `and eax,0x7fffffff` then `shl eax,4` on the incoming leaf
//   reference (0x554fa4), and the per-surface plane filter
//   `collision_bsp->bsp3d_nodes[reference.node].plane == accepted_plane` (stride 0xc, plane at +0).
// register convention: in_EAX -> raw_child (a leaf reference with bit 31 still set).
//   7 stack parameters otherwise.
//   // blam-cc: EAX -> raw_child, stack -> the rest
// UNSURE: 0x4ce8c0's own contract (math module, not rewritten). It is called with three triangle
//   vertices, the query point and two opaque forwarded values, and its boolean result gates the
//   match, which is what makes "point inside this triangle" the reading; its last two arguments
//   (this function's param_6 / param_7, themselves forwarded straight from
//   structure_bsp_resolve_position_to_surface's own caller) are not resolved here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

// blam-cc: EAX -> out_lightmap_index
extern void structure_surface_material_locate(ScenarioStructureBSP *global_structure_bsp, int32_t surface_index,
    int16_t *out_material_index, int16_t *out_lightmap_index); // 0x552110, this module

// math module, out of this batch. blam-cc: EAX -> v0, EDX -> v1, ECX -> v2, ESI -> point
extern uint8_t triangle_point_barycentric_2d(real_point3d *a, real_point3d *v_ecx, real_point3d *v_edx, real_point3d *p,
    real *out_u, real *out_v); // 0x4ce8c0, src/math: EAX a, ECX v_ecx, EDX v_edx, ESI p

// Walks one BSP leaf's surface references looking for a surface that lies on `accepted_plane`
// (the collision plane the caller's segment test reported) and whose triangle actually contains
// `point`. On a match the surface index is written to *out_surface and the lightmap / material
// indices that own it are left in *out_lightmap_index / *out_material_index by
// structure_surface_material_locate.
// blam-cc: EAX -> raw_child, stack -> the rest
uint8_t structure_bsp_leaf_find_material_surface(real_point3d *point, int32_t accepted_plane,
                                                  int16_t *out_lightmap_index,
                                                  int16_t *out_material_index,
                                                  int32_t *out_surface, void *param_6,
                                                  void *param_7, int32_t raw_child)
{
    int32_t leaf_index = raw_child & 0x7fffffff;
    ScenarioStructureBSPLeaf *leaf =
        &((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf_index];
    ScenarioStructureBSPSurfaceReference *leaf_surfaces =
        (ScenarioStructureBSPSurfaceReference *)global_structure_bsp->leaf_surfaces.pointer;
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    ModelCollisionGeometryBSP3DNode *bsp3d_nodes =
        (ModelCollisionGeometryBSP3DNode *)collision_bsp->bsp3d_nodes.pointer;
    ScenarioStructureBSPLightmap *lightmaps =
        (ScenarioStructureBSPLightmap *)global_structure_bsp->lightmaps.pointer;

    int32_t first = leaf->surface_references;
    int32_t end = first + leaf->surface_reference_count;
    int32_t i;

    for (i = first; i < end; i++) {
        int32_t node_value = leaf_surfaces[i].node;
        int32_t surface_index;
        ScenarioStructureBSPMaterial *material;
        ScenarioStructureBSPSurface *surface;
        real_point3d triangle[3];
        int32_t corner;

        if (node_value == -1 || (int32_t)bsp3d_nodes[node_value].plane != accepted_plane) {
            continue;
        }
        surface_index = leaf_surfaces[i].surface;
        surface = (ScenarioStructureBSPSurface *)global_structure_bsp->surfaces.pointer + surface_index;

        structure_surface_material_locate(global_structure_bsp, surface_index, out_material_index,
                                          out_lightmap_index);
        material = &((ScenarioStructureBSPMaterial *)
            lightmaps[*out_lightmap_index].materials.pointer)[*out_material_index];

        if (material->rendered_vertices_type == 1) {
            // compressed rendered vertices: 0x20 bytes each, position first
            uint8_t *vertices = (uint8_t *)material->compressed_vertices.pointer;
            for (corner = 0; corner < 3; corner = corner + 1) {
                float *v = (float *)(vertices + (&surface->vertex0_index)[corner] * 0x20);
                triangle[corner].x = v[0];
                triangle[corner].y = v[1];
                triangle[corner].z = v[2];
            }
        } else if (material->rendered_vertices_type == 0 ||
                   material->rendered_vertices_type == 0xc) {
            // uncompressed rendered vertices: 0x38 bytes each, position first
            uint8_t *vertices = (uint8_t *)material->uncompressed_vertices.pointer;
            for (corner = 0; corner < 3; corner = corner + 1) {
                float *v = (float *)(vertices + (&surface->vertex0_index)[corner] * 0x38);
                triangle[corner].x = v[0];
                triangle[corner].y = v[1];
                triangle[corner].z = v[2];
            }
        } else {
            continue;   // any other vertex type is skipped without testing the triangle
        }

        // 0x555141..0x55514d: EAX = &triangle[0], EDX = &triangle[1], ECX = &triangle[2] -- the definition's second
        // parameter is the ECX one, so triangle[2] goes second
        if (triangle_point_barycentric_2d(&triangle[0], &triangle[2], &triangle[1], point,
                                          (real *)param_6, (real *)param_7)) {
            *out_surface = surface_index;
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x554fa0):

uint FUN_00554fa0(undefined4 param_1,int param_2,short *param_3,short *param_4,undefined4 *param_5,
                 undefined4 param_6,undefined4 param_7)

{
  undefined4 *puVar1;
  short sVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  char cVar6;
  int in_EAX;
  uint uVar7;
  int iVar8;

  iVar5 = DAT_00746f9c;
  uVar7 = in_EAX * 0x10 + *(int *)(DAT_00746f9c + 0xe4);
  iVar8 = *(int *)(uVar7 + 0xc);
  if (*(short *)(uVar7 + 10) + iVar8 <= iVar8) {
    return uVar7 & 0xffffff00;
  }
  do {
    iVar3 = *(int *)(*(int *)(iVar5 + 0xf0) + 4 + iVar8 * 8);
    puVar1 = (undefined4 *)(*(int *)(iVar5 + 0xf0) + iVar8 * 8);
    if ((iVar3 != -1) && (*(int *)(*(int *)(*(int *)(iVar5 + 0xb4) + 4) + iVar3 * 0xc) == param_2))
    {
      FUN_00552110(iVar5,*puVar1,param_4);
      sVar2 = *(short *)(*param_4 * 0x100 + 0xb0 +
                        *(int *)(*param_3 * 0x20 + 0x18 + *(int *)(iVar5 + 0x108)));
      if (((sVar2 == 1) || ((sVar2 == 0 || (sVar2 == 0xc)))) &&
         (cVar6 = triangle_point_barycentric_2d(param_6,param_7), cVar6 != '\0')) {
        uVar4 = *puVar1;
        *param_5 = uVar4;
        return CONCAT31((int3)((uint)uVar4 >> 8),1);
      }
    }
    iVar8 = iVar8 + 1;
    if ((int)*(short *)(uVar7 + 10) + *(int *)(uVar7 + 0xc) <= iVar8) {
      return uVar7 & 0xffffff00;
    }
  } while( true );
}
#endif
