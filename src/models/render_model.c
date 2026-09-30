// render_model  (Ghidra: FUN_004d6fc0, unnamed; named "render_model" to match the extern
// declarations src/render/render_sky.c and src/render/render_object_list.c already use to call
// it, with that exact parameter list and order; types/models.h now uses this name too)
// address 0x4d6fc0, size 731 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// review pass fixes (against objdump 0x4d6fc0..0x4d7297): the node_matrices + 0x28 default
//   is for bounding_center, not lighting; the change_colors / function_out_values pointers
//   are stored into context +0x84 / +0x88 (the first rewrite dropped both stores, leaving
//   those two parameters dead); model_render_first_person is also cleared on the normal
//   exit after 0x52b530, not only on the immediate and culled exits.
// evidence: out/phase4/models_types_notes.md's rasterizer_model_draw_context, model_level_of_
//   detail, model_render_flags and Globals sections describe this function field-for-field;
//   Ghidra's own decompilation is otherwise complete (only the callee calls to
//   rasterizer_model_draw_prepare_states/chimera__rasterizer_set_model_skinning drop their
//   register-passed context pointer, already documented at their own definitions).
// register convention: model tag id in EAX (in_EAX), node matrices in ECX (in_ECX, NULL
//   allowed); pixels, region permutations, change colors, function values, lighting, bounding
//   center, bounding radius, effect, object index, forced shader permutation and flags as the
//   recognized stack parameters, in that order.
//   // blam-cc: EAX -> model_tag_id, ECX -> node_matrices, stack -> the rest
// reconciled: R43 rasterizer_model_draw_context unknown_84[2] -> change_colors/function_values (the render_animation pair), unknown_c0/c4/c8 -> bounding_radius/base_map_u_scale/base_map_v_scale (same offsets)

#include "tags.h"
#include "math.h"
#include "cache.h"
#include "models.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_rasterizer.h"
#include <stdint.h>

extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario; // 0x00746f8c
extern uint8_t model_render_first_person; // 0x007c0478, this module
extern uint8_t model_render_default_region_permutations[8]; // 0x006b7f40, this module
extern render_model_effect model_render_default_effect; // 0x006b7f18, this module
extern ColorRGB model_render_default_change_colors[4]; // 0x006b7f60, this module
extern float model_render_default_function_values[4]; // 0x006b7f08, this module
extern int16_t console_model_lod_override; // 0x006893e8, -1 default, this module
extern real_matrix4x3 render_camera_world_to_view; // 0x007c3178, UNSURE name, see
                                                    // src/interface/hud_waypoint_draw.c
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664
extern uint8_t rasterizer_caps_flag_689; // 0x0069c689, types/rasterizer.h
extern uint8_t console_debug_toggle_6893f2; // 0x006893f2, UNSURE, debug toggle range (see
                                            // src/rasterizer/rasterizer_model_draw_prepare_states.c)
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220
extern rasterizer_model_draw_context *rasterizer_object_shadow_model_context; // 0x0071d260
extern uint8_t rasterizer_object_shadow_model_active; // 0x0071d265

extern void rasterizer_model_draw_prepare_states(rasterizer_model_draw_context *context, uint8_t mode); // 0x526f50


extern void model_render_parts(GBXModel *model, uint8_t *region_permutations, rasterizer_node_matrices *node_matrices,
                                model_level_of_detail lod, uint16_t forced_shader_permutation, uint32_t flags); // 0x4d72a0, this batch

// Renders one model instance: builds the world-space node matrices (or, with no node matrices
// given, k_maximum_nodes_per_model copies of the current camera view matrix), picks a level of
// detail from the caller's pixel size (or the console override), fills a
// rasterizer_model_draw_context and hands it to model_render_parts. Substitutes the module's
// default globals for any NULL of region_permutations/effect/change_colors/function_values,
// and does nothing at all if the model is both too small on screen and not in immediate mode.
// TEMPORARY (2026-09-27): first-person draw diagnostics, src/interface/debug_play_diagnostics.c
extern void debug_fp_render_model_note(uint32_t model_tag, float pixels, int32_t lod, const float *node0,
    const float *center, int32_t early_out);
