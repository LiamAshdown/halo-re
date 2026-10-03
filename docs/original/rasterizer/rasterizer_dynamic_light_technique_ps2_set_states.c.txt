// rasterizer_dynamic_light_technique_ps2_set_states  (Ghidra: FUN_00521cc0)
// address 0x521cc0, size 720 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// evidence: out/phase2/results/rasterizer_01.json ("Configures fixed-function render/texture-
// stage states for another higher-tier shader_environment technique variant."); named for the
// dynamic-light draw pass since it sits directly before FUN_00521f90 ("Draws an additional
// (higher pixel-shader tier) dynamic-light contribution pass") in address order and shares its
// gate shape (render_force_flag == 0, pixel_shader_version > ps_1_3). A clean, fully
// linear sequence of SetRenderState/SetSamplerState calls with no register ambiguity.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f7; // 0x006893f7
extern int16_t render_force_flag;                             // 0x0069c67c UNSURE (read as a word)
extern void *rasterizer_device;             // 0x0071d174

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_dynamic_light_technique_ps2_set_states(void)
{
    void **vtable;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_sampler_state;

    if (console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f7 == 0 ||
        render_force_flag != 0 || rasterizer_caps.pixel_shader_version <= 0xffff0103) {
        return;
    }

    vtable = *(void ***)rasterizer_device;
    set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
    set_render_state(rasterizer_device, 0x16, 3);
    set_render_state(rasterizer_device, 0xa8, 7);
    set_render_state(rasterizer_device, 0x1b, 1);
    set_render_state(rasterizer_device, 0x13, 7);
    set_render_state(rasterizer_device, 0x14, 2);
    set_render_state(rasterizer_device, 0xab, 1);
    set_render_state(rasterizer_device, 0xf, 1);
    set_render_state(rasterizer_device, 0x18, 0);
    set_render_state(rasterizer_device, 7, 1);
    set_render_state(rasterizer_device, 0x17, 3);
    set_render_state(rasterizer_device, 0xe, 0);
    set_render_state(rasterizer_device, 0x1c, 0);

    vtable = *(void ***)rasterizer_device;
    set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
    set_sampler_state(rasterizer_device, 0, 1, 1);
    set_sampler_state(rasterizer_device, 0, 2, 1);
    set_sampler_state(rasterizer_device, 0, 5, 2);
    set_sampler_state(rasterizer_device, 0, 6, 2);
    set_sampler_state(rasterizer_device, 0, 7, 2);
    set_sampler_state(rasterizer_device, 1, 1, 3);
    set_sampler_state(rasterizer_device, 1, 2, 3);
    set_sampler_state(rasterizer_device, 1, 5, 2);
    set_sampler_state(rasterizer_device, 1, 6, 2);
    set_sampler_state(rasterizer_device, 1, 7, 2);
    set_sampler_state(rasterizer_device, 2, 1, 3);
    set_sampler_state(rasterizer_device, 2, 2, 3);
    set_sampler_state(rasterizer_device, 2, 3, 3);
    set_sampler_state(rasterizer_device, 2, 5, 2);
    set_sampler_state(rasterizer_device, 2, 6, 2);
    set_sampler_state(rasterizer_device, 2, 7, 2);
    set_sampler_state(rasterizer_device, 3, 1, 3);
    set_sampler_state(rasterizer_device, 3, 2, 3);
    set_sampler_state(rasterizer_device, 3, 3, 3);
    set_sampler_state(rasterizer_device, 3, 5, 2);
    set_sampler_state(rasterizer_device, 3, 6, 2);
    set_sampler_state(rasterizer_device, 3, 7, 2);
}

#if 0
Original Ghidra decompilation (0x521cc0):

void FUN_00521cc0(void)

{
  if ((((DAT_006893e4 == 0) && (DAT_006893f7 != '\0')) && (DAT_0069c67c == 0)) &&
     (0xffff0103 < DAT_007c118c)) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,2);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,1,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,2,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,7,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,1,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,2,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,3,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,7,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,1,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,2,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,3,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,7,2);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
