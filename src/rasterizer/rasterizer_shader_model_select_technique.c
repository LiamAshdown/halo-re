// rasterizer_shader_model_select_technique  (Ghidra: rasterizer_shader_environment_select_technique_texture;
//   renamed in the phase 4 review)
// address 0x527500, size 408 bytes (jump table 0x527698, nine entries)
// name confidence: 0.65   rewrite confidence: 0.9
// evidence: checked against the raw disassembly and the jump table read from the image. The ECX
//   parameter is a ShaderModel (types/tags.h), which resolves the TYPES-GAP of the earlier file:
//   +0x28 shader_model_flags (bit 0 detail_after_reflection adds 3, bit 4 true_atmospheric_fog
//   suppresses the ps_1_4 offset), +0x70 color_source, +0xd4 detail_function (the base index)
//   and +0xd6 detail_mask (the switch). Its only caller is
//   rasterizer_shader_model_draw_pixel_shader 0x529e00, which draws model shaders. vector3d_distance
//   gets EAX = active model context center, ECX = camera position. Effects: 121 no mask, 120
//   reflection mask, 117 self-illumination mask, 118 change color mask, 119 multipurpose alpha;
//   the odd detail masks (inverse) use the +6 half of each technique table. vtable +0xec is
//   ID3DXEffect::SetTechnique.
// register convention: ECX -> shader. Returns the effect slot, or NULL when the effect is not
//   loaded or SetTechnique fails.
// blam-cc: ECX -> shader
// UNSURE: the technique tables keep their environment_techniques_* names from types/rasterizer.h
//   although they hold model techniques.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern d3d_caps9 rasterizer_caps;                                    // 0x007c10c0
extern rasterizer_window_parameters rasterizer_window;               // 0x007c1220
extern rasterizer_model_draw_context *rasterizer_active_model_context; // 0x0071d1f0
extern int32_t environment_techniques_multipurpose[24];              // 0x006e1780
extern int32_t environment_techniques_self_illumination[24];         // 0x006e18d8
extern int32_t environment_techniques_plain[12];                     // 0x006e1938
extern int32_t environment_techniques_reflection[24];                // 0x006e1968
extern int32_t environment_techniques_change_color[24];              // 0x006e19d0
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410

// blam-cc: EAX -> a, ECX -> b
extern real vector3d_distance(const real_point3d *a, const real_point3d *b); // 0x4088b0
extern double fabs(double x);                                        // inline x87 fabs

typedef int32_t (__stdcall *d3dx_set_technique_fn)(void *effect, int32_t technique);

rasterizer_effect_slot *rasterizer_shader_model_select_technique(const ShaderModel *shader)
{
    uint16_t flags = shader->shader_model_flags;
    int32_t index = shader->detail_function;
    rasterizer_effect_slot *slot;
    int32_t technique;

    if (flags & 1) {
        index += 3;
    }
    if (rasterizer_caps.pixel_shader_version >= 0xffff0104 && !(flags & 0x10)) {
        index += (shader->detail_mask == 0) ? 6 : 0xc;
    }
    switch (shader->detail_mask) {
    case 0:
        slot = &rasterizer_effects[121];
        if (slot->effect == 0) {
            return NULL;
        }
        if (rasterizer_caps.pixel_shader_version >= 0xffff0101 && rasterizer_caps.pixel_shader_version < 0xffff0104 &&
            shader->color_source == 2 &&
            fabs(vector3d_distance(&rasterizer_active_model_context->center, &rasterizer_window.camera.position)) < 6.0) {
            index += 6;
        }
        technique = environment_techniques_plain[index];
        break;
    case 1: slot = &rasterizer_effects[120]; technique = environment_techniques_reflection[index + 6]; break;
    case 2: slot = &rasterizer_effects[120]; technique = environment_techniques_reflection[index]; break;
    case 3: slot = &rasterizer_effects[117]; technique = environment_techniques_self_illumination[index + 6]; break;
    case 4: slot = &rasterizer_effects[117]; technique = environment_techniques_self_illumination[index]; break;
    case 5: slot = &rasterizer_effects[118]; technique = environment_techniques_change_color[index + 6]; break;
    case 6: slot = &rasterizer_effects[118]; technique = environment_techniques_change_color[index]; break;
    case 7: slot = &rasterizer_effects[119]; technique = environment_techniques_multipurpose[index + 6]; break;
    case 8: slot = &rasterizer_effects[119]; technique = environment_techniques_multipurpose[index]; break;
    default:
        return NULL;
    }
    if (slot->effect == 0) {
        return NULL;
    }
    if (((d3dx_set_technique_fn)(*(void ***)(uintptr_t)slot->effect)[0xec / 4])((void *)(uintptr_t)slot->effect,
                                                                                technique) < 0) {
        return NULL;
    }
    return slot;
}

