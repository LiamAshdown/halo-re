// rasterizer_object_shadow_model_draw  (Ghidra: FUN_00531350; the phase 3 rewrite called it
//   rasterizer_motion_sensor_hud_blip_draw)
// address 0x531350, size 536 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: raw disassembly (phase 4 review). Only caller is the model draw 0x4d72a0 (reached
//   from model_render_draw 0x4d6fc0), on the branch taken when its flags have bit 1 set; that
//   branch runs after 0x4d6fc0 has pointed 0x0071d260 at its local rasterizer_model_draw_context
//   and set 0x0071d265, i.e. while an object is being drawn into the shadow silhouette target.
//   Call site: EAX -> shader (ShaderModel*), pushes (frame, part +0x44, part +0x54).
// What it does: for a shader_model (Shader.shader_type 4) in the 3D window with object shadows
//   on: two sided shaders draw without culling; alpha tested shaders (not_alpha_tested clear)
//   bind the base map with wrap / linear sampling so the silhouette keeps its cut-outs; uploads
//   c10 {detail_map_scale, detail_map_scale * detail_map_v_scale, 1, 1} and the animated base
//   map texture transform (c11, c12) evaluated against the shadow model context; sets the
//   declaration of the vertex buffer, vertex shader 33 and no pixel shader, and draws the part
//   through rasterizer_dynamic_geometry_chain_draw 0x51c5f0.
// register convention: EAX -> shader, stack -> (frame, index_buffer, vertex_buffer).
// blam-cc: EAX -> shader, stack -> (frame, index_buffer, vertex_buffer)
// reconciled: R43 rasterizer_model_draw_context unknown_84[2] -> change_colors/function_values (the render_animation pair), unknown_c0/c4/c8 -> bounding_radius/base_map_u_scale/base_map_v_scale (same offsets)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern rasterizer_frame_time rasterizer_time;               // 0x007c1200
extern uint8_t unknown_0069c689;                            // 0x0069c689
extern uint8_t console_debug_toggle_6893f2;                 // 0x006893f2 object shadows enabled
extern rasterizer_model_draw_context *rasterizer_object_shadow_model_context; // 0x0071d260
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960
// blam-cc: ECX -> function_source, ESI -> animation, EBX -> out_u, EDI -> out_v, stack -> the rest
extern void shader_texture_animation_evaluate(const void *function_source, const void *animation,
                                              float *out_u, float *out_v, float u_scale, float v_scale,
                                              float unused_z, float unused_w, float unused_5,
                                              float time); // 0x53fe50
// blam-cc: EAX -> vertex_buffer, EDI -> index_buffer, stack -> primitive_count
extern void rasterizer_dynamic_geometry_chain_draw(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                   rasterizer_index_buffer *index_buffer); // 0x51c5f0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}

void rasterizer_object_shadow_model_draw(const ShaderModel *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
                                         rasterizer_vertex_buffer *vertex_buffer)
{
    float constants[3][4];  // c10, then c11 / c12 written by the animation evaluate
    rasterizer_model_draw_context *context;

    if (rasterizer_window.type != 1 || unknown_0069c689 != 0 || console_debug_toggle_6893f2 == 0) {
        return;
    }
    if (shader->base.shader_type != 4) {
        return;
    }
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, 0x16,
                                              (shader->shader_model_flags & 2) ? 1 : 3); // CULLMODE
    if ((shader->shader_model_flags & 4) == 0) {                                     // alpha tested
        chimera__rasterizer_set_texture(*(const uint32_t *)&shader->base_map.tag_id, 0, 0, 1, frame);
        set_sampler_state(0, 1, 1);      // ADDRESSU WRAP
        set_sampler_state(0, 2, 1);      // ADDRESSV WRAP
        set_sampler_state(0, 5, 2);      // MAGFILTER LINEAR
        set_sampler_state(0, 6, 2);      // MINFILTER LINEAR
        set_sampler_state(0, 7, 2);      // MIPFILTER LINEAR
    }
    context = rasterizer_object_shadow_model_context;
    constants[0][0] = shader->detail_map_scale;
    constants[0][1] = shader->detail_map_v_scale * shader->detail_map_scale;
    constants[0][2] = 1.0f;
    constants[0][3] = 1.0f;
    constants[1][0] = 1.0f; constants[1][1] = 0.0f; constants[1][2] = 0.0f; constants[1][3] = 0.0f;
    constants[2][0] = 0.0f; constants[2][1] = 1.0f; constants[2][2] = 0.0f; constants[2][3] = 0.0f;
    shader_texture_animation_evaluate(&context->change_colors, &shader->u_animation_source, constants[1], constants[2],
                                      context->base_map_u_scale * shader->map_u_scale,
                                      context->base_map_v_scale * shader->map_v_scale, 0.0f, 0.0f, 0.0f,
                                      (float)rasterizer_time.time);
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xa, &constants[0][0], 3);
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[vertex_buffer->type].declaration);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[33].shader);
    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);
    rasterizer_dynamic_geometry_chain_draw(index_buffer->count, vertex_buffer, index_buffer);
}

#if 0
Original Ghidra decompilation (0x531350): phase 3 file rasterizer_motion_sensor_hud_blip_draw replaced

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00531350(undefined4 param_1,undefined4 param_2,short *param_3)

{
  int in_EAX;
  undefined4 uStack_8;
  
  if (((((short)DAT_007c1220 == 1) && (DAT_0069c689 == '\0')) && (DAT_006893f2 != '\0')) &&
     (*(short *)(in_EAX + 0x24) == 4)) {
    if ((*(byte *)(in_EAX + 0x28) & 2) == 0) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    }
    else {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,1);
    }
    if ((*(byte *)(in_EAX + 0x28) & 4) == 0) {
      chimera__rasterizer_set_texture(0,0,1,uStack_8);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,1);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,1);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
    }
    shader_texture_animation_evaluate
              (*(float *)(DAT_0071d260 + 0xc4) * *(float *)(in_EAX + 0x9c),
               *(float *)(DAT_0071d260 + 200) * *(float *)(in_EAX + 0xa0),0,0,0,(float)_DAT_007c1200
              );
    (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&stack0xffffffc4,3);
    (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,(&DAT_006e1a90)[*param_3 * 3]);
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e458);
    (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
    FUN_0051c5f0(uRam3f800004);
  }
  return;
}
#endif
