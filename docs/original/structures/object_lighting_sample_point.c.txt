// object_lighting_sample_point  (Ghidra: FUN_004f2550; name kept from an earlier phase)
// address 0x4f2550, size 1132 bytes (0x4f2550..0x4f29bb, `ret`s at 0x4f2629 and 0x4f29bb)
// name confidence: 0.5   rewrite confidence: 0.75 (orphan pass 4 review: rewritten from objdump
//   0x4f2550..0x4f29bc; the first draft read the second stack parameter as unused, passed the
//   probe table to the surface resolver as its start point, called the bitmap / texture-cache
//   helpers with no arguments and swapped the barycentric weights)
// evidence:
//   - callers (object_sample_ambient_lighting, 0x4f2104..0x4f2116 and 0x4f219d..0x4f21af) push
//     (flags, point, lighting): plain cdecl.
//   - 0x4f2556..0x4f2596: the output starts as the BSP's default lighting block (the 0x74 bytes
//     at ScenarioStructureBSP + 0x2c: default_ambient_color, the two default distant lights,
//     reflection tint, shadow vector and color, which is exactly the render_lighting layout of
//     types/rasterizer.h) with distant_light_count forced to 2, unless default_ambient_color.red
//     is 0.0 (`fucompp; test ah,0x44; jnp`: a NaN counts as non-zero), in which case the fixed
//     block at 0x0065dd20 is copied instead.
//   - 0x4f2598..0x4f2619: probe directions: flags bit 0 selects the four horizontal directions at
//     0x0065dda0 ((-10,0,0), (10,0,0), (0,-10,0), (0,10,0)), otherwise the single (0,0,-10) at
//     0x0065dd94. structure_bsp_resolve_position_to_surface (0x555190; EAX = point, ESI =
//     &contact, EDI = &lightmap_index, EBX = &weight_2, stack (direction, &material_index,
//     &surface_index, &weight_1)) is tried along each until one finds a surface.
//   - 0x4f262a..0x4f26aa: same material / environment-shader / lightmap-page gate as
//     object_sample_ambient_lightmap_point (0x4f1e60); then both bitmaps (lightmap page and the
//     base map page shader_permutation % count) must exist and texture_cache_get(bitmap, 1, 1)
//     must succeed for both.
//   - 0x4f2718..0x4f2748: bsp_material_sample_base_map_color (0x4f0900, EAX material, ECX
//     triangle) into base_map_color and bsp_lightmap_sample_vertex_color (0x4f0730, ECX material,
//     EDX triangle) into lightmap_color, both with stack (bitmap, weight_1, weight_2, out).
//   - 0x4f274d..0x4f283f: the three vertex shading normals (compressed: the 11:11:10 normal at
//     +0x0c of the 0x20-byte rendered vertex via 0x513490; uncompressed (vertex type 0 or 0xc):
//     the float normal at +0x0c of the 0x38-byte rendered vertex; any other type leaves them
//     unset, as the original does), blended with vector3d_barycentric_interpolate (0x4f06d0:
//     EAX out, ECX = normal 2, EDX = normal 1, ESI = normal 0, stack (weight_1, weight_2)) and
//     normalized (0x401990, ECX).
//   - 0x4f2841..0x4f2957: the same for the lightmap-section normals (compressed: 0x5134c0 on the
//     8-byte lightmap vertex at rendered_vertices_count * 4 + index; uncompressed: the float
//     normal at +0 of the 0x14-byte lightmap vertex after rendered_vertices_count * 0x38 bytes),
//     but each is normalized first and its length kept; the lengths are blended with the same
//     weights into an intensity.
//   - 0x4f2959..0x4f29a0: object_build_effect_parameter_block (0x4f2ff0) with ECX =
//     lightmap_color, EAX = the blended lightmap normal, EDX = base_map_color, ESI = lighting and
//     stack (flags, &shading_normal, intensity); then the result is 1.
// UNSURE: 0x4f2ff0's own file (src/objects/object_build_effect_parameter_block.c, confidence 0.2)
//   declares a three-argument signature and could not place its four register inputs; the
//   register assignment above is what this call site loads, and the extern below follows it.
// register convention: plain cdecl; returns a bool in AL.
//   // blam-cc: stack -> (flags, point, lighting)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "structures.h"
#include <stdint.h> // uintptr_t: tag block pointers are 32-bit fields

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ScenarioStructureBSP *global_structure_bsp;       // 0x00746f9c
extern tag_instance *tag_instances;                      // 0x0087bc14
extern render_lighting object_lighting_default;          // 0x0065dd20, UNSURE name
extern real_vector3d object_lightmap_probe_direction[1];      // 0x0065dd94, (0, 0, -10)
extern real_vector3d object_lighting_probe_sideways[4];  // 0x0065dda0, (+-10, 0, 0), (0, +-10, 0)

