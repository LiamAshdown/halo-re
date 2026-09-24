// rasterizer_underwater_tint_set_states  (Ghidra: FUN_0051f030)
// address 0x51f030, size 729 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: out/phase2/results/rasterizer_01.json ("Configures the render and texture-stage
// states for drawing the full-screen underwater tint/fog overlay."). A clean, fully linear
// sequence of SetRenderState (+0xe4), SetMaterial (+0xc4, index 49, single D3DMATERIAL9*
// argument) and SetSamplerState (+0x114) calls with no register ambiguity.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern uint8_t console_debug_toggle_6893f1; // 0x006893f1, gates this whole function
extern void *rasterizer_device;             // 0x0071d174
extern uint8_t rasterizer_underwater_material[0x44]; // 0x0069c6b8, D3DMATERIAL9, UNSURE owner
extern uint8_t rasterizer_fog_enabled;                              // 0x0069c6a8 latched by rasterizer_set_fog_constants
extern uint32_t renderer_unknown_69c684;    // 0x0069c684, UNSURE meaning

typedef int32_t (__stdcall *d3d_call1p_fn)(void *self, const void *a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_underwater_tint_set_states(void)
{
    void **vtable;
    d3d_call1p_fn set_material;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_sampler_state;
    uint32_t stage7_index;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }

    vtable = *(void ***)rasterizer_device;
    set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
    set_render_state(rasterizer_device, 0x16, 3);
    set_render_state(rasterizer_device, 0xa8, 0xf);
    set_render_state(rasterizer_device, 0x1b, 0);
    set_render_state(rasterizer_device, 0x18, 0x7f);
    set_render_state(rasterizer_device, 7, 1);
    set_render_state(rasterizer_device, 0x17, 4);
    set_render_state(rasterizer_device, 0xe, 1);

    vtable = *(void ***)rasterizer_device;
    set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        vtable = *(void ***)rasterizer_device;
        set_material = (d3d_call1p_fn)vtable[0x31]; // +0xc4
        set_material(rasterizer_device, rasterizer_underwater_material);

        set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
        set_render_state(rasterizer_device, 0x89, 1);
        set_render_state(rasterizer_device, 0x1c, rasterizer_fog_enabled);
        set_render_state(rasterizer_device, 0x22, 0xffffffff);
        set_render_state(rasterizer_device, 0x8b, renderer_unknown_69c684);

        set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
        set_sampler_state(rasterizer_device, 1, 1, 3);
        set_sampler_state(rasterizer_device, 1, 2, 3);
        set_sampler_state(rasterizer_device, 1, 5, 2);
        set_sampler_state(rasterizer_device, 1, 6, 2);
        stage7_index = 1;
    } else {
        set_render_state(rasterizer_device, 0x1c, 0);
        set_sampler_state(rasterizer_device, 1, 1, 1);
        set_sampler_state(rasterizer_device, 1, 2, 1);
        set_sampler_state(rasterizer_device, 2, 1, 3);
        set_sampler_state(rasterizer_device, 2, 2, 3);
        set_sampler_state(rasterizer_device, 2, 5, 2);
        set_sampler_state(rasterizer_device, 2, 6, 2);
        set_sampler_state(rasterizer_device, 2, 7, 2);
        set_sampler_state(rasterizer_device, 3, 1, 3);
        set_sampler_state(rasterizer_device, 3, 2, 3);
        set_sampler_state(rasterizer_device, 3, 3, 3);
        set_sampler_state(rasterizer_device, 3, 5, 2);
        set_sampler_state(rasterizer_device, 3, 6, 2);
        stage7_index = 3;
    }

    set_sampler_state(rasterizer_device, stage7_index, 7, 2);
    set_sampler_state(rasterizer_device, 0, 1, 1);
    set_sampler_state(rasterizer_device, 0, 2, 1);
    set_sampler_state(rasterizer_device, 0, 5, 2);
    set_sampler_state(rasterizer_device, 0, 6, 2);
    set_sampler_state(rasterizer_device, 0, 7, 2);
}

#if 0
Original Ghidra decompilation (0x51f030):

void FUN_0051f030(void)

{
  undefined4 uVar1;

  if (DAT_006893f1 != '\0') {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,0xf);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0x7f);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,4);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,1);
    if (DAT_007c118c < 0xffff0101) {
      (**(code **)(*DAT_0071d174 + 0xc4))(DAT_0071d174,&DAT_0069c6b8);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,1);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,DAT_0069c6a8);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x22,0xffffffff);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x8b,DAT_0069c684);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,2);
      uVar1 = 1;
    }
    else {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,1,1);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,2,1);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,7,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,3,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,6,2);
      uVar1 = 3;
    }
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,uVar1,7,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
  }
  return;
}
#endif
