// rasterizer_shader_transparent_chicago_draw  (Ghidra: FUN_00531ed0; the earlier rewrite called it
//   rasterizer_shader_effect_permutation_draw and did not implement its second half)
// address 0x531ed0, size 2882 bytes (blend function jump table 0x532a18, eight entries)
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: the shader_type 6 case of rasterizer_transparent_geometry_group_draw 0x533850 calls it
//   with (group, attached) on the stack. Every shader offset is ShaderTransparentChicago
//   (types/tags.h): +0x28 numeric_counter_limit, +0x29 flags (alpha_tested 1, two_sided 4,
//   first_map_is_in_screenspace 8, scale_first_map_with_distance 0x40, numeric 0x80), +0x2a
//   first_map_type, +0x2c framebuffer_blend_function, +0x2e framebuffer_fade_mode, +0x30
//   framebuffer_fade_source, +0x48 extra_layers, +0x54 maps (0xdc byte ShaderTransparentChicagoMap:
//   +0 flags, +0x54..+0x64 scale/offset/rotation, +0x78 map.tag_id, +0xa4 animation), +0x60
//   extra_flags. Rebuilt from the raw disassembly (Ghidra lost the register arguments of every
//   helper and the whole texture stage setup of the blend switch).
// What it does: draws the extra layer shaders first (as copies of the group), then binds up to
//   four maps with their address and filter modes and their texture animation as c13..c20, the
//   fade constant c10..c12, the map colour constants of 0x537bb0, and finally one extra texture
//   stage that applies the framebuffer blend function (with the fade argument), before drawing.
// register convention: stack -> (group, attached).
// blam-cc: stack -> (group, attached)
// UNSURE: the fade mode argument falls back to the first map type for fade modes above 2, as the
//   binary does (movsx of +0x2a into the D3DTA argument).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern rasterizer_frame_time rasterizer_time;               // 0x007c1200
extern d3d_caps9 rasterizer_caps;                           // 0x007c10c0
extern tag_instance *tag_instances;                         // 0x0087bc14
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern int16_t rasterizer_transparent_vertex_shader_table[]; // 0x0069e558 [vertex_type * 6 + permutation]
extern const int16_t rasterizer_first_map_bitmap_types[4];  // 0x0065e3c8 .rdata {0, 2, 2, 2}
extern const uint32_t rasterizer_first_map_address_modes[4]; // 0x0065e3d0 .rdata {1, 3, 3, 3}

// blam-cc: ECX -> shader, returns AX
extern int16_t chimera__shader_get_vertex_shader_permutation(const Shader *shader); // 0x53fd60
extern void rasterizer_transparent_geometry_group_draw(transparent_geometry_group *group, uint8_t attached); // 0x533850
// blam-cc: CX -> mode
extern void chimera__rasterizer_set_framebuffer_blend_function(int16_t mode); // 0x5185d0
// blam-cc: AX -> digit, returns AX
extern int16_t numeric_countdown_timer_get_digit(int16_t digit); // 0x5400c0
extern double floor(double x); // 0x623e40 CRT
// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960
// blam-cc: ECX -> function_source, ESI -> animation, EBX -> out_u, EDI -> out_v, stack -> the rest
extern void shader_texture_animation_evaluate(const void *function_source, const void *animation,
                                              float *out_u, float *out_v, float u_scale, float v_scale,
                                              float u_offset, float v_offset, float rotation,
                                              float time); // 0x53fe50
// blam-cc: EBX -> shader
extern uint8_t rasterizer_shader_transparent_chicago_set_texture_stages(const ShaderTransparentChicago *shader); // 0x537bb0
// blam-cc: ECX -> group, stack -> flag
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x533660

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void tss(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}
static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}

// floor(limit * value + 0.5) as the binary computes it: rounded through a float and fistp.
static int32_t numeric_value(float limit, float value)
{
    float rounded = (float)floor(limit * value + 0.5f);

    return (int32_t)rounded;  // fistp with the default rounding mode; rounded is integral already
}

