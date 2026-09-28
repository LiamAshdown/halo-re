// rasterizer_prepare_lighting_constants  (Ghidra: FUN_00518ce0, split in two by Ghidra)
// address 0x518ce0, size 603 bytes (0x518ce0..0x518f3a; Ghidra stopped it at 0x518d40)
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: raw disassembly (phase 4 review). Ghidra ended this function at 0x518d40, which is
//   only the target of the `jmp 0x518d40` inside its point light loop, and made the rest a
//   separate "function" render_objects_transparent 0x518d40 (a name borrowed from OpenSauce,
//   whose own note says inside=0x518ce0). Nothing calls 0x518d40; the four callers (0x52719b in
//   rasterizer_model_draw_prepare_states, 0x5337a0 in rasterizer_geometry_part_draw, 0x533b19
//   and 0x533e89 in rasterizer_transparent_geometry_group_draw) call 0x518ce0 with a
//   render_lighting pointer on the stack. The 0x518d40 file is removed; this file is the whole
//   function.
// What it does: fills the 11 register vertex shader lighting block c15..c25 and uploads it:
//   c15..c20 the two point lights (rasterizer_light_set_point_constants 0x518c10 with EAX = the
//   light index or -1 past point_light_count, CX = slot, EDX = the block), c21..c24 the two
//   distant lights as (direction, color) pairs (zero past distant_light_count), c25 the ambient
//   colour. When the debug float 0x00689418 is positive the block is zeroed and the ambient is
//   that value on all three channels instead. Below ps_1_1 it also sets D3DRS_AMBIENT to
//   clamp(0x0071d190.. + byte 0x0069c684 / 255 + ambient) packed as 0x00rrggbb.
// register convention: stack -> lighting (ebp frame, [ebp+8]).
// blam-cc: stack -> lighting
// UNSURE: in the normal path the w components of the distant light rows and of the ambient row
//   are never written (the block is only zeroed on the debug path); they upload stack garbage.
//   Zeroed here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;                 // 0x0071d174
extern d3d_caps9 rasterizer_caps;               // 0x007c10c0
extern float unknown_00689418;                  // 0x00689418 debug: solid ambient override when > 0
extern uint32_t renderer_unknown_69c684;        // 0x0069c684 low byte: ambient boost in 1/255 steps
extern ColorRGB zoom_static_tint_r;               // 0x0071d190 fixed function ambient base

// blam-cc: EAX -> light_index, CX -> slot, EDX -> dest_base
extern void rasterizer_light_set_point_constants(int32_t light_index, int16_t slot,
                                                 rasterizer_point_light_constants *dest_base); // 0x518c10

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);

typedef struct lighting_constant_block {
    rasterizer_point_light_constants point_lights[2]; // c15..c20
    float distant_lights[2][2][4];                    // c21..c24: direction, color
    float ambient[4];                                 // c25
} lighting_constant_block;

static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

