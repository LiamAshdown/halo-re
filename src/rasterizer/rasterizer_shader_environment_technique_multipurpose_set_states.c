// rasterizer_shader_environment_technique_multipurpose_set_states  (Ghidra: FUN_00520790)
// address 0x520790, size 369 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: out/phase2/results/rasterizer_01.json ("Configures fixed-function multitexture
// render/texture-stage states for one shader_environment technique variant and records the
// active technique table pointer."); types/rasterizer.h "global 0x006e1780:
// environment_techniques_multipurpose[24]" matches DAT_0069d890 being recorded here (name
// chosen for the "multipurpose" table since this is the first, gate-0 variant in address
// order). A clean, fully linear sequence of SetRenderState/SetSamplerState calls, unconditionally
// followed by recording the active technique table pointer.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"


extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f8; // 0x006893f8
extern uint8_t console_debug_toggle_6893fa; // 0x006893fa
extern int16_t renderer_unknown_69c67c;                             // 0x0069c67c UNSURE (read as a word)
extern void *rasterizer_device;             // 0x0071d174
// UNSURE: per rasterizer_shader_environment_technique_draw.c (0x520970), the table pointer is
// really a pointer to a {effect (void*), vertex_shader_index (int32_t)} pair, not a bare
// int32_t*; declared as the pair type here and there instead of guessing further structure.
extern rasterizer_effect_slot *rasterizer_active_environment_effect; // 0x0071d1d0, also cleared by the render module (0x50c351)
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410; effect 36 is 0x0069d890

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_shader_environment_technique_multipurpose_set_states(void)
{
    void **vtable;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_sampler_state;

    if (console_debug_toggle_6893e4 == 0 && console_debug_toggle_6893f8 != 0 &&
        console_debug_toggle_6893fa != 0 && renderer_unknown_69c67c == 0) {
        vtable = *(void ***)rasterizer_device;
        set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
        set_render_state(rasterizer_device, 0x16, 3);
        set_render_state(rasterizer_device, 0xa8, 8);
        set_render_state(rasterizer_device, 0x1b, 1);
        set_render_state(rasterizer_device, 0x13, 7);
        set_render_state(rasterizer_device, 0x14, 1);
        set_render_state(rasterizer_device, 0xab, 1);
        set_render_state(rasterizer_device, 0xf, 0);
        set_render_state(rasterizer_device, 7, 1);
        set_render_state(rasterizer_device, 0x17, 3);
        set_render_state(rasterizer_device, 0xe, 0);
        set_render_state(rasterizer_device, 0x1c, 0);

        vtable = *(void ***)rasterizer_device;
        set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
        set_sampler_state(rasterizer_device, 0, 1, 3);
        set_sampler_state(rasterizer_device, 0, 2, 3);
        set_sampler_state(rasterizer_device, 0, 5, 2);
        set_sampler_state(rasterizer_device, 0, 6, 2);
        set_sampler_state(rasterizer_device, 0, 7, 2);
    }
    rasterizer_active_environment_effect = &rasterizer_effects[36];
}

#if 0
Original Ghidra decompilation (0x520790):

void FUN_00520790(void)

{
  if ((((DAT_006893e4 == 0) && (DAT_006893f8 != '\0')) && (DAT_006893fa != '\0')) &&
     (DAT_0069c67c == 0)) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,8);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
  }
  DAT_0071d1d0 = &DAT_0069d890;
  return;
}
#endif
