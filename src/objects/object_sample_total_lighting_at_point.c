// object_sample_total_lighting_at_point
// address 0x4f1c20, size 571 bytes (0x4f1c20..0x4f1e5a, `ret`s at 0x4f1e2d, 0x4f1e4c, 0x4f1e5a)
// name confidence: 0.35 (still FUN_004f1c20 in Ghidra; functions.md's summary: "Computes total
//   lighting at a point by combining a default/lightmap ambient sample with nearby dynamic
//   lights, clamped to [0,1]")
// rewrite confidence: 0.75 (orphan pass 4 re-derived the body from objdump 0x4f1c20..0x4f1e5b while
//   reconciling its call into bsp_lightmap_sample_vertex_color 0x4f0730; the earlier rewrite read
//   the three stack parameters as EAX/ECX/EDX and invented the callees' argument lists)
// evidence:
//   - the only caller, unit_calculate_luminosity (0x56ec8f..0x56ec9f), pushes &luminosity-color,
//     object + 0x98 (location_leaf_index / location_cluster_index, a bsp_leaf_reference) and
//     object + 0x5c (position): plain cdecl (point, location, color).
//   - 0x4f1c20..0x4f1c3f: the color starts as the vector 0x00686b0c points at.
//   - 0x4f1c42..0x4f1c6e: structure_bsp_resolve_position_to_surface (0x555190; EAX = point, ESI =
//     &contact, EDI = &lightmap_index -- kept in the dead third-parameter slot --, EBX = &weight_2,
//     stack (0x0065dd94 = (0, 0, -10), &material_index, &surface_index, &weight_1)).
//   - 0x4f1c72..0x4f1ce2: unlike object_sample_ambient_lightmap_point this does no shader test:
//     when the BSP has a lightmaps bitmap (bsp + 0x0c) and the lightmap a page index (int16 at +0),
//     bitmap_group_get_bitmap_data (0x43f250, EAX tag, DX index), texture_cache_get (0x444550,
//     EAX bitmap, (0, 0)) and, when that is non-NULL, bsp_lightmap_sample_vertex_color (0x4f0730,
//     ECX = material, EDX = bsp->surfaces[surface_index], stack (bitmap, weight_1, weight_2,
//     color)). The bitmap pointer itself is not NULL-checked (kept).
//   - 0x4f1cea..0x4f1da1: when location->cluster_index != -1: light_frame_counter (0x008607c4)++,
//     the byte 0x008607c0 = 1 around object_lights_gather_nearest (0x4f2df0, AX = cluster_index,
//     stack (-1, point, 0.0, indices, &scores, weights, &count, 2)), then for each gathered light
//     whose flags bit 0 (_light_always_visible_bit, light + 0x02) is set, color += light color
//     (light + 0x14/0x18/0x1c) * weights[i]; light_data 0x00860b14, 0x7c-byte records.
//   - 0x4f1da3..0x4f1e5a: each channel: `c < 0 -> 0` (fcomp 0.0, test ah,5 / jp) else
//     `c > 1 -> 1` (fcomp 1.0, test ah,0x41); a NaN falls through both tests and is kept.
// register convention: plain cdecl.
//   // blam-cc: stack -> (point, location, color)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "structures.h"
#include "fn_objects.h"
#include "fn_structures.h"
#include <stdint.h> // uintptr_t: tag block pointers are 32-bit fields

extern real_vector3d *default_axis_b; // 0x00686b0c
extern real_vector3d object_lightmap_probe_direction;  // 0x0065dd94, (0, 0, -10)
extern ScenarioStructureBSP *global_structure_bsp;     // 0x00746f9c
extern int32_t light_frame_counter;                    // 0x008607c4
extern uint8_t light_render_unknown_7c0;               // 0x008607c0, UNSURE name
extern data_array *light_data;                         // 0x00860b14


    // 0x555190, blam-cc: EAX start_position, ESI position, EDI out_lightmap_index, EBX param_7, rest on the stack
extern BitmapData *bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index);
    // 0x43f250, blam-cc: EAX bitmap_tag_index, DX bitmap_data_index
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
    // 0x444550, blam-cc: EAX bitmap

    // 0x4f0730, blam-cc: ECX material, EDX triangle_vertex_indices, rest on the stack
extern void object_lights_gather_nearest(int16_t cluster_index, uint32_t self_object_index,
    real_point3d *probe_point, float search_margin, uint32_t *out_indices, float *out_intensities,
    uint32_t out_falloffs, int16_t *count, int16_t max_count);
    // 0x4f2df0, blam-cc: AX cluster_index, rest on the stack; the caller multiplies by the array it
    // passes as out_falloffs (the sixth argument)

