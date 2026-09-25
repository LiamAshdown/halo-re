// bsp_material_sample_base_map_color  (phase-2 name bsp_lightmap_sample_vertex_incident, renamed by
//   orphan pass 4: its one caller outside this directory, object_sample_ambient_lightmap_point
//   0x4f1e60, hands it the environment shader's BASE MAP bitmap (ShaderEnvironment.base_map,
//   shader + 0x94, page = shader_permutation % bitmap count), never a lightmap, and uses the
//   result as a color; see the UNSURE below, which this resolves)
// address 0x4f0900, size 284 bytes
// name confidence: 0.4 (kept the inherited name since the shape of the code -- barycentric
//   blend of a per-vertex 2D value across a triangle, then a bitmap sample -- matches a
//   "sample X at this vertex" family, but the actual fields read here are each rendered
//   vertex's *regular* texture_coords, not anything lightmap- or incident-direction-specific;
//   see UNSURE)
// rewrite confidence: 0.55 (control flow and the ScenarioStructureBSPMaterial layout are
//   confirmed against objdump and types/tags.h, same as the sibling function in this file)
// evidence: types/tags.h ScenarioStructureBSPMaterial (rendered_vertices_type 0xb0,
//   uncompressed_vertices.pointer 0xe4, compressed_vertices.pointer 0xf8),
//   ScenarioStructureBSPMaterialCompressedRenderedVertex (size 0x20: texture_coords Point2D at
//   0x18), ScenarioStructureBSPMaterialUncompressedRenderedVertex (size 0x38: texture_coords
//   Point2D at 0x30). Unlike bsp_lightmap_sample_vertex_color 0x4f0730 (this pass, same
//   directory), this function indexes straight into the rendered-vertex section of the shared
//   vertex buffer with no rendered_vertices_count skip, i.e. it reads the material's ordinary
//   render texture coordinates, not its lightmap texture coordinates. This pass:
//   out/phase4/objects_types_notes.md, "Not objects-module code": "0x4f0730 / 0x4f0900 /
//   0x4f1ef0 / 0x4f2550 sample BSP lightmaps and belong to the BSP or rendering module."
// register convention (confirmed via objdump): EAX = ScenarioStructureBSPMaterial *material,
//   ECX = uint16_t *triangle_vertex_indices; stack args in order: BitmapData *bitmap,
//   float weight_1, float weight_2, ColorRGB *out. Same cdecl(bitmap, uv, mip_bias) call to
//   0x524590 and the same color_rgb_int_to_real(out, packed) tail as
//   bsp_lightmap_sample_vertex_color.c -- see that file's header for the discrepancy with
//   src/rasterizer/rasterizer_bitmap_sample_texel.c's declared register convention.
// UNSURE: what this is actually used for -- reading the base render UV rather than the lightmap
//   UV and sampling a bitmap with mip bias 0.3 (0x3e99999a) at that point -- is not established;
//   name and purpose are inherited, not independently confirmed by this pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "structures.h"

extern int32_t rasterizer_bitmap_sample_texel(BitmapData *bitmap, float *uv, float mip_bias); // 0x524590, UNSURE cdecl signature, see bsp_lightmap_sample_vertex_color.c
extern void color_rgb_int_to_real(ColorRGB *out, uint32_t packed); // 0x43f630, EAX out, ECX packed (src/bitmaps)