extern uint8_t structure_bsp_resolve_position_to_surface(real_point3d *start_position,
    real_point3d *position, int16_t *out_lightmap_index, void *out_barycentric_v, real_vector3d *direction,
    int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u);
    // 0x555190, blam-cc: EAX start_position, ESI position, EDI out_lightmap_index, EBX out_barycentric_v, rest on the stack
extern BitmapData *bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index);
    // 0x43f250, blam-cc: EAX bitmap_tag_index, DX bitmap_data_index
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
    // 0x444550, blam-cc: EAX bitmap
extern void bsp_material_sample_base_map_color(BitmapData *bitmap, float weight_1, float weight_2,
    ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices);
    // 0x4f0900, blam-cc: EAX material, ECX triangle_vertex_indices, rest on the stack
extern void bsp_lightmap_sample_vertex_color(BitmapData *bitmap, float weight_1, float weight_2,
    ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices);
    // 0x4f0730, blam-cc: ECX material, EDX triangle_vertex_indices, rest on the stack
extern void vector3d_barycentric_interpolate(real_vector3d *out, real_vector3d *v1, real_vector3d *v2,
    real_vector3d *v0, float w2, float w1);
    // 0x4f06d0, blam-cc: EAX out, ECX v1, EDX v2, ESI v0, stack (w2, w1)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, blam-cc: ECX v
extern void bsp_compressed_rendered_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedRenderedVertex *vertex,
    real_vector3d *out); // 0x513490, blam-cc: EAX vertex, ESI out
extern void bsp_compressed_lightmap_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedLightmapVertex *vertex,
    real_vector3d *out); // 0x5134c0, blam-cc: EAX vertex, ESI out
extern void object_build_effect_parameter_block(uint8_t flags, real_vector3d *shading_normal, float intensity,
    ColorRGB *lightmap_color, real_vector3d *lightmap_normal, ColorRGB *base_map_color,
    render_lighting *lighting);
    // 0x4f2ff0, blam-cc: stack (flags, shading_normal, intensity), ECX lightmap_color,
    // EAX lightmap_normal, EDX base_map_color, ESI lighting (this call site; see UNSURE)

