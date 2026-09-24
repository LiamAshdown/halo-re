// rasterizer_light_cone_set_texture_stage_states  (Ghidra: FUN_0051d6a0)
// address 0x51d6a0, size 889 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase2/results/rasterizer_01.json ("Configures the multi-stage texture and
// blend states used to draw volumetric light cones/shafts."). Purely a sequence of
// SetSamplerState (+0x114, confirmed elsewhere in this module) and SetRenderState (+0xe4,
// confirmed) calls with literal arguments, plus two calls to
// chimera__rasterizer_set_texture_direct_d3dx with literal stage/bitmap-index arguments; no
// register ambiguity anywhere in this function. `DAT_007c110c` is d3d_caps9.texture_address_caps
// (types/rasterizer.h notes bit 3 as D3DPTADDRESSCAPS_BORDER).
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f3;      // 0x006893f3, UNSURE meaning
extern void *rasterizer_device;                  // 0x0071d174

extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
// blam-cc: EAX -> bitmap_tag_id, EDI -> effect_slot, stack -> (stage, frame)
extern uint8_t chimera__rasterizer_set_texture_direct_d3dx(uint32_t bitmap_tag_id, int16_t stage, int16_t frame,
                                                           rasterizer_effect_slot *effect_slot); // 0x518700

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_light_cone_set_texture_stage_states(void)
{
    void **vtable;
    d3d_call3_fn set_sampler_state;
    d3d_call2_fn set_render_state;

    if (console_debug_toggle_6893e4 == 0 && console_debug_toggle_6893f3 != 0 &&
        0xffff0100 < rasterizer_caps.pixel_shader_version &&rasterizer_effects[4].effect != 0) {

        vtable = *(void ***)rasterizer_device;
        set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
        set_sampler_state(rasterizer_device, 0, 1, 1);
        set_sampler_state(rasterizer_device, 0, 2, 1);
        set_sampler_state(rasterizer_device, 0, 5, 2);
        set_sampler_state(rasterizer_device, 0, 6, 2);
        set_sampler_state(rasterizer_device, 0, 7, 2);
        set_sampler_state(rasterizer_device, 1, 1, 3);
        set_sampler_state(rasterizer_device, 1, 2, 3);
        set_sampler_state(rasterizer_device, 1, 3, 3);
        set_sampler_state(rasterizer_device, 1, 5, 2);
        set_sampler_state(rasterizer_device, 1, 6, 2);
        set_sampler_state(rasterizer_device, 1, 7, 2);

        chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->distance_attenuation.tag_id, 2, 0,
                                                    &rasterizer_effects[4]);
        if ((rasterizer_caps.texture_address_caps & 8) == 0) {
            set_sampler_state(rasterizer_device, 2, 1, 3);
            set_sampler_state(rasterizer_device, 2, 2, 3);
            set_sampler_state(rasterizer_device, 2, 3, 3);
        } else {
            set_sampler_state(rasterizer_device, 2, 4, 0);
            set_sampler_state(rasterizer_device, 2, 1, 4);
            set_sampler_state(rasterizer_device, 2, 2, 4);
            set_sampler_state(rasterizer_device, 2, 3, 4);
        }
        set_sampler_state(rasterizer_device, 2, 5, 2);
        set_sampler_state(rasterizer_device, 2, 6, 1);
        set_sampler_state(rasterizer_device, 2, 7, 1);

        chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->vector_normalization.tag_id, 3, 0,
                                                    &rasterizer_effects[4]);
        set_sampler_state(rasterizer_device, 3, 1, 3);
        set_sampler_state(rasterizer_device, 3, 2, 3);
        set_sampler_state(rasterizer_device, 3, 3, 3);
        set_sampler_state(rasterizer_device, 3, 5, 2);
        set_sampler_state(rasterizer_device, 3, 6, 1);
        set_sampler_state(rasterizer_device, 3, 7, 1);

        vtable = *(void ***)rasterizer_device;
        set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
        set_render_state(rasterizer_device, 0x16, 3);
        set_render_state(rasterizer_device, 0xa8, 7);
        set_render_state(rasterizer_device, 0x1b, 1);
        set_render_state(rasterizer_device, 0x13, 2);
        set_render_state(rasterizer_device, 0x14, 2);
        set_render_state(rasterizer_device, 0xab, 1);
        set_render_state(rasterizer_device, 0xf, 1);
        set_render_state(rasterizer_device, 0x18, 0);
        set_render_state(rasterizer_device, 7, 1);
        set_render_state(rasterizer_device, 0x17, 3);
        set_render_state(rasterizer_device, 0xe, 0);
        set_render_state(rasterizer_device, 0x1c, 0);
    }
}

#if 0
Original Ghidra decompilation (0x51d6a0):

void FUN_0051d6a0(void)

{
  if ((((DAT_006893e4 == 0) && (DAT_006893f3 != '\0')) && (0xffff0100 < DAT_007c118c)) &&
     (DAT_0069d490 != 0)) {
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,1,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,2,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,3,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,7,2);
    chimera__rasterizer_set_texture_direct_d3dx(2,0);
    if ((DAT_007c110c & 8) == 0) {
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,3,3);
    }
    else {
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,4,0);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,1,4);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,2,4);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,3,4);
    }
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,6,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,7,1);
    chimera__rasterizer_set_texture_direct_d3dx(3,0);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,1,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,2,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,3,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,6,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,7,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,2);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,2);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
  }
  return;
}
#endif