extern void debug_fp_clip_note(const float *world, int32_t effect_type);
extern void debug_fp_state_arm(int32_t armed); // TEMPORARY

void render_model(TagID model_tag_id, void *node_matrices, float pixels, uint8_t *region_permutations,
                   ColorRGB *change_colors, float *function_out_values, render_lighting *lighting,
                   real_point3d *bounding_center, float bounding_radius, render_model_effect *effect,
                   datum_index object_index, uint16_t forced_shader_permutation, uint32_t flags)
{
    GBXModel *model;
    real_matrix4x3 node_matrix_array[k_maximum_nodes_per_model];
    rasterizer_model_draw_context context;
    model_level_of_detail lod;
    int16_t node;

    model = (GBXModel *)tag_instances[model_tag_id.index].data;


    if ((model->node_list_checksum == (int32_t)k_model_first_person_node_list_checksum) &&
        ((global_scenario->flags & 1) != 0)) { // Scenario.flags bit 0 (cortana_hack); this is
            // the "global_scenario +0x3e bit 0" the models.h notes leave unresolved -- +0x3e
            // is the low byte of Scenario.flags, so the tested bit is cortana_hack.
        model_render_first_person = 1;
    } else {
        model_render_first_person = 0;
    }

    if ((&model->super_high_detail_cutoff)[_model_lod_super_low] > pixels && (flags & _model_render_immediate_bit) == 0) {
        if (flags == 8) {
            debug_fp_render_model_note(*(uint32_t *)&model_tag_id, pixels, -1, (const float *)node_matrices,
                (const float *)bounding_center, 1); // TEMPORARY first-person diagnostics
        }
        model_render_first_person = 0;
        return;
    }

    if (region_permutations == 0) {
        region_permutations = model_render_default_region_permutations;
    }
    if (effect == 0) {
        effect = &model_render_default_effect;
    }
    if (change_colors == 0) {
        change_colors = model_render_default_change_colors;
    }
    if (function_out_values == 0) {
        function_out_values = model_render_default_function_values;
    }
    // review pass: the +0x28 default belongs to the bounding center (ebp+0x1c, the source of
    // the three dword copy into context +0xb4 at 0x4d7142), not to the lighting (ebp+0x18, the
    // 0x1d dword rep movsd into context +0x10, which has no default). node_matrices + 0x28 is
    // node_matrices[0].position, the root node position; it is garbage when node_matrices is
    // itself NULL, as in the original (lea eax,[edi+0x28] at 0x4d7059).
    if (bounding_center == 0) {
        bounding_center = &((real_matrix4x3 *)node_matrices)->position;
    }

    if (node_matrices == 0) {
        for (node = 0; (int32_t)node < model->nodes.count; node++) {
            node_matrix_array[node] = render_camera_world_to_view;
        }
    } else {
        for (node = 0; (int32_t)node < model->nodes.count; node++) {
            real_matrix4x3 *given = (real_matrix4x3 *)node_matrices + node;
            real_matrix4x3 *inverse_bind = (real_matrix4x3 *)((uint8_t *)model->nodes.pointer +
                                                               node * sizeof(ModelNode) + 0x68);
            matrix4x3_multiply_procedure(given, inverse_bind, &node_matrix_array[node]);
        }
    }

    lod = _model_lod_super_high;
    while (lod > _model_lod_super_low && (&model->super_high_detail_cutoff)[lod] > pixels) {
        lod = (model_level_of_detail)(lod - 1);
    }
    if (console_model_lod_override != -1) {
        if (console_model_lod_override < 0) {
            lod = _model_lod_super_low;
        } else if (console_model_lod_override < k_model_level_of_detail_count) {
            lod = (model_level_of_detail)console_model_lod_override;
        } else {
            lod = _model_lod_super_high;
        }
    }

    context.object_index = (uint32_t)object_index;
    context.lighting = *lighting;
    context.center = *bounding_center;
    context.bounding_radius = bounding_radius;
    context.group_parameters = *(rasterizer_geometry_group_parameters *)effect;
    // context +0x84 / +0x88 (0x4d716c / 0x4d717e): the change color and function value
    // pointers (rasterizer.h change_colors / function_values, R43)
    context.change_colors = (uint32_t)(uintptr_t)change_colors;
    context.function_values = (uint32_t)(uintptr_t)function_out_values;
    context.node_matrices = (uint32_t)(uintptr_t)node_matrix_array;
    context.base_map_u_scale = model->base_map_u_scale;
    context.node_count = (int16_t)model->nodes.count;
    context.base_map_v_scale = model->base_map_v_scale;

    context.flags = 0;
    if ((model->flags & 4) != 0) { // ignore_skinning
        context.flags |= 0x200;
    }
    if ((flags & _model_render_flag_1_bit) != 0) {
        context.flags |= 0x1f;
    }
    if ((flags & _model_render_outside_fog_plane_bit) != 0) {
        context.flags |= 0x40;
    }
    if ((flags & _model_render_frustum_z_bit) != 0) {
        context.flags |= 0x80;
    }
    if ((model->flags & 2) != 0) { // parts_have_local_nodes
        context.flags |= 0x100;
    }

    if ((flags & _model_render_immediate_bit) == 0) {
        rasterizer_model_draw_prepare_states(&context, 0);
    } else if (rasterizer_window.type == 1 && rasterizer_caps_flag_689 == 0 && console_debug_toggle_6893f2 != 0) {
        chimera__rasterizer_set_model_skinning((uint8_t)(~(context.flags >> 8) & 1),
                                                (rasterizer_node_matrices *)&context.node_matrices);
        rasterizer_object_shadow_model_context = &context;
        rasterizer_object_shadow_model_active = 1;
    }

    if (flags == 8) {
        debug_fp_render_model_note(*(uint32_t *)&model_tag_id, pixels, lod, (const float *)node_matrices,
            (const float *)bounding_center, 0); // TEMPORARY first-person diagnostics
        debug_fp_clip_note(node_matrices ? (const float *)node_matrices + 10 : 0, effect->type);
    }
    if (flags == 8) debug_fp_state_arm(1); // TEMPORARY first-person diagnostics
    model_render_parts(model, region_permutations, (rasterizer_node_matrices *)&context.node_matrices,
                        lod, forced_shader_permutation, flags);
    debug_fp_state_arm(0); // TEMPORARY

    if ((flags & _model_render_immediate_bit) != 0) {
        rasterizer_object_shadow_model_context = 0;
        model_render_first_person = 0;
        return;
    }
    rasterizer_model_draw_restore_states();
    model_render_first_person = 0; // 0x4d728f, the shared exit of the non immediate path
}