// Builds the render_lighting for `point` from the BSP surface found along one of the probe
// directions: the lightmap color and base map color there, the blended vertex shading normal and
// the blended lightmap normal and intensity. Returns whether a lit surface was found; otherwise
// `lighting` keeps the BSP (or fixed) default.
uint8_t object_lighting_sample_point(uint8_t flags, real_point3d *point, render_lighting *lighting)
{
    ScenarioStructureBSP *bsp = global_structure_bsp;
    real_vector3d *directions;
    int16_t direction_count;
    int16_t direction_index;
    real_point3d contact;
    int16_t lightmap_index;
    int16_t material_index;
    int32_t surface_index;
    float weight_1;
    float weight_2;
    ScenarioStructureBSPLightmap *lightmap;
    ScenarioStructureBSPMaterial *material;
    uint8_t *shader;
    uint8_t *base_map_tag;
    uint16_t *triangle;
    BitmapData *lightmap_bitmap;
    BitmapData *base_map_bitmap;
    ColorRGB base_map_color;
    ColorRGB lightmap_color;
    real_vector3d normals[3];
    real_vector3d shading_normal;
    real_vector3d lightmap_normal;
    float lengths[3];
    int16_t vertex_type;
    int32_t i;

    if (bsp->default_ambient_color.red == 0.0f) {
        *lighting = object_lighting_default;
    } else {
        *lighting = *(render_lighting *)&bsp->default_ambient_color;
        lighting->distant_light_count = 2;
    }

    if (flags & 1) {
        directions = object_lighting_probe_sideways;
        direction_count = 4;
    } else {
        directions = object_lightmap_probe_direction;
        direction_count = 1;
    }

    for (direction_index = 0; direction_index < direction_count; direction_index++) {
        if (structure_bsp_resolve_position_to_surface(point, &contact, &lightmap_index, &weight_2,
                &directions[direction_index], &material_index, &surface_index, &weight_1)) {
            break;
        }
    }
    if (direction_index >= direction_count) {
        return 0;
    }

    bsp = global_structure_bsp;
    lightmap = (ScenarioStructureBSPLightmap *)(uintptr_t)bsp->lightmaps.pointer + lightmap_index;
    material = (ScenarioStructureBSPMaterial *)(uintptr_t)lightmap->materials.pointer + material_index;
    shader = (uint8_t *)tag_instances[*(uint32_t *)&material->shader.tag_id & 0xffff].data;

    if (*(int16_t *)&((struct Shader *)shader)->shader_type != 3 ||                  // Shader.shader_type: environment
        *(int32_t *)&bsp->lightmaps_bitmap.tag_id == -1 ||
        (int16_t)lightmap->bitmap == -1 ||
        *(int32_t *)(shader + 0x94) == -1) {                 // ShaderEnvironment.base_map.tag_id
        return 0;
    }

    triangle = (uint16_t *)((ScenarioStructureBSPSurface *)(uintptr_t)bsp->surfaces.pointer + surface_index);
    lightmap_bitmap = bitmap_group_get_bitmap_data(*(datum_index *)&bsp->lightmaps_bitmap.tag_id,
        (int16_t)lightmap->bitmap);
    base_map_tag = (uint8_t *)tag_instances[*(uint32_t *)(shader + 0x94) & 0xffff].data;
    base_map_bitmap = bitmap_group_get_bitmap_data(*(datum_index *)(shader + 0x94),
        (int16_t)((int32_t)(int16_t)material->shader_permutation % *(int32_t *)(base_map_tag + 0x60)));
    if (lightmap_bitmap == 0 || base_map_bitmap == 0 ||
        texture_cache_get(lightmap_bitmap, 1, 1) == 0 || texture_cache_get(base_map_bitmap, 1, 1) == 0) {
        return 0;
    }

    bsp_material_sample_base_map_color(base_map_bitmap, weight_1, weight_2, &base_map_color, material, triangle);
    bsp_lightmap_sample_vertex_color(lightmap_bitmap, weight_1, weight_2, &lightmap_color, material, triangle);

    // vertex shading normals (rendered-vertex section)
    vertex_type = (int16_t)material->rendered_vertices_type;
    if (vertex_type == vertextype_structure_bsp_compressed_rendered_vertices) {
        ScenarioStructureBSPMaterialCompressedRenderedVertex *vertices =
            (ScenarioStructureBSPMaterialCompressedRenderedVertex *)(uintptr_t)material->compressed_vertices.pointer;
        for (i = 0; i < 3; i++) {
            bsp_compressed_rendered_vertex_unpack_normal(&vertices[triangle[i]], &normals[i]);
        }
    } else if (vertex_type == vertextype_structure_bsp_uncompressed_rendered_vertices || vertex_type == 0xc) {
        ScenarioStructureBSPMaterialUncompressedRenderedVertex *vertices =
            (ScenarioStructureBSPMaterialUncompressedRenderedVertex *)(uintptr_t)material->uncompressed_vertices.pointer;
        for (i = 0; i < 3; i++) {
            normals[i] = *(real_vector3d *)&vertices[triangle[i]].normal;
        }
    }
    vector3d_barycentric_interpolate(&shading_normal, &normals[2], &normals[1], &normals[0], weight_1, weight_2);
    vector3d_normalize_with_length(&shading_normal);

    // lightmap normals (lightmap-vertex section, after the rendered vertices)
    if (vertex_type == vertextype_structure_bsp_compressed_rendered_vertices) {
        ScenarioStructureBSPMaterialCompressedLightmapVertex *vertices =
            (ScenarioStructureBSPMaterialCompressedLightmapVertex *)(uintptr_t)material->compressed_vertices.pointer +
            material->rendered_vertices_count * 4;
        for (i = 0; i < 3; i++) {
            bsp_compressed_lightmap_vertex_unpack_normal(&vertices[triangle[i]], &normals[i]);
        }
    } else if (vertex_type == vertextype_structure_bsp_uncompressed_rendered_vertices || vertex_type == 0xc) {
        ScenarioStructureBSPMaterialUncompressedLightmapVertex *vertices =
            (ScenarioStructureBSPMaterialUncompressedLightmapVertex *)((uint8_t *)(uintptr_t)material->uncompressed_vertices.pointer +
            material->rendered_vertices_count * 0x38);
        for (i = 0; i < 3; i++) {
            normals[i] = *(real_vector3d *)&vertices[triangle[i]].normal;
        }
    }
    lengths[0] = vector3d_normalize_with_length(&normals[0]);
    lengths[1] = vector3d_normalize_with_length(&normals[1]);
    lengths[2] = vector3d_normalize_with_length(&normals[2]);
    vector3d_barycentric_interpolate(&lightmap_normal, &normals[2], &normals[1], &normals[0], weight_1, weight_2);
    vector3d_normalize_with_length(&lightmap_normal);

    object_build_effect_parameter_block(flags, &shading_normal,
        (lengths[1] - lengths[0]) * weight_1 + (lengths[2] - lengths[0]) * weight_2 + lengths[0],
        &lightmap_color, &lightmap_normal, &base_map_color, lighting);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4f2550):