#if 0
Original Ghidra decompilation (0x527500):

undefined4 * rasterizer_shader_environment_select_technique_texture(void)

{
  int *piVar1;
  int *piVar2;
  int in_ECX;
  undefined4 uVar3;
  int iVar4;
  undefined4 *puVar5;
  float10 fVar6;
  
  piVar1 = DAT_0069e330;
  iVar4 = (int)*(short *)(in_ECX + 0xd4);
  if ((*(ushort *)(in_ECX + 0x28) & 1) != 0) {
    iVar4 = iVar4 + 3;
  }
  if ((0xffff0103 < DAT_007c118c) && ((*(ushort *)(in_ECX + 0x28) & 0x10) == 0)) {
    if (*(short *)(in_ECX + 0xd6) == 0) {
      iVar4 = iVar4 + 6;
    }
    else {
      iVar4 = iVar4 + 0xc;
    }
  }
  piVar2 = DAT_0069e310;
  switch(*(undefined2 *)(in_ECX + 0xd6)) {
  case 0:
    puVar5 = &DAT_0069e330;
    if (DAT_0069e330 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    if ((((0xffff0100 < DAT_007c118c) && (DAT_007c118c < 0xffff0104)) &&
        (*(short *)(in_ECX + 0x70) == 2)) &&
       (fVar6 = (float10)vector3d_distance(), ABS(fVar6) < (float10)6.0)) {
      iVar4 = iVar4 + 6;
    }
    iVar4 = (**(code **)(*piVar1 + 0xec))(piVar1,(&DAT_006e1938)[iVar4]);
    goto LAB_00527688;
  case 1:
    puVar5 = &DAT_0069e310;
    if (DAT_0069e310 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    uVar3 = *(undefined4 *)(&DAT_006e1980 + iVar4 * 4);
    break;
  case 2:
    puVar5 = &DAT_0069e310;
    if (DAT_0069e310 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    uVar3 = (&DAT_006e1968)[iVar4];
    break;
  case 3:
    puVar5 = &DAT_0069e2b0;
    if (DAT_0069e2b0 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    uVar3 = *(undefined4 *)(&DAT_006e18f0 + iVar4 * 4);
    piVar2 = DAT_0069e2b0;
    break;
  case 4:
    puVar5 = &DAT_0069e2b0;
    if (DAT_0069e2b0 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    uVar3 = (&DAT_006e18d8)[iVar4];
    piVar2 = DAT_0069e2b0;
    break;
  case 5:
    puVar5 = &DAT_0069e2d0;
    if (DAT_0069e2d0 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    uVar3 = *(undefined4 *)(&DAT_006e19e8 + iVar4 * 4);
    piVar2 = DAT_0069e2d0;
    break;
  case 6:
    puVar5 = &DAT_0069e2d0;
    if (DAT_0069e2d0 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    uVar3 = (&DAT_006e19d0)[iVar4];
    piVar2 = DAT_0069e2d0;
    break;
  case 7:
    puVar5 = &DAT_0069e2f0;
    if (DAT_0069e2f0 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    uVar3 = *(undefined4 *)(&DAT_006e1798 + iVar4 * 4);
    piVar2 = DAT_0069e2f0;
    break;
  case 8:
    puVar5 = &DAT_0069e2f0;
    if (DAT_0069e2f0 == (int *)0x0) {
      return (undefined4 *)0x0;
    }
    uVar3 = (&DAT_006e1780)[iVar4];
    piVar2 = DAT_0069e2f0;
    break;
  default:
    goto switchD_00527549_default;
  }
  iVar4 = (**(code **)(*piVar2 + 0xec))(piVar2,uVar3);
LAB_00527688:
  if (-1 < iVar4) {
    return puVar5;
  }
switchD_00527549_default:
  return (undefined4 *)0x0;
}
#endif