#if 0
Original Ghidra decompilation (0x4d6fc0):

void FUN_004d6fc0(float param_1,undefined *param_2,undefined *param_3,undefined *param_4,
                 undefined4 *param_5,undefined4 *param_6,undefined4 param_7,undefined4 *param_8,
                 undefined4 param_9,undefined4 param_10,uint param_11)

{
  uint *puVar1;
  short sVar2;
  uint in_EAX;
  int in_ECX;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 local_de0 [832];
  uint local_e0;
  undefined4 local_dc;
  undefined4 *local_d8;
  undefined2 local_d4;
  undefined4 local_d0 [29];
  undefined *local_5c;
  undefined *local_58;
  undefined4 local_54 [10];
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  uint local_1c;
  uint local_18;
  int local_c;

  puVar1 = *(uint **)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((puVar1[1] != 0x769c097) || (DAT_007c0478 = 1, (*(byte *)(global_scenario + 0x3e) & 1) == 0))
  {
    DAT_007c0478 = 0;
  }
  if (((float)puVar1[2] <= param_1) || ((param_11 & 2) != 0)) {
    if (param_2 == (undefined *)0x0) {
      param_2 = &DAT_006b7f40;
    }
    if (param_8 == (undefined4 *)0x0) {
      param_8 = &DAT_006b7f18;
    }
    if (param_3 == (undefined *)0x0) {
      param_3 = &DAT_006b7f60;
    }
    if (param_4 == (undefined *)0x0) {
      param_4 = &DAT_006b7f08;
    }
    if (param_6 == (undefined4 *)0x0) {
      param_6 = (undefined4 *)(in_ECX + 0x28);
    }
    if (in_ECX == 0) {
      sVar2 = 0;
      if (0 < (int)puVar1[0x2e]) {
        iVar3 = 0;
        do {
          sVar2 = sVar2 + 1;
          iVar5 = iVar3 * 0xd;
          iVar3 = (int)sVar2;
          puVar6 = &DAT_007c3178;
          puVar7 = local_de0 + iVar5;
          for (iVar4 = 0xd; iVar4 != 0; iVar4 = iVar4 + -1) {
            *puVar7 = *puVar6;
            puVar6 = puVar6 + 1;
            puVar7 = puVar7 + 1;
          }
        } while (iVar3 < (int)puVar1[0x2e]);
      }
    }
    else {
      sVar2 = 0;
      if (0 < (int)puVar1[0x2e]) {
        iVar3 = 0;
        do {
          (*(code *)PTR_matrix4x3_multiply_00696664)
                    (iVar3 * 0x34 + in_ECX,iVar3 * 0x9c + 0x68 + puVar1[0x2f],
                     local_de0 + iVar3 * 0xd);
          sVar2 = sVar2 + 1;
          iVar3 = (int)sVar2;
        } while (iVar3 < (int)puVar1[0x2e]);
      }
    }
    local_c = 4;
    do {
      if ((float)puVar1[(short)local_c + 2] <= param_1) break;
      local_c = local_c + -1;
    } while (0 < (short)local_c);
    sVar2 = (short)DAT_006893e8;
    if (sVar2 != -1) {
      if (sVar2 < 0) {
        local_c = 0;
      }
      else {
        local_c = 4;
        if (sVar2 < 5) {
          local_c = DAT_006893e8;
        }
      }
    }
    iVar3 = local_c;
    local_dc = param_9;
    puVar6 = local_d0;
    for (iVar5 = 0x1d; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar6 = *param_5;
      param_5 = param_5 + 1;
      puVar6 = puVar6 + 1;
    }
    local_2c = *param_6;
    local_28 = param_6[1];
    local_24 = param_6[2];
    local_20 = param_7;
    puVar6 = local_54;
    for (iVar5 = 10; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar6 = *param_8;
      param_8 = param_8 + 1;
      puVar6 = puVar6 + 1;
    }
    local_5c = param_3;
    local_d8 = local_de0;
    local_1c = puVar1[0xc];
    local_58 = param_4;
    local_d4 = (undefined2)puVar1[0x2e];
    local_18 = puVar1[0xd];
    local_e0 = 0;
    if ((*puVar1 & 4) != 0) {
      local_e0 = 0x200;
    }
    if ((param_11 & 1) != 0) {
      local_e0 = local_e0 | 0x1f;
    }
    if ((param_11 & 4) != 0) {
      local_e0 = local_e0 | 0x40;
    }
    if ((param_11 & 8) != 0) {
      local_e0 = local_e0 | 0x80;
    }
    if ((*puVar1 & 2) != 0) {
      local_e0 = local_e0 | 0x100;
    }
    if ((param_11 & 2) == 0) {
      FUN_00526f50(0);
      iVar3 = local_c;
    }
    else if ((((short)DAT_007c1220 == 1) && (DAT_0069c689 == '\0')) && (DAT_006893f2 != '\0')) {
      chimera__rasterizer_set_model_skinning(~(byte)(local_e0 >> 8) & 1);
      DAT_0071d260 = &local_e0;
      DAT_0071d265 = 1;
      iVar3 = local_c;
    }
    FUN_004d72a0(puVar1,param_2,&local_d8,iVar3,param_10,param_11);
    if ((param_11 & 2) != 0) {
      DAT_0071d260 = (uint *)0x0;
      DAT_007c0478 = 0;
      return;
    }
    FUN_0052b530();
  }
  DAT_007c0478 = 0;
  return;
}
#endif
