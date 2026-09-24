// object_sample_ambient_lightmap_point
// address 0x4f1e60, size 144 bytes
// name confidence: 0.3 (still FUN_004f1e60 in Ghidra; functions.md's summary: "Samples a
//   point's ambient lightmap color and incident-direction vector, applying a small brightening
//   bias to the color")
// rewrite confidence: 0.2
// evidence: global 0x00686b08 (one of "the shared constant vectors" per types/objects.h's
//   globals list, used here as the default color/incident pair); global 0x0087bc14
//   tag_instances. Everything else this function reads -- the BSP probe result at
//   0x006f... via structure_bsp_resolve_position_to_surface, the structure_bsp material lookup chain
//   (DAT_00746f9c+0x108, +0xc, +0x14+tag_instances, tag offsets 0x24/0x94) -- belongs to the
//   BSP/rendering module (see out/phase4/objects_types_notes.md's "Not objects-module code"
//   section, which explicitly places the sibling functions bsp_lightmap_sample_vertex_color/
//   _incident there); this module's types/objects.h has no structs for any of it, so every
//   offset below is raw and UNSURE.
// register convention: unknown -- param_1 is unused in the decompilation; color output pointer
//   in ECX (param_2), incident-direction output pointer in EDX (param_3), a "prefer
//   alternate lightmap" flag in EBX (param_4).
// UNSURE: essentially the entire body past the two default-vector writes. Kept as a literal,
//   offset-for-offset transliteration of the decompilation with no invented field names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern real_vector3d object_ambient_lightmap_default; // 0x00686b08
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *structure_bsp_globals; // 0x00746f9c, see objects_update.c and
                                           //   object_set_cluster_and_parent.c; UNSURE: the
                                           //   +0xc and +0x108 sub-offsets used here are not
                                           //   established by either of those two functions

extern int32_t object_probe_globals; // 0x0065dd94, UNSURE: foreign (BSP) module global, address taken
extern char structure_bsp_resolve_position_to_surface(void *probe_globals, int32_t *out_leaf, void *out_b, void *out_c); // 0x555190
extern void *bitmap_group_get_bitmap_data(void); // UNSURE: address not resolved in this batch's pack
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
    // cache module, 0x444550. The BitmapData pointer travels in EAX and is not visible at
    // these call sites; the two values Ghidra shows pushed are `wait` and
    // `allocate_if_missing`. UNSURE: the bitmap argument is passed as NULL here.
extern void bsp_lightmap_sample_vertex_color(void *bitmap, int32_t a, void *b, real_vector3d *out);
extern void bsp_lightmap_sample_vertex_incident(void *bitmap, int32_t a, void *b, void *out);

void object_sample_ambient_lightmap_point(uint32_t param_1, real_vector3d *color,
                                           void *incident, uint8_t prefer_alternate)
    // blam-cc: EAX -> param_1 (unused), ECX -> color, EDX -> incident, EBX -> prefer_alternate
{
    int32_t probe_leaf, probe_index;
    uint8_t probe_scratch[16]; // UNSURE: exact size/type of local_10/local_14 not established

    *color = object_ambient_lightmap_default;
    *(real_vector3d *)incident = object_ambient_lightmap_default;

    if (structure_bsp_resolve_position_to_surface(&object_probe_globals, &probe_leaf, &probe_index, probe_scratch) != 0) {
        // UNSURE: the whole BSP material/lightmap resolution chain below is foreign-module and
        // preserved only as raw offsets; see file header.
        int16_t *material_ref = (int16_t *)(probe_leaf * 0x20 +
            *(int32_t *)(structure_bsp_globals + 0x108));
        uint8_t *material = (uint8_t *)tag_instances[
            *(uint32_t *)((int16_t)probe_index * 0x100 + 0xc + *(int32_t *)(material_ref + 6)) & 0xffff].data;

        if (*(int16_t *)(material + 0x24) == 3 &&
            *(int32_t *)(structure_bsp_globals + 0xc) != -1 &&
            *(int32_t *)(material + 0x94) != -1 && *material_ref != -1) {
            void *primary_bitmap = bitmap_group_get_bitmap_data();
            void *secondary_bitmap = bitmap_group_get_bitmap_data();

            if (primary_bitmap != 0 &&
                ((prefer_alternate != 0 && texture_cache_get(0, 1, 1) != 0) || texture_cache_get(0, 0, 0) != 0)) {
                bsp_lightmap_sample_vertex_color(primary_bitmap, *(int32_t *)probe_scratch,
                                                  probe_scratch + 4, color);
                color->i = color->i + 0.1f > 1.0f ? 1.0f : color->i + 0.1f;
                color->j = color->j + 0.1f > 1.0f ? 1.0f : color->j + 0.1f;
                color->k = color->k + 0.1f > 1.0f ? 1.0f : color->k + 0.1f;
            }

            if (secondary_bitmap != 0 &&
                ((prefer_alternate != 0 && texture_cache_get(0, 1, 1) != 0) || texture_cache_get(0, 0, 0) != 0)) {
                bsp_lightmap_sample_vertex_incident(secondary_bitmap, *(int32_t *)probe_scratch,
                                                     probe_scratch + 4, incident);
            }
        }
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
