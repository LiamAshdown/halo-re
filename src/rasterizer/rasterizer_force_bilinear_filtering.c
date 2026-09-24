// rasterizer_force_bilinear_filtering  (Ghidra: FUN_0051e9f0)
// address 0x51e9f0, size 295 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: out/phase2/results/rasterizer_01.json ("Forces bilinear texture filtering on the
// active texture stages before re-applying the current shader-stage configuration."). A clean,
// fully linear sequence of SetSamplerState (+0x114) and one SetRenderState (+0xe4) call with no
// register ambiguity, ending with the already-named rasterizer_set_shader_stage_config.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern void *rasterizer_device;                       // 0x0071d174
// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200

typedef int32_t (*d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (*d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_force_bilinear_filtering(void)
{
    void **vtable;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_sampler_state;

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        vtable = *(void ***)rasterizer_device;
        set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
        set_render_state(rasterizer_device, 0x89, 0);
    }

    vtable = *(void ***)rasterizer_device;
    set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
    set_sampler_state(rasterizer_device, 0, 6, 2);
    set_sampler_state(rasterizer_device, 1, 6, 2);
    set_sampler_state(rasterizer_device, 0, 5, 2);
    set_sampler_state(rasterizer_device, 1, 5, 2);
    set_sampler_state(rasterizer_device, 0, 10, 1);
    set_sampler_state(rasterizer_device, 1, 10, 1);

    if (0xffff0100 < rasterizer_caps.pixel_shader_version) {
        set_sampler_state(rasterizer_device, 2, 6, 2);
        set_sampler_state(rasterizer_device, 3, 6, 2);
        set_sampler_state(rasterizer_device, 2, 5, 2);
        set_sampler_state(rasterizer_device, 3, 5, 2);
        set_sampler_state(rasterizer_device, 2, 10, 1);
        set_sampler_state(rasterizer_device, 3, 10, 1);
    }

    rasterizer_set_shader_stage_config(2);
}

#if 0
Original Ghidra decompilation (0x51e9f0):

void FUN_0051e9f0(void)

{
  if (DAT_007c118c < 0xffff0101) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,0);
  }
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,2);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,10,1);
  (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,10,1);
  if (0xffff0100 < DAT_007c118c) {
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,10,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,10,1);
  }
  rasterizer_set_shader_stage_config();
  return;
}
#endif