void rasterizer_prepare_lighting_constants(render_lighting *lighting)
{
    lighting_constant_block block;
    float ambient_red, ambient_green, ambient_blue;
    int16_t i;

    if (unknown_00689418 > 0.0f) {
        float *words = (float *)&block;
        int32_t w;

        for (w = 0; w < 0x2c; w++) {
            words[w] = 0.0f;
        }
        ambient_red = ambient_green = ambient_blue = unknown_00689418;
    } else {
        for (i = 0; i < 2; i++) {
            int32_t light_index = (i < lighting->point_light_count) ? lighting->point_light_indices[i] : -1;

            rasterizer_light_set_point_constants(light_index, i, &block.point_lights[0]);
        }
        for (i = 0; i < 2; i++) {
            const render_distant_light *light = &lighting->distant_lights[i];

            if (i < lighting->distant_light_count) {
                block.distant_lights[i][0][0] = light->direction.i;
                block.distant_lights[i][0][1] = light->direction.j;
                block.distant_lights[i][0][2] = light->direction.k;
                block.distant_lights[i][0][3] = 0.0f;   // not written by the original
                block.distant_lights[i][1][0] = light->color.red;
                block.distant_lights[i][1][1] = light->color.green;
                block.distant_lights[i][1][2] = light->color.blue;
                block.distant_lights[i][1][3] = 0.0f;   // not written by the original
            } else {
                int32_t k;
                for (k = 0; k < 4; k++) {
                    block.distant_lights[i][0][k] = 0.0f;
                    block.distant_lights[i][1][k] = 0.0f;
                }
            }
        }
        ambient_red = lighting->ambient_color.red;
        ambient_green = lighting->ambient_color.green;
        ambient_blue = lighting->ambient_color.blue;
    }
    block.ambient[0] = ambient_red;
    block.ambient[1] = ambient_green;
    block.ambient[2] = ambient_blue;
    block.ambient[3] = 0.0f;                          // not written by the original

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        float boost = (float)(renderer_unknown_69c684 & 0xff) * 0.003921569f;
        uint32_t red = (uint32_t)(int32_t)(clamp01(zoom_static_tint_r.red + boost + ambient_red) * 255.0f);
        uint32_t green = (uint32_t)(int32_t)(clamp01(zoom_static_tint_r.green + boost + ambient_green) * 255.0f);
        uint32_t blue = (uint32_t)(int32_t)(clamp01(zoom_static_tint_r.blue + boost + ambient_blue) * 255.0f);

        ((d3d_call2_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(
            rasterizer_device, 0x8b, (((red & 0xff) << 8 | (green & 0xff)) << 8) | (blue & 0xff)); // AMBIENT
    }
    ((d3d_set_constant_f_fn)(*(void ***)rasterizer_device)[0x178 / 4])(rasterizer_device, 0xf, (const float *)&block, 0xb);
}

#if 0
Original Ghidra decompilation (0x518ce0): 0x518d40 render_objects_transparent is the tail of this function; its file was removed

/* WARNING: Removing unreachable block (ram,0x00518e0e) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00518ce0(float *param_1)

{
  float fVar1;
  int *piVar2;
  undefined1 uVar3;
  undefined1 uVar4;
  short sVar5;
  uint uVar6;
  int iVar7;
  float *pfVar8;
  float *pfVar9;
  short sVar10;
  undefined4 *puVar11;
  undefined4 local_b8 [24];
  float afStack_58 [16];
  float fStack_18;
  float local_14;
  float local_10;
  
  if (DAT_00689418 <= 0.0) {
    sVar10 = 0;
    do {
      if (sVar10 < *(short *)(param_1 + 0x10)) {
        render_objects_transparent();
        return;
      }
      FUN_00518c10();
      sVar10 = sVar10 + 1;
    } while (sVar10 < 2);
    sVar10 = *(short *)(param_1 + 3);
    sVar5 = 0;
    pfVar9 = afStack_58;
    pfVar8 = param_1 + 4;
    do {
      if ((sVar5 < sVar10) && (pfVar8 != (float *)0x0)) {
        *pfVar9 = pfVar8[3];
        fVar1 = pfVar8[5];
        pfVar9[1] = pfVar8[4];
        pfVar9[2] = fVar1;
        pfVar9[4] = *pfVar8;
        fVar1 = pfVar8[2];
        pfVar9[5] = pfVar8[1];
        pfVar9[6] = fVar1;
      }
      else {
        *pfVar9 = 0.0;
        pfVar9[1] = 0.0;
        pfVar9[2] = 0.0;
        pfVar9[3] = 0.0;
        pfVar9[4] = 0.0;
        pfVar9[5] = 0.0;
        pfVar9[6] = 0.0;
        pfVar9[7] = 0.0;
      }
      sVar5 = sVar5 + 1;
      pfVar8 = pfVar8 + 6;
      pfVar9 = pfVar9 + 8;
    } while (sVar5 < 2);
    local_14 = param_1[1];
    local_10 = param_1[2];
    fStack_18 = *param_1;
  }
  else {
    puVar11 = local_b8;
    for (iVar7 = 0x2c; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar11 = 0;
      puVar11 = puVar11 + 1;
    }
    local_10 = DAT_00689418;
    local_14 = DAT_00689418;
    fStack_18 = DAT_00689418;
  }
  piVar2 = DAT_0071d174;
  if (DAT_007c118c < 0xffff0101) {
    iVar7 = *DAT_0071d174;
    uVar3 = __ftol();
    uVar4 = __ftol();
    uVar6 = __ftol();
    (**(code **)(iVar7 + 0xe4))(piVar2,0x8b,(uint)CONCAT11(uVar3,uVar4) << 8 | uVar6 & 0xff);
  }
  (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xf,local_b8,0xb);
  return;
}
#endif