undefined1 object_lighting_sample_point(uint param_1,undefined4 param_2,undefined4 *param_3)

{
  ushort *puVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  short sVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  int iVar9;
  float10 fVar10;
  undefined1 local_89;
  float local_88;
  float local_84;
  undefined4 local_80;
  short local_7c [2];
  short local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined *local_50;
  float local_4c;
  float local_48;
  float local_44;
  int local_40;
  undefined1 local_3c [24];
  undefined1 local_24 [12];
  undefined1 local_18 [24];

  local_89 = 0;
  iVar5 = 0x1d;
  puVar7 = (undefined4 *)(DAT_00746f9c + 0x2c);
  puVar8 = param_3;
  if (*(float *)(DAT_00746f9c + 0x2c) == 0.0) {
    puVar7 = &DAT_0065dd20;
    for (; iVar5 != 0; iVar5 = iVar5 + -1) {
      *param_3 = *puVar7;
      puVar7 = puVar7 + 1;
      param_3 = param_3 + 1;
    }
  }
  else {
    for (; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar8 = *puVar7;
      puVar7 = puVar7 + 1;
      puVar8 = puVar8 + 1;
    }
    *(undefined2 *)(param_3 + 3) = 2;
  }
  if ((param_1 & 1) == 0) {
    local_50 = &DAT_0065dd94;
    local_80 = 1;
  }
  else {
    local_50 = &DAT_0065dda0;
    local_80 = 4;
  }
  sVar6 = 0;
  if ((short)local_80 != 0) {
    while (cVar2 = structure_bsp_resolve_position_to_surface
                             (local_50 + sVar6 * 0xc,local_7c,&local_40,&local_84), cVar2 == '\0') {
      sVar6 = sVar6 + 1;
      if ((short)local_80 <= sVar6) {
        return 0;
      }
    }
    iVar5 = *(int *)(local_78 * 0x20 + 0x18 + *(int *)(DAT_00746f9c + 0x108));
    iVar9 = local_7c[0] * 0x100 + iVar5;
    iVar5 = *(int *)((*(uint *)(local_7c[0] * 0x100 + 0xc + iVar5) & 0xffff) * 0x20 + 0x14 +
                    DAT_0087bc14);
    if ((((*(short *)(iVar5 + 0x24) == 3) && (*(int *)(DAT_00746f9c + 0xc) != -1)) &&
        (*(short *)(local_78 * 0x20 + *(int *)(DAT_00746f9c + 0x108)) != -1)) &&
       (*(int *)(iVar5 + 0x94) != -1)) {
      puVar1 = (ushort *)(*(int *)(DAT_00746f9c + 0xfc) + local_40 * 6);
      iVar5 = bitmap_group_get_bitmap_data();
      iVar3 = bitmap_group_get_bitmap_data();
      if ((iVar5 != 0) && (iVar3 != 0)) {
        iVar4 = texture_cache_get(1,1);
        if (iVar4 != 0) {
          iVar4 = texture_cache_get(1,1);
          if (iVar4 != 0) {
            bsp_lightmap_sample_vertex_incident(iVar3,local_84,local_88,local_24);
            bsp_lightmap_sample_vertex_color(iVar5,local_84,local_88,local_18);
            sVar6 = *(short *)(iVar9 + 0xb0);
            if (sVar6 == 1) {
              lens_flare_unpack_direction();
              lens_flare_unpack_direction();
              lens_flare_unpack_direction();
            }
            else if ((sVar6 == 0) || (sVar6 == 0xc)) {
              iVar5 = *(int *)(iVar9 + 0xe4);
              iVar3 = (uint)*puVar1 * 0x38 + iVar5;
              local_74 = *(undefined4 *)(iVar3 + 0xc);
              local_70 = *(undefined4 *)(iVar3 + 0x10);
              local_6c = *(undefined4 *)(iVar3 + 0x14);
              iVar3 = (uint)puVar1[1] * 0x38 + iVar5;
              local_68 = *(undefined4 *)(iVar3 + 0xc);
              local_64 = *(undefined4 *)(iVar3 + 0x10);
              local_60 = *(undefined4 *)(iVar3 + 0x14);
              iVar5 = (uint)puVar1[2] * 0x38 + iVar5;
              local_5c = *(undefined4 *)(iVar5 + 0xc);
              local_58 = *(undefined4 *)(iVar5 + 0x10);
              local_54 = *(undefined4 *)(iVar5 + 0x14);
            }
            vector3d_barycentric_interpolate(local_84,local_88);
            vector3d_normalize_with_length();
            if (sVar6 == 1) {
              lens_flare_unpack_up();
              lens_flare_unpack_up();
              lens_flare_unpack_up();
            }
            else if ((sVar6 == 0) || (sVar6 == 0xc)) {
              iVar3 = *(int *)(iVar9 + 0xb4) * 0x38;
              iVar9 = *(int *)(iVar9 + 0xe4);
              iVar5 = iVar3 + (uint)*puVar1 * 0x14;
              local_74 = *(undefined4 *)(iVar5 + iVar9);
              iVar5 = iVar5 + iVar9;
              local_70 = *(undefined4 *)(iVar5 + 4);
              local_6c = *(undefined4 *)(iVar5 + 8);
              iVar5 = iVar3 + (uint)puVar1[1] * 0x14;
              local_68 = *(undefined4 *)(iVar5 + iVar9);
              iVar5 = iVar5 + iVar9;
              local_64 = *(undefined4 *)(iVar5 + 4);
              local_60 = *(undefined4 *)(iVar5 + 8);
              iVar3 = iVar3 + (uint)puVar1[2] * 0x14;
              local_5c = *(undefined4 *)(iVar3 + iVar9);
              local_58 = *(undefined4 *)(iVar3 + 4 + iVar9);
              local_54 = *(undefined4 *)(iVar3 + iVar9 + 8);
            }
            fVar10 = (float10)vector3d_normalize_with_length();
            local_4c = (float)fVar10;
            fVar10 = (float10)vector3d_normalize_with_length();
            local_48 = (float)fVar10;
            fVar10 = (float10)vector3d_normalize_with_length();
            local_44 = (float)fVar10;
            vector3d_barycentric_interpolate(local_84,local_88);
            vector3d_normalize_with_length();
            object_build_effect_parameter_block
                      (param_1,local_3c,
                       (local_44 - local_4c) * local_88 + (local_48 - local_4c) * local_84 +
                       local_4c);
            local_89 = 1;
          }
        }
      }
    }
  }
  return local_89;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
