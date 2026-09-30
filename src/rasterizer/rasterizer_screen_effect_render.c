// rasterizer_screen_effect_render  (Ghidra: rasterizer_screen_effect_video_technique_select; the
//   earlier placeholder kept that name)
// address 0x52d8a0, size 2599 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: rebuilt from the raw disassembly; Ghidra lost the stack frame (the parameter block
//   pointer is kept in the argument slot), every device and ID3DXEffect call argument and the
//   technique selection. Caller: first_person_weapon_update_screen_effects 0x494730, twice, with
//   its weapon_screen_effect_parameters block, which cinematic_screen_effect_update 0x512360 (EAX in, EAX
//   out) merges into the block this function reads. Effect 114 (0x0069e250) is the screen effect
//   ID3DXEffect; its techniques are screen_effect_techniques 0x0071d210 (VideoOn ..
//   VideoOffConvolvedFilterDesaturation, 0x52d740) and its first two constant handles take the
//   desaturation and night vision colors.
// What it does: (count + 1) * 2 passes that ping-pong between render targets 1 and 2 (a single
//   pass, count 0 with one pass, draws straight to the window target), each drawing the static
//   full screen quad 0x006e1a30 with the source target size in its texture coordinates. Odd
//   passes of a block with the two extra video maps (+0x23) bind the target, +0x28 and +0x34 and
//   draw technique 0 with a noise scale; other passes bind the mask and the target to up to four
//   stages and draw either the current technique (no convolution) or one of the
//   night vision / desaturation techniques on the last pass. The last pass blends onto the frame.
// register convention: stack -> input (weapon_screen_effect_parameters*, handed to 0x512360 in EAX).
// blam-cc: stack -> input
// UNSURE: the source target is rasterizer_render_targets[-1] (the 0x14 bytes before the table)
//   when there is a single pass; the binary reads it for the quad texture coordinates.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern uint8_t rasterizer_software_vertex_processing;       // 0x0069c680
extern uint8_t console_debug_toggle_689428;                 // 0x00689428 screen effects enabled
extern uint32_t config_use_alternate_convolve_mask;         // 0x00722b78 UNSURE: picks technique 3 over 2
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern uint32_t screen_effect_techniques[k_rasterizer_screen_effect_techniques]; // 0x0071d210
extern rasterizer_dynamic_screen_vertex rasterizer_screen_effect_quad[4];        // 0x006e1a30

// blam-cc: EAX -> input, returns EAX
extern weapon_screen_effect_parameters *cinematic_screen_effect_update(weapon_screen_effect_parameters *input); // 0x512360
// blam-cc: ESI -> bitmap, EDI -> effect_slot, stack -> stage
extern uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot); // 0x5186c0
// blam-cc: AX -> target_index, EDX -> effect_slot, stack -> handle_index
extern void rasterizer_render_target_bind_effect_texture(int16_t target_index, rasterizer_effect_slot *effect_slot,
                                                         int16_t handle_index); // 0x52ce10
// blam-cc: EAX -> target_index, stack -> (clear_color, clear)
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear); // 0x52ccc0
// blam-cc: ECX -> width, EAX -> height, stack -> (params, pass, pass_count, shift_down)
extern void rasterizer_screen_effect_compute_uv_transform(uint32_t width, uint32_t height,
                                                          weapon_screen_effect_parameters *params, int16_t pass,
                                                          int16_t pass_count, uint8_t shift_down); // 0x52ce50

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t count, const void *data, uint32_t stride);
typedef int32_t (__stdcall *d3dx_effect_set_vector_fn)(void *effect, uint32_t handle, const float *vector);
typedef int32_t (__stdcall *d3dx_set_technique_fn)(void *effect, uint32_t technique);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}
static void set_sampler_states(uint32_t sampler, uint32_t address, uint32_t filter, uint32_t mip_filter)
{
    set_sampler_state(sampler, 1, address);
    set_sampler_state(sampler, 2, address);
    set_sampler_state(sampler, 5, filter);
    set_sampler_state(sampler, 6, filter);
    set_sampler_state(sampler, 7, mip_filter);
}
static void *screen_effect(void) { return (void *)(uintptr_t)rasterizer_effects[114].effect; }
static void draw_screen_quad(void)
{
    ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, rasterizer_screen_effect_quad,
                                                           sizeof(rasterizer_dynamic_screen_vertex));
}
static void set_technique(uint32_t technique)
{
    ((d3dx_set_technique_fn)(*(void ***)screen_effect())[0xec / 4])(screen_effect(), technique);
}
static void set_vector(int handle_index, const float *vector)
{
    uint32_t *handles = (uint32_t *)(uintptr_t)rasterizer_effects[114].constant_handles;

    ((d3dx_effect_set_vector_fn)(*(void ***)screen_effect())[0x88 / 4])(screen_effect(), handles[handle_index], vector);
}
static void set_blend(uint32_t source, uint32_t destination)
{
    set_render_state(0x1b, 1);             // ALPHABLENDENABLE
    set_render_state(0x13, source);        // SRCBLEND
    set_render_state(0x14, destination);   // DESTBLEND
    set_render_state(0xab, 1);             // BLENDOP ADD
}

