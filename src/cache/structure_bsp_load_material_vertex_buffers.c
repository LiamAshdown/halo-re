// structure_bsp_load_material_vertex_buffers  (Ghidra: FUN_00443020)
// address 0x443020, size 384 bytes
// name confidence: 0.55 (out/phase4/cache_functions.md summary, corrected by
// out/phase4/cache_types_notes.md item 2: this is the BSP counterpart of
// model_load_vertex_buffers, walking ScenarioStructureBSP::lightmaps -> materials, not
// "clusters" as the summary says -- ScenarioStructureBSPCluster is never touched)
// rewrite confidence: 0.70
// evidence: types/tags.h ScenarioStructureBSP (lightmaps reflexive), ScenarioStructureBSPLightmap
// (materials reflexive), ScenarioStructureBSPMaterial (every field offset below matches its
// definition exactly: shader 0x00, rendered_vertices_type/count 0xb0/0xb4,
// rendered_vertices_index_pointer 0xc0, lightmap_vertices_type/count 0xc4/0xc8,
// lightmap_vertices_index_pointer 0xd4, uncompressed_vertices.pointer 0xe4); the sister disposal
// function structure_bsp_dispose_material_vertex_buffers (0x4431a0) confirms the same
// ScenarioStructureBSPCompiledHeader* argument and the same two index-pointer fields.
// register convention: ScenarioStructureBSPCompiledHeader *compiled_header in EAX (in_EAX),
// confirmed by the caller (structure_bsp_load @0x4424b0, disassembly) loading structure_bsp_data
// into EAX immediately before the call and not touching it afterward.
// UNSURE: rasterizer_vertex_buffer_create's parameter meanings are reconstructed only from how
// this function and its disposal counterpart use them (a field-group pointer, a format code, a
// vertex count, a source data pointer, an extra source pointer, and a byte size); the function
// itself belongs to the rasterizer module, not cache, and was not decoded here.
// reconciled: R09 0x007c117c is rasterizer_caps.max_streams (D3DCAPS9 +0xbc), not a rasterizer_vertex_processing global; 0x007c118c likewise is rasterizer_caps.pixel_shader_version (+0xcc)

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "fn_cache.h"

extern int8_t rasterizer_vertex_buffer_create(void *fields, int32_t format, int32_t vertex_count,
    void *rendered_data, void *lightmap_data, int32_t size); // UNSURE, see file header;
    // rasterizer module, 0x524980. Returns a success flag (see model_load_vertex_buffers.c,
    // where the same function's result is checked), unused by this caller.

extern d3d_caps9 rasterizer_caps;                 // 0x007c10c0; max_streams is 0x007c117c,
                                                  // pixel_shader_version is 0x007c118c

