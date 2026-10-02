// bsp_lightmap_sample_vertex_color  (orphan pass 4: FUN_004f0730, no Ghidra name)
// address 0x4f0730, size 459 bytes
// name confidence: 0.6 (matches its own body exactly: reads a triangle's 3 lightmap texture
//   coordinates out of a ScenarioStructureBSPMaterial's vertex buffer, blends two of them
//   toward the third with barycentric weights, and samples the lightmap bitmap there)
// rewrite confidence: 0.55 (control flow and the ScenarioStructureBSPMaterial field layout are
//   confirmed against objdump and types/tags.h; the register-args-vs-stack-args split for the
//   two callees at the tail is confirmed by tracing the actual push sequence, which disagrees
//   with the pre-existing extern signature for rasterizer_bitmap_sample_texel -- see UNSURE)
// evidence: types/tags.h ScenarioStructureBSPMaterial (rendered_vertices_type 0xb0,
//   rendered_vertices_count 0xb4, uncompressed_vertices 0xd8, compressed_vertices 0xec, each a
//   TagDataOffset whose .pointer field sits at +0xc, landing on 0xe4/0xf8 exactly where this
//   function reads them), ScenarioStructureBSPMaterialUncompressedLightmapVertex (size 0x14:
//   normal 0x00, texture_coords 0x0c), ScenarioStructureBSPMaterialCompressedLightmapVertex
//   (size 0x08: normal 0x00, texture_coordinate_x 0x04, texture_coordinate_y 0x06),
//   ScenarioStructureBSPMaterialUncompressedRenderedVertex (size 0x38),
//   ScenarioStructureBSPMaterialCompressedRenderedVertex (size 0x20). The uncompressed and
//   compressed vertex buffers each hold the material's rendered_vertices_count RENDERED-format
//   vertices immediately followed by its lightmap-format vertices: rendered_vertices_count*0x38
//   (uncompressed) or rendered_vertices_count*4 (compressed -- 0x20/0x08 = 4 lightmap-sized
//   units per rendered vertex) is exactly the byte/unit offset that skips the rendered section
//   to reach the lightmap section, which is what this function computes before indexing by the
//   triangle-local lightmap vertex index. This pass: out/phase4/objects_types_notes.md, "Not
//   objects-module code": "0x4f0730 / 0x4f0900 / 0x4f1ef0 / 0x4f2550 sample BSP lightmaps and
//   belong to the BSP or rendering module; they touch structure_bsp lightmap blocks, not
//   object records, so no types for them are defined here." Moved to src/structures, alongside
//   the other structure_bsp_* functions that already work with ScenarioStructureBSPMaterial.
//   Caller (object_sample_total_lighting_at_point 0x4f1c20) resolves ECX to
//   &scenario_structure_bsp->lightmaps[cluster_lightmap_index].materials.pointer[material_index]
//   (0x20-byte ScenarioStructureBSPLightmap stride, 0x100-byte ScenarioStructureBSPMaterial
//   stride) and EDX to &surface_vertex_indices[triangle_index*3], confirming both register
//   arguments.
// register convention (confirmed via objdump): ECX = ScenarioStructureBSPMaterial *material,
//   EDX = uint16_t *triangle_vertex_indices (3 local vertex indices), stack args in order:
//   BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out.
// UNSURE: the call to rasterizer_bitmap_sample_texel here pushes bitmap, &uv and the mip-bias
//   constant as three ordinary cdecl stack arguments (confirmed by objdump: the callee at
//   0x524590 reads its first parameter from [esp+4] into EBP, not from ECX), which disagrees
//   with src/rasterizer/rasterizer_bitmap_sample_texel.c's own header (that file is itself only
//   0.25 rewrite confidence and says its register-vs-stack split was "inferred from context,
//   not verified against the raw disassembly"). This file declares its own extern matching what
//   is actually observed at this call site; the other file's signature looks stale/wrong and is
//   left alone as out of scope for this pass.
// UNSURE: the format-type field checked (material->rendered_vertices_type) governs whether the
//   shared vertex buffer is compressed for BOTH the rendered and lightmap sections (they share
//   one buffer), so checking rendered_vertices_type instead of the parallel
//   lightmap_vertices_type is consistent, not a bug. The literal 0xc value accepted alongside 0
//   for the uncompressed path does not correspond to any value in types/tags.h's VertexType
//   enum; preserved as a literal.
// UNSURE: the 1.5259022e-05 constant applied to the compressed path's int16 texture coordinates
//   is preserved as a raw literal (it is 1/65535, i.e. `(2*v + 1) / 65535`, a symmetric
//   normalization of a signed 16-bit quantity to a float), matching how the original computed it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "structures.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// UNSURE: signature reconstructed from this call site; see file header.
extern int32_t rasterizer_bitmap_sample_texel(BitmapData *bitmap, float *uv, float mip_bias); // 0x524590 (0x4f08e0 calls it)
extern void color_rgb_int_to_real(ColorRGB *out, uint32_t packed); // 0x43f630, EAX out, ECX packed (src/bitmaps)

