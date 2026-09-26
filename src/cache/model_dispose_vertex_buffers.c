// model_dispose_vertex_buffers  (Ghidra: FUN_00442f00)
// address 0x442f00, size 266 bytes
// name confidence: 0.5 (disposal counterpart of model_load_vertex_buffers @0x442d10, per
// out/phase4/cache_types_notes.md item 2: same "mod2" tag_iterator filter and the same
// GBXModel::geometries -> parts walk at strides 0x30/0x84; the cache_functions.md summary calls
// this "structure_bsp cluster" resources, which the notes identify as a misattribution)
// rewrite confidence: 0.45
// evidence: types/tags.h GBXModel (geometries reflexive 0xd0/0xd4), GBXModelGeometry (parts
// reflexive 0x24/0x28), ModelGeometryPart (vertex_offset 0x64, triangle_offset_2 0x50);
// out/phase4/cache_types_notes.md tag_iterator section (both model functions' iterator setup
// recovered from raw disassembly: index at base+0x04, "mod2" filter at base+0x10).
// register convention: no explicit parameters; the tag_iterator built on the stack is passed to
// tag_iterator_next by pointer in ESI, as in model_load_vertex_buffers and tag_iterator_next's
// other callers.
// UNSURE: the two fields released here, ModelGeometryPart::vertex_offset (0x64) and
// ::triangle_offset_2 (0x50), are TagDataOffset-style on-disk fields (source byte offsets) in
// types/tags.h. This function treats them as live Direct3D object pointers instead, which is only
// consistent if rasterizer_vertex_buffer_create / rasterizer_index_buffer_create -- both outside
// this module and not decoded here -- overwrite them with the created object once loaded, the
// same repurposing types/cache.h documents for BitmapData::pointer and
// SoundPermutation::samples_pointer. Not confirmed against the rasterizer module itself.

#include "tags.h"
#include "cache.h"

extern datum_index tag_iterator_next(tag_iterator *iterator); // blam-cc: ESI; this module, 0x4425d0

extern tag_instance *tag_instances; // 0x0087bc14
extern void *d3d_device;            // 0x0071d174

// Releases the Direct3D vertex and index buffer objects (see UNSURE above) held by every part of
// every geometry of every gbxmodel tag, and clears both fields. The disposal counterpart of
// model_load_vertex_buffers.
void model_dispose_vertex_buffers(void)
{
    tag_iterator iterator;
    datum_index tag_id;
    GBXModel *model;
    GBXModelGeometry *geometry;
    GBXModelGeometryPart *part;
    void **object;
    void (__stdcall **vtable)(void *); // COM methods are __stdcall
    int32_t geometry_index;
    int32_t part_index;

    iterator.next_index = 0;
    iterator.group_tag = _tag_group_gbxmodel; // "mod2"

    tag_id = tag_iterator_next(&iterator);
    while (tag_id != (datum_index)0xffffffff) {
        model = (GBXModel *)tag_instances[(uint16_t)tag_id].data;

        for (geometry_index = 0; geometry_index < (int32_t)model->geometries.count;
             geometry_index++) {
            geometry = (GBXModelGeometry *)(model->geometries.pointer +
                geometry_index * sizeof(GBXModelGeometry));

            for (part_index = 0; part_index < (int32_t)geometry->parts.count; part_index++) {
                part = (GBXModelGeometryPart *)(geometry->parts.pointer +
                    part_index * sizeof(GBXModelGeometryPart));

                if (d3d_device != 0 && (void *)part != (void *)-0x54) {
                    object = (void **)part->base.vertex_offset;
                    if (object != 0) {
                        vtable = *(void (__stdcall ***)(void *))object;
                        vtable[2](object); // slot +8, Release()
                        part->base.vertex_offset = 0;
                    }
                }
                if (d3d_device != 0 && (void *)part != (void *)-0x44) {
                    object = (void **)part->base.triangle_offset_2;
                    if (object != 0) {
                        vtable = *(void (__stdcall ***)(void *))object;
                        vtable[2](object);
                        part->base.triangle_offset_2 = 0;
                    }
                }
            }
        }
        tag_id = tag_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x442f00):

void FUN_00442f00(void)

{
  int iVar1;
  int *piVar2;
  short sVar3;
  short sVar4;
  uint uVar5;
  int iVar6;
  int iVar7;

  uVar5 = tag_iterator_next();
  while (uVar5 != 0xffffffff) {
    iVar1 = *(int *)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar4 = 0;
    if (0 < *(int *)(iVar1 + 0xd0)) {
      iVar6 = 0;
      do {
        iVar7 = iVar6 * 0x30 + *(int *)(iVar1 + 0xd4);
        sVar3 = 0;
        if (0 < *(int *)(iVar6 * 0x30 + 0x24 + *(int *)(iVar1 + 0xd4))) {
          iVar6 = 0;
          do {
            iVar6 = iVar6 * 0x84 + *(int *)(iVar7 + 0x28);
            if (((DAT_0071d174 != 0) && (iVar6 != -0x54)) &&
               (piVar2 = *(int **)(iVar6 + 100), piVar2 != (int *)0x0)) {
              (**(code **)(*piVar2 + 8))(piVar2);
              *(undefined4 *)(iVar6 + 100) = 0;
            }
            if (((DAT_0071d174 != 0) && (iVar6 != -0x44)) &&
               (piVar2 = *(int **)(iVar6 + 0x50), piVar2 != (int *)0x0)) {
              (**(code **)(*piVar2 + 8))(piVar2);
              *(undefined4 *)(iVar6 + 0x50) = 0;
            }
            sVar3 = sVar3 + 1;
            iVar6 = (int)sVar3;
          } while (iVar6 < *(int *)(iVar7 + 0x24));
        }
        sVar4 = sVar4 + 1;
        iVar6 = (int)sVar4;
      } while (iVar6 < *(int *)(iVar1 + 0xd0));
    }
    uVar5 = tag_iterator_next();
  }
  return;
}
#endif
