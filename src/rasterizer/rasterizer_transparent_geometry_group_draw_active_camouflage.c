// rasterizer_transparent_geometry_group_draw_active_camouflage  (Ghidra: FUN_00519f70, unnamed)
// address 0x519f70, size 1768 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: draws one transparent_geometry_group in the main window (window type 1) as active
//   camouflage. On the pixel shader path it binds the frame captured by
//   rasterizer_render_target_capture_frame (render target 2 texture, 0x0069d390) on stage 2 and
//   the GlobalsRasterizerData active_camouflage_distortion bitmap (+0x5c tag id) on stage 0,
//   uploads refraction_amount / distance_falloff / tint_color (+0x174..+0x184) lerped toward
//   the hyper stealth values (+0x188..+0x198) by group.parameters.distortion_factor, and draws
//   the group's vertices once through effect 105 (0x0069e130) and vertex shader 30
//   (0x0069e440), pass flags & 1. Without pixel shaders the camouflage amount is capped at 0.9.
//   If the amount (group.parameters.blend_factor) is below 1 the group is drawn again with its
//   own shader, faded by 1 - amount (0x0071d200, latched by 0x0071d1fe), through a model draw
//   context built on the stack (rasterizer_model_draw_prepare_states / rasterizer_shader_environment_draw_dispatch / rasterizer_model_draw_restore_states).
//   Ghidra lost the blend factor into an unaffected register (unaff_EBX), dropped the 0.9 cap
//   (0x51a474..0x51a485), the stack context's flags/node fields and every device call argument
//   list; all of these follow the raw code 0x519f70..0x51a657.
// register convention: __cdecl, group on the stack (ebp+8); the frame is 8 byte aligned.
// reconciled: R43 rasterizer_model_draw_context unknown_84[2] -> change_colors/function_values (the render_animation pair), unknown_c0/c4/c8 -> bounding_radius/base_map_u_scale/base_map_v_scale (same offsets)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern uint32_t rasterizer_frustum_z_values[2];                     // 0x0069c664
extern uint8_t rasterizer_caps_flag_688;                            // 0x0069c688
extern uint8_t rasterizer_caps_flag_68a;                            // 0x0069c68a
extern uint8_t console_debug_toggle_689421;                         // 0x00689421 render target capture enable
extern uint8_t console_debug_toggle_6893ec;                         // 0x006893ec
extern uint8_t rasterizer_widescreen_camouflage_scale;              // 0x0071d18c UNSURE: nonzero scales
                                                                    //   the distortion by the half viewport
extern uint8_t rasterizer_camouflage_fade_active;                   // 0x0071d1fe
extern float rasterizer_camouflage_fade;                            // 0x0071d200 1 - camouflage amount
extern uint8_t rasterizer_render_states_dirty; // 0x0069c74c
extern uint8_t unknown_0071d1fa;                                    // 0x0071d1fa UNSURE

// blam-cc: stack -> (z_near, z_far) as raw float bits
extern void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far); // 0x518f40
// blam-cc: EDX -> group
extern uint32_t transparent_geometry_group_get_vertex_type_reference(transparent_geometry_group *group); // 0x515400
// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, frame)
extern uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage, int16_t frame); // 0x518770
// blam-cc: ECX -> group
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x00533660
// blam-cc: ESI -> context, stack -> flag
extern void rasterizer_model_draw_prepare_states(rasterizer_model_draw_context *context, uint8_t flag); // 0x526f50
// blam-cc: EBX -> dynamic_vertex_slot, the rest on the stack
extern void rasterizer_shader_environment_draw_dispatch(int32_t dynamic_vertex_slot, uint32_t shader, uint32_t shader_permutation, uint32_t index_buffer,
                         int32_t dynamic_index_slot, int32_t primitive_count, uint32_t vertex_buffer); // 0x52b050
extern void rasterizer_model_draw_restore_states(void);                                     // 0x52b530
extern void render_lighting_disable_workaround(void);                                     // 0x511ef0, render module

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_texture_fn)(void *self, uint32_t stage, void *texture);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *self, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *self, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *self);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