void rasterizer_shader_transparent_chicago_draw(transparent_geometry_group *group, uint8_t attached)
{
    uint8_t *shader = (uint8_t *)(uintptr_t)group->shader;
    uint8_t *maps;
    uint8_t ok = 1;
    int16_t permutation;
    int16_t vertex_type;
    int16_t frame;
    int16_t first_map_type;
    int32_t map_count;
    int16_t layer;
    int16_t map;
    uint32_t fade_argument;
    int16_t stage;
    float map_constants[8][4];
    float fade_constants[3][4];

    permutation = chimera__shader_get_vertex_shader_permutation((const Shader *)shader);
    vertex_type = -1;
    if (group->vertex_buffer != 0) {
        vertex_type = *(int16_t *)(uintptr_t)group->vertex_buffer;
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    frame = (int16_t)group->shader_permutation;
    maps = (uint8_t *)(uintptr_t)((struct ShaderTransparentChicago *)shader)->maps.pointer;
    if (maps == NULL || *(uint32_t *)(maps + 0x70) == 0) {        // first map has no bitmap path
        return;
    }
    if (((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device,
            rasterizer_vertex_shaders[rasterizer_transparent_vertex_shader_table[vertex_type * 6 + permutation]].shader) < 0) {
        ok = 0;
    }
    if (((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[vertex_type].declaration) < 0) {
        ok = 0;
    }
    if (((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0) < 0) {
        ok = 0;
    }

    // extra layers: the same geometry drawn with each layer shader first
    for (layer = 0; layer < *(int32_t *)&((struct ShaderTransparentChicago *)shader)->extra_layers.count; layer++) {
        transparent_geometry_group copy = *group;
        const uint8_t *layers = (const uint8_t *)(uintptr_t)((struct ShaderTransparentChicago *)shader)->extra_layers.pointer;
        uint32_t tag_id = *(uint32_t *)(layers + layer * 0x10 + 0xc);

        copy.sorted_index = -1;
        copy.shader = (uint32_t)(uintptr_t)tag_instances[tag_id & 0xffff].data;
        rasterizer_transparent_geometry_group_draw(&copy, attached);
    }

    set_render_state(0x16, (shader[0x29] & 4) ? 1 : 3);   // two_sided
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0x0f, shader[0x29] & 1);              // alpha_tested
    set_render_state(0x18, 0x7f);
    set_render_state(0x1c, 0);
    chimera__rasterizer_set_framebuffer_blend_function(*(int16_t *)&((struct ShaderTransparentChicago *)shader)->framebuffer_blend_function);

    // numeric shaders pick the frame of the first map from a function value or the game timer
    if ((int8_t)shader[0x29] < 0 && group->lighting_extra != 0 && *(int32_t *)&((struct ShaderTransparentChicago *)shader)->maps.count > 0) {
        const uint8_t *bitmap = (const uint8_t *)tag_instances[*(uint32_t *)(maps + 0x78) & 0xffff].data;
        int16_t base = *(int16_t *)(bitmap + 0x60);               // Bitmap.bitmap_data.count, the digit count

        if (shader[0x60] & 2) {                                   // numeric_countdown_timer
            frame = numeric_countdown_timer_get_digit((int16_t)group->shader_permutation);
        } else {
            const float *function_values = *(const float **)(uintptr_t)(group->lighting_extra + 4);
            int32_t limit = (int16_t)shader[0x28];                // numeric_counter_limit
            int value_index = (base != 8) ? 0 : 3;
            int16_t value;
            int16_t digit;

            if (numeric_value((float)limit, function_values[value_index]) < 0) {
                value = 0;
            } else if (numeric_value((float)limit, function_values[value_index]) > limit) {
                value = (int16_t)limit;
            } else {
                value = (int16_t)numeric_value((float)limit, function_values[value_index]);
            }
            for (digit = (int16_t)group->shader_permutation; digit > 0; digit--) {
                value = (int16_t)(value / base);
            }
            frame = (int16_t)(value % base);
        }
    }

    // maps 0..3: textures, sampler states, texture animation rows
    first_map_type = *(int16_t *)&((struct ShaderTransparentChicago *)shader)->first_map_type;
    for (map = 0; map < 4; map++) {
        map_count = *(int32_t *)&((struct ShaderTransparentChicago *)shader)->maps.count;
        if (map < map_count) {
            uint8_t *entry = maps + map * 0xdc;
            int16_t bitmap_type = (map != 0) ? 0 : rasterizer_first_map_bitmap_types[first_map_type];
            uint32_t address_u, address_v, address_w;
            uint32_t filter = (entry[0] & 1) ? 1 : 2;             // unfiltered

            chimera__rasterizer_set_texture(*(uint32_t *)(entry + 0x78), map, bitmap_type, 0, frame);
            if (bitmap_type == 0 && (entry[0] & 4)) {
                address_u = 3;                                    // u_clamped
            } else {
                address_u = (map != 0) ? 1 : rasterizer_first_map_address_modes[first_map_type];
            }
            if (bitmap_type == 0 && (entry[0] & 8)) {
                address_v = 3;                                    // v_clamped
            } else {
                address_v = (map != 0) ? 1 : rasterizer_first_map_address_modes[first_map_type];
            }
            address_w = (map != 0) ? 1 : rasterizer_first_map_address_modes[first_map_type];
            set_sampler_state(map, 1, address_u);
            set_sampler_state(map, 2, address_v);
            set_sampler_state(map, 3, address_w);
            set_sampler_state(map, 5, 2);
            set_sampler_state(map, 6, filter);
            set_sampler_state(map, 7, filter);
        }
        map_count = *(int32_t *)&((struct ShaderTransparentChicago *)shader)->maps.count;
        if (map < map_count && (map > 0 || first_map_type == 0)) {
            uint8_t *entry = maps + map * 0xdc;
            float u_scale = *(float *)(entry + 0x54);
            float v_scale = *(float *)(entry + 0x58);

            if (map == 0) {
                if (shader[0x29] & 0x40) {                        // scale_first_map_with_distance
                    u_scale = -(u_scale * group->depth);
                    v_scale = -(v_scale * group->depth);
                }
                if (!(shader[0x29] & 8)) {                        // not in screen space
                    u_scale *= group->base_map_u_scale;
                    v_scale *= group->base_map_v_scale;
                }
            } else {
                u_scale *= group->base_map_u_scale;
                v_scale *= group->base_map_v_scale;
            }
            shader_texture_animation_evaluate((const void *)(uintptr_t)group->lighting_extra, entry + 0xa4,
                                              map_constants[map * 2], map_constants[map * 2 + 1], u_scale, v_scale,
                                              *(float *)(entry + 0x5c), *(float *)(entry + 0x60),
                                              *(float *)(entry + 0x64), (float)rasterizer_time.time);
        } else if (map < map_count && (shader[0x29] & 8)) {
            // screen space first map: the view_to_world forward and left axes
            const real_matrix4x3 *view_to_world = &rasterizer_window.frustum.view_to_world;

            map_constants[0][0] = view_to_world->forward.i;
            map_constants[0][1] = view_to_world->forward.j;
            map_constants[0][2] = view_to_world->forward.k;
            map_constants[1][0] = view_to_world->left.i;
            map_constants[1][1] = view_to_world->left.j;
            map_constants[1][2] = view_to_world->left.k;
            map_constants[0][3] = 0.0f;
            map_constants[1][3] = 0.0f;
        } else {
            map_constants[map * 2][0] = 1.0f;
            map_constants[map * 2][1] = 0.0f;
            map_constants[map * 2][2] = 0.0f;
            map_constants[map * 2 + 1][0] = 0.0f;
            map_constants[map * 2 + 1][1] = 1.0f;
            map_constants[map * 2 + 1][2] = 0.0f;
            map_constants[map * 2][3] = 0.0f;
            map_constants[map * 2 + 1][3] = 0.0f;
        }
    }
    if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &map_constants[0][0], 8) >= 0 && ok) {
        rasterizer_shader_transparent_chicago_set_texture_stages((const ShaderTransparentChicago *)shader);
    }

    stage = (int16_t)*(int32_t *)&((struct ShaderTransparentChicago *)shader)->maps.count;                 // one stage past the maps
    if ((group->flags & 0x10) && *(int16_t *)&((struct ShaderTransparentChicago *)shader)->framebuffer_blend_function == 0) {
        goto draw;
    }
    {
        int16_t fade_source = *(int16_t *)&((struct ShaderTransparentChicago *)shader)->framebuffer_fade_source;
        int i;

        for (i = 0; i < 3; i++) {
            fade_constants[i][0] = 0.0f;
            fade_constants[i][1] = 0.0f;
            fade_constants[i][2] = 0.0f;
            fade_constants[i][3] = 0.0f;
        }
        fade_constants[2][2] = 1.0f;
        if (group->parameters.mode == 1 && !(shader[0x60] & 1)) { // don't_fade_active_camouflage
            float fade = 1.0f - group->parameters.blend_factor;

            fade_constants[2][2] = fade < 0.0f ? 0.0f : (fade > 1.0f ? 1.0f : fade);
        }
        if (fade_source > 0 && group->lighting_extra != 0) {
            const float *function_values = *(const float **)(uintptr_t)(group->lighting_extra + 4);

            if (function_values != NULL) {
                const float *value = &function_values[fade_source - 1];

                if (*value == 0.0f && rasterizer_caps.pixel_shader_version < 0xffff0101) {
                    return;   // fully faded on fixed function hardware: nothing is drawn
                }
                fade_constants[2][2] *= *value;
            }
        }
        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 10, &fade_constants[0][0], 3);
    }
    switch (*(int16_t *)&((struct ShaderTransparentChicago *)shader)->framebuffer_fade_mode) {                        // framebuffer_fade_mode
    case 0: fade_argument = 0x20; break;                          // DIFFUSE | ALPHAREPLICATE
    case 1: fade_argument = 0x24; break;                          // SPECULAR | ALPHAREPLICATE
    case 2: fade_argument = 4; break;                             // SPECULAR
    default: fade_argument = (uint32_t)(uint16_t)first_map_type; break;
    }

    map_count = *(int32_t *)&((struct ShaderTransparentChicago *)shader)->maps.count;
    switch (*(int16_t *)&((struct ShaderTransparentChicago *)shader)->framebuffer_blend_function) {                        // framebuffer_blend_function
    case 0:                                                       // alpha blend
        if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
            stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
            tss(0, 4, 4);                                         // ALPHAOP MODULATE
            tss(0, 6, fade_argument);                             // ALPHAARG2
        } else {
            stage = (int16_t)map_count;
            tss(stage, 1, 2);                                     // COLOROP SELECTARG1
            tss(stage, 2, 1);                                     // COLORARG1 CURRENT
            tss(stage, 4, 4);                                     // ALPHAOP MODULATE
            tss(stage, 5, 1);                                     // ALPHAARG1 CURRENT
            tss(stage, 6, fade_argument);
        }
        stage++;
        break;
    case 1:                                                       // multiply
    case 5:                                                       // component min
        stage = (int16_t)(rasterizer_caps.max_simultaneous_textures > 2 ? map_count
                                                                       : (map_count - 1 > 1 ? map_count - 1 : 1));
        tss(stage, 1, 0x19);                                      // COLOROP MULTIPLYADD
        tss(stage, 2, fade_argument | 0x10);                      // COLORARG1 complement
        tss(stage, 3, 1);                                         // COLORARG2 CURRENT
        tss(stage, 0x1a, fade_argument);                          // COLORARG0
        tss(stage, 4, 2);                                         // ALPHAOP SELECTARG1
        tss(stage, 5, 1);                                         // ALPHAARG1 CURRENT
        stage++;
        break;
    case 2:                                                       // double multiply
        stage = (int16_t)(rasterizer_caps.max_simultaneous_textures > 2 ? map_count
                                                                       : (map_count - 1 > 1 ? map_count - 1 : 1));
        set_render_state(0x3c, 0x7f7f7f7f);                       // TEXTUREFACTOR grey
        tss(stage, 1, 0x1a);                                      // COLOROP LERP
        tss(stage, 2, fade_argument);
        tss(stage, 3, 1);
        tss(stage, 0x1a, 3);                                      // COLORARG0 TFACTOR
        tss(stage, 4, 2);
        tss(stage, 5, 1);
        stage++;
        break;
    case 3:                                                       // add
    case 4:                                                       // subtract
    case 6:                                                       // component max
        if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
            stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
            tss(0, 1, 4);                                         // COLOROP MODULATE
            tss(0, 3, fade_argument);                             // COLORARG2
        } else {
            stage = (int16_t)map_count;
            tss(stage, 1, 4);
            tss(stage, 2, 1);
            tss(stage, 3, fade_argument);
            tss(stage, 4, 2);
            tss(stage, 5, 1);
        }
        stage++;
        break;
    case 7:                                                       // alpha multiply add
        if (rasterizer_caps.max_simultaneous_textures == 2 && map_count >= 2) {
            stage = (int16_t)(map_count - 1 > 1 ? map_count - 1 : 1);
            tss(0, 1, 4);
            tss(0, 3, fade_argument);
            tss(0, 4, 4);
            tss(0, 6, fade_argument);
        } else {
            stage = (int16_t)map_count;
            tss(stage, 1, 4);
            tss(stage, 2, 1);
            tss(stage, 3, fade_argument);
            tss(stage, 4, 4);
            tss(stage, 5, 1);
            tss(stage, 6, fade_argument);
        }
        stage++;
        break;
    default:
        break;
    }