// The masked/unmasked night vision and desaturation technique for the last pass, or 0.
static uint32_t select_filter_technique(const weapon_screen_effect_parameters *p, int first)
{
    uint32_t technique = 0;

    if (p->night_vision_masked && p->desaturation_masked &&
        p->night_vision_intensity > 0.0f && p->desaturation_intensity > 0.0f) {
        technique = screen_effect_techniques[first];
    } else if (p->night_vision_masked && p->night_vision_intensity > 0.0f) {
        technique = screen_effect_techniques[first + 1];
    } else if (p->desaturation_masked && p->desaturation_intensity > 0.0) {
        technique = screen_effect_techniques[first + 2];
    }
    return technique;
}

void rasterizer_screen_effect_render(weapon_screen_effect_parameters *input)
{
    weapon_screen_effect_parameters *p;
    uint8_t *raw;
    int16_t pass_count;
    int16_t pass;
    int16_t source;
    int16_t destination;
    uint32_t source_width, source_height;
    uint32_t passes;
    uint32_t effect_pass;

    p = cinematic_screen_effect_update(input);
    if (p == NULL) {
        return;
    }
    raw = (uint8_t *)p;
    if (p->convolution_type == 0 && p->mask_bitmap_data == 0 && !(p->night_vision_intensity > 0.0f) &&
        !(p->desaturation_intensity > 0.0f) && raw[0x23] == 0) {
        return;
    }
    if (!console_debug_toggle_689428 || rasterizer_window.type != 1) {
        return;
    }
    pass_count = (int16_t)((uint16_t)(p->convolution_extra_passes + 1) << 1);

    rasterizer_screen_effect_quad[0].color = 0xffffffff;
    rasterizer_screen_effect_quad[1].color = 0xffffffff;
    rasterizer_screen_effect_quad[2].color = 0xffffffff;
    rasterizer_screen_effect_quad[3].color = 0xffffffff;
    rasterizer_screen_effect_quad[0].x = -1.0f;
    rasterizer_screen_effect_quad[0].y = -1.0f;
    rasterizer_screen_effect_quad[1].x = 1.0f;
    rasterizer_screen_effect_quad[1].y = -1.0f;
    rasterizer_screen_effect_quad[2].x = 1.0f;
    rasterizer_screen_effect_quad[2].y = 1.0f;
    rasterizer_screen_effect_quad[3].x = -1.0f;
    rasterizer_screen_effect_quad[3].y = 1.0f;
    rasterizer_screen_effect_quad[3].z = 0.0f;
    rasterizer_screen_effect_quad[2].z = 0.0f;
    rasterizer_screen_effect_quad[1].z = 0.0f;
    rasterizer_screen_effect_quad[0].z = 0.0f;
    if (screen_effect() == NULL) {
        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
        return;
    }
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
                                               rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                               ((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[0].shader);

    for (pass = 0; pass < pass_count; pass++) {
        if (pass_count == 1) {
            source = -1;
            destination = -1;
        } else if ((pass & 1) == 0) {
            source = 1;
            destination = 2;
        } else {
            source = 2;
            destination = 1;
        }
        source_width = rasterizer_render_targets[source].width;
        source_height = rasterizer_render_targets[source].height;
        rasterizer_screen_effect_quad[0].u = 0.0f;
        rasterizer_screen_effect_quad[0].v = (float)source_height;
        rasterizer_screen_effect_quad[1].u = (float)source_width;
        rasterizer_screen_effect_quad[1].v = (float)source_height;
        rasterizer_screen_effect_quad[2].u = (float)source_width;
        rasterizer_screen_effect_quad[2].v = 0.0f;
        rasterizer_screen_effect_quad[3].u = 0.0f;
        rasterizer_screen_effect_quad[3].v = 0.0f;

        if ((pass & 1) && raw[0x23]) {
            // video noise pass: the target, then the two video maps
            rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], 0);
            set_sampler_states(0, 3, 1, 1);
            rasterizer_bind_texture_d3dx(1, (BitmapData *)(uintptr_t)*(uint32_t *)(raw + 0x28), &rasterizer_effects[114]);
            set_sampler_states(1, 3, 1, 1);
            rasterizer_bind_texture_d3dx(2, (BitmapData *)(uintptr_t)*(uint32_t *)(raw + 0x34), &rasterizer_effects[114]);
            set_sampler_states(2, 1, 2, 1);
        } else {
            int16_t stage;
            BitmapData *mask = (BitmapData *)(uintptr_t)p->mask_bitmap_data;

            for (stage = 0; stage < 4; stage++) {
                if (p->convolution_type == 0) {
                    if (pass_count == 1) {
                        if (stage != 0) {
                            continue;
                        }
                        rasterizer_bind_texture_d3dx(0, mask, &rasterizer_effects[114]);
                    } else if (pass == 0) {
                        if (stage != 0) {
                            continue;
                        }
                        rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], 0);
                    } else if (pass == 1) {
                        if (mask == NULL) {
                            if (stage != 0) {
                                continue;
                            }
                            rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], 0);
                        } else if (stage == 0) {
                            rasterizer_bind_texture_d3dx(0, mask, &rasterizer_effects[114]);
                        } else {
                            rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], stage);
                        }
                    }
                    // later passes of an unconvolved effect only reset the samplers
                } else if (mask != NULL) {
                    if (stage == 0) {
                        rasterizer_bind_texture_d3dx(0, mask, &rasterizer_effects[114]);
                    } else {
                        rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], stage);
                    }
                } else {
                    if (stage != 0) {
                        break;
                    }
                    rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], 0);
                }
                set_sampler_states((uint32_t)stage, 3, 2, 1);
            }
        }

        set_render_state(0x16, 1);    // CULLMODE NONE
        set_render_state(0xa8, 7);
        set_render_state(0x1b, 0);
        set_render_state(0x0f, 0);
        set_render_state(0x07, 0);
        set_render_state(0x1c, 0);
        rasterizer_screen_effect_compute_uv_transform(source_width, source_height, p, pass, pass_count, 0);
        if (destination != -1) {
            rasterizer_render_target_set_active(destination, 0, 0);
        }

        if (raw[0x23]) {
            uint32_t technique = screen_effect_techniques[0];

            if (pass == 1) {
                static const int32_t k_noise_scales[3] = { 1, 2, 4 };
                float noise[4];
                float amount = *(float *)(raw + 0x2c);

                noise[0] = (float)k_noise_scales[*(int16_t *)(raw + 0x24)];
                noise[1] = noise[0];
                noise[2] = noise[0];
                noise[3] = amount < 0.0f ? 0.0f : (amount > 1.0f ? 1.0f : amount);
                if (rasterizer_effects[114].constant_handles != 0) {
                    set_vector(0, noise);
                }
                set_blend(6, 1);          // INVSRCALPHA, ZERO
            }
            set_technique(technique);
            ((d3dx_effect_begin_fn)(*(void ***)screen_effect())[0x100 / 4])(screen_effect(), &passes, 3);
            ((d3dx_effect_pass_fn)(*(void ***)screen_effect())[0x104 / 4])(screen_effect(), (uint32_t)(int32_t)pass);
            draw_screen_quad();
            ((d3dx_effect_end_fn)(*(void ***)screen_effect())[0x108 / 4])(screen_effect());
            continue;
        }

        if (p->convolution_type != 0) {
            float tint[4];
            float night_vision[4];
            uint32_t technique;

            tint[0] = p->desaturation_tint[0];
            tint[1] = p->desaturation_tint[1];
            tint[2] = p->desaturation_tint[2];
            tint[3] = p->desaturation_intensity;
            night_vision[0] = p->night_vision_intensity;
            night_vision[1] = p->night_vision_intensity;
            night_vision[2] = p->night_vision_intensity;
            night_vision[3] = p->night_vision_intensity;
            if (p->mask_bitmap_data != 0) {
                technique = (pass == pass_count - 1) ? select_filter_technique(p, 4) : 0;
                if (technique == 0) {
                    technique = config_use_alternate_convolve_mask ? screen_effect_techniques[3]
                                                                   : screen_effect_techniques[2];
                }
                set_technique(technique);
                if (rasterizer_effects[114].constant_handles != 0) {
                    set_vector(0, tint);
                    set_vector(1, night_vision);
                }
            } else {
                technique = (pass == pass_count - 1) ? select_filter_technique(p, 8) : 0;
                if (technique == 0) {
                    technique = screen_effect_techniques[7];
                }
                set_technique(technique);
            }
        }
        if (pass_count == 1) {
            set_blend(1, 6);              // ZERO, INVSRCALPHA
        } else if (pass == pass_count - 1) {
            set_blend(6, 1);              // INVSRCALPHA, ZERO
        }
        ((d3dx_effect_begin_fn)(*(void ***)screen_effect())[0x100 / 4])(screen_effect(), &passes, 3);
        for (effect_pass = 0; effect_pass < passes; effect_pass++) {
            ((d3dx_effect_pass_fn)(*(void ***)screen_effect())[0x104 / 4])(screen_effect(), effect_pass);
            draw_screen_quad();
        }
        ((d3dx_effect_end_fn)(*(void ***)screen_effect())[0x108 / 4])(screen_effect());
    }
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x52d8a0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void rasterizer_screen_effect_video_technique_select(void)