// Lighting at `point`: the BSP lightmap color under it plus the always-visible dynamic lights of
// its cluster, each channel clamped to [0, 1].
void object_sample_total_lighting_at_point(real_point3d *point, bsp_leaf_reference *location,
    real_vector3d *color)
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

    *color = *default_axis_b;

    if (structure_bsp_resolve_position_to_surface(point, &contact, &lightmap_index, &weight_2,
            &object_lightmap_probe_direction, &material_index, &surface_index, &weight_1)) {
        bsp = global_structure_bsp;
        lightmap = (ScenarioStructureBSPLightmap *)(uintptr_t)bsp->lightmaps.pointer + lightmap_index;
        material = (ScenarioStructureBSPMaterial *)(uintptr_t)lightmap->materials.pointer + material_index;

        if (*(int32_t *)&bsp->lightmaps_bitmap.tag_id != -1 && (int16_t)lightmap->bitmap != -1) {
            BitmapData *bitmap = bitmap_group_get_bitmap_data(*(datum_index *)&bsp->lightmaps_bitmap.tag_id,
                (int16_t)lightmap->bitmap);
            uint16_t *triangle =
                (uint16_t *)((ScenarioStructureBSPSurface *)(uintptr_t)bsp->surfaces.pointer + surface_index);

            if (texture_cache_get(bitmap, 0, 0) != 0) {
                bsp_lightmap_sample_vertex_color(bitmap, weight_1, weight_2, (ColorRGB *)color, material, triangle);
            }
        }
    }

    if (location->cluster_index != -1) {
        uint32_t indices[2];
        float scores[2];
        float weights[2];
        int16_t count = 0;
        int32_t i;

        light_frame_counter++;
        light_render_unknown_7c0 = 1;
        object_lights_gather_nearest(location->cluster_index, 0xffffffff, point, 0.0f, indices, scores,
            (uint32_t)(uintptr_t)weights, &count, 2);
        light_render_unknown_7c0 = 0;

        for (i = 0; i < count; i++) {
            uint8_t *entry = (uint8_t *)light_data->data + (indices[i] & 0xffff) * 0x7c;

            if (*(entry + 2) & _light_always_visible_bit) {
                color->i += *(float *)(entry + 0x14) * weights[i];
                color->j += *(float *)(entry + 0x18) * weights[i];
                color->k += *(float *)(entry + 0x1c) * weights[i];
            }
        }
    }

    if (color->i < 0.0f) {
        color->i = 0.0f;
    } else if (color->i > 1.0f) {
        color->i = 1.0f;
    }
    if (color->j < 0.0f) {
        color->j = 0.0f;
    } else if (color->j > 1.0f) {
        color->j = 1.0f;
    }
    if (color->k < 0.0f) {
        color->k = 0.0f;
    } else if (color->k > 1.0f) {
        color->k = 1.0f;
    }
}

#if 0
Original Ghidra decompilation (0x4f1c20):

void FUN_004f1c20(undefined4 param_1,int param_2,float *param_3)

{
  float fVar1;
  undefined *puVar2;
  float *pfVar3;
  char cVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  undefined1 local_28 [4];
  undefined1 local_24 [8];
  uint local_1c [2];
  float local_14 [5];

  pfVar3 = param_3;
  puVar2 = PTR_DAT_00686b0c;
  *param_3 = *(float *)PTR_DAT_00686b0c;
  param_3[1] = *(float *)(puVar2 + 4);
  param_3[2] = *(float *)(puVar2 + 8);
  cVar4 = FUN_00555190(&DAT_0065dd94,local_28,local_24,local_14);
  if (((cVar4 != '\0') && (*(int *)(DAT_00746f9c + 0xc) != -1)) &&
     (*(short *)((short)param_3 * 0x20 + *(int *)(DAT_00746f9c + 0x108)) != -1)) {
    uVar5 = bitmap_group_get_bitmap_data();
    iVar6 = FUN_00444550(0,0);
    if (iVar6 != 0) {
      bsp_lightmap_sample_vertex_color(uVar5,local_14[0],local_1c[0],pfVar3);
    }
  }
  if (*(short *)(param_2 + 4) != -1) {
    DAT_008607c4 = DAT_008607c4 + 1;
    DAT_008607c0 = 1;
    param_3 = (float *)0x0;
    FUN_004f2df0(0xffffffff,param_1,0,local_1c,local_24,local_14,&param_3,2);
    iVar6 = DAT_00860b14;
    DAT_008607c0 = 0;
    if (0 < (short)param_3) {
      iVar8 = 0;
      uVar9 = (uint)param_3 & 0xffff;
      do {
        iVar7 = (*(uint *)((int)local_1c + iVar8) & 0xffff) * 0x7c + *(int *)(iVar6 + 0x34);
        if ((*(byte *)(iVar7 + 2) & 1) != 0) {
          *pfVar3 = *(float *)(iVar7 + 0x14) * *(float *)((int)local_14 + iVar8) + *pfVar3;
          pfVar3[1] = *(float *)(iVar7 + 0x18) * *(float *)((int)local_14 + iVar8) + pfVar3[1];
          pfVar3[2] = *(float *)(iVar7 + 0x1c) * *(float *)((int)local_14 + iVar8) + pfVar3[2];
        }
        iVar8 = iVar8 + 4;
        uVar9 = uVar9 - 1;
      } while (uVar9 != 0);
    }
  }
  if (0.0 <= *pfVar3) {
    if (*pfVar3 <= 1.0) {
      fVar1 = *pfVar3;
    }
    else {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = 0.0;
  }
  *pfVar3 = fVar1;
  if (0.0 <= pfVar3[1]) {
    if (pfVar3[1] <= 1.0) {
      fVar1 = pfVar3[1];
    }
    else {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = 0.0;
  }
  pfVar3[1] = fVar1;
  if (pfVar3[2] < 0.0) {
    pfVar3[2] = 0.0;
    return;
  }
  if (1.0 < pfVar3[2]) {
    pfVar3[2] = 1.0;
    return;
  }
  pfVar3[2] = pfVar3[2];
  return;
}
#endif