draw:
    ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, (uint32_t)(int32_t)stage, 0);   // SetTexture(stage, NULL)
    tss((uint32_t)(int32_t)stage, 1, 1);                          // COLOROP DISABLE
    tss((uint32_t)(int32_t)stage, 4, 1);                          // ALPHAOP DISABLE
    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    set_render_state(0xab, 1);                                    // BLENDOP ADD
}

#if 0
Original Ghidra decompilation (0x531ed0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00531ed0(int param_1)

{
  byte bVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int extraout_EDX;
  undefined4 uVar7;
  short sVar8;
  undefined4 *puVar9;
  int iVar10;
  undefined4 *puVar11;
  byte *pbVar12;
  float10 fVar13;
  int *piVar14;
  undefined4 uVar15;
  char cVar16;
  uint uVar17;
  float fStack_14c;
  int *piStack_148;
  float fStack_144;
  undefined4 uStack_140;
  int *piStack_13c;
  undefined4 uStack_138;
  undefined4 uStack_134;
  int *piStack_130;
  undefined4 *puStack_12c;
  uint uStack_128;
  int *piStack_124;
  undefined4 uStack_120;
  int *piStack_11c;
  undefined4 uStack_118;
  int *piStack_114;
  uint uStack_110;
  undefined4 uStack_fc;
  undefined1 local_f5;
  undefined2 local_f0 [22];
  undefined4 auStack_c4 [11];
  void *pvStack_98;
  byte *pbStack_6c;
  int iStack_5c;
  undefined4 uStack_2c;
  undefined4 *puStack_14;
  undefined4 uStack_10;
  
  iVar10 = *(int *)(param_1 + 0xc);
  local_f5 = 1;
  uStack_110 = 0x531ef0;
  sVar3 = chimera__shader_get_vertex_shader_permutation();
  sVar8 = -1;
  if (*(short **)(extraout_EDX + 0x58) == (short *)0x0) {
    if (*(int *)(extraout_EDX + 0x54) != -1) {
      sVar8 = *(short *)(&DAT_006d99d8 + *(int *)(extraout_EDX + 0x54) * 0x10);
    }
  }
  else {
    sVar8 = **(short **)(extraout_EDX + 0x58);
  }
  local_f0[0] = *(undefined2 *)(extraout_EDX + 0x10);
  if (*(int *)(iVar10 + 0x58) == 0) {
    return;
  }
  if (*(int *)(*(int *)(iVar10 + 0x58) + 0x70) == 0) {
    return;
  }
  uStack_110 = (&DAT_0069e350)[*(short *)(&DAT_0069e558 + ((int)sVar3 + sVar8 * 6) * 2) * 2];
  piStack_114 = DAT_0071d174;
  uStack_118 = 0x531f5b;
  (**(code **)(*DAT_0071d174 + 0x170))();
  uStack_118 = (&DAT_006e1a90)[sVar8 * 3];
  piStack_11c = DAT_0071d174;
  uStack_120 = 0x531f7e;
  (**(code **)(*DAT_0071d174 + 0x15c))();
  uStack_120 = 0;
  piStack_124 = DAT_0071d174;
  uStack_128 = 0x531f97;
  iVar4 = (**(code **)(*DAT_0071d174 + 0x1ac))();
  if (iVar4 < 0) {
    uStack_110 = uStack_110 & 0xffffff;
  }
  sVar8 = 0;
  if (0 < *(int *)(iVar10 + 0x48)) {
    iVar4 = 0;
    do {
      iVar2 = *(int *)(iVar10 + 0x4c);
      puVar9 = puStack_14;
      puVar11 = auStack_c4;
      for (iVar5 = 0x2a; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar11 = *puVar9;
        puVar9 = puVar9 + 1;
        puVar11 = puVar11 + 1;
      }
      uStack_2c = 0xffffffff;
      auStack_c4[3] =
           *(undefined4 *)
            ((*(uint *)(iVar4 * 0x10 + 0xc + iVar2) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      uStack_128 = uStack_10;
      puStack_12c = auStack_c4;
      piStack_130 = (int *)0x531fff;
      rasterizer_transparent_geometry_group_draw();
      sVar8 = sVar8 + 1;
      iVar4 = (int)sVar8;
    } while (iVar4 < *(int *)(iVar10 + 0x48));
  }
  uStack_128 = ~(uint)(*(byte *)(iVar10 + 0x29) >> 1) & 2 | 1;
  puStack_12c = (undefined4 *)0x16;
  piStack_130 = DAT_0071d174;
  uStack_134 = 0x53202d;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_134 = 7;
  uStack_138 = 0xa8;
  piStack_13c = DAT_0071d174;
  uStack_140 = 0x532042;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_140 = 1;
  fStack_144 = 3.78351e-44;
  piStack_148 = DAT_0071d174;
  fStack_14c = 7.633943e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  fStack_14c = (float)(*(byte *)(iVar10 + 0x29) & 1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0x7f);
  uVar17 = 0;
  uVar15 = 0;
  piVar14 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
  chimera__rasterizer_set_framebuffer_blend_function();
  if (((*(char *)(iVar10 + 0x29) < '\0') && (*(int *)(iStack_5c + 0x74) != 0)) &&
     (0 < *(int *)(iVar10 + 0x54))) {
    if ((*(byte *)(iVar10 + 0x60) & 2) == 0) {
      bVar1 = *(byte *)(iVar10 + 0x28);
      fStack_14c = (float)(int)(short)(ushort)bVar1;
      iVar4 = (short)((*(short *)(*(int *)((*(uint *)(*(int *)(iVar10 + 0x58) + 0x78) & 0xffff) *
                                           0x20 + 0x14 + DAT_0087bc14) + 0x60) != 8) - 1 & 3) * 4;
      fVar13 = (float10)FUN_00623e40((double)(fStack_14c *
                                              *(float *)(*(int *)(*(int *)(iStack_5c + 0x74) + 4) +
                                                        iVar4) + 0.5));
      if ((-1 < (int)ROUND((float)fVar13)) &&
         (fVar13 = (float10)FUN_00623e40((double)(fStack_14c *
                                                  *(float *)(*(int *)(*(int *)(iStack_5c + 0x74) + 4
                                                                     ) + iVar4) + 0.5)),
         (int)ROUND((float)fVar13) <= (int)(short)(ushort)bVar1)) {
        FUN_00623e40((double)(fStack_14c *
                              *(float *)(*(int *)(*(int *)(iStack_5c + 0x74) + 4) + iVar4) + 0.5));
      }
      if (0 < (short)*(ushort *)(iStack_5c + 0x10)) {
        uVar6 = (uint)*(ushort *)(iStack_5c + 0x10);
        do {
          uVar6 = uVar6 - 1;
        } while (uVar6 != 0);
      }
    }
    else {
      game_timer_get_digit_pair();
    }
  }
  fStack_14c = 0.0;
  do {
    sVar8 = SUB42(fStack_14c,0);
    iVar4 = (int)sVar8;
    if (iVar4 < *(int *)(iVar10 + 0x54)) {
      sVar3 = *(short *)(iVar10 + 0x2a);
      pbVar12 = (byte *)(iVar4 * 0xdc + *(int *)(iVar10 + 0x58));
      if (sVar8 == 0) {
        uStack_140 = CONCAT22(uStack_140._2_2_,*(undefined2 *)(&DAT_0065e3c8 + sVar3 * 2));
      }
      else {
        uStack_140 = 0;
      }
      chimera__rasterizer_set_texture(fStack_14c,uStack_140,0);
      if (((((short)uStack_140 == 0) && ((*pbVar12 & 4) != 0)) || ((short)uStack_140 == 0)) &&
         ((*pbVar12 & 8) != 0)) {
        uVar7 = 3;
      }
      else if (sVar8 == 0) {
        uVar7 = *(undefined4 *)(&DAT_0065e3d0 + sVar3 * 4);
      }
      else {
        uVar7 = 1;
      }
      sVar8 = 1;
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar4);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar4,2,uVar7);
      if ((short)piVar14 == 0) {
        uVar7 = *(undefined4 *)(&DAT_0065e3d0 + sVar8 * 4);
      }
      else {
        uVar7 = 1;
      }
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar4,3,uVar7);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar4,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar4,6,2 - (uint)((*pbVar12 & 1) != 0));
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar4,7,2 - (uint)((*pbVar12 & 1) != 0));
    }
    if (iVar4 < *(int *)(iVar10 + 0x54)) {
      sVar8 = SUB42(fStack_14c,0);
      if ((sVar8 < 1) && (*(short *)(iVar10 + 0x2a) != 0)) {
        if ((*(int *)(iVar10 + 0x54) <= iVar4) || ((*(byte *)(iVar10 + 0x29) & 8) == 0))
        goto LAB_00532485;
        *(undefined4 *)(local_f0 + iVar4 * 0x10 + -0xe) = DAT_007c12c4;
        *(undefined4 *)(local_f0 + iVar4 * 0x10 + -0xc) = DAT_007c12c8;
        *(undefined4 *)(local_f0 + iVar4 * 0x10 + -10) = DAT_007c12cc;
        (&uStack_fc)[iVar4 * 8] = DAT_007c12d0;
        *(undefined4 *)(&stack0xffffff08 + iVar4 * 0x20) = DAT_007c12d4;
        *(undefined4 *)(local_f0 + iVar4 * 0x10 + -2) = DAT_007c12d8;
        goto LAB_005324a5;
      }
      fStack_144 = *(float *)(iVar4 * 0xdc + 0x58 + *(int *)(iVar10 + 0x58));
      iVar4 = iVar4 * 0xdc + *(int *)(iVar10 + 0x58);
      piStack_148 = *(int **)(iVar4 + 0x54);
      if (sVar8 == 0) {
        if ((*(byte *)(iVar10 + 0x29) & 0x40) != 0) {
          piStack_148 = (int *)-((float)piStack_148 * *(float *)(iStack_5c + 0x78));
          fStack_144 = -(fStack_144 * *(float *)(iStack_5c + 0x78));
        }
LAB_005323c7:
        if ((*(byte *)(iVar10 + 0x29) & 8) == 0) goto LAB_005323d6;
      }
      else {
        if (sVar8 < 1) goto LAB_005323c7;
LAB_005323d6:
        piStack_148 = (int *)((float)piStack_148 * *(float *)(iStack_5c + 0x3c));
        fStack_144 = fStack_144 * *(float *)(iStack_5c + 0x40);
      }
      shader_texture_animation_evaluate
                (piStack_148,fStack_144,*(undefined4 *)(iVar4 + 0x5c),*(undefined4 *)(iVar4 + 0x60),
                 *(undefined4 *)(iVar4 + 100));
    }
    else {
LAB_00532485:
      *(undefined4 *)(local_f0 + iVar4 * 0x10 + -0xe) = 0x3f800000;
      *(undefined4 *)(local_f0 + iVar4 * 0x10 + -0xc) = 0;
      *(undefined4 *)(local_f0 + iVar4 * 0x10 + -10) = 0;
      (&uStack_fc)[iVar4 * 8] = 0;
      *(undefined4 *)(&stack0xffffff08 + iVar4 * 0x20) = 0x3f800000;
      *(undefined4 *)(local_f0 + iVar4 * 0x10 + -2) = 0;
LAB_005324a5:
      *(undefined4 *)(local_f0 + iVar4 * 0x10 + -8) = 0;
      *(undefined4 *)(local_f0 + iVar4 * 0x10) = 0;
    }
    cVar16 = (char)((uint)uVar15 >> 0x18);
    fStack_14c = (float)((int)fStack_14c + 1);
  } while (SUB42(fStack_14c,0) < 4);
  iVar4 = (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&stack0xfffffef4,8);
  if ((-1 < iVar4) && (cVar16 != '\0')) {
    FUN_00537bb0();
  }
  sVar8 = *(short *)(iVar10 + 0x54);
  if (((*pbStack_6c & 0x10) != 0) && (*(short *)(iVar10 + 0x2c) == 0))
  goto switchD_00532646_default;
  sVar3 = *(short *)(iVar10 + 0x30);
  fStack_14c = 0.0;
  piStack_148 = (int *)0x0;
  fStack_144 = 0.0;
  uStack_140 = 0;
  piStack_13c = (int *)0x0;
  uStack_138 = 0;
  uStack_134 = 0;
  piStack_130 = (int *)0x0;
  puStack_12c = (undefined4 *)0x0;
  uStack_128 = 0;
  piStack_124 = (int *)0x3f800000;
  uStack_120 = 0;
  if ((*(short *)(pbStack_6c + 0x14) == 1) && ((*(byte *)(iVar10 + 0x60) & 1) == 0)) {
    piStack_124 = (int *)(1.0 - *(float *)(pbStack_6c + 0x18));
    if (0.0 <= (float)piStack_124) {
      if (1.0 < (float)piStack_124) {
        piStack_124 = (int *)0x3f800000;
      }
    }
    else {
      piStack_124 = (int *)0x0;
    }
  }
  if (((0 < sVar3) && (*(int *)(pbStack_6c + 0x74) != 0)) &&
     (iVar4 = *(int *)(*(int *)(pbStack_6c + 0x74) + 4), iVar4 != 0)) {
    if ((*(float *)(iVar4 + -4 + sVar3 * 4) == 0.0) && (DAT_007c118c < 0xffff0101)) {
      return;
    }
    piStack_124 = (int *)((float)piStack_124 * *(float *)(iVar4 + -4 + sVar3 * 4));
  }
  (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&fStack_14c,3);
  sVar3 = *(short *)(iVar10 + 0x2e);
  if (sVar3 == 0) {
    uVar17 = 0x20;
  }
  else if (sVar3 == 1) {
    uVar17 = 0x24;
  }
  else if (sVar3 == 2) {
    uVar17 = 4;
  }
  switch(*(undefined2 *)(iVar10 + 0x2c)) {
  case 0:
    if ((DAT_007c1158 == 2) && (1 < *(int *)(iVar10 + 0x54))) {
      iVar10 = *(int *)(iVar10 + 0x54) + -1;
      if (iVar10 < 2) {
        iVar10 = 1;
      }
      sVar8 = (short)iVar10;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
      uVar15 = 6;
      iVar10 = 0;
      goto LAB_0053299b;
    }
    sVar8 = *(short *)(iVar10 + 0x54);
    iVar10 = (int)sVar8;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,1,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,2,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,4,4);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,5,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,6,uVar17);
    break;
  case 1:
  case 5:
    if (DAT_007c1158 < 3) {
      uVar6 = *(int *)(iVar10 + 0x54) - 1;
      if ((int)uVar6 < 2) {
        uVar6 = 1;
      }
    }
    else {
      uVar6 = (uint)*(ushort *)(iVar10 + 0x54);
    }
    sVar8 = (short)uVar6;
    iVar10 = (int)sVar8;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,1,0x19);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,2,uVar17 | 0x10);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,3,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,0x1a,uVar17);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,4,2);
    uVar17 = 1;
    uVar15 = 5;
    goto LAB_0053299b;
  case 2:
    if (DAT_007c1158 < 3) {
      uVar6 = *(int *)(iVar10 + 0x54) - 1;
      if ((int)uVar6 < 2) {
        uVar6 = 1;
      }
    }
    else {
      uVar6 = (uint)*(ushort *)(iVar10 + 0x54);
    }
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0x7f7f7f7f);
    sVar8 = (short)uVar6;
    iVar10 = (int)sVar8;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,1,0x1a);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,2,uVar17);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,3,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,0x1a,3);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,4,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,5,1);
    break;
  case 3:
  case 4:
  case 6:
    if ((DAT_007c1158 != 2) || (*(int *)(iVar10 + 0x54) < 2)) {
      sVar8 = *(short *)(iVar10 + 0x54);
      iVar10 = (int)sVar8;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,2,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,3,uVar17);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,4,2);
      uVar17 = 1;
      uVar15 = 5;
      goto LAB_0053299b;
    }
    iVar10 = *(int *)(iVar10 + 0x54) + -1;
    if (iVar10 < 2) {
      iVar10 = 1;
    }
    sVar8 = (short)iVar10;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,uVar17);
    break;
  case 7:
    if ((DAT_007c1158 == 2) && (1 < *(int *)(iVar10 + 0x54))) {
      iVar10 = *(int *)(iVar10 + 0x54) + -1;
      if (iVar10 < 2) {
        iVar10 = 1;
      }
      sVar8 = (short)iVar10;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,uVar17);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
      uVar15 = 6;
      iVar10 = 0;
    }
    else {
      sVar8 = *(short *)(iVar10 + 0x54);
      iVar10 = (int)sVar8;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,2,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,3,uVar17);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,4,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,5,1);
      uVar15 = 6;
    }
LAB_0053299b:
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,uVar15,uVar17);
    break;
  default:
    goto switchD_00532646_default;
  }
  sVar8 = sVar8 + 1;
switchD_00532646_default:
  iVar10 = (int)sVar8;
  (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,iVar10,0);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,1,1);
  piVar14 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar10,4,1);
  rasterizer_transparent_geometry_group_draw_vertices(pvStack_98,(void *)0x0,(char)piVar14);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
  return;
}
#endif
