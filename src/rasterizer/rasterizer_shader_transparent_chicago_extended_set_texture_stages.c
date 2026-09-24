// rasterizer_shader_transparent_chicago_extended_set_texture_stages  (Ghidra: FUN_00537d60; the
//   phase 3 rewrite called it rasterizer_object_lights_apply_vertex_shader_constants_compat)
// address 0x537d60, size 526 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: raw disassembly (phase 4 review). Only caller is
//   rasterizer_shader_transparent_chicago_extended_draw 0x532a40 (at 0x53310e, ECX =
//   group->shader, the result ignored). ECX +0x54 / +0x58 and +0x60 / +0x64 are
//   ShaderTransparentChicagoExtended.maps_4_stage and maps_2_stage; the pixel shader version
//   picks the list, not a tag format. The records are ShaderTransparentChicagoMap (0xdc bytes).
// What it does: the same fixed function combiner setup as
//   rasterizer_shader_transparent_chicago_set_texture_stages 0x537bb0, over the 2-stage maps
//   below ps_1_1 and the 4-stage maps otherwise. Returns 0 when maps_4_stage is empty (checked
//   first as a dword, whatever the device), 1 otherwise.
// register convention: ECX -> shader.
// blam-cc: ECX -> shader
// UNSURE: the element pointers are first copied into a four entry stack array with no bound on
//   the count; a tag with more than four maps would overrun it in the original too.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;   // 0x0071d174
extern d3d_caps9 rasterizer_caps; // 0x007c10c0
// three dwords per ShaderColorFunctionType: D3DTOP op, arg1, arg2
extern uint32_t rasterizer_chicago_color_function_stage_states[][3]; // 0x0069e710

typedef int32_t (*d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    void **vt = *(void ***)rasterizer_device;
    ((d3d_call3_fn)vt[0x10c / 4])(rasterizer_device, stage, type, value);
}

uint8_t rasterizer_shader_transparent_chicago_extended_set_texture_stages(const ShaderTransparentChicagoExtended *shader)
{
    const ShaderTransparentChicagoMap *maps[4];
    const TagReflexive *list;
    int16_t count;
    int16_t map_index;

    if ((int32_t)shader->maps_4_stage.count <= 0) {
        return 0;
    }
    list = (rasterizer_caps.pixel_shader_version < 0xffff0101) ? &shader->maps_2_stage : &shader->maps_4_stage;
    count = (int16_t)list->count;
    if (count <= 0) {
        return 1;
    }
    for (map_index = 0; map_index < count; map_index++) {
        maps[map_index] = (const ShaderTransparentChicagoMap *)(uintptr_t)list->pointer + map_index;
    }
    for (map_index = 0; map_index < count; map_index++) {
        const ShaderTransparentChicagoMap *map = maps[map_index];
        uint32_t replicate = (*(const uint8_t *)&map->flags & 2) << 4;

        if (map_index == count - 1) {
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
Original Ghidra decompilation (0x537d60): phase 3 file rasterizer_object_lights_apply_vertex_shader_constants_compat replaced

uint FUN_00537d60(void)

{
  byte bVar1;
  ushort uVar2;
  byte *pbVar3;
  undefined2 uVar5;
  uint uVar4;
  int in_ECX;
  int iVar6;
  int *piVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  undefined4 uVar11;
  uint local_18;
  int local_10 [4];
  
  uVar4 = *(uint *)(in_ECX + 0x54);
  if ((int)uVar4 < 1) {
    return uVar4 & 0xffffff00;
  }
  uVar5 = (undefined2)(uVar4 >> 0x10);
  if (DAT_007c118c < 0xffff0101) {
    uVar2 = *(ushort *)(in_ECX + 0x60);
    uVar4 = CONCAT22(uVar5,uVar2);
    if ((short)uVar2 < 1) goto LAB_00537f61;
    iVar6 = *(int *)(in_ECX + 100);
    piVar7 = local_10;
    uVar9 = (uint)uVar2;
    do {
      *piVar7 = iVar6;
      iVar6 = iVar6 + 0xdc;
      piVar7 = piVar7 + 1;
      uVar9 = uVar9 - 1;
    } while (uVar9 != 0);
  }
  else {
    uVar2 = *(ushort *)(in_ECX + 0x54);
    uVar4 = CONCAT22(uVar5,uVar2);
    if ((short)uVar2 < 1) goto LAB_00537f61;
    iVar6 = *(int *)(in_ECX + 0x58);
    piVar7 = local_10;
    uVar9 = (uint)uVar2;
    do {
      *piVar7 = iVar6;
      iVar6 = iVar6 + 0xdc;
      piVar7 = piVar7 + 1;
      uVar9 = uVar9 - 1;
    } while (uVar9 != 0);
  }
  if (0 < (short)uVar4) {
    local_18 = uVar4 & 0xffff;
    iVar6 = 1;
    piVar7 = local_10;
    do {
      pbVar3 = (byte *)*piVar7;
      bVar1 = *pbVar3;
      if (iVar6 + -1 == (short)uVar4 + -1) {
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
        (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
        iVar8 = *DAT_0071d174;
        uVar11 = 0;
        iVar10 = 0;
      }
      else {
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar6,1,
                   *(undefined4 *)(&DAT_0069e710 + *(short *)(pbVar3 + 0x2c) * 0xc));
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar6,2,
                   *(uint *)(&DAT_0069e714 + *(short *)(pbVar3 + 0x2c) * 0xc) | (bVar1 & 2) << 4);
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar6,3,
                   *(undefined4 *)(&DAT_0069e718 + *(short *)(pbVar3 + 0x2c) * 0xc));
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar6,4,
                   *(undefined4 *)(&DAT_0069e710 + *(short *)(pbVar3 + 0x2e) * 0xc));
        (**(code **)(*DAT_0071d174 + 0x10c))
                  (DAT_0071d174,iVar6,5,
                   *(undefined4 *)(&DAT_0069e714 + *(short *)(pbVar3 + 0x2e) * 0xc));
        iVar8 = *DAT_0071d174;
        uVar11 = *(undefined4 *)(&DAT_0069e718 + *(short *)(pbVar3 + 0x2e) * 0xc);
        iVar10 = iVar6;
      }
      (**(code **)(iVar8 + 0x10c))(DAT_0071d174,iVar10,6,uVar11);
      piVar7 = piVar7 + 1;
      iVar6 = iVar6 + 1;
      local_18 = local_18 - 1;
    } while (local_18 != 0);
    uVar4 = 0;
  }
LAB_00537f61:
  return CONCAT31((int3)(uVar4 >> 8),1);
}
#endif
