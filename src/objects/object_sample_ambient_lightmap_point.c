// object_sample_ambient_lightmap_point  (Ghidra: FUN_004f1e60; name kept from the objects pass)
// address 0x4f1e60, size 583 bytes (0x4f1e60..0x4f20a6, single `ret`). Ghidra's metadata said 144
//   bytes and catalogued the rest of the body as the bogus function 0x4f1ef0 "object_cause_damage"
//   (no prologue: `mov ebp,ds:0x87bc14` in the middle of the material lookup; the epilogue at
//   0x4f209f..0x4f20a6 is this function's own). Orphan pass 4 rewrote the whole function from
//   objdump 0x4f1e60..0x4f20a7; the earlier rewrite had the wrong calling convention (it read the
//   four stack parameters as ECX/EDX/EBX registers) and left every callee's arguments unresolved.
// name confidence: 0.45   rewrite confidence: 0.75
// evidence:
//   - both callers (0x4536ba in particle_system_new_at_point, 0x455a7d in particle_new) push four
//     arguments: point, the two output colors and a byte flag.
//   - 0x4f1e60..0x4f1e92: both outputs start as the vector 0x00686b08 points at (0x0065514c).
//   - 0x4f1e95..0x4f1ec3: structure_bsp_resolve_position_to_surface (0x555190; EAX = point,
//     ESI = &contact, EDI = &lightmap_index, EBX = &weight_2, stack (0x0065dd94 = (0, 0, -10), the
//     probe direction straight down, &material_index, &surface_index, &weight_1)) finds the BSP
//     surface under the point.
//   - 0x4f1ec9..0x4f1f32: lightmap = bsp->lightmaps[lightmap_index] (0x20 stride, pointer at
//     bsp + 0x108), material = lightmap->materials[material_index] (0x100 stride), shader = the
//     material's shader tag (tag_instances 0x0087bc14, data at +0x14). Nothing is sampled unless
//     the shader is an environment shader (Shader.shader_type 3, +0x24), the BSP has a lightmaps
//     bitmap (bsp + 0x0c tag id), the environment shader has a base map (+0x94 tag id) and the
//     lightmap has a bitmap index (int16 at +0, -1 = none).
//   - 0x4f1f38..0x4f1f64: bitmap_group_get_bitmap_data (0x43f250, EAX tag, DX index) for the
//     lightmap page, and for the base map with index shader_permutation (material + 0x10) modulo
//     the base map's bitmap_data count (Bitmap + 0x60), `idiv` = signed remainder.
//   - texture_cache_get (0x444550, EAX = bitmap, stack (wait, allocate)) is tried with (1, 1) when
//     wait_for_textures is set, and with (0, 0) otherwise or when that fails; a bitmap is only
//     sampled when one of them returns non-NULL.
//   - 0x4f1fa4..0x4f1fcc: bsp_lightmap_sample_vertex_color (0x4f0730; ECX = material, EDX = the
//     surface's three vertex indices bsp->surfaces[surface_index] (6-byte stride, pointer at
//     bsp + 0xfc), stack (bitmap, weight_1, weight_2, out)); then each lightmap channel is
//     brightened by 0.1 (0x00672bac) and capped at 1.0 (0x00672ac4) with `fcom; test ah,0x41`,
//     which also replaces a NaN with 1.0.
//   - 0x4f203b..0x4f2097: bsp_material_sample_base_map_color (0x4f0900; EAX = material, ECX = the
//     same surface indices, computed here if the lightmap branch did not run; same stack).
// register convention: plain cdecl, four stack parameters.
//   // blam-cc: stack -> (point, lightmap_color, base_map_color, wait_for_textures)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "structures.h"
#include <stdint.h> // uintptr_t: tag block pointers are 32-bit fields

extern real_vector3d *object_ambient_lightmap_default; // 0x00686b08 -> 0x0065514c
extern real_vector3d object_lightmap_probe_direction;  // 0x0065dd94, (0, 0, -10)
extern tag_instance *tag_instances;                    // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp;     // 0x00746f9c

