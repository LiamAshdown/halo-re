// structure_bsp_dispose_material_vertex_buffers  (Ghidra: FUN_004431a0)
// address 0x4431a0, size 196 bytes
// name confidence: 0.55 (disposal counterpart of structure_bsp_load_material_vertex_buffers
// @0x443020: identical lightmaps/materials walk, releasing exactly the two index-pointer fields
// that function creates; out/phase4/cache_functions.md calls these "clusters", corrected by
// out/phase4/cache_types_notes.md item 2 to lightmaps/materials)
// rewrite confidence: 0.6
// evidence: types/tags.h ScenarioStructureBSPMaterial (rendered_vertices_index_pointer 0xc0,
// lightmap_vertices_index_pointer 0xd4); called from structure_bsp_dispose (0x442520, EAX =
// structure_bsp_data) and cache_file_unload (0x442430, same).
// register convention: ScenarioStructureBSPCompiledHeader *compiled_header in EAX (in_EAX).
// UNSURE: the `material_base != -0xc4` / `!= -0xb0` comparisons are reproduced literally
// (`iVar6 != -0xc4` / `!= -0xb0` in the original). A material's address is never within 0xc4 or
// 0xb0 bytes of address 0 in this engine, so these are effectively always-true guards inherited
// from generic decompiled pointer-safety code, not meaningful engine logic; not simplified away.
// D3D vtable slot +8 is Release() on the rasterizer's vertex buffer object.

#include "tags.h"
#include "cache.h"

extern void *d3d_device; // 0x0071d174

// blam-cc: compiled_header in EAX
// Releases the Direct3D vertex buffer objects every material in every lightmap of a structure_bsp
// holds (both the rendered-geometry buffer and, if present, the separate lightmap-vertex buffer),
// and clears both index-pointer fields. The disposal counterpart of
// structure_bsp_load_material_vertex_buffers.
void structure_bsp_dispose_material_vertex_buffers(
    ScenarioStructureBSPCompiledHeader *compiled_header)
{
    ScenarioStructureBSP *bsp;
    ScenarioStructureBSPLightmap *lightmap;
    ScenarioStructureBSPMaterial *material;
    void **object;
    void (__stdcall **vtable)(void *); // IUnknown::Release is __stdcall: no add esp after 0x443206 / 0x443228
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

            if (d3d_device != 0 && (void *)material != (void *)-0xc4) {
                object = (void **)material->lightmap_vertices_index_pointer;
                if (object != 0) {
                    vtable = *(void (__stdcall ***)(void *))object; // object's vtable pointer
                    vtable[2](object);                     // slot +8 == vtable[2], Release()
                    material->lightmap_vertices_index_pointer = 0;
                }
            }
            if (d3d_device != 0 && (void *)material != (void *)-0xb0) {
                object = (void **)material->rendered_vertices_index_pointer;
                if (object != 0) {
                    vtable = *(void (__stdcall ***)(void *))object;
                    vtable[2](object);
                    material->rendered_vertices_index_pointer = 0;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4431a0):

void FUN_004431a0(void)

{
  int iVar1;
  int *piVar2;
  short sVar3;
  short sVar4;
  int *in_EAX;
  int iVar5;
  int iVar6;

  iVar1 = *in_EAX;
  sVar4 = 0;
  if (0 < *(int *)(iVar1 + 0x104)) {
    iVar5 = 0;
    do {
      iVar5 = iVar5 * 0x20 + *(int *)(iVar1 + 0x108);
      sVar3 = 0;
      if (0 < *(int *)(iVar5 + 0x14)) {
        iVar6 = 0;
        do {
          iVar6 = iVar6 * 0x100 + *(int *)(iVar5 + 0x18);
          if (((DAT_0071d174 != 0) && (iVar6 != -0xc4)) &&
             (piVar2 = *(int **)(iVar6 + 0xd4), piVar2 != (int *)0x0)) {
            (**(code **)(*piVar2 + 8))(piVar2);
            *(undefined4 *)(iVar6 + 0xd4) = 0;
          }
          if (((DAT_0071d174 != 0) && (iVar6 != -0xb0)) &&
             (piVar2 = *(int **)(iVar6 + 0xc0), piVar2 != (int *)0x0)) {
            (**(code **)(*piVar2 + 8))(piVar2);
            *(undefined4 *)(iVar6 + 0xc0) = 0;
          }
          sVar3 = sVar3 + 1;
          iVar6 = (int)sVar3;
        } while (iVar6 < *(int *)(iVar5 + 0x14));
      }
      sVar4 = sVar4 + 1;
      iVar5 = (int)sVar4;
    } while (iVar5 < *(int *)(iVar1 + 0x104));
  }
  return;
}
#endif
