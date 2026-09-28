// model_load_vertex_buffers  (Ghidra: chimera__on_map_load_client)
// address 0x442d10, size 475 bytes
// name confidence: 0.5 (out/phase4/cache_types_notes.md item 2: the Ghidra/Chimera-derived name
// and the cache_functions.md summary both say "structure_bsp cluster", but the iterator filter
// is the literal "mod2" and the walk is GBXModel::geometries -> parts at strides 0x30/0x84 with
// shaders fetched through GBXModel::shaders -- this is model code, not BSP code. The BSP
// counterpart is structure_bsp_load_material_vertex_buffers @0x443020.)
// rewrite confidence: 0.75
// evidence: types/cache.h cache_file_tag_header (model_data_file_offset 0x14,
// model_index_data_offset 0x1c, model_data_size 0x20) and tag_iterator; types/tags.h GBXModel
// (geometries/shaders reflexives), GBXModelGeometry (parts reflexive), GBXModelGeometryPart /
// ModelGeometryPart (every field offset below -- shader_index 0x04, triangle_buffer_type 0x44,
// triangle_offset_2 0x50, vertex_type 0x54, vertex_count 0x58, vertex_offset 0x64 -- matches
// exactly), ModelShaderReference. rasterizer_vertex_sizes table documented in types/cache.h.
// register convention: cache_file_tag_header *header in EAX (in_EAX); confirmed by the caller,
// cache_file_load @0x442290, which loads tag_data_base (== tag_header) into EAX immediately
// before the call. The cache_io_completion built on the stack is passed to cache_io_request_new
// by pointer in ESI, and the tag_iterator likewise by pointer in ESI to tag_iterator_next, as
// elsewhere in this module.
// UNSURE: `model->flags & 4` is reproduced literally; types/tags.h documents ModelFlags only as
// an ordered bitfield comment ("blend_shared_normals, parts_have_local_nodes, ignore_skinning")
// with no bit values, so this is inferred to be ignore_skinning (third flag, bit 2) but not
// confirmed against any code that names it.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "cache.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t rasterizer_vertex_buffer_create(int16_t *param_1, int16_t vertex_type, int32_t count, uint32_t *source_data, int32_t param_5, uint32_t size); // UNSURE, see structure_bsp_load_material_vertex_
    // buffers.c; rasterizer module, 0x524980
extern uint8_t rasterizer_index_buffer_create(int32_t count, int16_t type, rasterizer_index_buffer *out,
    const void *source); // 0x525030, blam-cc: EAX -> count, DX -> type, stack -> out, source
    // module, 0x525030
extern int16_t cache_io_request_new(cache_io_completion *completion, // blam-cc: ESI
    int32_t offset, uint32_t size, void *destination, uint8_t priority,
    uint8_t data_file_index); // this module, 0x442b20
extern datum_index tag_iterator_next(tag_iterator *iterator); // blam-cc: ESI; this module, 0x4425d0
extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes);
extern void *__stdcall GlobalFree(void *memory);
extern void __stdcall Sleep(uint32_t milliseconds);

extern tag_instance *tag_instances;         // 0x0087bc14
extern uint32_t rasterizer_device_version;  // 0x007c118c
extern int16_t rasterizer_vertex_sizes[];   // 0x0065de00

// blam-cc: header in EAX
// Called after a map's tag data is resident. Reads the whole model geometry data block (vertex
// and index data for every GBXModel tag) into one scratch buffer, then walks every gbxmodel tag's
// geometries and parts, creating each part's rasterizer vertex buffer (and, on success, its
// index buffer) pointing into that buffer. "soso" (shader_model) parts that share normals and
// "swat" (shader_transparent_water) parts get a specialised vertex format when hardware vertex
// processing is available; everything else uses the part's own vertex_type, sized from the
// rasterizer_vertex_sizes table.
void model_load_vertex_buffers(cache_file_tag_header *header)
{
    void *model_buffer;
    cache_io_completion completion;
    uint8_t completion_flag;
    tag_iterator iterator;
    datum_index tag_id;
    GBXModel *model;
    GBXModelGeometry *geometry;
    GBXModelGeometryPart *part;
    ModelShaderReference *shader;
    void *vertex_data;
    void *index_base;
    int32_t vertex_count;
    int8_t success;
    int8_t shared_normals_model_shader;
    int32_t geometry_index;
    int32_t part_index;

    model_buffer = GlobalAlloc(0, header->model_data_size);

    completion_flag = 0;
    completion.flag = &completion_flag;
    completion.procedure = 0;
    completion.data = 0;
    cache_io_request_new(&completion, header->model_data_file_offset, header->model_data_size,
        model_buffer, 1, 0);
    while (completion_flag == 0) {
        Sleep(0);
    }

    index_base = (void *)((uint8_t *)model_buffer + header->model_index_data_offset);

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
                vertex_data = (void *)((uint8_t *)model_buffer + part->base.vertex_offset);
                shader = (ModelShaderReference *)(model->shaders.pointer +
                    part->base.shader_index * sizeof(ModelShaderReference));
                vertex_count = part->base.vertex_count;

                shared_normals_model_shader = (model->flags & 4) != 0 && // UNSURE: ignore_skinning?
                    shader->shader.tag_fourcc == _tag_group_shader_model; // "soso"

                if (rasterizer_device_version < 0xffff0101 &&
                    (shared_normals_model_shader ||
                     shader->shader.tag_fourcc == _tag_group_shader_transparent_water)) {
                    success = rasterizer_vertex_buffer_create(&part->base.vertex_type, 0xe,
                        vertex_count, vertex_data, 0, vertex_count << 5);
                } else {
                    success = rasterizer_vertex_buffer_create(&part->base.vertex_type,
                        part->base.vertex_type, vertex_count, vertex_data, 0,
                        rasterizer_vertex_sizes[part->base.vertex_type] * vertex_count);
                }

                if (success != 0) {
                    // 0x442e8a..0x442e9c: EAX the count (+0x48), DX the type (+0x44), stack (the buffer record at
                    // +0x44, the index data)
                    rasterizer_index_buffer *indices = (rasterizer_index_buffer *)&part->base.triangle_buffer_type;
                    rasterizer_index_buffer_create(indices->count, indices->type, indices,
                        (uint8_t *)index_base + part->base.triangle_offset_2);
                }
            }
        }
        tag_id = tag_iterator_next(&iterator);
    }
    GlobalFree(model_buffer);
}

