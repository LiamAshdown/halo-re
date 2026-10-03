#include "tags.h"

#include "halo/cache/cache.hpp"

#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "halo/cache/globals.hpp"
#include "halo/cache/api.hpp"
#include "halo/rasterizer/api.hpp"


namespace halo::cache {

/**
 * Releases the per-material rendering resources of the resident structure bsp and clears the owning
 * tag's data pointer and the global.
 *
 * @address 0x442520
 */
void structure_bsp_loader::dispose(ScenarioBSP *bsp)
{
    halo::cache::structure_bsp_loader::dispose_material_vertex_buffers((ScenarioStructureBSPCompiledHeader *)globals().structure_bsp_data);
    globals().tag_instances[bsp->structure_bsp.tag_id.index].data = 0;
    globals().structure_bsp_data = 0;
}

/**
 * Releases the vertex buffer objects of every material of every lightmap and clears both index pointer
 * fields.
 *
 * @address 0x4431a0
 */
void structure_bsp_loader::dispose_material_vertex_buffers(ScenarioStructureBSPCompiledHeader *compiled_header)
{
    ScenarioStructureBSP *bsp;
    ScenarioStructureBSPLightmap *lightmap;
    ScenarioStructureBSPMaterial *material;
    void **object;
    void (__stdcall **vtable)(void *);
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

            if (halo::rasterizer::globals().device != 0 && (void *)material != (void *)-0xc4) {
                object = (void **)material->lightmap_vertices_index_pointer;
                if (object != 0) {
                    vtable = *(void (__stdcall ***)(void *))object;
                    vtable[2](object);
                    material->lightmap_vertices_index_pointer = 0;
                }
            }
            if (halo::rasterizer::globals().device != 0 && (void *)material != (void *)-0xb0) {
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

/**
 * Reads a structure bsp tag's compiled data block, installs it as structure_bsp_data and as the tag's
 * data pointer, and builds the per-material rendering resources.
 *
 * @address 0x4424b0
 */
uint32_t structure_bsp_loader::load(ScenarioBSP *bsp)
{
    cache_io_completion completion;
    uint8_t completion_flag;
    ScenarioStructureBSPCompiledHeader *header;

    completion_flag = 0;
    completion.flag = &completion_flag;
    completion.procedure = 0;
    completion.data = 0;
    halo::cache::cache_io::request_new(&completion, bsp->bsp_start, bsp->bsp_size, (void *)bsp->bsp_address, 1, 0);
    while (completion_flag == 0) {

    }

    globals().structure_bsp_data = (void *)bsp->bsp_address;

    halo::cache::structure_bsp_loader::load_material_vertex_buffers((ScenarioStructureBSPCompiledHeader *)globals().structure_bsp_data);

    header = (ScenarioStructureBSPCompiledHeader *)globals().structure_bsp_data;
    globals().tag_instances[bsp->structure_bsp.tag_id.index].data = (void *)header->pointer;
    return 1;
}

/**
 * Creates the rasterizer vertex buffers of every material of every lightmap. Environment, water and
 * glass shaders get a combined rendered plus lightmap buffer unless hardware vertex processing is
 * available; all other shaders get the plain pair. Both index pointer fields are cleared first.
 *
 * @address 0x443020
 */
void structure_bsp_loader::load_material_vertex_buffers(ScenarioStructureBSPCompiledHeader *compiled_header)
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

            if (halo::rasterizer::globals().caps.pixel_shader_version < 0xffff0101 &&
                (material->shader.tag_fourcc == _tag_group_shader_environment ||
                 material->shader.tag_fourcc == _tag_group_shader_transparent_water ||
                 material->shader.tag_fourcc == _tag_group_shader_transparent_glass)) {
                if ((int32_t)halo::rasterizer::globals().caps.max_streams < 2 && material->lightmap_vertices_count != 0) {
                    halo::rasterizer::rasterizer_vertex_buffer_create((rasterizer_vertex_buffer *)(&material->rendered_vertices_type), 0x13,
                        material->rendered_vertices_count,
                        (uint32_t *)((void *)material->uncompressed_vertices.pointer), (int32_t)lightmap_vertex_data,
                        (int16_t)material->lightmap_vertices_count * 0x28);
                } else {
                    halo::rasterizer::rasterizer_vertex_buffer_create((rasterizer_vertex_buffer *)(&material->rendered_vertices_type), 0xc,
                        material->rendered_vertices_count,
                        (uint32_t *)((void *)material->uncompressed_vertices.pointer), 0,
                        (int16_t)material->rendered_vertices_count << 5);
                    if (material->lightmap_vertices_count != 0) {
                        halo::rasterizer::rasterizer_vertex_buffer_create((rasterizer_vertex_buffer *)(&material->lightmap_vertices_type), 0xd,
                            (int16_t)material->lightmap_vertices_count, (uint32_t *)lightmap_vertex_data, 0,
                            (int16_t)material->lightmap_vertices_count * 8);
                    }
                }
            } else {
                halo::rasterizer::rasterizer_vertex_buffer_create((rasterizer_vertex_buffer *)(&material->rendered_vertices_type), 0,
                    (int16_t)material->rendered_vertices_count,
                    (uint32_t *)((void *)material->uncompressed_vertices.pointer), 0,
                    (int16_t)material->rendered_vertices_count * 0x38);
                if (material->lightmap_vertices_count != 0) {
                    halo::rasterizer::rasterizer_vertex_buffer_create((rasterizer_vertex_buffer *)(&material->lightmap_vertices_type), 2,
                        (int16_t)material->lightmap_vertices_count, (uint32_t *)lightmap_vertex_data, 0,
                        (int16_t)material->lightmap_vertices_count * 0x14);
                }
            }
        }
    }
}

} // namespace halo::cache