// blam-cc: compiled_header in EAX
// Called after a structure_bsp's data block has been loaded. Walks every lightmap's materials and
// builds the rasterizer vertex buffer(s) each needs to render: senv/swat/sgla shaders get a
// combined rendered+lightmap buffer when hardware vertex processing is unavailable (or two
// buffers, one per vertex stream, when it is), while every other shader always gets the
// non-combined pair. Both index-pointer fields are cleared first so a partial failure leaves them
// null rather than dangling.
void structure_bsp_load_material_vertex_buffers(ScenarioStructureBSPCompiledHeader *compiled_header)
{
    ScenarioStructureBSP *bsp;
    ScenarioStructureBSPLightmap *lightmap;
    ScenarioStructureBSPMaterial *material;
    void *lightmap_vertex_data;
    int32_t lightmap_index;
    int32_t material_index;

    bsp = (ScenarioStructureBSP *)compiled_header->pointer;

    for (lightmap_index = 0; lightmap_index < (int32_t)bsp->lightmaps.count; lightmap_index++) {
        lightmap = (ScenarioStructureBSPLightmap *)(bsp->lightmaps.pointer +
            lightmap_index * sizeof(ScenarioStructureBSPLightmap));

        for (material_index = 0; material_index < (int32_t)lightmap->materials.count;
             material_index++) {
            material = (ScenarioStructureBSPMaterial *)(lightmap->materials.pointer +
                material_index * sizeof(ScenarioStructureBSPMaterial));

            material->lightmap_vertices_index_pointer = 0;
            material->rendered_vertices_index_pointer = 0;
            lightmap_vertex_data = (void *)(material->uncompressed_vertices.pointer +
                material->rendered_vertices_count * 0x38);

            if (rasterizer_caps.pixel_shader_version < 0xffff0101 &&
                (material->shader.tag_fourcc == _tag_group_shader_environment ||
                 material->shader.tag_fourcc == _tag_group_shader_transparent_water ||
                 material->shader.tag_fourcc == _tag_group_shader_transparent_glass)) {
                if ((int32_t)rasterizer_caps.max_streams < 2 && material->lightmap_vertices_count != 0) { // 0x4430b5 signed
                    rasterizer_vertex_buffer_create(&material->rendered_vertices_type, 0x13,
                        material->rendered_vertices_count,
                        (void *)material->uncompressed_vertices.pointer, lightmap_vertex_data,
                        (int16_t)material->lightmap_vertices_count * 0x28);
                } else {
                    rasterizer_vertex_buffer_create(&material->rendered_vertices_type, 0xc,
                        material->rendered_vertices_count,
                        (void *)material->uncompressed_vertices.pointer, 0,
                        (int16_t)material->rendered_vertices_count << 5);
                    if (material->lightmap_vertices_count != 0) {
                        rasterizer_vertex_buffer_create(&material->lightmap_vertices_type, 0xd,
                            (int16_t)material->lightmap_vertices_count, lightmap_vertex_data, 0,
                            (int16_t)material->lightmap_vertices_count * 8);
                    }
                }
            } else {
                rasterizer_vertex_buffer_create(&material->rendered_vertices_type, 0,
                    (int16_t)material->rendered_vertices_count,
                    (void *)material->uncompressed_vertices.pointer, 0,
                    (int16_t)material->rendered_vertices_count * 0x38);
                if (material->lightmap_vertices_count != 0) {
                    rasterizer_vertex_buffer_create(&material->lightmap_vertices_type, 2,
                        (int16_t)material->lightmap_vertices_count, lightmap_vertex_data, 0,
                        (int16_t)material->lightmap_vertices_count * 0x14);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x443020):

void FUN_00443020(void)

{
  int iVar1;
  short sVar2;
  short sVar3;
  int *in_EAX;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int *piVar8;
  undefined4 uVar9;
  int iVar10;
  int iVar11;

  iVar1 = *in_EAX;
  sVar3 = 0;
  if (0 < *(int *)(iVar1 + 0x104)) {
    iVar4 = 0;
    do {
      iVar4 = iVar4 * 0x20 + *(int *)(iVar1 + 0x108);
      sVar2 = 0;
      if (0 < *(int *)(iVar4 + 0x14)) {
        iVar7 = 0;
        do {
          iVar6 = *(int *)(iVar4 + 0x18);
          iVar7 = iVar7 * 0x100;
          iVar5 = *(int *)(iVar7 + 0xb4 + iVar6);
          iVar10 = *(int *)(iVar7 + 0xe4 + iVar6);
          piVar8 = (int *)(iVar7 + iVar6);
          piVar8[0x35] = 0;
          piVar8[0x30] = 0;
          iVar7 = iVar5 * 0x38 + iVar10;
          if ((DAT_007c118c < 0xffff0101) &&
             (((iVar6 = *piVar8, iVar6 == 0x73656e76 || (iVar6 == 0x73776174)) ||
              (iVar6 == 0x73676c61)))) {
            if ((DAT_007c117c < 2) && (piVar8[0x32] != 0)) {
              iVar6 = (short)piVar8[0x32] * 0x28;
              uVar9 = 0x13;
              piVar8 = piVar8 + 0x2c;
              iVar11 = iVar7;
LAB_0044315c:
              rasterizer_vertex_buffer_create(piVar8,uVar9,iVar5,iVar10,iVar11,iVar6);
            }
            else {
              rasterizer_vertex_buffer_create
                        (piVar8 + 0x2c,0xc,iVar5,iVar10,0,(int)(short)piVar8[0x2d] << 5);
              if (piVar8[0x32] != 0) {
                iVar5 = (int)(short)piVar8[0x32];
                iVar6 = iVar5 * 8;
                uVar9 = 0xd;
                goto LAB_00443156;
              }
            }
          }
          else {
            rasterizer_vertex_buffer_create
                      (piVar8 + 0x2c,0,(int)(short)piVar8[0x2d],iVar10,0,(short)piVar8[0x2d] * 0x38)
            ;
            if (piVar8[0x32] != 0) {
              iVar5 = (int)(short)piVar8[0x32];
              iVar6 = iVar5 * 0x14;
              uVar9 = 2;
LAB_00443156:
              iVar11 = 0;
              piVar8 = piVar8 + 0x31;
              iVar10 = iVar7;
              goto LAB_0044315c;
            }
          }
          sVar2 = sVar2 + 1;
          iVar7 = (int)sVar2;
        } while (iVar7 < *(int *)(iVar4 + 0x14));
      }
      sVar3 = sVar3 + 1;
      iVar4 = (int)sVar3;
    } while (iVar4 < *(int *)(iVar1 + 0x104));
  }
  return;
}
#endif
