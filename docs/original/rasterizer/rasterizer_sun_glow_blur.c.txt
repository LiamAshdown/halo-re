// rasterizer_sun_glow_blur  (Ghidra: FUN_00525720; the earlier rewrite called it
//   rasterizer_light_shadow_render_target_composite)
// address 0x525720, size 898 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: raw disassembly (phase 4 review). Its only caller is rasterizer_sun_glow_render
//   0x525ab0 with (6, 7, 4). The earlier file swapped the ping-pong direction, always activated
//   target 0, wrote COLORWRITEENABLE 0xf instead of 7 and uploaded a three float pixel constant.
// What it does: `passes` times, alternately reads first into second and second into first
//   (even passes read `first`), binding the source target to stages 0..3 with the four texel
//   offsets of the .rdata table 0x0065e098 (+-1/128) as c13..c20, and draws the full target quad
//   through pass 1 of effect 76 with DESTALPHA x ZERO blending; the first pass is weighted 1.0,
//   the later ones 0.5 (pixel shader constant c0). Returns the target holding the result.
// register convention: stack -> (first, second, passes).
// blam-cc: stack -> (first, second, passes)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern uint8_t rasterizer_software_vertex_processing;       // 0x0069c680
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_dynamic_screen_vertex rasterizer_shadow_screen_quad[4]; // 0x006e1720
extern const float rasterizer_sun_glow_blur_offsets[8][4];  // 0x0065e098 .rdata: four texel offset pairs of +-1/128

// blam-cc: EAX -> target_index, stack -> (clear_color, clear)
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear); // 0x52ccc0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t count, const void *data, uint32_t stride);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_quad_vertex(int i, float x, float y, float u, float v)
{
    rasterizer_shadow_screen_quad[i].x = x;
    rasterizer_shadow_screen_quad[i].y = y;
    rasterizer_shadow_screen_quad[i].z = 0.0f;
    rasterizer_shadow_screen_quad[i].color = 0xffffffff;
    rasterizer_shadow_screen_quad[i].u = u;
    rasterizer_shadow_screen_quad[i].v = v;
}

int16_t rasterizer_sun_glow_blur(int16_t first, int16_t second, int16_t passes)
{
    int16_t pass;
    void *effect = (void *)(uintptr_t)rasterizer_effects[76].effect;

    if (effect == NULL || passes <= 0) {
        return (passes & 1) ? second : first;
    }
    set_render_state(0x16, 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0x13, 7);        // SRCBLEND DESTALPHA
    set_render_state(0x14, 1);        // DESTBLEND ZERO
    set_render_state(0xab, 1);
    set_render_state(0x0f, 0);
    set_render_state(0x07, 0);
    set_render_state(0x1c, 0);
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
                                               rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                               ((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[0].shader);
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &rasterizer_sun_glow_blur_offsets[0][0], 8);

    for (pass = 0; pass < passes; pass++) {
        int16_t source = (pass & 1) ? second : first;
        int16_t destination = (pass & 1) ? first : second;
        float weight[4];
        uint32_t stage;
        uint32_t effect_passes;

        for (stage = 0; stage < 4; stage++) {
            uint32_t texture = (source < 9 && source >= 0) ? rasterizer_render_targets[source].texture : 0;

            ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, stage, texture);   // SetTexture
            ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, stage, 7, 1);       // MIPFILTER POINT
        }
        rasterizer_render_target_set_active(destination, 0, 0);
        set_quad_vertex(0, -1.015625f, 1.015625f, 0.0f, 0.0f);
        set_quad_vertex(1, 0.984375f, 1.015625f, 1.0f, 0.0f);
        set_quad_vertex(2, 0.984375f, -0.984375f, 1.0f, 1.0f);
        set_quad_vertex(3, -1.015625f, -0.984375f, 0.0f, 1.0f);
        weight[0] = weight[1] = weight[2] = weight[3] = (pass > 0) ? 0.5f : 1.0f;
        ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, weight, 1);
        effect = (void *)(uintptr_t)rasterizer_effects[76].effect;
        ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &effect_passes, 3);
        effect = (void *)(uintptr_t)rasterizer_effects[76].effect;
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, 1);
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, rasterizer_shadow_screen_quad,
                                                               sizeof(rasterizer_dynamic_screen_vertex));
        effect = (void *)(uintptr_t)rasterizer_effects[76].effect;
        ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
    }
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
    return (passes & 1) ? second : first;
}

#if 0
Original Ghidra decompilation (0x525720):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_00525720(short param_1,short param_2,ushort param_3)

