// object_sample_total_lighting_at_point
// address 0x4f1c20, size 571 bytes
// name confidence: 0.35 (still FUN_004f1c20 in Ghidra; functions.md's summary: "Computes total
//   lighting at a point by combining a default/lightmap ambient sample with nearby dynamic
//   lights, clamped to [0,1]")
// rewrite confidence: 0.3
// evidence: global 0x00686b0c (one of "the shared constant vectors" per types/objects.h's
//   globals list); types/objects.h light (flags with _light_always_visible_bit); global
//   0x00860b14 light_data; callee object_lights_gather_nearest (0x4f2df0, this batch), whose
//   established signature this call site is matched onto (search_margin/self_object_index/
//   max_count are literal constants here, which line up cleanly). The BSP lightmap-material
//   lookup (the `if` guard just after the default-vector writes) belongs to the BSP module, same
//   as in object_sample_ambient_lightmap_point (0x4f1e60, this batch); its offsets are raw and
//   UNSURE for the same reason.
// register convention: probe point in EAX (param_1), a cluster-bearing pointer in ECX (param_2),
//   color output in EDX (param_3).
// UNSURE: the original decompilation reads the BSP material-table guard as
//   `*(short *)((short)param_3 * 0x20 + ...)`, multiplying the OUTPUT pointer by 0x20 -- clearly
//   a Ghidra register misattribution, since param_3 is the unrelated output color pointer. The
//   sibling function object_sample_ambient_lightmap_point (0x4f1e60) performs the identical
//   lookup using its BSP probe's leaf result at the same `*(int32_t *)(base+0x108)` offset, so
//   this rewrite substitutes the probe's own leaf output here instead of transcribing the
//   corrupted expression literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern real_vector3d object_ambient_lighting_default; // 0x00686b0c
extern uint8_t *structure_bsp_globals; // 0x00746f9c, see object_sample_ambient_lightmap_point.c
extern int32_t object_probe_globals; // 0x0065dd94, see object_sample_ambient_lightmap_point.c
extern int32_t light_frame_counter; // 0x008607c4
extern int32_t light_render_unknown_7c0; // 0x008607c0
extern data_array *light_data; // 0x00860b14

extern char structure_bsp_resolve_position_to_surface(void *probe_globals, int32_t *out_leaf, void *out_b, void *out_c); // 0x555190
extern void *bitmap_group_get_bitmap_data(void);
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
    // cache module, 0x444550. The BitmapData pointer travels in EAX and is not visible at
    // these call sites; the two values Ghidra shows pushed are `wait` and
    // `allocate_if_missing`. UNSURE: the bitmap argument is passed as NULL here.
extern void bsp_lightmap_sample_vertex_color(void *bitmap, int32_t a, void *b, real_vector3d *out);
extern void object_lights_gather_nearest(int16_t cluster_index, uint32_t self_object_index,
    real_point3d *probe_point, float search_margin, uint32_t *out_indices, float *out_intensities,
    uint32_t out_falloffs, int16_t *count, int16_t max_count); // 0x4f2df0, this batch

void object_sample_total_lighting_at_point(real_point3d *probe_point, int32_t *param_2, real_vector3d *color)
    // blam-cc: EAX -> probe_point, ECX -> param_2, EDX -> color
{
    int32_t probe_leaf;
    uint8_t probe_b[8];
    float probe_c[5];
    int16_t count;
    uint32_t indices[2];

    *color = object_ambient_lighting_default;

    if (structure_bsp_resolve_position_to_surface(&object_probe_globals, &probe_leaf, probe_b, probe_c) != 0 &&
        *(int32_t *)(structure_bsp_globals + 0xc) != -1 &&
        *(int16_t *)(probe_leaf * 0x20 + *(int32_t *)(structure_bsp_globals + 0x108)) != -1) {
        void *bitmap = bitmap_group_get_bitmap_data();
        if (texture_cache_get(0, 0, 0) != 0) {
            bsp_lightmap_sample_vertex_color(bitmap, *(int32_t *)probe_b, probe_c, color);
        }
    }

    if (*(int16_t *)((uint8_t *)param_2 + 4) != -1) {
        int16_t i;
        light_frame_counter = light_frame_counter + 1;
        light_render_unknown_7c0 = 1;
        count = 0;
        object_lights_gather_nearest(*(int16_t *)((uint8_t *)param_2 + 4), 0xffffffff, probe_point,
                                      0.0f, indices, probe_c, (uint32_t)probe_b, &count, 2);
        light_render_unknown_7c0 = 0;

        for (i = 0; i < count; i++) {
            light *entry = (light *)light_data->data + (indices[i] & 0xffff);
            if ((entry->flags & _light_always_visible_bit) != 0) {
                float intensity = probe_c[i];
                color->i += *(float *)((uint8_t *)entry + 0x14) * intensity;
                color->j += *(float *)((uint8_t *)entry + 0x18) * intensity;
                color->k += *(float *)((uint8_t *)entry + 0x1c) * intensity;
            }
        }
    }

    color->i = color->i < 0.0f ? 0.0f : (color->i > 1.0f ? 1.0f : color->i);
    color->j = color->j < 0.0f ? 0.0f : (color->j > 1.0f ? 1.0f : color->j);
    color->k = color->k < 0.0f ? 0.0f : (color->k > 1.0f ? 1.0f : color->k);
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