{
  int iVar1;
  short sVar2;
  short *extraout_EAX;
  int iVar3;
  short sVar4;
  int *piVar5;
  uint uVar6;
  bool bVar7;
  undefined4 uVar8;
  int *piVar9;
  uint uVar10;
  int *piStack_8c;
  int *piStack_88;
  undefined4 uStack_84;
  int *piStack_80;
  int *piStack_7c;
  int *piStack_78;
  int *apiStack_74 [6];
  int local_4c;
  undefined1 *puStack_44;
  
  screen_effect_update();
  if ((((extraout_EAX != (short *)0x0) &&
       ((((extraout_EAX[1] != 0 || (*(int *)(extraout_EAX + 4) != 0)) ||
         (0.0 < *(float *)(extraout_EAX + 6))) ||
        ((0.0 < *(float *)(extraout_EAX + 8) || (*(char *)((int)extraout_EAX + 0x23) != '\0'))))))
      && (DAT_00689428 != '\0')) && ((short)DAT_007c1220 == 1)) {
    _DAT_006e1a3c = 0xffffffff;
    _DAT_006e1a54 = 0xffffffff;
    _DAT_006e1a6c = 0xffffffff;
    _DAT_006e1a84 = 0xffffffff;
    local_4c = (uint)(ushort)(*extraout_EAX + 1) << 1;
    _DAT_006e1a30 = 0xbf800000;
    _DAT_006e1a34 = 0xbf800000;
    _DAT_006e1a48 = 0x3f800000;
    _DAT_006e1a4c = 0xbf800000;
    _DAT_006e1a60 = 0x3f800000;
    _DAT_006e1a64 = 0x3f800000;
    _DAT_006e1a78 = 0xbf800000;
    _DAT_006e1a7c = 0x3f800000;
    _DAT_006e1a80 = 0;
    _DAT_006e1a68 = 0;
    _DAT_006e1a50 = 0;
    _DAT_006e1a38 = 0;
    if (DAT_0069e250 != (int *)0x0) {
      apiStack_74[5] = (int *)0x52d9db;
      (**(code **)(*DAT_0071d174 + 0x15c))();
      apiStack_74[5] = (int *)(-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10);
      apiStack_74[4] = DAT_0071d174;
      apiStack_74[3] = (int *)0x52da02;
      (**(code **)(*DAT_0071d174 + 0x134))();
      apiStack_74[3] = DAT_0069e350;
      apiStack_74[2] = DAT_0071d174;
      apiStack_74[1] = (int *)0x52da17;
      (**(code **)(*DAT_0071d174 + 0x170))();
      puStack_44 = (undefined1 *)0x0;
      if (0 < (short)local_4c) {
        do {
          sVar2 = (short)local_4c;
          if ((short)local_4c == 1) {
            sVar4 = -1;
          }
          else if (((uint)puStack_44 & 1) == 0) {
            sVar4 = 1;
          }
          else {
            sVar4 = 2;
          }
          _DAT_006e1a44 = (float)*(int *)(&DAT_0069d35c + sVar4 * 0x14);
          _DAT_006e1a40 = 0;
          if (*(int *)(&DAT_0069d35c + sVar4 * 0x14) < 0) {
            _DAT_006e1a44 = _DAT_006e1a44 + 4.2949673e+09;
          }
          _DAT_006e1a58 = (float)*(int *)(&DAT_0069d358 + sVar4 * 0x14);
          if (*(int *)(&DAT_0069d358 + sVar4 * 0x14) < 0) {
            _DAT_006e1a58 = _DAT_006e1a58 + 4.2949673e+09;
          }
          _DAT_006e1a74 = 0;
          _DAT_006e1a88 = 0;
          _DAT_006e1a8c = 0;
          _DAT_006e1a5c = _DAT_006e1a44;
          _DAT_006e1a70 = _DAT_006e1a58;
          if ((((uint)puStack_44 & 1) == 0) || (*(char *)((int)extraout_EAX + 0x23) == '\0')) {
            sVar4 = 0;
            do {
              if (extraout_EAX[1] == 0) {
                if ((short)local_4c == 1) {
                  if (sVar4 == 0) {
                    apiStack_74[5] = (int *)0x0;
                    apiStack_74[4] = (int *)0x52dd9d;
                    FUN_005186c0();
                    goto LAB_0052de6b;
                  }
                }
                else {
                  if ((short)puStack_44 != 0) {
                    if ((short)puStack_44 == 1) {
                      if (*(int *)(extraout_EAX + 4) == 0) goto joined_r0x0052de09;
                      if (sVar4 != 0) {
                        if (sVar4 == 1) {
                          apiStack_74[5] = (int *)0x1;
                        }
                        else if (sVar4 == 2) {
                          apiStack_74[5] = (int *)0x2;
                        }
                        else {
                          if (sVar4 != 3) goto LAB_0052ded5;
                          apiStack_74[5] = (int *)0x3;
                        }
                        goto LAB_0052de5c;
                      }
                      apiStack_74[5] = (int *)0x0;
                      apiStack_74[4] = (int *)0x52dddb;
                      FUN_005186c0();
                    }
                    goto LAB_0052de6b;
                  }
joined_r0x0052de09:
                  if (sVar4 == 0) goto LAB_0052de5a;
                }
              }
              else {
                if (*(int *)(extraout_EAX + 4) == 0) {
                  if (sVar4 != 0) break;
LAB_0052de5a:
                  apiStack_74[5] = (int *)0x0;
                }
                else {
                  if (sVar4 == 0) {
                    apiStack_74[5] = (int *)0x0;
                    apiStack_74[4] = (int *)0x52de29;
                    FUN_005186c0();
                    goto LAB_0052de6b;
                  }
                  if (sVar4 == 1) {
                    apiStack_74[5] = (int *)0x1;
                  }
                  else if (sVar4 == 2) {
                    apiStack_74[5] = (int *)0x2;
                  }
                  else {
                    if (sVar4 != 3) break;
                    apiStack_74[5] = (int *)0x3;
                  }
                }
LAB_0052de5c:
                apiStack_74[4] = (int *)0x52de68;
                FUN_0052ce10();
LAB_0052de6b:
                apiStack_74[5] = (int *)0x3;
                piVar5 = (int *)(int)sVar4;
                apiStack_74[4] = (int *)0x1;
                apiStack_74[2] = DAT_0071d174;
                apiStack_74[1] = (int *)0x52de81;
                apiStack_74[3] = piVar5;
                (**(code **)(*DAT_0071d174 + 0x114))();
                apiStack_74[1] = (int *)0x3;
                apiStack_74[0] = (int *)0x2;
                piStack_7c = DAT_0071d174;
                piStack_80 = (int *)0x52de94;
                piStack_78 = piVar5;
                (**(code **)(*DAT_0071d174 + 0x114))();
                piStack_80 = (int *)0x2;
                uStack_84 = 5;
                piStack_8c = DAT_0071d174;
                piStack_88 = piVar5;
                (**(code **)(*DAT_0071d174 + 0x114))();
                (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,piVar5,6,2);
                (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,piVar5,7,1);
              }
LAB_0052ded5:
              sVar4 = sVar4 + 1;
            } while (sVar4 < 4);
          }
          else {
            apiStack_74[5] = (int *)0x0;
            apiStack_74[4] = (int *)0x52db0f;
            FUN_0052ce10();
            apiStack_74[5] = (int *)0x3;
            apiStack_74[4] = (int *)0x1;
            apiStack_74[3] = (int *)0x0;
            apiStack_74[2] = DAT_0071d174;
            apiStack_74[1] = (int *)0x52db25;
            (**(code **)(*DAT_0071d174 + 0x114))();
            apiStack_74[1] = (int *)0x3;
            apiStack_74[0] = (int *)0x2;
            piStack_78 = (int *)0x0;
            piStack_7c = DAT_0071d174;
            piStack_80 = (int *)0x52db39;
            (**(code **)(*DAT_0071d174 + 0x114))();
            piStack_80 = (int *)0x1;
            uStack_84 = 5;
            piStack_88 = (int *)0x0;
            piStack_8c = DAT_0071d174;
            (**(code **)(*DAT_0071d174 + 0x114))();
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,1);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,1);
            FUN_005186c0(1);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,1,3);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,2,3);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,1);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,1);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,7,1);
            FUN_005186c0(2);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,1,1);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,2,1);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,5,2);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,6,2);
            (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,7,1);
          }
          apiStack_74[5] = (int *)0x1;
          apiStack_74[4] = (int *)&DAT_00000016;
          apiStack_74[3] = DAT_0071d174;
          apiStack_74[2] = (int *)0x52dc70;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          apiStack_74[2] = (int *)0x7;
          apiStack_74[1] = (int *)0xa8;
          apiStack_74[0] = DAT_0071d174;
          piStack_78 = (int *)0x52dc85;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          piStack_78 = (int *)0x0;
          piStack_7c = (int *)0x1b;
          piStack_80 = DAT_0071d174;
          uStack_84 = 0x52dc97;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_84 = 0;
          piStack_88 = (int *)&DAT_0000000f;
          piStack_8c = DAT_0071d174;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uVar10 = 0;
          (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7);
          piVar9 = DAT_0071d174;
          (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
          piVar5 = piStack_8c;
          rasterizer_screen_effect_compute_uv_transform(extraout_EAX,piStack_8c,local_4c,0);
          if ((short)piStack_88 != -1) {
            FUN_0052ccc0(0,0);
          }
          uVar8 = DAT_0071d210;
          if (*(char *)((int)extraout_EAX + 0x23) == '\0') {
            if (extraout_EAX[1] == 0) {
              bVar7 = (short)local_4c == 1;
              if (bVar7) goto LAB_0052e29c;
LAB_0052dfe9:
              if (uVar10 == (int)sVar2 - 1U) {
                (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
                (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,6);
                uVar8 = 1;
                goto LAB_0052e01b;
              }
            }
            else {
              apiStack_74[3] = *(int **)(extraout_EAX + 10);
              apiStack_74[4] = *(int **)(extraout_EAX + 0xc);
              apiStack_74[5] = *(int **)(extraout_EAX + 0xe);
              iVar1 = *(int *)(extraout_EAX + 6);
              if (*(int *)(extraout_EAX + 4) == 0) {
                if (((uVar10 != (int)(short)local_4c - 1U) ||
                    (((*(char *)((int)extraout_EAX + 0x21) == '\0' ||
                      (((((char)extraout_EAX[0x11] == '\0' || (*(float *)(extraout_EAX + 6) <= 0.0))
                        || (iVar3 = DAT_0071d230, *(float *)(extraout_EAX + 8) <= 0.0)) &&
                       ((*(char *)((int)extraout_EAX + 0x21) == '\0' ||
                        (iVar3 = DAT_0071d234, *(float *)(extraout_EAX + 6) <= 0.0)))))) &&
                     (((char)extraout_EAX[0x11] == '\0' ||
                      (iVar3 = DAT_0071d238, *(float *)(extraout_EAX + 8) <= 0.0)))))) ||
                   (iVar3 == 0)) {
                  iVar3 = DAT_0071d22c;
                }
                (**(code **)(*DAT_0069e250 + 0xec))(DAT_0069e250,iVar3);
              }
              else {
                if (((uVar10 != (int)(short)local_4c - 1U) ||
                    ((((*(char *)((int)extraout_EAX + 0x21) == '\0' ||
                       (((((char)extraout_EAX[0x11] == '\0' || (*(float *)(extraout_EAX + 6) <= 0.0)
                          ) || (iVar3 = DAT_0071d220, *(float *)(extraout_EAX + 8) <= 0.0)) &&
                        ((*(char *)((int)extraout_EAX + 0x21) == '\0' ||
                         (iVar3 = DAT_0071d224, *(float *)(extraout_EAX + 6) <= 0.0)))))) &&
                      (((char)extraout_EAX[0x11] == '\0' ||
                       (iVar3 = DAT_0071d228, *(float *)(extraout_EAX + 8) <= 0.0)))) ||
                     (iVar3 == 0)))) && (iVar3 = DAT_0071d21c, DAT_00722b78 == 0)) {
                  iVar3 = DAT_0071d218;
                }
                (**(code **)(*DAT_0069e250 + 0xec))(DAT_0069e250,iVar3);
                if (DAT_0069e268 != (undefined4 *)0x0) {
                  (**(code **)(*DAT_0069e250 + 0x88))(DAT_0069e250,*DAT_0069e268,apiStack_74 + 3);
                  (**(code **)(*DAT_0069e250 + 0x88))(DAT_0069e250,DAT_0069e268[1],apiStack_74 + 4);
                }
              }
              bVar7 = (short)local_4c != 1;
              local_4c = iVar1;
              if (bVar7) goto LAB_0052dfe9;
LAB_0052e29c:
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,1);
              uVar8 = 6;
LAB_0052e01b:
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,uVar8);
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
            }
            (**(code **)(*DAT_0069e250 + 0x100))(DAT_0069e250,&uStack_84,3);
            uVar6 = 0;
            if (uVar10 != 0) {
              do {
                (**(code **)(*DAT_0069e250 + 0x104))(DAT_0069e250,uVar6);
                (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006e1a30,0x18);
                uVar6 = uVar6 + 1;
              } while (uVar6 < uVar10);
            }
            (**(code **)(*DAT_0069e250 + 0x108))(DAT_0069e250);
          }
          else {
            if ((short)piVar5 == 1) {
              apiStack_74[0] = (int *)0x1;
              apiStack_74[1] = (int *)0x2;
              apiStack_74[2] = (int *)&DAT_00000004;
              apiStack_74[3] = (int *)(float)(int)apiStack_74[extraout_EAX[0x12]];
              apiStack_74[4] = apiStack_74[3];
              apiStack_74[5] = apiStack_74[3];
              if (DAT_0069e268 != (undefined4 *)0x0) {
                (**(code **)(*DAT_0069e250 + 0x88))(DAT_0069e250,*DAT_0069e268,apiStack_74 + 3);
              }
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,6);
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,1);
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
            }
            (**(code **)(*DAT_0069e250 + 0xec))(DAT_0069e250,uVar8);
            (**(code **)(*DAT_0069e250 + 0x100))(DAT_0069e250,&piStack_8c,3);
            (**(code **)(*DAT_0069e250 + 0x104))(DAT_0069e250,piVar9);
            (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006e1a30,0x18);
            (**(code **)(*DAT_0069e250 + 0x108))(DAT_0069e250);
          }
          puStack_44 = (undefined1 *)((int)piVar5 + 1);
        } while ((short)puStack_44 < (short)local_4c);
      }
      apiStack_74[5] = (int *)0x52e0bf;
      FUN_0052ccc0();
    }
    (**(code **)(*DAT_0071d174 + 0x134))();
  }
  return;
}
#endif