extern uint8_t structure_bsp_resolve_position_to_surface(real_point3d *start_position,
    real_point3d *position, int16_t *out_lightmap_index, void *param_7, real_vector3d *direction,
    int16_t *out_material_index, int32_t *out_surface, void *param_6);
    // 0x555190, blam-cc: EAX start_position, ESI position, EDI out_lightmap_index, EBX param_7, rest on the stack
extern BitmapData *bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index);
    // 0x43f250, blam-cc: EAX bitmap_tag_index, DX bitmap_data_index
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
    // 0x444550, blam-cc: EAX bitmap
extern void bsp_lightmap_sample_vertex_color(BitmapData *bitmap, float weight_1, float weight_2,
    ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices);
    // 0x4f0730, blam-cc: ECX material, EDX triangle_vertex_indices, rest on the stack
extern void bsp_material_sample_base_map_color(BitmapData *bitmap, float weight_1, float weight_2,
    ColorRGB *out, ScenarioStructureBSPMaterial *material, uint16_t *triangle_vertex_indices);
    // 0x4f0900, blam-cc: EAX material, ECX triangle_vertex_indices, rest on the stack

// texture_cache_get's two-step attempt, inlined at both sample sites
static void *object_lightmap_texture_ready(BitmapData *bitmap, uint8_t wait_for_textures)
{
    void *texture = 0;

    if (wait_for_textures) {
        texture = texture_cache_get(bitmap, 1, 1);
    }
    if (texture == 0) {
        texture = texture_cache_get(bitmap, 0, 0);
    }
    return texture;
}

// Samples the BSP lightmap and the environment shader's base map on the surface straight below
// `point`, writing the (slightly brightened) lightmap color and the base map color. Both outputs
// keep the default vector when there is no suitable surface or texture.
void object_sample_ambient_lightmap_point(real_point3d *point, real_vector3d *lightmap_color,
    real_vector3d *base_map_color, uint8_t wait_for_textures)
{
    real_point3d contact;
    int16_t lightmap_index;
    int16_t material_index;
    int32_t surface_index;
    float weight_1;
    float weight_2;
    ScenarioStructureBSP *bsp;
    ScenarioStructureBSPLightmap *lightmap;
    ScenarioStructureBSPMaterial *material;
    uint8_t *shader;
    uint8_t *base_map_tag;
    datum_index base_map;
    BitmapData *lightmap_bitmap;
    BitmapData *base_map_bitmap;
    uint16_t *triangle = 0;

    *lightmap_color = *object_ambient_lightmap_default;
    *base_map_color = *object_ambient_lightmap_default;

    if (!structure_bsp_resolve_position_to_surface(point, &contact, &lightmap_index, &weight_2,
            &object_lightmap_probe_direction, &material_index, &surface_index, &weight_1)) {
        return;
    }

    bsp = global_structure_bsp;
    lightmap = (ScenarioStructureBSPLightmap *)(uintptr_t)bsp->lightmaps.pointer + lightmap_index;
    material = (ScenarioStructureBSPMaterial *)(uintptr_t)lightmap->materials.pointer + material_index;
    shader = (uint8_t *)tag_instances[*(uint32_t *)&material->shader.tag_id & 0xffff].data;

    if (*(int16_t *)(shader + 0x24) != 3 ||                  // Shader.shader_type: environment
        *(int32_t *)&bsp->lightmaps_bitmap.tag_id == -1 ||
        *(int32_t *)(shader + 0x94) == -1 ||                 // ShaderEnvironment.base_map.tag_id
        (int16_t)lightmap->bitmap == -1) {
        return;
    }

    lightmap_bitmap = bitmap_group_get_bitmap_data(*(datum_index *)&bsp->lightmaps_bitmap.tag_id,
        (int16_t)lightmap->bitmap);
    base_map = *(datum_index *)(shader + 0x94);
    base_map_tag = (uint8_t *)tag_instances[base_map & 0xffff].data;
    base_map_bitmap = bitmap_group_get_bitmap_data(base_map,
        (int16_t)((int32_t)(int16_t)material->shader_permutation % *(int32_t *)(base_map_tag + 0x60)));

    if (lightmap_bitmap != 0 && object_lightmap_texture_ready(lightmap_bitmap, wait_for_textures) != 0) {
        triangle = (uint16_t *)((ScenarioStructureBSPSurface *)(uintptr_t)bsp->surfaces.pointer + surface_index);
        bsp_lightmap_sample_vertex_color(lightmap_bitmap, weight_1, weight_2, (ColorRGB *)lightmap_color,
            material, triangle);
        lightmap_color->i += 0.1f;
        if (!(lightmap_color->i <= 1.0f)) {
            lightmap_color->i = 1.0f;
        }
        lightmap_color->j += 0.1f;
        if (!(lightmap_color->j <= 1.0f)) {
            lightmap_color->j = 1.0f;
        }
        lightmap_color->k += 0.1f;
        if (!(lightmap_color->k <= 1.0f)) {
            lightmap_color->k = 1.0f;
        }
    }

    if (base_map_bitmap != 0 && object_lightmap_texture_ready(base_map_bitmap, wait_for_textures) != 0) {
        if (triangle == 0) {
            triangle = (uint16_t *)((ScenarioStructureBSPSurface *)(uintptr_t)bsp->surfaces.pointer + surface_index);
        }
        bsp_material_sample_base_map_color(base_map_bitmap, weight_1, weight_2, (ColorRGB *)base_map_color,
            material, triangle);
    }
}