{
  undefined4 uVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  ushort uVar5;
  int *piStack_b8;
  undefined4 uStack_b4;
  undefined *puStack_b0;
  int *piStack_ac;
  int *piStack_a8;
  int *piStack_a4;
  int *piStack_a0;
  int *piStack_9c;
  int *piStack_98;
  undefined4 uStack_94;
  int *piStack_90;
  int *piStack_8c;
  undefined4 uStack_88;
  int *piStack_84;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  int *piStack_78;
  undefined4 uStack_74;
  undefined4 uStack_70;
  int *piStack_6c;
  undefined4 uStack_68;
  undefined4 uStack_64;
  int *piStack_60;
  undefined4 uStack_5c;
  undefined4 uStack_58;
  int *piStack_54;
  undefined4 uStack_50;
  undefined4 uStack_4c;
  int *piStack_48;
  undefined4 uStack_44;
  undefined4 uStack_40;
  int *piStack_3c;
  undefined4 uStack_38;
  undefined4 uStack_34;
  int *piStack_30;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  
  if ((DAT_0069dd90 != (int *)0x0) && (0 < (short)param_3)) {
    uStack_28 = 3;
    uStack_2c = 0x16;
    piStack_30 = DAT_0071d174;
    uStack_34 = 0x525760;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_34 = 7;
    uStack_38 = 0xa8;
    piStack_3c = DAT_0071d174;
    uStack_40 = 0x525775;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_40 = 1;
    uStack_44 = 0x1b;
    piStack_48 = DAT_0071d174;
    uStack_4c = 0x525787;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_4c = 7;
    uStack_50 = 0x13;
    piStack_54 = DAT_0071d174;
    uStack_58 = 0x525799;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_58 = 1;
    uStack_5c = 0x14;
    piStack_60 = DAT_0071d174;
    uStack_64 = 0x5257ab;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_64 = 1;
    uStack_68 = 0xab;
    piStack_6c = DAT_0071d174;
    uStack_70 = 0x5257c0;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_70 = 0;
    uStack_74 = 0xf;
    piStack_78 = DAT_0071d174;
    uStack_7c = 0x5257d2;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_7c = 0;
    uStack_80 = 7;
    piStack_84 = DAT_0071d174;
    uStack_88 = 0x5257e4;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_88 = 0;
    piStack_8c = (int *)0x1c;
    piStack_90 = DAT_0071d174;
    uStack_94 = 0x5257f6;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_94 = DAT_006e1af0;
    piStack_98 = DAT_0071d174;
    piStack_9c = (int *)0x52580b;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    piStack_9c = (int *)(-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10);
    piStack_a0 = DAT_0071d174;
    piStack_a4 = (int *)0x525832;
    (**(code **)(*DAT_0071d174 + 0x134))();
    piStack_a4 = DAT_0069e350;
    piStack_a8 = DAT_0071d174;
    piStack_ac = (int *)0x525847;
    (**(code **)(*DAT_0071d174 + 0x170))();
    piStack_ac = (int *)0x8;
    puStack_b0 = &DAT_0065e098;
    uStack_b4 = 0xd;
    piStack_b8 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x178))();
    uVar5 = 0;
    if (0 < (short)param_3) {
      do {
        sVar3 = (short)piStack_8c;
        if ((uVar5 & 1) == 0) {
          sVar3 = (short)piStack_90;
          piStack_ac = piStack_8c;
        }
        else {
          piStack_ac = piStack_90;
        }
        iVar4 = 0;
        iVar2 = 4;
        do {
          uVar1 = 0;
          if ((sVar3 < 9) && (-1 < sVar3)) {
            uVar1 = (&DAT_0069d368)[sVar3 * 5];
          }
          (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,iVar4,uVar1);
          (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar4,7,1);
          iVar4 = iVar4 + 1;
          iVar2 = iVar2 + -1;
        } while (iVar2 != 0);
        FUN_0052ccc0(0,0);
        _DAT_006e172c = 0xffffffff;
        _DAT_006e1730 = 0;
        _DAT_006e1734 = 0;
        _DAT_006e1720 = 0xbf820000;
        _DAT_006e1724 = 0x3f820000;
        _DAT_006e1744 = 0xffffffff;
        _DAT_006e1748 = 0x3f800000;
        _DAT_006e174c = 0;
        _DAT_006e1738 = 0x3f7c0000;
        _DAT_006e173c = 0x3f820000;
        _DAT_006e175c = 0xffffffff;
        _DAT_006e1760 = 0x3f800000;
        _DAT_006e1764 = 0x3f800000;
        _DAT_006e1750 = 0x3f7c0000;
        _DAT_006e1754 = 0xbf7c0000;
        _DAT_006e1774 = 0xffffffff;
        _DAT_006e1778 = 0;
        _DAT_006e177c = 0x3f800000;
        _DAT_006e1768 = 0xbf820000;
        _DAT_006e176c = 0xbf7c0000;
        _DAT_006e1770 = 0;
        _DAT_006e1758 = 0;
        _DAT_006e1740 = 0;
        _DAT_006e1728 = 0;
        if ((short)uVar5 < 1) {
          piStack_a4 = (int *)0x3f800000;
        }
        else {
          piStack_a4 = (int *)0x3f000000;
        }
        piStack_a0 = piStack_a4;
        piStack_9c = piStack_a4;
        piStack_98 = piStack_a4;
        (**(code **)(*DAT_0071d174 + 0x1b4))(DAT_0071d174,0,&piStack_a4,1);
        (**(code **)(*DAT_0069dd90 + 0x100))(DAT_0069dd90,&piStack_b8,3);
        (**(code **)(*DAT_0069dd90 + 0x104))(DAT_0069dd90,1);
        (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006e1720,0x18);
        (**(code **)(*DAT_0069dd90 + 0x108))(DAT_0069dd90);
        uVar5 = uVar5 + 1;
      } while ((short)uVar5 < (short)uStack_88);
    }
    FUN_0052ccc0(0,0);
    (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  }
  if ((param_3 & 1) == 0) {
    param_2 = param_1;
  }
  return (int)param_2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
