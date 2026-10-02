// rasterizer_light_set_point_constants  (Ghidra: FUN_00518c10, unnamed; named from its
// behaviour -- see types/rasterizer.h's rasterizer_point_light_constants note, which already
// attributes this exact field layout to this function)
// address 0x518c10, size 195 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: every field written matches rasterizer_point_light_constants exactly (position,
//   inverse_radius_squared, forward, color) sourced from rasterizer_light[light_index]
//   (0x007c1484, stride 0x38, types/rasterizer.h); falloff_scale/falloff_offset come from the
//   light definition's Light tag at +0x1c/+0x20 (cos_falloff_angle/cos_cutoff_angle, per the
//   type header's own evidence table), with -4.0f (0xc0800000 as a float, i.e. -0x40800000 as
//   raw bits) as the "omni light" sentinel.
// register convention: light index (-1 clears) in in_EAX, destination slot index in in_CX,
//   destination buffer base in in_EDX. // blam-cc: EAX -> light_index, CX -> slot,
//   EDX -> dest_base

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern rasterizer_light rasterizer_lights[k_rasterizer_maximum_lights]; // 0x007c1484

// blam-cc: EAX -> light_index, CX -> slot, EDX -> dest_base
// Copies (or clears) a rasterizer_point_light_constants record at dest_base[slot] from
// rasterizer_lights[light_index], or zeroes it when light_index == -1.
void rasterizer_light_set_point_constants(int32_t light_index, int16_t slot,
                                           rasterizer_point_light_constants *dest_base)
{
    rasterizer_point_light_constants *dest = &dest_base[slot];

    if (light_index == -1) {
        uint32_t *raw = (uint32_t *)dest;
        int i;
        for (i = 0; i < 0xc; i++) {
            raw[i] = 0;
        }
        return;
    }

    {
        rasterizer_light *light = &rasterizer_lights[light_index];
        uint8_t *definition = (uint8_t *)light->definition;
        float cos_falloff_angle = *(float *)(definition + 0x1c);
        float cos_cutoff_angle = *(float *)(definition + 0x20);

        dest->position = light->position;
        dest->inverse_radius_squared = 1.0f / (light->radius * light->radius);
        dest->forward = light->forward;
        dest->color = light->color;

        if (*(int32_t *)(definition + 0x1c) != -0x40800000) { // not the omni-light sentinel
            float falloff_scale = 1.0f / (cos_falloff_angle - cos_cutoff_angle);
            dest->falloff_scale = falloff_scale;
            dest->falloff_offset = -(falloff_scale * cos_cutoff_angle);
        } else {
            dest->falloff_scale = 0.0f;
            dest->falloff_offset = 1.0f;
        }
    }
}

#if 0
Original Ghidra decompilation (0x518c10):

void FUN_00518c10(void)

{
  float fVar1;
  int in_EAX;
  int iVar2;
  short in_CX;
  undefined4 *puVar3;
  int in_EDX;

  if (in_EAX == -1) {
    puVar3 = (undefined4 *)(in_CX * 0x30 + in_EDX);
    for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    return;
  }
  iVar2 = in_EAX * 0x38;
  puVar3 = (undefined4 *)(in_CX * 0x30 + in_EDX);
  *puVar3 = (&DAT_007c1488)[in_EAX * 0xe];
  puVar3[1] = *(undefined4 *)(&DAT_007c148c + iVar2);
  puVar3[2] = *(undefined4 *)(&DAT_007c1490 + iVar2);
  puVar3[3] = 1.0 / (*(float *)(&DAT_007c14b8 + iVar2) * *(float *)(&DAT_007c14b8 + iVar2));
  puVar3[4] = *(undefined4 *)(&DAT_007c1494 + iVar2);
  puVar3[5] = *(undefined4 *)(&DAT_007c1498 + iVar2);
  puVar3[6] = *(undefined4 *)(&DAT_007c149c + iVar2);
  puVar3[8] = *(undefined4 *)(&DAT_007c14ac + iVar2);
  puVar3[9] = *(undefined4 *)(&DAT_007c14b0 + iVar2);
  puVar3[10] = *(undefined4 *)(&DAT_007c14b4 + iVar2);
  iVar2 = (&DAT_007c1484)[in_EAX * 0xe];
  if (*(int *)(iVar2 + 0x1c) != -0x40800000) {
    fVar1 = 1.0 / (*(float *)(iVar2 + 0x1c) - *(float *)(iVar2 + 0x20));
    puVar3[7] = fVar1;
    puVar3[0xb] = -(fVar1 * *(float *)((&DAT_007c1484)[in_EAX * 0xe] + 0x20));
    return;
  }
  puVar3[7] = 0;
  puVar3[0xb] = 0x3f800000;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