#if 0
Original Ghidra decompilation (0x4f1e60):

void FUN_004f1e60(undefined4 param_1,float *param_2,undefined4 *param_3,char param_4)

{
  float fVar1;
  undefined *puVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  short *psVar7;
  short local_24;
  int local_20;
  undefined4 local_1c;
  int iStack_18;
  undefined4 local_14;
  undefined4 local_10 [4];

  puVar2 = PTR_DAT_00686b08;
  *param_2 = *(float *)PTR_DAT_00686b08;
  param_2[1] = *(float *)(puVar2 + 4);
  param_2[2] = *(float *)(puVar2 + 8);
  *param_3 = *(undefined4 *)puVar2;
  param_3[1] = *(undefined4 *)(puVar2 + 4);
  param_3[2] = *(undefined4 *)(puVar2 + 8);
  cVar3 = FUN_00555190(&DAT_0065dd94,&local_20,&local_1c,local_10);
  if (cVar3 != '\0') {
    psVar7 = (short *)(local_24 * 0x20 + *(int *)(DAT_00746f9c + 0x108));
    iVar5 = *(int *)((*(uint *)((short)local_20 * 0x100 + 0xc + *(int *)(psVar7 + 0xc)) & 0xffff) *
                     0x20 + 0x14 + DAT_0087bc14);
    iStack_18 = DAT_00746f9c;
    if ((((*(short *)(iVar5 + 0x24) == 3) && (*(int *)(DAT_00746f9c + 0xc) != -1)) &&
        (*(int *)(iVar5 + 0x94) != -1)) && (*psVar7 != -1)) {
      iVar4 = bitmap_group_get_bitmap_data();
      iVar5 = bitmap_group_get_bitmap_data();
      local_20 = iVar5;
      if ((iVar4 != 0) &&
         (((param_4 != '\0' && (iVar6 = FUN_00444550(1,1), iVar6 != 0)) ||
          (iVar6 = FUN_00444550(0,0), iVar6 != 0)))) {
        bsp_lightmap_sample_vertex_color(iVar4,local_10[0],local_14,param_2);
        fVar1 = *param_2 + 0.1;
        if (1.0 < fVar1) {
          fVar1 = 1.0;
        }
        *param_2 = fVar1;
        fVar1 = param_2[1] + 0.1;
        if (1.0 < fVar1) {
          fVar1 = 1.0;
        }
        param_2[1] = fVar1;
        fVar1 = param_2[2] + 0.1;
        if (1.0 < fVar1) {
          fVar1 = 1.0;
        }
        param_2[2] = fVar1;
        iVar5 = local_20;
      }
      if ((iVar5 != 0) &&
         (((param_4 != '\0' && (iVar4 = FUN_00444550(1,1), iVar4 != 0)) ||
          (iVar4 = FUN_00444550(0,0), iVar4 != 0)))) {
        bsp_lightmap_sample_vertex_incident(iVar5,local_10[0],local_14,param_3);
      }
    }
  }
  return;
}
#endif