static void rasterizer_set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, stage, type, value);
}

static float real_lerp(float from, float to, float t)
{
    return (1.0f - t) * from + to * t;
}

void rasterizer_transparent_geometry_group_draw_active_camouflage(transparent_geometry_group *group)
{
    const Shader *shader;
    float amount;

    if (console_debug_toggle_689421 == 0 || rasterizer_window.type != 1) {
        return;
    }
    amount = group->parameters.blend_factor;
    shader = (const Shader *)group->shader;
    if ((int8_t)group->flags < 0) {                                 // _group_sort_first_bit
        chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
    }

    if (rasterizer_caps_flag_688 == 0 && rasterizer_caps_flag_68a == 0 &&
        rasterizer_caps.pixel_shader_version >= 0xffff0101) {
        void *effect = (void *)rasterizer_effects[105].effect;

        if (effect != 0) {
            const GlobalsRasterizerData *data = rasterizer_globals_data;
            float t = group->parameters.distortion_factor;
            float half_height;
            float constants[12];
            uint32_t passes;
            int16_t vertex_type;

            vertex_type = (int16_t)transparent_geometry_group_get_vertex_type_reference(group);
            ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device,
                                                             (void *)rasterizer_vertex_declarations[vertex_type].declaration);
            // two sided shaders (flags bit 1) get D3DCULL_NONE, the rest D3DCULL_CCW
            rasterizer_set_render_state(0x16, (~(uint32_t)*(uint16_t *)((const uint8_t *)shader + 0x28) & 2) | 1);
            rasterizer_set_render_state(0xa8, 7);                   // D3DRS_COLORWRITEENABLE rgb
            rasterizer_set_render_state(7, 1);                      // D3DRS_ZENABLE
            rasterizer_set_render_state(0xe, 1);                    // D3DRS_ZWRITEENABLE
            rasterizer_set_render_state(0x17, 4);                   // D3DRS_ZFUNC lessequal
            rasterizer_set_render_state(0x1c, 0);                   // D3DRS_FOGENABLE
            rasterizer_set_render_state(0x1b, 0);                   // D3DRS_ALPHABLENDENABLE
            chimera__rasterizer_set_texture_direct_d3d9(*(const uint32_t *)&data->active_camouflage_distortion.tag_id, 0, 0);
            rasterizer_set_sampler_state(0, 1, 3);
            rasterizer_set_sampler_state(0, 2, 3);
            rasterizer_set_sampler_state(0, 3, 3);
            rasterizer_set_sampler_state(0, 5, 2);
            rasterizer_set_sampler_state(0, 6, 2);
            rasterizer_set_sampler_state(0, 7, 2);
            ((d3d_set_texture_fn)device_vtable()[0x104 / 4])(rasterizer_device, 2, (void *)rasterizer_render_targets[2].texture);
            rasterizer_set_sampler_state(2, 1, 3);
            rasterizer_set_sampler_state(2, 2, 3);
            rasterizer_set_sampler_state(2, 5, 2);
            rasterizer_set_sampler_state(2, 6, 2);
            rasterizer_set_sampler_state(2, 7, 1);
            ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device, (void *)rasterizer_vertex_shaders[30].shader);

            half_height = (float)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top) * 0.5f;
            // c10: refraction scaled by the camouflage amount, distance falloff, distortion scale
            constants[0] = (1.0f / real_lerp(data->refraction_amount, data->hyper_stealth_refraction, t)) * amount;
            constants[1] = real_lerp(data->distance_falloff, data->hyper_stealth_distance_falloff, t);
            if (rasterizer_widescreen_camouflage_scale != 0) {
                constants[2] = (float)(rasterizer_window.camera.viewport_bounds.right -
                                       rasterizer_window.camera.viewport_bounds.left) * 0.5f;
                constants[3] = half_height;
            } else {
                constants[2] = 1.0f;
                constants[3] = 1.0f;
            }
            // c11: zero
            constants[4] = 0.0f;
            constants[5] = 0.0f;
            constants[6] = 0.0f;
            constants[7] = 0.0f;
            // c12: tint
            constants[8] = real_lerp(data->tint_color.red, data->hyper_stealth_tint_color.red, t);
            constants[9] = real_lerp(data->tint_color.green, data->hyper_stealth_tint_color.green, t);
            constants[10] = real_lerp(data->tint_color.blue, data->hyper_stealth_tint_color.blue, t);
            constants[11] = 0.0f;
            ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 10, constants, 3);

            // c27, c28: view forward and left axes
            constants[0] = rasterizer_window.frustum.view_to_world.forward.i;
            constants[1] = rasterizer_window.frustum.view_to_world.forward.j;
            constants[2] = rasterizer_window.frustum.view_to_world.forward.k;
            constants[3] = 1.0f;
            constants[4] = rasterizer_window.frustum.view_to_world.left.i;
            constants[5] = rasterizer_window.frustum.view_to_world.left.j;
            constants[6] = rasterizer_window.frustum.view_to_world.left.k;
            constants[7] = 3.0f;
            ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0x1b, constants, 2);

            // c4, c5: camera position and forward
            constants[0] = rasterizer_window.camera.position.x;
            constants[1] = rasterizer_window.camera.position.y;
            constants[2] = rasterizer_window.camera.position.z;
            constants[3] = 2.0f;
            constants[4] = rasterizer_window.camera.forward.i;
            constants[5] = rasterizer_window.camera.forward.j;
            constants[6] = rasterizer_window.camera.forward.k;
            constants[7] = 0.5f;
            ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 4, constants, 2);

            // pixel shader c0: the camouflage amount in all four channels
            constants[0] = amount;
            constants[1] = amount;
            constants[2] = amount;
            constants[3] = amount;
            ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, constants, 1);

            ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
            ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, data->flags & 1);
            rasterizer_transparent_geometry_group_draw_vertices(group, 0);
            ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
        }
        rasterizer_set_render_state(0x17, 3);                       // D3DRS_ZFUNC equal
    } else {
        if (amount > 0.9f) {
            amount = 0.9f;
        }
        rasterizer_set_render_state(0x17, 3);                       // D3DRS_ZFUNC equal
    }

    if (amount < 1.0f) {
        rasterizer_model_draw_context context;

        // uninitialised in the original: unknown_04, unknown_0e, bounding_radius
        context.flags = group->flags & (_group_sort_first_bit | _group_node_parts_bit);
        context.node_matrices = group->node_matrices;
        context.node_count = group->node_count;
        if (group->lighting != 0) {
            context.lighting = *(const render_lighting *)group->lighting;
        } else {
            uint32_t *words = (uint32_t *)&context.lighting;
            int32_t i;

            for (i = 0; i < 0x1d; i++) {
                words[i] = 0;
            }
        }
        if (group->lighting_extra != 0) {
            context.change_colors = ((const uint32_t *)group->lighting_extra)[0];
            context.function_values = ((const uint32_t *)group->lighting_extra)[1];
        } else {
            context.change_colors = 0;
            context.function_values = 0;
        }
        {
            uint32_t *words = (uint32_t *)&context.group_parameters;
            int32_t i;

            for (i = 0; i < 10; i++) {
                words[i] = 0;
            }
        }
        context.center = group->position;
        context.base_map_u_scale = group->base_map_u_scale;
        context.base_map_v_scale = group->base_map_v_scale;

        rasterizer_set_render_state(0xe, 0);                        // D3DRS_ZWRITEENABLE off
        rasterizer_camouflage_fade_active = 1;
        rasterizer_camouflage_fade = 1.0f - amount;
        if (console_debug_toggle_6893ec != 0) {
            rasterizer_render_states_dirty = 1;
            unknown_0071d1fa = 0;
            if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
                rasterizer_set_render_state(0x89, 1);               // D3DRS_LIGHTING
            }
        }
        rasterizer_model_draw_prepare_states(&context, 1);
        rasterizer_shader_environment_draw_dispatch(group->dynamic_vertex_slot, group->shader, group->shader_permutation, group->index_buffer,
                     group->dynamic_index_slot, group->primitive_count, group->vertex_buffer);
        rasterizer_model_draw_restore_states();
        render_lighting_disable_workaround();
        rasterizer_camouflage_fade_active = 0;
    }

    if ((int8_t)group->flags < 0) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
}

