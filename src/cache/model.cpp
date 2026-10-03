#include "tags.h"

#include "halo/cache/cache.hpp"

#include "win32.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "halo/cache/globals.hpp"

extern "C" {
extern void *rasterizer_device;
extern uint8_t rasterizer_vertex_buffer_create(int16_t *record, int16_t vertex_type, int32_t count, uint32_t *source_data, int32_t second_stream, uint32_t size);
extern uint8_t rasterizer_index_buffer_create(int32_t count, int16_t type, rasterizer_index_buffer *out, const void *source);
extern uint32_t rasterizer_device_version;
extern int16_t rasterizer_vertex_sizes[];
}

namespace halo::cache {

/**
 * Releases the vertex buffer objects of every geometry part of every gbxmodel tag and clears both
 * buffer pointers.
 *
 * @address 0x442f00
 */
void model_vertex_buffers::dispose()
{
    tag_iterator iterator;
    datum_index tag_id;
    GBXModel *model;
    GBXModelGeometry *geometry;
    GBXModelGeometryPart *part;
    void **object;
    void (__stdcall **vtable)(void *);
    int32_t geometry_index;
    int32_t part_index;

    iterator.next_index = 0;
    iterator.group_tag = _tag_group_gbxmodel;

    tag_id = halo::cache::view(&iterator)->next();
    while (tag_id != (datum_index)0xffffffff) {
        model = (GBXModel *)globals().tag_instances[(uint16_t)tag_id].data;

        for (geometry_index = 0; geometry_index < (int32_t)model->geometries.count;
             geometry_index++) {
            geometry = (GBXModelGeometry *)(model->geometries.pointer +
                geometry_index * sizeof(GBXModelGeometry));

            for (part_index = 0; part_index < (int32_t)geometry->parts.count; part_index++) {
                part = (GBXModelGeometryPart *)(geometry->parts.pointer +
                    part_index * sizeof(GBXModelGeometryPart));

                if (rasterizer_device != 0 && (void *)part != (void *)-0x54) {
                    object = (void **)part->base.vertex_offset;
                    if (object != 0) {
                        vtable = *(void (__stdcall ***)(void *))object;
                        vtable[2](object);
                        part->base.vertex_offset = 0;
                    }
                }
                if (rasterizer_device != 0 && (void *)part != (void *)-0x44) {
                    object = (void **)part->base.triangle_offset_2;
                    if (object != 0) {
                        vtable = *(void (__stdcall ***)(void *))object;
                        vtable[2](object);
                        part->base.triangle_offset_2 = 0;
                    }
                }
            }
        }
        tag_id = halo::cache::view(&iterator)->next();
    }
}

/**
 * Reads the model geometry data block into one scratch buffer and creates the rasterizer vertex
 * buffer, and when that works the index buffer, of every part of every gbxmodel tag. Shader model
 * parts with shared normals and transparent water parts use a specialised vertex format when hardware
 * vertex processing is available.
 *
 * @address 0x442d10
 */
void model_vertex_buffers::load(cache_file_tag_header *header)
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
    halo::cache::cache_io::request_new(&completion, header->model_data_file_offset, header->model_data_size, model_buffer, 1, 0);
    while (completion_flag == 0) {
        Sleep(0);
    }

    index_base = (void *)((uint8_t *)model_buffer + header->model_index_data_offset);

    iterator.next_index = 0;
    iterator.group_tag = _tag_group_gbxmodel;

    tag_id = halo::cache::view(&iterator)->next();
    while (tag_id != (datum_index)0xffffffff) {
        model = (GBXModel *)globals().tag_instances[(uint16_t)tag_id].data;

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

                shared_normals_model_shader = (model->flags & 4) != 0 &&
                    shader->shader.tag_fourcc == _tag_group_shader_model;

                if (rasterizer_device_version < 0xffff0101 &&
                    (shared_normals_model_shader ||
                     shader->shader.tag_fourcc == _tag_group_shader_transparent_water)) {
                    success = rasterizer_vertex_buffer_create(&part->base.vertex_type, 0xe,
                        vertex_count, (uint32_t *)vertex_data, 0, vertex_count << 5);
                } else {
                    success = rasterizer_vertex_buffer_create(&part->base.vertex_type,
                        part->base.vertex_type, vertex_count, (uint32_t *)vertex_data, 0,
                        rasterizer_vertex_sizes[part->base.vertex_type] * vertex_count);
                }

                if (success != 0) {

                    rasterizer_index_buffer *indices = (rasterizer_index_buffer *)&part->base.triangle_buffer_type;
                    rasterizer_index_buffer_create(indices->count, indices->type, indices,
                        (uint8_t *)index_base + part->base.triangle_offset_2);
                }
            }
        }
        tag_id = halo::cache::view(&iterator)->next();
    }
    GlobalFree(model_buffer);
}

/**
 * Walks a predicted resource list and pulls each entry into its streaming cache: bitmaps through
 * texture_cache_manager::get without waiting, sounds through touch_tag_permutations. Other entry types
 * are skipped.
 *
 * @address 0x4449f0
 */
void predicted_resources::touch(TagReflexive *resources)
{
    int16_t index;
    PredictedResource *element;

    index = 0;
    if (0 < (int32_t)resources->count) {
        do {
            element = &((PredictedResource *)resources->pointer)[index];

            if (element->type == predictedresourcetype_bitmap) {
                Bitmap *bitmap_tag = (Bitmap *)globals().tag_instances[element->tag.index].data;
                BitmapData *bitmap = (BitmapData *)((uint8_t *)bitmap_tag->bitmap_data.pointer +
                    (int32_t)(int16_t)element->resource_index * sizeof(BitmapData));
                halo::cache::texture_cache_manager::get(bitmap, 0, 1);
            } else if (element->type == predictedresourcetype_sound) {
                halo::cache::sound_cache_manager::touch_tag_permutations(element->tag);
            }

            index = index + 1;
        } while ((int32_t)index < (int32_t)resources->count);
    }
    return;
}

} // namespace halo::cache
