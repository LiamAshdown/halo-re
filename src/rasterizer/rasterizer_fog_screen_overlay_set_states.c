// rasterizer_fog_screen_overlay_set_states  (Ghidra: FUN_0051def0)
// address 0x51def0, size 934 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase2/results/rasterizer_01.json ("Configures the render and texture-stage
// states used to draw the screen-space fog overlay, on either the fixed-function or shader path
// depending on driver capability."). A clean, fully linear sequence of SetRenderState (+0xe4)
// and SetSamplerState (+0x114) calls with no register ambiguity, ending in a call to the
// already-named rasterizer_set_shader_stage_config (0x519200). `DAT_007c10e4` is
// d3d_caps9.raster_caps, `DAT_007c112c` is max_anisotropy, `DAT_007c1100` is texture_filter_caps,
// `DAT_007c118c` is pixel_shader_version -- all per types/rasterizer.h's d3d_caps9 layout.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern uint8_t console_debug_toggle_6893f4;    // 0x006893f4, gates this whole function
extern void *rasterizer_device;                // 0x0071d174
extern uint8_t console_debug_toggle_68941d;    // 0x0068941d, UNSURE meaning
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t config_use_anisotropic_filter;        // 0x00722b68, UNSURE meaning; gates anisotropic filtering below
extern uint8_t rasterizer_fog_enabled;                              // 0x0069c6a8 latched by rasterizer_set_fog_constants

extern uint32_t color_rgb_float_to_int(const ColorRGB *color); // 0x4ab5d0
// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_fog_screen_overlay_set_states(void)
{
    void **vtable;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_sampler_state;
    uint32_t stage5_filter, stage6_filter;
    uint32_t max_anisotropy;
    uint32_t fog_color;

    if (console_debug_toggle_6893f4 == 0) {
        return;
    }

    vtable = *(void ***)rasterizer_device;
    set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
    set_render_state(rasterizer_device, 0x16, 3);
    set_render_state(rasterizer_device, 0xa8, (console_debug_toggle_68941d != 0) * 8 + 7);
    set_render_state(rasterizer_device, 0x1b, 1);
    set_render_state(rasterizer_device, 0x13, (-(uint32_t)(console_debug_toggle_6893e4 != 1) & 7) + 2);
    set_render_state(rasterizer_device, 0x14, (console_debug_toggle_6893e4 == 1) + 1);
    set_render_state(rasterizer_device, 0xab, 1);
    set_render_state(rasterizer_device, 0xf, 0);
    set_render_state(rasterizer_device, 7, console_debug_toggle_6893e4 != 1);
    set_render_state(rasterizer_device, 0x17, 3);
    set_render_state(rasterizer_device, 0xe, 0);

    stage5_filter = 2;
    stage6_filter = 2;
    if (config_use_anisotropic_filter != 0 && (rasterizer_caps.raster_caps & 0x20000) != 0 &&
        1 < rasterizer_caps.max_anisotropy) {
        max_anisotropy = 8;
        if (rasterizer_caps.max_anisotropy < 8) {
            max_anisotropy = rasterizer_caps.max_anisotropy;
        }
        vtable = *(void ***)rasterizer_device;
        set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
        set_sampler_state(rasterizer_device, 0, 10, max_anisotropy);
        set_sampler_state(rasterizer_device, 1, 10, max_anisotropy);
        if (0xffff0100 < rasterizer_caps.pixel_shader_version) {
            set_sampler_state(rasterizer_device, 2, 10, max_anisotropy);
            set_sampler_state(rasterizer_device, 3, 10, max_anisotropy);
        }
        if ((rasterizer_caps.texture_filter_caps & 0x400) != 0) {
            stage6_filter = 3;
        }
        if ((rasterizer_caps.texture_filter_caps & 0x4000000) != 0) {
            stage5_filter = 3;
        }
    }

    vtable = *(void ***)rasterizer_device;
    set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
    set_sampler_state(rasterizer_device, 0, 1, 1);
    set_sampler_state(rasterizer_device, 0, 2, 1);
    set_sampler_state(rasterizer_device, 0, 5, stage5_filter);
    set_sampler_state(rasterizer_device, 0, 6, stage6_filter);
    set_sampler_state(rasterizer_device, 0, 7, 2);
    set_sampler_state(rasterizer_device, 1, 1, 1);
    set_sampler_state(rasterizer_device, 1, 2, 1);
    set_sampler_state(rasterizer_device, 1, 5, stage5_filter);
    set_sampler_state(rasterizer_device, 1, 6, stage6_filter);
    set_sampler_state(rasterizer_device, 1, 7, 2);

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        vtable = *(void ***)rasterizer_device;
        set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
        set_render_state(rasterizer_device, 0x1c, rasterizer_fog_enabled);
        fog_color = color_rgb_float_to_int(&rasterizer_window.fog.atmospheric_color);
        set_render_state(rasterizer_device, 0x22, fog_color);
        rasterizer_set_shader_stage_config(5);
        return;
    }

    vtable = *(void ***)rasterizer_device;
    set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
    set_render_state(rasterizer_device, 0x1c, 0);

    vtable = *(void ***)rasterizer_device;
    set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
    set_sampler_state(rasterizer_device, 2, 1, 1);
    set_sampler_state(rasterizer_device, 2, 2, 1);
    set_sampler_state(rasterizer_device, 2, 5, stage5_filter);
    set_sampler_state(rasterizer_device, 2, 6, stage6_filter);
    set_sampler_state(rasterizer_device, 2, 7, 2);
    set_sampler_state(rasterizer_device, 3, 1, 1);
    set_sampler_state(rasterizer_device, 3, 2, 1);
    set_sampler_state(rasterizer_device, 3, 5, stage5_filter);
    set_sampler_state(rasterizer_device, 3, 6, stage6_filter);
    set_sampler_state(rasterizer_device, 3, 7, 2);

    rasterizer_set_shader_stage_config(5);
}

#if 0
Original Ghidra decompilation (0x51def0):

void FUN_0051def0(void)

{
  uint uVar1;
  undefined4 uVar2;
  undefined4 uVar3;

  if (DAT_006893f4 == '\0') {
    return;
  }
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,(DAT_0068941d != '\0') * '\b' + '\a');
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,(-(DAT_006893e4 != 1) & 7U) + 2);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,(DAT_006893e4 == 1) + '\x01');
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,DAT_006893e4 != 1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,3);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,0);
  uVar2 = 2;
  uVar3 = 2;
  if (((DAT_00722b68 != 0) && ((_DAT_007c10e4 & 0x20000) != 0)) && (1 < DAT_007c112c)) {
    uVar1 = 8;
    if (DAT_007c112c < 8) {
      uVar1 = DAT_007c112c;
    }
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,10,uVar1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,10,uVar1);
    if (0xffff0100 < DAT_007c118c) {
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,10,uVar1);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,10,uVar1);
    }
    if ((DAT_007c1100 & 0x400) != 0) {
      uVar3 = 3;
    }
    if ((DAT_007c1100 & 0x4000000) != 0) {
      uVar2 = 3;
    }
  }
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,uVar2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,uVar3);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,1,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,2,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,uVar2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,uVar3);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,7,2);
  if (DAT_007c118c < 0xffff0101) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,DAT_0069c6a8);
    uVar1 = color_rgb_float_to_int((float *)&DAT_007c140c);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x22,uVar1);
    rasterizer_set_shader_stage_config();
    return;
  }
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,1,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,2,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,5,uVar2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,6,uVar3);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,7,2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,1,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,2,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,5,uVar2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,6,uVar3);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,7,2);
  rasterizer_set_shader_stage_config();
  return;
}
#endif
