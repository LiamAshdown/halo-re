// structure_lightmap_uv_rect_build  (orphan pass 4: FUN_0044db30, no Ghidra name)
// address 0x44db30, size 253 bytes
// name confidence: 0.3 (out/phase4/effects_types_notes.md: "builds a lightmap uv rectangle
//   from a ScenarioStructureBSPLightmap material row")
// rewrite confidence: 0.25 (control flow and arithmetic transcribed literally from the
//   decompilation; the tag/material record layout it walks does not match this pass's own
//   ScenarioStructureBSPMaterial/ScenarioStructureBSPLightmap field offsets closely enough to
//   claim those types, so every offset is left raw -- see UNSURE)
// evidence: out/phase4/effects_types_notes.md "0x44db30 | bitmaps or structures | ...". Global
//   0x0087bc14 tag_instances (matches the same `(tag_id & 0xffff) * 0x20 + 0x14 + tag_instances`
//   dereference pattern used throughout this codebase, e.g. antenna_new.c).
// register convention: EDX = float *out_uv_rect (in_EDX, 4 floats), stack arguments = int16_t
//   lightmap_index (param_1), int16_t material_index (param_2), float scale (param_3); EDI =
//   a struct with a tag id at +0xe4, a flag byte at +1, and a value at +0xfc (unaff_EDI).
// blam-cc: structure_lightmap_uv_rect_build(int16_t lightmap_index /*stack*/, int16_t material_index /*stack*/,
//   float scale /*stack*/, float *out_uv_rect /*EDX*/)
// UNSURE (function-wide): every struct offset below (tag+0x58, lightmap_row+0x38, the 0x40-byte
//   lightmap-row stride, the 0x30-byte per-material record, the vertex-rect fields at +8/+0xc/
//   +0x10/+0x14/+0x18/+0x1c) is preserved as a raw byte offset rather than a named field,
//   because it does not cleanly match this pass's own ScenarioStructureBSPMaterial /
//   ScenarioStructureBSPLightmap layout (types/tags.h) closely enough to claim those types.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "structures.h"

extern tag_instance *tag_instances; // 0x0087bc14

void structure_lightmap_uv_rect_build(int16_t lightmap_index, int16_t material_index, float scale,
                                       float *out_uv_rect, uint8_t *owner /*EDI, UNSURE: see file header*/)
{
    float aspect = 1.0f;
    uint8_t *tag_data = tag_instances[*(uint16_t *)(owner + 0xe4)].data;
    uint8_t *lightmap_row = *(uint8_t **)(*(uint32_t *)(tag_data + 0x58) + lightmap_index * 0x40 + 0x38);
    uint8_t *material_row = lightmap_row + material_index * 0x20;
    uint8_t *material_record = *(uint8_t **)(tag_data + 100) + *(int16_t *)(lightmap_row + material_index * 0x20) * 0x30;

    float *rect = (float *)(material_row + 8); // {min_u, min_v, max_u, max_v}
    out_uv_rect[0] = rect[0];
    out_uv_rect[1] = rect[1];
    out_uv_rect[2] = rect[2];
    out_uv_rect[3] = rect[3];

    if (owner[1] & 1) {
        aspect = ((float)(int32_t)*(int16_t *)(material_record + 6) / (float)(int32_t)*(int16_t *)(material_record + 4)) *
                 ((rect[1] - rect[0]) / (rect[3] - rect[2]));
    }

    scale = scale / *(float *)(owner + 0xfc);
    {
        float scale_u = (float)(int32_t)*(int16_t *)(material_record + 4) * scale;
        float scale_v = (float)(int32_t)*(int16_t *)(material_record + 6) * scale * aspect;

        out_uv_rect[0] = -*(float *)(material_row + 0x18) * scale_u;
        out_uv_rect[1] = ((rect[1] - *(float *)(material_row + 0x18)) - rect[0]) * scale_u;
        out_uv_rect[2] = -*(float *)(material_row + 0x1c) * scale_v;
        out_uv_rect[3] = ((rect[3] - *(float *)(material_row + 0x1c)) - rect[2]) * scale_v;
    }
}

#if 0
Original Ghidra decompilation (0x44db30):

void FUN_0044db30(short param_1,short param_2,float param_3,float *param_4)

{
  float *pfVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  int iVar6;
  float *in_EDX;
  int unaff_EDI;

  fVar3 = 1.0;
  iVar6 = *(int *)((*(uint *)(unaff_EDI + 0xe4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)(param_1 * 0x40 + 0x38 + *(int *)(iVar6 + 0x58));
  iVar5 = param_2 * 0x20 + iVar2;
  iVar6 = *(short *)(param_2 * 0x20 + iVar2) * 0x30 + *(int *)(iVar6 + 100);
  pfVar1 = (float *)(iVar5 + 8);
  *in_EDX = *pfVar1;
  in_EDX[1] = *(float *)(iVar5 + 0xc);
  in_EDX[2] = *(float *)(iVar5 + 0x10);
  in_EDX[3] = *(float *)(iVar5 + 0x14);
  if ((*(byte *)(unaff_EDI + 1) & 1) != 0) {
    fVar3 = ((float)(int)*(short *)(iVar6 + 6) / (float)(int)*(short *)(iVar6 + 4)) *
            ((*(float *)(iVar5 + 0xc) - *pfVar1) /
            (*(float *)(iVar5 + 0x14) - *(float *)(iVar5 + 0x10)));
  }
  param_3 = param_3 / *(float *)(unaff_EDI + 0xfc);
  fVar4 = (float)(int)*(short *)(iVar6 + 4) * param_3;
  fVar3 = (float)(int)*(short *)(iVar6 + 6) * param_3 * fVar3;
  *param_4 = -*(float *)(iVar5 + 0x18) * fVar4;
  param_4[1] = ((*(float *)(iVar5 + 0xc) - *(float *)(iVar5 + 0x18)) - *pfVar1) * fVar4;
  param_4[2] = -*(float *)(iVar5 + 0x1c) * fVar3;
  param_4[3] = ((*(float *)(iVar5 + 0x14) - *(float *)(iVar5 + 0x1c)) - *(float *)(iVar5 + 0x10)) *
               fVar3;
  return;
}
#endif