// blam-cc: ECX -> material, EDX -> triangle_vertex_indices, stack -> bitmap, weight_1, weight_2, out
void bsp_lightmap_sample_vertex_color(BitmapData *bitmap, float weight_1, float weight_2, ColorRGB *out,
                                       ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices)
{
    float u0, u1, u2, v0, v1, v2;
    float uv[2];
    int32_t packed;

    if (material->rendered_vertices_type == vertextype_structure_bsp_compressed_rendered_vertices) {
        uint8_t *base = (uint8_t *)material->compressed_vertices.pointer;
        int32_t skip = material->rendered_vertices_count * 4;
        ScenarioStructureBSPMaterialCompressedLightmapVertex *e0 =
            (ScenarioStructureBSPMaterialCompressedLightmapVertex *)base + triangle_vertex_indices[0] + skip;
        ScenarioStructureBSPMaterialCompressedLightmapVertex *e1 =
            (ScenarioStructureBSPMaterialCompressedLightmapVertex *)base + triangle_vertex_indices[1] + skip;
        ScenarioStructureBSPMaterialCompressedLightmapVertex *e2 =
            (ScenarioStructureBSPMaterialCompressedLightmapVertex *)base + triangle_vertex_indices[2] + skip;

        u0 = ((float)(int32_t)e0->texture_coordinate_x * 2.0f + 1.0f) * 1.5259022e-05f;
        v0 = ((float)(int32_t)e0->texture_coordinate_y * 2.0f + 1.0f) * 1.5259022e-05f;
        u1 = ((float)(int32_t)e1->texture_coordinate_x * 2.0f + 1.0f) * 1.5259022e-05f;
        v1 = ((float)(int32_t)e1->texture_coordinate_y * 2.0f + 1.0f) * 1.5259022e-05f;
        u2 = ((float)(int32_t)e2->texture_coordinate_x * 2.0f + 1.0f) * 1.5259022e-05f;
        v2 = ((float)(int32_t)e2->texture_coordinate_y * 2.0f + 1.0f) * 1.5259022e-05f;
    } else if (material->rendered_vertices_type == vertextype_structure_bsp_uncompressed_rendered_vertices ||
               material->rendered_vertices_type == 0xc) {
        uint8_t *base = (uint8_t *)material->uncompressed_vertices.pointer + material->rendered_vertices_count * 0x38;
        ScenarioStructureBSPMaterialUncompressedLightmapVertex *e0 =
            (ScenarioStructureBSPMaterialUncompressedLightmapVertex *)(base + triangle_vertex_indices[0] * 0x14);
        ScenarioStructureBSPMaterialUncompressedLightmapVertex *e1 =
            (ScenarioStructureBSPMaterialUncompressedLightmapVertex *)(base + triangle_vertex_indices[1] * 0x14);
        ScenarioStructureBSPMaterialUncompressedLightmapVertex *e2 =
            (ScenarioStructureBSPMaterialUncompressedLightmapVertex *)(base + triangle_vertex_indices[2] * 0x14);

        u0 = e0->texture_coords.x; v0 = e0->texture_coords.y;
        u1 = e1->texture_coords.x; v1 = e1->texture_coords.y;
        u2 = e2->texture_coords.x; v2 = e2->texture_coords.y;
    } else {
        // UNSURE: no case matches; the original leaves the locals uninitialized here too.
        u0 = v0 = u1 = v1 = u2 = v2 = 0.0f;
    }

    uv[0] = (u1 - u0) * weight_1 + (u2 - u0) * weight_2 + u0;
    uv[1] = (v1 - v0) * weight_1 + (v2 - v0) * weight_2 + v0;

    packed = rasterizer_bitmap_sample_texel(bitmap, uv, 1.0f);
    color_rgb_int_to_real(out, (uint32_t)packed);
}

