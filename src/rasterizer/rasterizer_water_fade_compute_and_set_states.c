// rasterizer_water_fade_compute_and_set_states  (Ghidra: FUN_0051eb20)
// address 0x51eb20, size 818 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: out/phase2/results/rasterizer_01.json ("Computes the water-surface fade factor from
// the current clip plane and configures the refraction texture stages for the ripple effect.").
// The plane/camera arithmetic matches types/rasterizer.h exactly: DAT_007c1228/2c/30 is
// render_camera.position (window +0x8), DAT_007c1428.. is render_fog.plane (window +0x1e8+0x20),
// DAT_007c1420 is render_fog.atmospheric_maximum_distance (+0x1e8+0x18), DAT_007c144c is
// render_fog.planar_maximum_depth (+0x1e8+0x44), and DAT_007c1408 is render_fog.flags
// (+0x1e8+0x00) -- bit 0x02 tested here is not the documented `_render_fog_no_planar_bit`
// (0x04), so it is a second, undocumented flags bit. No register ambiguity anywhere in this
// function.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t rasterizer_fog_enabled;                              // 0x0069c6a8 latched by rasterizer_set_fog_constants
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern float water_fade_plane_distance;        // 0x006e09ec, UNSURE owner
extern float water_fade_factor_a;              // 0x006e09f0, UNSURE owner
extern float water_fade_factor_b;              // 0x006e09f4, UNSURE owner
extern void *rasterizer_device;                // 0x0071d174

extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
// blam-cc: EAX -> bitmap_tag_id, EDI -> effect_slot, stack -> (stage, frame)
extern uint8_t chimera__rasterizer_set_texture_direct_d3dx(uint32_t bitmap_tag_id, int16_t stage, int16_t frame,
                                                           rasterizer_effect_slot *effect_slot); // 0x518700

typedef int32_t (*d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (*d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

void rasterizer_water_fade_compute_and_set_states(void)
{
    void **vtable;
    d3d_call2_fn set_render_state;
    d3d_call3_fn set_sampler_state;
    float plane_distance;

    if (console_debug_toggle_6893e4 != 0 || rasterizer_fog_enabled == 0 ||
        rasterizer_caps.pixel_shader_version <= 0xffff0100) {
        return;
    }

    plane_distance = rasterizer_window.fog.plane.normal.i * rasterizer_window.camera.position.x +
                      rasterizer_window.fog.plane.normal.j * rasterizer_window.camera.position.y +
                      rasterizer_window.fog.plane.normal.k * rasterizer_window.camera.position.z -
                      rasterizer_window.fog.plane.d;
    water_fade_plane_distance = plane_distance;

    water_fade_factor_a = plane_distance / rasterizer_window.fog.atmospheric_maximum_distance;
    if (0.0f <= water_fade_factor_a) {
        if (1.0f < water_fade_factor_a) {
            water_fade_factor_a = 1.0f;
        }
    } else {
        water_fade_factor_a = 0.0f;
    }

    water_fade_factor_b = -(plane_distance / rasterizer_window.fog.planar_maximum_depth);
    if (0.0f <= water_fade_factor_b) {
        if (1.0f < water_fade_factor_b) {
            water_fade_factor_b = 1.0f;
        }
    } else {
        water_fade_factor_b = 0.0f;
    }

    if ((rasterizer_window.fog.flags & 2) != 0) { // UNSURE: undocumented flags bit, see header
        water_fade_factor_a = 1.0f;
    }

    if (rasterizer_effects[112].effect != 0) {
        vtable = *(void ***)rasterizer_device;
        set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
        set_render_state(rasterizer_device, 0x16, 3);
        set_render_state(rasterizer_device, 0xa8, 7);
        set_render_state(rasterizer_device, 0x1b, 1);
        set_render_state(rasterizer_device, 0xab, 1);
        set_render_state(rasterizer_device, 0xf, 1);
        set_render_state(rasterizer_device, 0x18, 0);
        set_render_state(rasterizer_device, 7, 1);
        set_render_state(rasterizer_device, 0x17, 3);
        set_render_state(rasterizer_device, 0xe, 0);

        if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
            set_render_state(rasterizer_device, 0x1c, 1);
            set_render_state(rasterizer_device, 0x13, 2);
            set_render_state(rasterizer_device, 0x14, 4);
            return;
        }

        chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->atmospheric_fog_density.tag_id, 0, 0,
                                                    &rasterizer_effects[112]);
        vtable = *(void ***)rasterizer_device;
        set_sampler_state = (d3d_call3_fn)vtable[0x45]; // +0x114
        set_sampler_state(rasterizer_device, 0, 1, 3);
        set_sampler_state(rasterizer_device, 0, 2, 3);
        set_sampler_state(rasterizer_device, 0, 5, 2);
        set_sampler_state(rasterizer_device, 0, 6, 2);
        set_sampler_state(rasterizer_device, 0, 7, 2);

        chimera__rasterizer_set_texture_direct_d3dx(*(uint32_t *)&rasterizer_globals_data->planar_fog_density.tag_id, 1, 0,
                                                    &rasterizer_effects[112]);
        set_sampler_state(rasterizer_device, 1, 1, 3);
        set_sampler_state(rasterizer_device, 1, 2, 3);
        set_sampler_state(rasterizer_device, 1, 5, 2);
        set_sampler_state(rasterizer_device, 1, 6, 2);
        set_sampler_state(rasterizer_device, 1, 7, 2);

        vtable = *(void ***)rasterizer_device;
        set_render_state = (d3d_call2_fn)vtable[0x39]; // +0xe4
        set_render_state(rasterizer_device, 0x1c, 0);
        set_render_state(rasterizer_device, 0x13, 2);
        set_render_state(rasterizer_device, 0x14, 6);
    }
}

#if 0
Original Ghidra decompilation (0x51eb20):

void FUN_0051eb20(void)

{
  if (((DAT_006893e4 == 0) && (DAT_0069c6a8 != '\0')) && (0xffff0100 < DAT_007c118c)) {
    _DAT_006e09ec =
         (_DAT_007c1428 * DAT_007c1228 + _DAT_007c142c * DAT_007c122c + _DAT_007c1430 * DAT_007c1230
         ) - _DAT_007c1434;
    DAT_006e09f0 = _DAT_006e09ec / DAT_007c1420;
    if (0.0 <= DAT_006e09f0) {
      if (1.0 < DAT_006e09f0) {
        DAT_006e09f0 = 1.0;
      }
    }
    else {
      DAT_006e09f0 = 0.0;
    }
    _DAT_006e09f4 = -(_DAT_006e09ec / _DAT_007c144c);
    if (0.0 <= _DAT_006e09f4) {
      if (1.0 < _DAT_006e09f4) {
        _DAT_006e09f4 = 1.0;
      }
    }
    else {
      _DAT_006e09f4 = 0.0;
    }
    if ((DAT_007c1408 & 2) != 0) {
      DAT_006e09f0 = 1.0;
    }
    if (DAT_0069e210 != 0) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,1);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,3);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,0);
      if (DAT_007c118c < 0xffff0101) {
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,1);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,2);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,4);
        return;
      }
      chimera__rasterizer_set_texture_direct_d3dx(0,0);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
      chimera__rasterizer_set_texture_direct_d3dx(1,0);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,7,2);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,2);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,6);
    }
  }
  return;
}
#endif
