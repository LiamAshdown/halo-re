// rasterizer_shader_transparent_chicago_extended_draw  (Ghidra: FUN_00532a40; the earlier rewrite
//   called it rasterizer_shader_effect_permutation_draw_compat and did not implement its second half)
// address 0x532a40, size 3051 bytes (blend function jump table 0x533634, eight entries)
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: the shader_type 7 case of rasterizer_transparent_geometry_group_draw 0x533850 calls it
//   with (group, attached). The code is instruction for instruction the one of
//   rasterizer_shader_transparent_chicago_draw 0x531ed0 (compared with a diff of the two
//   disassemblies) with the ShaderTransparentChicagoExtended layout (types/tags.h): maps_4_stage
//   +0x54, maps_2_stage +0x60 (used below ps_1_1), extra_flags +0x6c; its own vertex shader table
//   0x0069e630, first map tables 0x0065e3e0/0x0065e3e8 and map constant helper 0x537d60 (ECX).
// Differences from 0x531ed0 besides the layout: the map loop covers only the maps present (the
//   rows of absent maps are left as they were on the stack, not set to identity), and the default
//   case of the fade mode switch loads the stack slot that holds the shader pointer.
// register convention: stack -> (group, attached).
// blam-cc: stack -> (group, attached)
// UNSURE: fade modes above 2 feed the low bits of the shader pointer to the D3DTA argument (dead
//   for valid tags, which only have modes 0..2); numeric shaders read the 4 stage map list even
//   on cards that draw the 2 stage one.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern rasterizer_frame_time rasterizer_time;               // 0x007c1200
extern d3d_caps9 rasterizer_caps;                           // 0x007c10c0
extern tag_instance *tag_instances;                         // 0x0087bc14
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern int16_t rasterizer_transparent_extended_vertex_shader_table[]; // 0x0069e630 [vertex_type * 6 + permutation]
extern const int16_t rasterizer_extended_first_map_bitmap_types[4];  // 0x0065e3e0 .rdata {0, 2, 2, 2}
extern const uint32_t rasterizer_extended_first_map_address_modes[4]; // 0x0065e3e8 .rdata {1, 3, 3, 3}

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
// blam-cc: ECX -> shader
extern uint8_t rasterizer_shader_transparent_chicago_extended_set_texture_stages(const ShaderTransparentChicagoExtended *shader); // 0x537d60
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