#if 0
Original Ghidra decompilation (0x4f0730):

void bsp_lightmap_sample_vertex_color(undefined4 param_1,float param_2,float param_3)

{
  short sVar1;
  int iVar2;
  float fVar3;
  int iVar4;
  int in_ECX;
  int iVar5;
  ushort *in_EDX;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  sVar1 = *(short *)(in_ECX + 0xb0);
  if (sVar1 == 1) {
    iVar2 = *(int *)(in_ECX + 0xf8);
    iVar4 = *(int *)(in_ECX + 0xb4) * 4;
    fVar3 = (float)(int)*(short *)(iVar2 + 4 + ((uint)*in_EDX + iVar4) * 8);
    local_18 = (fVar3 + fVar3 + 1.0) * 1.5259022e-05;
    iVar5 = iVar2 + ((uint)in_EDX[2] + iVar4) * 8;
    fVar3 = (float)(int)*(short *)(iVar2 + ((uint)*in_EDX + iVar4) * 8 + 6);
    local_14 = (fVar3 + fVar3 + 1.0) * 1.5259022e-05;
    fVar3 = (float)(int)*(short *)(iVar2 + 4 + ((uint)in_EDX[1] + iVar4) * 8);
    local_10 = (fVar3 + fVar3 + 1.0) * 1.5259022e-05;
    fVar3 = (float)(int)*(short *)(iVar2 + ((uint)in_EDX[1] + iVar4) * 8 + 6);
    local_c = (fVar3 + fVar3 + 1.0) * 1.5259022e-05;
    fVar3 = (float)(int)*(short *)(iVar5 + 4);
    local_8 = (fVar3 + fVar3 + 1.0) * 1.5259022e-05;
    fVar3 = (float)(int)*(short *)(iVar5 + 6);
    local_4 = (fVar3 + fVar3 + 1.0) * 1.5259022e-05;
  }
  else if ((sVar1 == 0) || (sVar1 == 0xc)) {
    iVar2 = *(int *)(in_ECX + 0xe4);
    iVar4 = *(int *)(in_ECX + 0xb4) * 0x38;
    iVar5 = iVar2 + (uint)*in_EDX * 0x14;
    local_18 = *(float *)(iVar5 + 0xc + iVar4);
    local_14 = *(float *)(iVar5 + iVar4 + 0x10);
    iVar5 = iVar2 + (uint)in_EDX[1] * 0x14 + iVar4;
    local_10 = *(float *)(iVar5 + 0xc);
    local_c = *(float *)(iVar5 + 0x10);
    iVar4 = iVar2 + (uint)in_EDX[2] * 0x14 + iVar4;
    local_8 = *(float *)(iVar4 + 0xc);
    local_4 = *(float *)(iVar4 + 0x10);
  }
  local_20 = (local_10 - local_18) * param_2 + (local_8 - local_18) * param_3 + local_18;
  local_1c = (local_c - local_14) * param_2 + (local_4 - local_14) * param_3 + local_14;
  rasterizer_bitmap_sample_texel(param_1,&local_20,0x3f800000);
  color_rgb_int_to_real();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