// blam-cc: EAX -> material, ECX -> triangle_vertex_indices, stack -> bitmap, weight_1, weight_2, out
void bsp_material_sample_base_map_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out,
                                          ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices)
{
    float u0, u1, u2, v0, v1, v2;
    float uv[2];
    int32_t packed;

    if (material->rendered_vertices_type == vertextype_structure_bsp_compressed_rendered_vertices) {
        ScenarioStructureBSPMaterialCompressedRenderedVertex *base =
            (ScenarioStructureBSPMaterialCompressedRenderedVertex *)material->compressed_vertices.pointer;

        u0 = base[triangle_vertex_indices[0]].texture_coords.x;
        v0 = base[triangle_vertex_indices[0]].texture_coords.y;
        u1 = base[triangle_vertex_indices[1]].texture_coords.x;
        v1 = base[triangle_vertex_indices[1]].texture_coords.y;
        u2 = base[triangle_vertex_indices[2]].texture_coords.x;
        v2 = base[triangle_vertex_indices[2]].texture_coords.y;
    } else if (material->rendered_vertices_type == vertextype_structure_bsp_uncompressed_rendered_vertices ||
               material->rendered_vertices_type == 0xc) {
        ScenarioStructureBSPMaterialUncompressedRenderedVertex *base =
            (ScenarioStructureBSPMaterialUncompressedRenderedVertex *)material->uncompressed_vertices.pointer;

        u0 = base[triangle_vertex_indices[0]].texture_coords.x;
        v0 = base[triangle_vertex_indices[0]].texture_coords.y;
        u1 = base[triangle_vertex_indices[1]].texture_coords.x;
        v1 = base[triangle_vertex_indices[1]].texture_coords.y;
        u2 = base[triangle_vertex_indices[2]].texture_coords.x;
        v2 = base[triangle_vertex_indices[2]].texture_coords.y;
    } else {
        // UNSURE: no case matches; the original leaves the locals uninitialized here too.
        u0 = v0 = u1 = v1 = u2 = v2 = 0.0f;
    }

    uv[0] = (u2 - u0) * weight_2 + (u1 - u0) * weight_1 + u0;
    uv[1] = (v2 - v0) * weight_2 + (v1 - v0) * weight_1 + v0;

    packed = rasterizer_bitmap_sample_texel(bitmap, uv, 0.3f);
    color_rgb_int_to_real(out, (uint32_t)packed);
}

#if 0
Original Ghidra decompilation (0x4f0900):

void bsp_lightmap_sample_vertex_incident(undefined4 param_1,float param_2,float param_3)

{
  short sVar1;
  int iVar2;
  int in_EAX;
  int iVar3;
  ushort *in_ECX;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  sVar1 = *(short *)(in_EAX + 0xb0);
  if (sVar1 == 1) {
    iVar2 = *(int *)(in_EAX + 0xf8);
    local_18 = *(float *)((uint)*in_ECX * 0x20 + 0x18 + iVar2);
    local_14 = *(float *)((uint)*in_ECX * 0x20 + iVar2 + 0x1c);
    local_10 = *(float *)((uint)in_ECX[1] * 0x20 + 0x18 + iVar2);
    local_c = *(float *)((uint)in_ECX[1] * 0x20 + 0x1c + iVar2);
    local_8 = *(float *)((uint)in_ECX[2] * 0x20 + 0x18 + iVar2);
    local_4 = *(float *)((uint)in_ECX[2] * 0x20 + 0x1c + iVar2);
  }
  else if ((sVar1 == 0) || (sVar1 == 0xc)) {
    iVar2 = *(int *)(in_EAX + 0xe4);
    local_18 = *(float *)((uint)*in_ECX * 0x38 + 0x30 + iVar2);
    local_14 = *(float *)((uint)*in_ECX * 0x38 + iVar2 + 0x34);
    iVar3 = (uint)in_ECX[1] * 0x38 + iVar2;
    local_10 = *(float *)(iVar3 + 0x30);
    local_c = *(float *)(iVar3 + 0x34);
    local_8 = *(float *)((uint)in_ECX[2] * 0x38 + 0x30 + iVar2);
    local_4 = *(float *)((uint)in_ECX[2] * 0x38 + iVar2 + 0x34);
  }
  local_20 = (local_8 - local_18) * param_3 + (local_10 - local_18) * param_2 + local_18;
  local_1c = (local_4 - local_14) * param_3 + (local_c - local_14) * param_2 + local_14;
  rasterizer_bitmap_sample_texel(param_1,&local_20,0x3e99999a);
  color_rgb_int_to_real();
  return;
}
#endif