#if 0
Original Ghidra decompilation (0x442d10):

void chimera__on_map_load_client(void)

{
  short sVar1;
  bool bVar2;
  char cVar3;
  int in_EAX;
  HGLOBAL hMem;
  uint uVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  byte *pbVar8;
  int iVar9;
  int iVar10;
  char local_29;
  int local_28;
  int local_24;
  HGLOBAL local_20;
  byte *local_1c;
  int local_18;
  char *local_14;
  uint local_10;
  undefined4 local_c;
  undefined4 local_4;

  hMem = GlobalAlloc(0,*(SIZE_T *)(in_EAX + 0x20));
  local_14 = &local_29;
  local_10 = 0;
  local_c = 0;
  local_20 = hMem;
  cache_io_request_new(*(undefined4 *)(in_EAX + 0x14),*(undefined4 *)(in_EAX + 0x20),hMem,1,0);
  while (local_29 == '\0') {
    Sleep(0);
  }
  local_18 = *(int *)(in_EAX + 0x1c) + (int)hMem;
  local_10 = local_10 & 0xffff0000;
  local_4 = 0x6d6f6432;
  uVar4 = tag_iterator_next();
  while (uVar4 != 0xffffffff) {
    pbVar8 = *(byte **)((uVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    local_24 = 0;
    local_1c = pbVar8;
    if (0 < *(int *)(pbVar8 + 0xd0)) {
      iVar9 = 0;
      do {
        iVar10 = iVar9 * 0x30 + *(int *)(pbVar8 + 0xd4);
        local_28 = 0;
        if (0 < *(int *)(iVar9 * 0x30 + 0x24 + *(int *)(pbVar8 + 0xd4))) {
          iVar9 = 0;
          do {
            iVar5 = iVar9 * 0x84 + *(int *)(iVar10 + 0x28);
            iVar7 = *(int *)(iVar5 + 100) + (int)local_20;
            piVar6 = (int *)(*(short *)(iVar5 + 4) * 0x20 + *(int *)(pbVar8 + 0xe0));
            iVar9 = *(int *)(iVar5 + 0x58);
            if (((*pbVar8 & 4) == 0) || (*piVar6 != 0x736f736f)) {
              bVar2 = false;
            }
            else {
              bVar2 = true;
            }
            if ((DAT_007c118c < 0xffff0101) && ((bVar2 || (*piVar6 == 0x73776174)))) {
              cVar3 = rasterizer_vertex_buffer_create(iVar5 + 0x54,0xe,iVar9,iVar7,0,iVar9 << 5);
            }
            else {
              sVar1 = *(short *)(iVar5 + 0x54);
              cVar3 = rasterizer_vertex_buffer_create
                                ((short *)(iVar5 + 0x54),sVar1,iVar9,iVar7,0,
                                 *(short *)(&DAT_0065de00 + sVar1 * 2) * iVar9);
              pbVar8 = local_1c;
            }
            if (cVar3 != '\0') {
              rasterizer_index_buffer_create(iVar5 + 0x44,*(int *)(iVar5 + 0x50) + local_18);
            }
            local_28 = local_28 + 1;
            iVar9 = (int)(short)local_28;
          } while (iVar9 < *(int *)(iVar10 + 0x24));
        }
        local_24 = local_24 + 1;
        iVar9 = (int)(short)local_24;
        hMem = local_20;
      } while (iVar9 < *(int *)(pbVar8 + 0xd0));
    }
    uVar4 = tag_iterator_next();
  }
  GlobalFree(hMem);
  return;
}
#endif
