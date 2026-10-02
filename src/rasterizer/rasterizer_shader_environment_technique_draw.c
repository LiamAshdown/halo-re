// rasterizer_shader_environment_technique_draw  (Ghidra: FUN_00520970)
// address 0x520970, size 539 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: draws a shader_environment surface through the environment effect selected by
//   rasterizer_shader_environment_technique_multipurpose_set_states (0x0071d1d0 =
//   &rasterizer_effects[36], cleared by the render module at 0x50c351). Runs only with
//   0x006893f8 and 0x006893fa set, 0x0069c67c and 0x006e0a0c clear, a positive perpendicular or
//   parallel brightness and ShaderEnvironment.lightmap_brightness_scale (+0x2d4) below 1: the
//   bump transform at vertex shader c10..c12 (the draw is abandoned when that upload fails),
//   the lightmap brightness scale in all four lanes of pixel shader c1, the declaration of
//   vertex type 2 and the effect vertex shader, then one two stream draw (0x51c310, second
//   stream = the lightmap vertices at vertex_buffer + 1) per effect pass.
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x520970..0x520b8a; EAX is the
//   vertex buffer and ECX the ShaderEnvironment, and the draw arguments are the stack arguments.
// register convention: EAX = vertex_buffer, ECX = shader, stack = (dynamic_index_slot,
//   first_primitive, primitive_count).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                                     // 0x0071d174
extern rasterizer_frame_time rasterizer_time;                       // 0x007c1200
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_effect_slot *rasterizer_active_environment_effect; // 0x0071d1d0
extern uint8_t rasterizer_environment_lightmap_missing;             // 0x006e0a0c set by 0x520910
extern int16_t render_force_flag;                             // 0x0069c67c UNSURE (read as a word)
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f8;                         // 0x006893f8
extern uint8_t console_debug_toggle_6893fa;                         // 0x006893fa

// blam-cc: ESI -> shader_environment
extern void shader_environment_texture_scrolling_evaluate(float *u, float *v, double time, const ShaderEnvironment *shader); // 0x540060
// blam-cc: EAX -> primitive_count, EDI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive, second_stream)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                        int32_t dynamic_index_slot, int32_t first_primitive,
                                                                        rasterizer_vertex_buffer *second_stream); // 0x51c310

typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

// blam-cc: EAX -> vertex_buffer, ECX -> shader
void rasterizer_shader_environment_technique_draw(rasterizer_vertex_buffer *vertex_buffer, const ShaderEnvironment *shader,
                                                  int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count)
{
    const uint8_t *raw = (const uint8_t *)shader;
    rasterizer_effect_slot *effect_slot;
    void *effect;
    float constants[12];
    float scale[4];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f8 == 0 || console_debug_toggle_6893fa == 0 ||
        render_force_flag != 0 || rasterizer_environment_lightmap_missing != 0) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->perpendicular_brightness > 0.0f) && !(((struct ShaderEnvironment *)raw)->parallel_brightness > 0.0f)) {
        return;
    }
    if (!(((struct ShaderEnvironment *)raw)->lightmap_brightness_scale < 1.0f)) {             // lightmap_brightness_scale
        return;
    }

    constants[0] = *(float *)&((struct ShaderEnvironment *)raw)->bump_map_scale_xy;
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 1.0f;
    constants[3] = 1.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, shader);
    if (((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xa, constants, 3) < 0) {
        return;
    }

    effect_slot = rasterizer_active_environment_effect;
    if (effect_slot == 0 || effect_slot->effect == 0) {
        return;
    }
    effect = (void *)effect_slot->effect;
    scale[0] = ((struct ShaderEnvironment *)raw)->lightmap_brightness_scale;
    scale[1] = scale[0];
    scale[2] = scale[0];
    scale[3] = scale[0];
    ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 1, scale, 1);
    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[2].declaration);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device,
                                                     (void *)rasterizer_vertex_shaders[effect_slot->vertex_shader_index].shader);

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive,
                                                                    vertex_buffer + 1);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x520970):

void FUN_00520970(void)

{
  int in_EAX;
  int iVar1;
  int in_ECX;
  int *piVar2;
  int *piVar3;
  undefined4 uStack_74;
  int *piStack_70;
  undefined4 uStack_6c;
  undefined1 *puStack_68;
  undefined4 uStack_64;
  int *piStack_60;
  undefined4 *puStack_5c;
  undefined8 local_58;
  undefined4 uStack_38;
  undefined4 uStack_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  if ((((((DAT_006893e4 == 0) && (DAT_006893f8 != '\0')) && (DAT_006893fa != '\0')) &&
       ((DAT_0069c67c == 0 && (DAT_006e0a0c == '\0')))) &&
      ((0.0 < *(float *)(in_ECX + 0x2f4) || (0.0 < *(float *)(in_ECX + 0x2f8))))) &&
     (*(float *)(in_ECX + 0x2d4) < 1.0)) {
    local_30 = *(undefined4 *)(in_ECX + 0x138);
    local_2c = *(undefined4 *)(in_ECX + 0x13c);
    puStack_5c = &local_4;
    local_58 = _DAT_007c1200;
    piStack_60 = &local_14;
    local_28 = 0x3f800000;
    local_24 = 0x3f800000;
    local_20 = 0x3f800000;
    local_1c = 0;
    local_18 = 0;
    local_14 = 0;
    local_10 = 0;
    local_c = 0x3f800000;
    local_8 = 0;
    local_4 = 0;
    uStack_64 = 0x520a81;
    FUN_00540060();
    local_58 = CONCAT44(3,&local_30);
    puStack_5c = (undefined4 *)0xa;
    piStack_60 = DAT_0071d174;
    uStack_64 = 0x520a9b;
    iVar1 = (**(code **)(*DAT_0071d174 + 0x178))();
    if (((-1 < iVar1) && (DAT_0071d1d0 != (int *)0x0)) && (*DAT_0071d1d0 != 0)) {
      uStack_64 = 1;
      puStack_68 = &stack0xffffffb0;
      uStack_6c = 1;
      piStack_70 = DAT_0071d174;
      uStack_74 = 0x520aea;
      (**(code **)(*DAT_0071d174 + 0x1b4))();
      uStack_74 = DAT_006e1aa8;
      (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174);
      piVar3 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,(&DAT_0069e350)[DAT_0071d1d0[1] * 2]);
      (**(code **)(*(int *)*DAT_0071d1d0 + 0x100))((int *)*DAT_0071d1d0,&uStack_74,3);
      piVar2 = (int *)0x0;
      if (piVar3 != (int *)0x0) {
        do {
          (**(code **)(*(int *)*DAT_0071d1d0 + 0x104))((int *)*DAT_0071d1d0,piVar2);
          chimera__rasterizer_draw_dynamic_triangles_static_vertices2
                    (uStack_38,uStack_34,in_EAX + 0x14);
          piVar2 = (int *)((int)piVar2 + 1);
        } while (piVar2 < piVar3);
      }
      (**(code **)(*(int *)*DAT_0071d1d0 + 0x108))((int *)*DAT_0071d1d0);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
