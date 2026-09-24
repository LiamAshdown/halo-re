// rasterizer_shader_transparent_chicago_set_texture_stages  (Ghidra: FUN_00537bb0; the phase 3
//   rewrite called it rasterizer_object_lights_apply_vertex_shader_constants)
// address 0x537bb0, size 416 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: raw disassembly (phase 4 review). Only caller is
//   rasterizer_shader_transparent_chicago_draw 0x531ed0 (at 0x5324e9, EBX = group->shader, the
//   result ignored). EBX +0x54 / +0x58 is ShaderTransparentChicago.maps (count / elements, 0xdc
//   bytes each); +0x2c / +0x2e are ShaderTransparentChicagoMap.color_function / alpha_function
//   and bit 1 of the map flags is alpha replicate. No light or vertex shader constant is
//   involved: every call is SetTextureStageState.
// What it does: the fixed function combiner for each map. Map i (all but the last) programs
//   stage i + 1 from the three dword row of the colour function table 0x0069e710 (op, arg1,
//   arg2) for colour and from the alpha function row for alpha, with D3DTA_ALPHAREPLICATE
//   (0x20) on COLORARG1 when the map asks for it. The last map sets stage 0 to SELECTARG1 of
//   the texture for colour and alpha (ARG2 diffuse). Returns 0 when there are no maps.
// register convention: EBX -> shader.
// blam-cc: EBX -> shader

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device; // 0x0071d174
// three dwords per ShaderColorFunctionType: D3DTOP op, arg1, arg2
extern uint32_t rasterizer_chicago_color_function_stage_states[][3]; // 0x0069e710

typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    void **vt = *(void ***)rasterizer_device;
    ((d3d_call3_fn)vt[0x10c / 4])(rasterizer_device, stage, type, value);
}

uint8_t rasterizer_shader_transparent_chicago_set_texture_stages(const ShaderTransparentChicago *shader)
{
    int16_t map_index;

    if ((int32_t)shader->maps.count <= 0) {
        return 0;
    }
    for (map_index = 0; map_index < (int32_t)shader->maps.count; map_index++) {
        const ShaderTransparentChicagoMap *map =
            (const ShaderTransparentChicagoMap *)(uintptr_t)shader->maps.pointer + map_index;
        uint32_t replicate = (*(const uint8_t *)&map->flags & 2) << 4;

        if (map_index == (int32_t)shader->maps.count - 1) {
            set_texture_stage_state(0, 1, 2);    // COLOROP SELECTARG1
            set_texture_stage_state(0, 2, 2);    // COLORARG1 TEXTURE
            set_texture_stage_state(0, 3, 0);    // COLORARG2 DIFFUSE
            set_texture_stage_state(0, 4, 2);    // ALPHAOP SELECTARG1
            set_texture_stage_state(0, 5, 2);    // ALPHAARG1 TEXTURE
            set_texture_stage_state(0, 6, 0);    // ALPHAARG2 DIFFUSE
        } else {
            uint32_t stage = (uint32_t)map_index + 1;
            const uint32_t *color = rasterizer_chicago_color_function_stage_states[map->color_function];
            const uint32_t *alpha = rasterizer_chicago_color_function_stage_states[map->alpha_function];

            set_texture_stage_state(stage, 1, color[0]);
            set_texture_stage_state(stage, 2, color[1] | replicate);
            set_texture_stage_state(stage, 3, color[2]);
            set_texture_stage_state(stage, 4, alpha[0]);
            set_texture_stage_state(stage, 5, alpha[1]);
            set_texture_stage_state(stage, 6, alpha[2]);
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x537bb0): phase 3 file rasterizer_object_lights_apply_vertex_shader_constants replaced

undefined4 FUN_00537bb0(void)

{
  byte bVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int unaff_EBX;
  byte *pbVar5;
  undefined4 uVar6;
  
  iVar4 = *(int *)(unaff_EBX + 0x54);
  if (0 < iVar4) {
    sVar2 = 0;
    iVar3 = 0;
    do {
      pbVar5 = (byte *)(iVar3 * 0xdc + *(int *)(unaff_EBX + 0x58));
      bVar1 = *pbVar5;
      if (iVar3 == iVar4 + -1) {
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
        iVar4 = *DAT_0071d174;
        uVar6 = 0;
        iVar3 = 0;
      }
      else {
        iVar3 = iVar3 + 1;
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar3,1,
                   *(undefined4 *)(&DAT_0069e710 + *(short *)(pbVar5 + 0x2c) * 0xc));
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar3,2,
                   *(uint *)(&DAT_0069e714 + *(short *)(pbVar5 + 0x2c) * 0xc) | (bVar1 & 2) << 4);
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar3,3,
                   *(undefined4 *)(&DAT_0069e718 + *(short *)(pbVar5 + 0x2c) * 0xc));
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar3,4,
                   *(undefined4 *)(&DAT_0069e710 + *(short *)(pbVar5 + 0x2e) * 0xc));
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar3,5,
                   *(undefined4 *)(&DAT_0069e714 + *(short *)(pbVar5 + 0x2e) * 0xc));
        iVar4 = *DAT_0071d174;
        uVar6 = *(undefined4 *)(&DAT_0069e718 + *(short *)(pbVar5 + 0x2e) * 0xc);
      }
      (**(code **)(iVar4 + 0x10c))(DAT_0071d174,iVar3,6,uVar6);
      iVar4 = *(int *)(unaff_EBX + 0x54);
      sVar2 = sVar2 + 1;
      iVar3 = (int)sVar2;
    } while (iVar3 < iVar4);
    return 1;
  }
  return 0;
}
#endif