#if 0
Original Ghidra decompilation (0x519f70):

void FUN_00519f70(char *param_1)

{
  int iVar1;
  short sVar2;
  int iVar3;
  float unaff_EBX;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int *piVar6;
  float fVar7;
  undefined4 uStack_26c;
  int *piStack_268;
  undefined4 uStack_264;
  int *piStack_260;
  undefined4 uStack_25c;
  float fStack_258;
  float fStack_254;
  int *piStack_250;
  float fStack_24c;
  float fStack_248;
  float fStack_244;
  int *piStack_240;
  undefined4 uStack_23c;
  undefined4 uStack_238;
  undefined4 uStack_234;
  int *piStack_230;
  float fStack_22c;
  float fStack_228;
  float fStack_224;
  int *piStack_220;
  undefined4 uStack_21c;
  undefined4 uStack_218;
  float fStack_214;
  int *piStack_210;
  undefined4 uStack_20c;
  undefined4 uStack_208;
  int *piStack_204;
  undefined4 uStack_200;
  undefined4 uStack_1fc;
  undefined4 uStack_1f8;
  int *piStack_1f4;
  undefined4 uStack_1f0;
  undefined4 uStack_1ec;
  undefined4 uStack_1e8;
  int *piStack_1e4;
  undefined4 uStack_1e0;
  undefined4 uStack_1dc;
  undefined4 uStack_1d8;
  int *piStack_1d4;
  undefined4 uStack_1d0;
  undefined4 uStack_1cc;
  undefined4 uStack_1c8;
  int *piStack_1c4;
  undefined4 uStack_1c0;
  undefined4 uStack_1bc;
  undefined4 uStack_1b8;
  int *piStack_1b4;
  undefined4 uStack_1b0;
  undefined4 uStack_1ac;
  undefined4 uStack_1a8;
  int *piStack_1a4;
  undefined4 uStack_1a0;
  undefined4 uStack_19c;
  int *piStack_198;
  undefined4 uStack_194;
  undefined4 uStack_190;
  int *piStack_18c;
  undefined4 uStack_188;
  undefined4 uStack_184;
  int *piStack_180;
  undefined4 uStack_17c;
  undefined4 uStack_178;
  int *piStack_174;
  undefined4 uStack_170;
  uint uStack_16c;
  int *piStack_168;
  undefined4 uStack_164;
  int *piStack_160;
  int *piStack_15c;
  undefined4 uStack_158;
  int *piStack_154;
  int *piStack_150;
  undefined4 uStack_14c;
  undefined4 auStack_d4 [29];
  undefined4 uStack_60;
  undefined4 uStack_5c;
  undefined4 uStack_58;
  undefined4 uStack_54;
  undefined4 uStack_50;
  undefined4 uStack_4c;
  undefined4 uStack_48;
  undefined4 uStack_44;
  undefined4 uStack_40;
  undefined4 uStack_3c;
  undefined4 uStack_38;
  undefined4 uStack_34;
  undefined4 uStack_30;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  undefined4 uStack_20;
  undefined4 uStack_1c;
  
  if ((DAT_00689421 != '\0') && ((short)DAT_007c1220 == 1)) {
    iVar3 = *(int *)(param_1 + 0xc);
    if (*param_1 < '\0') {
      uStack_14c = DAT_0069c668;
      piStack_150 = (int *)DAT_0069c664;
      piStack_154 = (int *)0x519fbf;
      chimera__rasterizer_set_frustum_z_func();
    }
    piVar6 = DAT_0071d174;
    if (((DAT_0069c688 == '\0') && (DAT_0069c68a == '\0')) && (0xffff0100 < DAT_007c118c)) {
      if (DAT_0069e130 != (int *)0x0) {
        iVar1 = *DAT_0071d174;
        uStack_14c = 0x51a019;
        sVar2 = FUN_00515400();
        uStack_14c = (&DAT_006e1a90)[sVar2 * 3];
        piStack_150 = piVar6;
        piStack_154 = (int *)0x51a032;
        (**(code **)(iVar1 + 0x15c))();
        piStack_154 = (int *)(~(uint)*(ushort *)(iVar3 + 0x28) & 2 | 1);
        uStack_158 = 0x16;
        piStack_15c = DAT_0071d174;
        piStack_160 = (int *)0x51a051;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        piStack_160 = (int *)0x7;
        uStack_164 = 0xa8;
        piStack_168 = DAT_0071d174;
        uStack_16c = 0x51a066;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_16c = 1;
        uStack_170 = 7;
        piStack_174 = DAT_0071d174;
        uStack_178 = 0x51a078;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_178 = 1;
        uStack_17c = 0xe;
        piStack_180 = DAT_0071d174;
        uStack_184 = 0x51a08a;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_184 = 4;
        uStack_188 = 0x17;
        piStack_18c = DAT_0071d174;
        uStack_190 = 0x51a09c;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_190 = 0;
        uStack_194 = 0x1c;
        piStack_198 = DAT_0071d174;
        uStack_19c = 0x51a0ae;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_19c = 0;
        uStack_1a0 = 0x1b;
        piStack_1a4 = DAT_0071d174;
        uStack_1a8 = 0x51a0c0;
        (**(code **)(*DAT_0071d174 + 0xe4))();
        uStack_1a8 = 0;
        uStack_1ac = 0;
        uStack_1b0 = 0x51a0d1;
        chimera__rasterizer_set_texture_direct_d3d9();
        uStack_1a8 = 3;
        uStack_1ac = 1;
        uStack_1b0 = 0;
        piStack_1b4 = DAT_0071d174;
        uStack_1b8 = 0x51a0e8;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_1b8 = 3;
        uStack_1bc = 2;
        uStack_1c0 = 0;
        piStack_1c4 = DAT_0071d174;
        uStack_1c8 = 0x51a0fc;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_1c8 = 3;
        uStack_1cc = 3;
        uStack_1d0 = 0;
        piStack_1d4 = DAT_0071d174;
        uStack_1d8 = 0x51a110;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_1d8 = 2;
        uStack_1dc = 5;
        uStack_1e0 = 0;
        piStack_1e4 = DAT_0071d174;
        uStack_1e8 = 0x51a124;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_1e8 = 2;
        uStack_1ec = 6;
        uStack_1f0 = 0;
        piStack_1f4 = DAT_0071d174;
        uStack_1f8 = 0x51a138;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_1f8 = 2;
        uStack_1fc = 7;
        uStack_200 = 0;
        piStack_204 = DAT_0071d174;
        uStack_208 = 0x51a14c;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_208 = DAT_0069d390;
        uStack_20c = 2;
        piStack_210 = DAT_0071d174;
        fStack_214 = 7.49657e-39;
        (**(code **)(*DAT_0071d174 + 0x104))();
        fStack_214 = 4.2039e-45;
        uStack_218 = 1;
        uStack_21c = 2;
        piStack_220 = DAT_0071d174;
        fStack_224 = 7.496598e-39;
        (**(code **)(*DAT_0071d174 + 0x114))();
        fStack_224 = 4.2039e-45;
        fStack_228 = 2.8026e-45;
        fStack_22c = 2.8026e-45;
        piStack_230 = DAT_0071d174;
        uStack_234 = 0x51a18b;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_234 = 2;
        uStack_238 = 5;
        uStack_23c = 2;
        piStack_240 = DAT_0071d174;
        fStack_244 = 7.496654e-39;
        (**(code **)(*DAT_0071d174 + 0x114))();
        fStack_244 = 2.8026e-45;
        fStack_248 = 8.40779e-45;
        fStack_24c = 2.8026e-45;
        piStack_250 = DAT_0071d174;
        fStack_254 = 7.496682e-39;
        (**(code **)(*DAT_0071d174 + 0x114))();
        fStack_254 = 1.4013e-45;
        fStack_258 = 9.80909e-45;
        uStack_25c = 2;
        piStack_260 = DAT_0071d174;
        uStack_264 = 0x51a1c7;
        (**(code **)(*DAT_0071d174 + 0x114))();
        uStack_264 = DAT_0069e440;
        piStack_268 = DAT_0071d174;
        uStack_26c = 0x51a1dc;
        (**(code **)(*DAT_0071d174 + 0x170))();
        piStack_250 = (int *)((float)((int)(short)DAT_007c1258 - (int)(short)DAT_007c1254) * 0.5);
        fStack_258 = 1.0 - *(float *)(param_1 + 0x1c);
        fStack_22c = *(float *)(DAT_0071d164 + 400) * *(float *)(param_1 + 0x1c) +
                     fStack_258 * *(float *)(DAT_0071d164 + 0x17c);
        fStack_228 = *(float *)(DAT_0071d164 + 0x194) * *(float *)(param_1 + 0x1c) +
                     fStack_258 * *(float *)(DAT_0071d164 + 0x180);
        fStack_224 = *(float *)(DAT_0071d164 + 0x198) * *(float *)(param_1 + 0x1c) +
                     fStack_258 * *(float *)(DAT_0071d164 + 0x184);
        fStack_24c = (1.0 / (*(float *)(DAT_0071d164 + 0x188) * *(float *)(param_1 + 0x1c) +
                            fStack_258 * *(float *)(DAT_0071d164 + 0x174))) * fStack_254;
        fStack_248 = *(float *)(DAT_0071d164 + 0x18c) * *(float *)(param_1 + 0x1c) +
                     fStack_258 * *(float *)(DAT_0071d164 + 0x178);
        if (DAT_0071d18c == '\0') {
          fStack_244 = 1.0;
          piStack_240 = (int *)0x3f800000;
        }
        else {
          fStack_258 = (float)((int)DAT_007c1258._2_2_ - (int)DAT_007c1254._2_2_);
          fStack_244 = (float)(int)fStack_258 * 0.5;
          piStack_240 = piStack_250;
        }
        uStack_26c = 3;
        uStack_23c = 0;
        uStack_238 = 0;
        uStack_234 = 0;
        piStack_230 = (int *)0x0;
        piStack_220 = (int *)0x0;
        fStack_214 = fStack_224;
        (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&fStack_24c);
        fStack_224 = (float)DAT_007c12cc;
        fStack_228 = (float)DAT_007c12c8;
        fStack_214 = (float)DAT_007c12d8;
        fStack_22c = DAT_007c12c4;
        uStack_218 = DAT_007c12d4;
        fStack_254 = (float)DAT_007c1230;
        uStack_21c = DAT_007c12d0;
        fStack_258 = (float)DAT_007c122c;
        fStack_244 = (float)DAT_007c123c;
        uStack_25c = DAT_007c1228;
        fStack_248 = (float)DAT_007c1238;
        fVar7 = 3.78351e-44;
        piStack_220 = (int *)0x3f800000;
        piStack_210 = (int *)0x40400000;
        piStack_250 = (int *)0x40000000;
        fStack_24c = DAT_007c1234;
        piStack_240 = (int *)0x3f000000;
        (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0x1b,&fStack_22c,2);
        (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,4,&uStack_26c,2);
        fStack_24c = fVar7;
        fStack_248 = fVar7;
        fStack_244 = fVar7;
        piStack_240 = (int *)fVar7;
        (**(code **)(*DAT_0071d174 + 0x1b4))(DAT_0071d174,0,&fStack_24c,1);
        (**(code **)(*DAT_0069e130 + 0x100))(DAT_0069e130,&uStack_23c,3);
        piVar6 = DAT_0069e130;
        (**(code **)(*DAT_0069e130 + 0x104))(DAT_0069e130,*(byte *)(DAT_0071d164 + 0x170) & 1);
        rasterizer_transparent_geometry_group_draw_vertices(param_1,(void *)0x0,(char)piVar6);
        (**(code **)(*DAT_0069e130 + 0x108))(DAT_0069e130);
      }
      uStack_14c = 3;
      piStack_150 = (int *)0x17;
      piStack_154 = DAT_0071d174;
      uStack_158 = 0x51a472;
      (**(code **)(*DAT_0071d174 + 0xe4))();
    }
    else {
      uStack_14c = 3;
      piStack_150 = (int *)0x17;
      piStack_154 = DAT_0071d174;
      uStack_158 = 0x51a49f;
      (**(code **)(*DAT_0071d174 + 0xe4))();
    }
    if (unaff_EBX < 1.0) {
      uStack_30 = *(undefined4 *)(param_1 + 0x7c);
      uStack_58 = 0;
      uStack_54 = 0;
      uStack_50 = 0;
      uStack_4c = 0;
      uStack_48 = 0;
      uStack_2c = *(undefined4 *)(param_1 + 0x80);
      uStack_28 = *(undefined4 *)(param_1 + 0x84);
      uStack_44 = 0;
      uStack_40 = 0;
      uStack_20 = *(undefined4 *)(param_1 + 0x3c);
      uStack_3c = 0;
      uStack_1c = *(undefined4 *)(param_1 + 0x40);
      uStack_38 = 0;
      uStack_34 = 0;
      iVar3 = 0x1d;
      if (*(undefined4 **)(param_1 + 0x70) == (undefined4 *)0x0) {
        puVar4 = auStack_d4;
        for (; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar4 = 0;
          puVar4 = puVar4 + 1;
        }
      }
      else {
        puVar4 = *(undefined4 **)(param_1 + 0x70);
        puVar5 = auStack_d4;
        for (; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar5 = *puVar4;
          puVar4 = puVar4 + 1;
          puVar5 = puVar5 + 1;
        }
      }
      puVar4 = *(undefined4 **)(param_1 + 0x74);
      if (puVar4 == (undefined4 *)0x0) {
        uStack_60 = 0;
        uStack_5c = 0;
      }
      else {
        uStack_60 = *puVar4;
        uStack_5c = puVar4[1];
      }
      uStack_158 = 0;
      piStack_15c = (int *)0xe;
      piStack_160 = DAT_0071d174;
      uStack_164 = 0x51a5ac;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      DAT_0071d200 = 1.0 - unaff_EBX;
      DAT_0071d1fe = 1;
      if (DAT_006893ec != '\0') {
        DAT_0069c74c = 1;
        DAT_0071d1fa = 0;
        if (DAT_007c118c < 0xffff0101) {
          uStack_158 = 1;
          piStack_15c = (int *)0x89;
          piStack_160 = DAT_0071d174;
          uStack_164 = 0x51a5fb;
          (**(code **)(*DAT_0071d174 + 0xe4))();
        }
      }
      uStack_158 = 1;
      piStack_15c = (int *)0x51a606;
      FUN_00526f50();
      piStack_15c = *(int **)(param_1 + 0x58);
      piStack_160 = *(int **)(param_1 + 0x50);
      uStack_164 = *(undefined4 *)(param_1 + 0x44);
      piStack_168 = *(int **)(param_1 + 0x48);
      uStack_16c = (uint)*(ushort *)(param_1 + 0x10);
      uStack_170 = *(undefined4 *)(param_1 + 0xc);
      piStack_174 = (int *)0x51a629;
      FUN_0052b050();
      uStack_158 = 0x51a631;
      FUN_0052b530();
      uStack_158 = 0x51a636;
      FUN_00511ef0();
      DAT_0071d1fe = 0;
    }
    if (*param_1 < '\0') {
      uStack_158 = 0;
      piStack_15c = (int *)0x0;
      piStack_160 = (int *)0x51a64e;
      chimera__rasterizer_set_frustum_z_func();
    }
  }
  return;
}
#endif