void rasterizer_shader_transparent_chicago_extended_draw(transparent_geometry_group *group, uint8_t attached)
{
    uint8_t *shader = (uint8_t *)(uintptr_t)group->shader;
    uint8_t *maps;
    uint8_t *map_list[4];
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
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        maps = (uint8_t *)(uintptr_t)((struct ShaderTransparentChicagoExtended *)shader)->maps_2_stage.pointer;   // maps_2_stage
    } else {
        maps = (uint8_t *)(uintptr_t)((struct ShaderTransparentChicagoExtended *)shader)->maps_4_stage.pointer;   // maps_4_stage
    }
    if (maps == NULL || *(uint32_t *)(maps + 0x70) == 0) {        // first map has no bitmap path
        return;
    }
    if (((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device,
            rasterizer_vertex_shaders[rasterizer_transparent_extended_vertex_shader_table[vertex_type * 6 + permutation]].shader) < 0) {
        ok = 0;
    }
    if (((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[vertex_type].declaration) < 0) {
        ok = 0;
    }
    if (((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0) < 0) {
        ok = 0;
    }

    // extra layers: the same geometry drawn with each layer shader first
    for (layer = 0; layer < *(int32_t *)&((struct ShaderTransparentChicagoExtended *)shader)->extra_layers.count; layer++) {
        transparent_geometry_group copy = *group;
        const uint8_t *layers = (const uint8_t *)(uintptr_t)((struct ShaderTransparentChicagoExtended *)shader)->extra_layers.pointer;
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
    chimera__rasterizer_set_framebuffer_blend_function(*(int16_t *)&((struct ShaderTransparentChicagoExtended *)shader)->framebuffer_blend_function);

    // numeric shaders pick the frame of the first map from a function value or the game timer
    if ((int8_t)shader[0x29] < 0 && group->lighting_extra != 0 && *(int32_t *)&((struct ShaderTransparentChicagoExtended *)shader)->maps_4_stage.count > 0) {
        const uint8_t *maps_4_stage = (const uint8_t *)(uintptr_t)((struct ShaderTransparentChicagoExtended *)shader)->maps_4_stage.pointer;
        const uint8_t *bitmap = (const uint8_t *)tag_instances[*(uint32_t *)(maps_4_stage + 0x78) & 0xffff].data;
        int16_t base = *(int16_t *)(bitmap + 0x60);               // Bitmap.bitmap_data.count, the digit count

        if (shader[0x6c] & 2) {                                   // numeric_countdown_timer
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

    // the map list of the stage count this card draws
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        map_count = (int16_t)*(int32_t *)&((struct ShaderTransparentChicagoExtended *)shader)->maps_2_stage.count;
        maps = (uint8_t *)(uintptr_t)((struct ShaderTransparentChicagoExtended *)shader)->maps_2_stage.pointer;
    } else {
        map_count = (int16_t)*(int32_t *)&((struct ShaderTransparentChicagoExtended *)shader)->maps_4_stage.count;
        maps = (uint8_t *)(uintptr_t)((struct ShaderTransparentChicagoExtended *)shader)->maps_4_stage.pointer;
    }
    for (map = 0; map < map_count; map++) {
        map_list[map] = maps + map * 0xdc;
    }

    // maps present: textures, sampler states, texture animation rows
    first_map_type = *(int16_t *)&((struct ShaderTransparentChicagoExtended *)shader)->first_map_type;
    for (map = 0; map < map_count; map++) {
        uint8_t *entry = map_list[map];
        int16_t bitmap_type = (map != 0) ? 0 : rasterizer_extended_first_map_bitmap_types[first_map_type];
        uint32_t address_u, address_v, address_w;
        uint32_t filter = (entry[0] & 1) ? 1 : 2;                 // unfiltered

        chimera__rasterizer_set_texture(*(uint32_t *)(entry + 0x78), map, bitmap_type, 0, frame);
        if (bitmap_type == 0 && (entry[0] & 4)) {
            address_u = 3;
        } else {
            address_u = (map != 0) ? 1 : rasterizer_extended_first_map_address_modes[first_map_type];
        }
        if (bitmap_type == 0 && (entry[0] & 8)) {
            address_v = 3;
        } else {
            address_v = (map != 0) ? 1 : rasterizer_extended_first_map_address_modes[first_map_type];
        }
        address_w = (map != 0) ? 1 : rasterizer_extended_first_map_address_modes[first_map_type];
        set_sampler_state(map, 1, address_u);
        set_sampler_state(map, 2, address_v);
        set_sampler_state(map, 3, address_w);
        set_sampler_state(map, 5, 2);
        set_sampler_state(map, 6, filter);
        set_sampler_state(map, 7, filter);

        if (map > 0 || first_map_type == 0) {
            float u_scale = *(float *)(entry + 0x54);
            float v_scale = *(float *)(entry + 0x58);

            if (map == 0) {
                if (shader[0x29] & 0x40) {
                    u_scale = -(u_scale * group->depth);
                    v_scale = -(v_scale * group->depth);
                }
                if (!(shader[0x29] & 8)) {
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
        } else if (shader[0x29] & 8) {
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
            map_constants[map * 2][3] = 0.0f;
            map_constants[map * 2 + 1][0] = 0.0f;
            map_constants[map * 2 + 1][1] = 1.0f;
            map_constants[map * 2 + 1][2] = 0.0f;
            map_constants[map * 2 + 1][3] = 0.0f;
        }
    }
    if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &map_constants[0][0], 8) >= 0 && ok) {
        rasterizer_shader_transparent_chicago_extended_set_texture_stages((const ShaderTransparentChicagoExtended *)shader);
    }

    stage = (int16_t)map_count;                                   // one stage past the maps
    if ((group->flags & 0x10) && *(int16_t *)&((struct ShaderTransparentChicagoExtended *)shader)->framebuffer_blend_function == 0) {
        goto draw;
    }
    {
        int16_t fade_source = *(int16_t *)&((struct ShaderTransparentChicagoExtended *)shader)->framebuffer_fade_source;
        int i;

        for (i = 0; i < 3; i++) {
            fade_constants[i][0] = 0.0f;
            fade_constants[i][1] = 0.0f;
            fade_constants[i][2] = 0.0f;
            fade_constants[i][3] = 0.0f;
        }
        fade_constants[2][2] = 1.0f;
        if (group->parameters.mode == 1 && !(shader[0x6c] & 1)) { // don't_fade_active_camouflage
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
    switch (*(int16_t *)&((struct ShaderTransparentChicagoExtended *)shader)->framebuffer_fade_mode) {                        // framebuffer_fade_mode
    case 0: fade_argument = 0x20; break;                          // DIFFUSE | ALPHAREPLICATE
    case 1: fade_argument = 0x24; break;                          // SPECULAR | ALPHAREPLICATE
    case 2: fade_argument = 4; break;                             // SPECULAR
    default: fade_argument = (uint32_t)(uintptr_t)shader; break;  // the stack slot of the shader pointer
    }

    switch (*(int16_t *)&((struct ShaderTransparentChicagoExtended *)shader)->framebuffer_blend_function) {                        // framebuffer_blend_function
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
Original Ghidra decompilation (0x532a40):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00532a40(int param_1)

{
  ushort uVar1;
  short sVar2;
  byte *pbVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  int *piVar8;
  undefined4 uVar9;
  int extraout_EDX;
  uint uVar10;
  short sVar11;
  undefined4 *puVar12;
  int iVar13;
  int iVar14;
  undefined4 *puVar15;
  float10 fVar16;
  int *piVar17;
  undefined4 uVar18;
  char cVar19;
  undefined4 uVar20;
  undefined4 uVar21;
  undefined2 uVar22;
  float fVar23;
  float fStack_15c;
  float fStack_158;
  int *piStack_154;
  int iStack_150;
  undefined4 uStack_14c;
  int *piStack_148;
  undefined4 *puStack_144;
  uint uStack_140;
  int *piStack_13c;
  undefined4 uStack_138;
  int *piStack_134;
  undefined4 uStack_130;
  int *piStack_12c;
  uint uStack_128;
  undefined4 auStack_10c [6];
  undefined2 local_f4 [2];
  int local_f0 [11];
  undefined4 auStack_c4 [11];
  void *pvStack_98;
  byte *pbStack_6c;
  int iStack_5c;
  undefined4 uStack_2c;
  undefined4 *puStack_14;
  undefined4 uStack_10;
  
  iVar14 = *(int *)(param_1 + 0xc);
  uStack_128 = 0x532a64;
  local_f0[0] = iVar14;
  sVar4 = chimera__shader_get_vertex_shader_permutation();
  sVar11 = -1;
  if (*(short **)(extraout_EDX + 0x58) == (short *)0x0) {
    if (*(int *)(extraout_EDX + 0x54) != -1) {
      sVar11 = *(short *)(&DAT_006d99d8 + *(int *)(extraout_EDX + 0x54) * 0x10);
    }
  }
  else {
    sVar11 = **(short **)(extraout_EDX + 0x58);
  }
  local_f4[0] = *(undefined2 *)(extraout_EDX + 0x10);
  if (DAT_007c118c < 0xffff0101) {
    if (*(int *)(iVar14 + 100) == 0) {
      return;
    }
    if (*(int *)(*(int *)(iVar14 + 100) + 0x70) == 0) {
      return;
    }
  }
  else {
    if (*(int *)(iVar14 + 0x58) == 0) {
      return;
    }
    if (*(int *)(*(int *)(iVar14 + 0x58) + 0x70) == 0) {
      return;
    }
  }
  uStack_128 = (&DAT_0069e350)[*(short *)(&DAT_0069e630 + ((int)sVar4 + sVar11 * 6) * 2) * 2];
  piStack_12c = DAT_0071d174;
  uStack_130 = 0x532af8;
  (**(code **)(*DAT_0071d174 + 0x170))();
  uStack_130 = (&DAT_006e1a90)[sVar11 * 3];
  piStack_134 = DAT_0071d174;
  uStack_138 = 0x532b1b;
  (**(code **)(*DAT_0071d174 + 0x15c))();
  uStack_138 = 0;
  piStack_13c = DAT_0071d174;
  uStack_140 = 0x532b34;
  iVar5 = (**(code **)(*DAT_0071d174 + 0x1ac))();
  if (iVar5 < 0) {
    uStack_128 = uStack_128 & 0xffffff;
  }
  sVar11 = 0;
  if (0 < *(int *)(iVar14 + 0x48)) {
    iVar5 = 0;
    do {
      iVar13 = *(int *)(iVar14 + 0x4c);
      puVar12 = puStack_14;
      puVar15 = auStack_c4;
      for (iVar6 = 0x2a; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar15 = *puVar12;
        puVar12 = puVar12 + 1;
        puVar15 = puVar15 + 1;
      }
      uStack_2c = 0xffffffff;
      auStack_c4[3] =
           *(undefined4 *)
            ((*(uint *)(iVar5 * 0x10 + 0xc + iVar13) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      uStack_140 = uStack_10;
      puStack_144 = auStack_c4;
      piStack_148 = (int *)0x532ba2;
      rasterizer_transparent_geometry_group_draw();
      sVar11 = sVar11 + 1;
      iVar5 = (int)sVar11;
    } while (iVar5 < *(int *)(iVar14 + 0x48));
  }
  uStack_140 = ~(uint)(*(byte *)(iVar14 + 0x29) >> 1) & 2 | 1;
  puStack_144 = (undefined4 *)0x16;
  piStack_148 = DAT_0071d174;
  uStack_14c = 0x532bd0;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_14c = 7;
  iStack_150 = 0xa8;
  piStack_154 = DAT_0071d174;
  fStack_158 = 7.638093e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  fStack_158 = 1.4013e-45;
  fStack_15c = 3.78351e-44;
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,*(byte *)(iVar14 + 0x29) & 1);
  uVar21 = 0x7f;
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0x7f);
  uVar20 = 0;
  uVar18 = 0;
  piVar17 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
  chimera__rasterizer_set_framebuffer_blend_function();
  if (((*(char *)(iVar14 + 0x29) < '\0') && (*(int *)(iStack_5c + 0x74) != 0)) &&
     (0 < *(int *)(iVar14 + 0x54))) {
    sVar11 = *(short *)(*(int *)((*(uint *)(*(int *)(iVar14 + 0x58) + 0x78) & 0xffff) * 0x20 + 0x14
                                + DAT_0087bc14) + 0x60);
    if ((*(byte *)(iVar14 + 0x6c) & 2) == 0) {
      iVar5 = (int)(short)(ushort)*(byte *)(iVar14 + 0x28);
      fVar23 = (float)iVar5;
      iVar13 = (short)((sVar11 != 8) - 1 & 3) * 4;
      fVar16 = (float10)FUN_00623e40((double)(fVar23 * *(float *)(*(int *)(*(int *)(iStack_5c + 0x74
                                                                                   ) + 4) + iVar13)
                                             + 0.5));
      if ((int)ROUND((float)fVar16) < 0) {
        iVar5 = 0;
      }
      else {
        fVar16 = (float10)FUN_00623e40((double)(fVar23 * *(float *)(*(int *)(*(int *)(iStack_5c +
                                                                                     0x74) + 4) +
                                                                   iVar13) + 0.5));
        if ((int)ROUND((float)fVar16) <= iVar5) {
          fVar16 = (float10)FUN_00623e40((double)(fVar23 * *(float *)(*(int *)(*(int *)(iStack_5c +
                                                                                       0x74) + 4) +
                                                                     iVar13) + 0.5));
          iVar5 = (int)ROUND((float)fVar16);
        }
      }
      sVar4 = (short)iVar5;
      if (0 < (short)*(ushort *)(iStack_5c + 0x10)) {
        uVar7 = (uint)*(ushort *)(iStack_5c + 0x10);
        do {
          iVar5 = (int)(short)iVar5 / (int)sVar11;
          sVar4 = (short)iVar5;
          uVar7 = uVar7 - 1;
        } while (uVar7 != 0);
      }
      piStack_154 = (int *)((int)sVar4 % (int)sVar11);
    }
    else {
      piStack_154 = (int *)game_timer_get_digit_pair();
    }
  }
  sVar11 = (short)uVar21;
  cVar19 = (char)((uint)uVar18 >> 0x18);
  if (DAT_007c118c < 0xffff0101) {
    uVar1 = *(ushort *)(iVar14 + 0x60);
    uVar7 = (uint)uVar1;
    if (0 < (short)uVar1) {
      iVar5 = *(int *)(iVar14 + 100);
      piVar8 = (int *)&stack0xfffffee4;
      uVar10 = (uint)uVar1;
      do {
        *piVar8 = iVar5;
        iVar5 = iVar5 + 0xdc;
        piVar8 = piVar8 + 1;
        uVar10 = uVar10 - 1;
      } while (uVar10 != 0);
    }
  }
  else {
    uVar1 = *(ushort *)(iVar14 + 0x54);
    uVar7 = (uint)uVar1;
    if (0 < (short)uVar1) {
      iVar5 = *(int *)(iVar14 + 0x58);
      piVar8 = (int *)&stack0xfffffee4;
      uVar10 = (uint)uVar1;
      do {
        *piVar8 = iVar5;
        iVar5 = iVar5 + 0xdc;
        piVar8 = piVar8 + 1;
        uVar10 = uVar10 - 1;
      } while (uVar10 != 0);
    }
  }
  iVar5 = 0;
  if (0 < (short)uVar7) {
    iVar13 = 0;
    if ((short)uVar7 < 1) goto LAB_00533090;
    do {
      iVar5 = iVar13;
      sVar11 = (short)iVar5;
      iVar13 = (int)sVar11;
      pbVar3 = (byte *)auStack_10c[iVar13 + -4];
      if (sVar11 == 0) {
        uVar22 = *(undefined2 *)(&DAT_0065e3e0 + *(short *)(iVar14 + 0x2a) * 2);
      }
      else {
        uVar22 = 0;
      }
      iVar6 = iVar5;
      chimera__rasterizer_set_texture(iVar5,uVar22,0,piStack_154);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar13);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar13,2,uVar20);
      if (sVar11 == 0) {
        uVar9 = *(undefined4 *)(&DAT_0065e3e8 + (short)piVar17 * 4);
      }
      else {
        uVar9 = 1;
      }
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar13,3,uVar9);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar13,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar13,6,2 - (uint)((*pbVar3 & 1) != 0));
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,iVar13,7,2 - (uint)((*pbVar3 & 1) != 0));
      if ((sVar11 < 1) && (*(short *)(iVar14 + 0x2a) != 0)) {
        if ((*(byte *)(iVar14 + 0x29) & 8) == 0) {
LAB_00533090:
          iVar13 = (int)(short)iVar5;
          auStack_10c[iVar13 * 8] = 0x3f800000;
          auStack_10c[iVar13 * 8 + 1] = 0;
          auStack_10c[iVar13 * 8 + 2] = 0;
          auStack_10c[iVar13 * 8 + 3] = 0;
          auStack_10c[iVar13 * 8 + 4] = 0;
          auStack_10c[iVar13 * 8 + 5] = 0x3f800000;
          *(undefined4 *)(local_f4 + iVar13 * 0x10) = 0;
          local_f0[iVar13 * 8] = 0;
        }
        else {
          auStack_10c[iVar13 * 8] = DAT_007c12c4;
          auStack_10c[iVar13 * 8 + 3] = 0;
          local_f0[iVar13 * 8] = 0;
          auStack_10c[iVar13 * 8 + 1] = DAT_007c12c8;
          auStack_10c[iVar13 * 8 + 2] = DAT_007c12cc;
          auStack_10c[iVar13 * 8 + 4] = DAT_007c12d0;
          auStack_10c[iVar13 * 8 + 5] = DAT_007c12d4;
          *(undefined4 *)(local_f4 + iVar13 * 0x10) = DAT_007c12d8;
        }
      }
      else {
        fStack_15c = *(float *)(pbVar3 + 0x54);
        fStack_158 = *(float *)(pbVar3 + 0x58);
        if (sVar11 == 0) {
          if ((*(byte *)(iVar14 + 0x29) & 0x40) != 0) {
            fStack_15c = -(fStack_15c * *(float *)(iStack_5c + 0x78));
            fStack_158 = -(fStack_158 * *(float *)(iStack_5c + 0x78));
          }
LAB_00532fb2:
          if ((*(byte *)(iVar14 + 0x29) & 8) == 0) goto LAB_00532fc1;
        }
        else {
          if (sVar11 < 1) goto LAB_00532fb2;
LAB_00532fc1:
          fStack_15c = fStack_15c * *(float *)(iStack_5c + 0x3c);
          fStack_158 = fStack_158 * *(float *)(iStack_5c + 0x40);
        }
        iVar5 = iVar6;
        shader_texture_animation_evaluate
                  (fStack_15c,fStack_158,*(undefined4 *)(pbVar3 + 0x5c),
                   *(undefined4 *)(pbVar3 + 0x60));
        iVar14 = iStack_150;
      }
      sVar11 = (short)uVar21;
      cVar19 = (char)((uint)uVar18 >> 0x18);
      iVar13 = iVar5 + 1;
    } while ((short)(iVar5 + 1) < (short)uVar7);
  }
  sVar4 = (short)uVar7;
  uVar10 = uVar7;
  iVar5 = (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,auStack_10c,8);
  if ((-1 < iVar5) && (cVar19 != '\0')) {
    FUN_00537d60();
  }
  if (((*pbStack_6c & 0x10) != 0) && (*(short *)(iVar14 + 0x2c) == 0))
  goto switchD_00533269_default;
  sVar2 = *(short *)(iVar14 + 0x30);
  fStack_15c = 0.0;
  fStack_158 = 0.0;
  piStack_154 = (int *)0x0;
  iStack_150 = 0;
  uStack_14c = 0;
  piStack_148 = (int *)0x0;
  puStack_144 = (undefined4 *)0x0;
  uStack_140 = 0;
  piStack_13c = (int *)0x0;
  uStack_138 = 0;
  piStack_134 = (int *)0x3f800000;
  uStack_130 = 0;
  if ((*(short *)(pbStack_6c + 0x14) == 1) && ((*(byte *)(iVar14 + 0x6c) & 1) == 0)) {
    piStack_134 = (int *)(1.0 - *(float *)(pbStack_6c + 0x18));
    if (0.0 <= (float)piStack_134) {
      if (1.0 < (float)piStack_134) {
        piStack_134 = (int *)0x3f800000;
      }
    }
    else {
      piStack_134 = (int *)0x0;
    }
  }
  if (((0 < sVar2) && (*(int *)(pbStack_6c + 0x74) != 0)) &&
     (iVar5 = *(int *)(*(int *)(pbStack_6c + 0x74) + 4), iVar5 != 0)) {
    if ((*(float *)(iVar5 + -4 + sVar2 * 4) == 0.0) && (DAT_007c118c < 0xffff0101)) {
      return;
    }
    piStack_134 = (int *)((float)piStack_134 * *(float *)(iVar5 + -4 + sVar2 * 4));
  }
  (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&fStack_15c,3);
  sVar2 = *(short *)(iVar14 + 0x2e);
  if (sVar2 == 0) {
    uVar10 = 0x20;
  }
  else if (sVar2 == 1) {
    uVar10 = 0x24;
  }
  else if (sVar2 == 2) {
    uVar10 = 4;
  }
  switch(*(undefined2 *)(iVar14 + 0x2c)) {
  case 0:
    if ((DAT_007c1158 == 2) && (1 < sVar4)) {
      iVar14 = sVar4 + -1;
      if (iVar14 < 2) {
        iVar14 = 1;
      }
      sVar11 = (short)iVar14;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
      uVar18 = 6;
      iVar14 = 0;
      goto LAB_005335b7;
    }
    iVar14 = (int)sVar11;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,1,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,2,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,4,4);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,5,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,6,uVar10);
    break;
  case 1:
  case 5:
    if ((DAT_007c1158 < 3) && (uVar7 = (int)sVar4 - 1, (int)uVar7 < 2)) {
      uVar7 = 1;
    }
    sVar11 = (short)uVar7;
    iVar14 = (int)sVar11;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,1,0x19);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,2,uVar10 | 0x10);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,3,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,0x1a,uVar10);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,4,2);
    uVar10 = 1;
    uVar18 = 5;
    goto LAB_005335b7;
  case 2:
    if ((DAT_007c1158 < 3) && (uVar7 = (int)sVar4 - 1, (int)uVar7 < 2)) {
      uVar7 = 1;
    }
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0x7f7f7f7f);
    sVar11 = (short)uVar7;
    iVar14 = (int)sVar11;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,1,0x1a);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,2,uVar10);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,3,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,0x1a,3);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,4,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,5,1);
    break;
  case 3:
  case 4:
  case 6:
    if ((DAT_007c1158 != 2) || (sVar4 < 2)) {
      iVar14 = (int)sVar11;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,2,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,3,uVar10);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,4,2);
      uVar10 = 1;
      uVar18 = 5;
      goto LAB_005335b7;
    }
    iVar14 = sVar4 + -1;
    if (iVar14 < 2) {
      iVar14 = 1;
    }
    sVar11 = (short)iVar14;
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,uVar10);
    break;
  case 7:
    if ((DAT_007c1158 == 2) && (1 < sVar4)) {
      iVar14 = sVar4 + -1;
      if (iVar14 < 2) {
        iVar14 = 1;
      }
      sVar11 = (short)iVar14;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,uVar10);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
      uVar18 = 6;
      iVar14 = 0;
    }
    else {
      iVar14 = (int)sVar11;
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,2,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,3,uVar10);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,4,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,5,1);
      uVar18 = 6;
    }
LAB_005335b7:
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,uVar18,uVar10);
    break;
  default:
    goto switchD_00533269_default;
  }
  sVar4 = sVar11 + 1;
switchD_00533269_default:
  iVar14 = (int)sVar4;
  (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,iVar14,0);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,1,1);
  piVar17 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,iVar14,4,1);
  rasterizer_transparent_geometry_group_draw_vertices(pvStack_98,(void *)0x0,(char)piVar17);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
