// rasterizer_transparent_geometry_group_build  (Ghidra: FUN_0052b180, unnamed; Ghidra also split
//   its second half off as a fake function rasterizer_model_draw_environment_shader_environment
//   at 0x52b340, which has no prologue, no caller and returns through this frame)
// address 0x52b180, size 515 + 425 + 67 bytes (0x52b180..0x52b52b)
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: rebuilt from the raw disassembly of the whole range 0x52b180..0x52b52b. Field
//   offsets on the active model draw context (0x0071d1f0) and on the group match
//   rasterizer_model_draw_context and transparent_geometry_group: the ten dword rep movsd from
//   context +0x8c into group +0x14, context +0xc4/+0xc8 into group +0x3c/+0x40, the camera depth
//   formula into group +0x78. Callers: rasterizer_shader_environment_draw_dispatch 0x52b050 (twice),
//   FUN_0046b2f0, FUN_004d72a0 and flag_render 0x4fc350, each with eight stack arguments and the
//   link block (or NULL) in EAX.
// Phase 4 review (this file replaces the earlier transcription): the link block is 0xc bytes
//   with the index word at +8, not +4; 0x53fde0 takes the shader in ECX; the decal/immediate test
//   runs whenever the active mode is not 1 or the shader is a flagged model shader (the earlier
//   file had the mode test inverted); node part list, tint and the three scratch copies are now
//   written as the binary writes them; the static immediate record gets sorted_index -1.
// register convention: EAX -> link (transparent_geometry_group_link*, may be NULL), stack ->
//   (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer,
//   dynamic_vertex_slot, position).
// blam-cc: EAX -> link, stack -> (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot, position)
// Returns the group taken from the primary pool, or NULL (also NULL for secondary and immediate
//   groups).
// reconciled: R43 rasterizer_model_draw_context unknown_84[2] -> change_colors/function_values (the render_animation pair), unknown_c0/c4/c8 -> bounding_radius/base_map_u_scale/base_map_v_scale (same offsets)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern uint8_t console_debug_toggle_6893ec;                 // 0x006893ec
extern uint8_t console_debug_toggle_6893ed;                 // 0x006893ed
extern int16_t rasterizer_active_model_mode;                // 0x0071d1f8
extern rasterizer_model_draw_context *rasterizer_active_model_context; // 0x0071d1f0
extern transparent_geometry_group transparent_geometry_group_environment_immediate; // 0x006e1828
extern uint8_t transparent_geometry_group_overflow_c;       // 0x0071d204
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern uint8_t model_render_first_person;           // 0x007c0478 set around first person draws by FUN_004d6fc0
extern int32_t transparent_geometry_group_last_drawn_key;   // 0x006e1d58
extern uint8_t rasterizer_secondary_groups_drawn;           // 0x0071d274
extern uint8_t rasterizer_render_states_dirty;              // 0x0069c74c UNSURE name: set after draws that leave device states changed
extern uint8_t *rasterizer_node_part_indices;               // 0x0071d19c
extern int32_t rasterizer_node_part_count;                  // 0x0071d1a0
extern uint8_t rasterizer_model_scratch_valid;              // 0x0071d1f4 cleared per model by 0x526f50
extern void *rasterizer_model_scratch_node_matrices;        // 0x006e18d4 copy of context->node_matrices
extern int16_t rasterizer_model_scratch_node_count;         // 0x006e19c8
extern void *rasterizer_model_scratch_lighting;             // 0x006e17e0 copy of context->lighting (0x74)
extern void *rasterizer_model_scratch_function_source;      // 0x006e18d0 copy of context->change_colors (8)

extern transparent_geometry_group *transparent_geometry_group_allocate(void);           // 0x515230
extern transparent_geometry_group *transparent_geometry_group_allocate_secondary(void); // 0x515260
// blam-cc: ECX -> group
extern int16_t transparent_geometry_group_index_from_pointer(transparent_geometry_group *group); // 0x5152d0
// blam-cc: ECX -> shader
extern uint8_t shader_is_decal(const Shader *shader); // 0x0053fde0
extern void rasterizer_transparent_geometry_group_draw(transparent_geometry_group *group, uint8_t attached); // 0x533850
// blam-cc: EAX -> source, ECX -> size
extern void *chimera__rasterizer_memory_alloc(const void *source, uint32_t size); // 0x514560

transparent_geometry_group *rasterizer_transparent_geometry_group_build(
    transparent_geometry_group_link *link, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
    int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_vertex_slot, const real_point3d *position)
{
    rasterizer_model_draw_context *context;
    transparent_geometry_group *allocated = NULL;
    transparent_geometry_group *group;
    uint8_t skip;
    uint8_t test_immediate;
    uint32_t flags;

    if (!console_debug_toggle_6893ec || !console_debug_toggle_6893ed) {
        return NULL;
    }
    skip = (shader != NULL && *(int16_t *)&((struct Shader *)shader)->shader_type == 4 && (shader[0x28] & 8) != 0);
    if (rasterizer_active_model_mode != 1) {
        test_immediate = 1;
    } else {
        test_immediate = (shader != NULL && *(int16_t *)&((struct Shader *)shader)->shader_type == 4 && *(int16_t *)(shader + 0x28) != 0);
    }
    if (skip) {
        if (link != NULL) {
            link->group_index = -1;
            link->previous_group_index = 0;
            link->next_group_index = 0;
        }
        return NULL;
    }

    context = rasterizer_active_model_context;
    flags = context->flags;
    if (test_immediate) {
        if (shader_is_decal((const Shader *)shader)) {
            flags |= 3;
        }
        if (flags & 2) {
            group = &transparent_geometry_group_environment_immediate;
            group->sorted_index = -1;
            goto fill;
        }
    }
    if (rasterizer_active_model_mode == 1 && shader != NULL && *(int16_t *)&((struct Shader *)shader)->shader_type != 4) {
        group = transparent_geometry_group_allocate_secondary();
    } else {
        group = transparent_geometry_group_allocate();
        allocated = group;
    }
    if (link != NULL) {
        link->group_index = transparent_geometry_group_index_from_pointer(group);
        link->previous_group_index = (uint32_t)(uintptr_t)&group->previous_group_index;
        link->next_group_index = (uint32_t)(uintptr_t)&group->next_group_index;
    }
    if (group == NULL) {
        transparent_geometry_group_overflow_c = 1;
        return allocated;
    }

fill:
    group->flags = flags;
    group->object_index = context->object_index;
    if (flags & 0x100) {
        group->node_part_indices = (uint32_t)(uintptr_t)rasterizer_node_part_indices;
        group->node_part_count = rasterizer_node_part_count;
    } else {
        group->node_part_indices = 0;
        group->node_part_count = 0;
    }
    if (context->group_parameters.mode == 0) {
        group->sort_key = 0;
        group->position = *position;
    } else {
        group->sort_key = context->group_parameters.sort_key;
        group->position = context->group_parameters.position;
    }
    group->shader_permutation = (uint16_t)frame;
    group->shader = (uint32_t)(uintptr_t)shader;
    group->parameters = context->group_parameters;
    group->index_buffer = (uint32_t)(uintptr_t)index_buffer;
    group->primitive_count = primitive_count;
    group->dynamic_index_slot = dynamic_index_slot;
    group->dynamic_vertex_slot = dynamic_vertex_slot;
    group->vertex_buffer = (uint32_t)(uintptr_t)vertex_buffer;
    group->first_index = 0;
    group->lightmap_bitmap = 0;
    group->tint.alpha = 0.0f;
    group->tint.red = 0.0f;
    group->tint.green = 0.0f;
    group->tint.blue = 0.0f;
    group->depth = -(rasterizer_window.camera.forward.i * (group->position.x - rasterizer_window.camera.position.x) +
                     rasterizer_window.camera.forward.j * (group->position.y - rasterizer_window.camera.position.y) +
                     rasterizer_window.camera.forward.k * (group->position.z - rasterizer_window.camera.position.z));
    group->base_map_u_scale = context->base_map_u_scale;
    group->base_map_v_scale = context->base_map_v_scale;
    group->previous_group_index = -1;
    group->next_group_index = -1;
    if (rasterizer_active_model_mode == 1 && *(int16_t *)&((struct Shader *)shader)->shader_type != 4) {
        group->attached_sort_key = context->group_parameters.sort_key;  // attached to the model's key
    } else {
        group->attached_sort_key = 0;
    }
    group->first_person = model_render_first_person;

    if (flags & 2) {
        // immediate: draw now with the live context
        group->node_matrices = context->node_matrices;
        group->node_count = context->node_count;
        group->lighting = (uint32_t)(uintptr_t)&context->lighting;
        group->lighting_extra = (uint32_t)(uintptr_t)&context->change_colors;
        transparent_geometry_group_last_drawn_key = 0;
        rasterizer_secondary_groups_drawn = 0;
        rasterizer_transparent_geometry_group_draw(group, 0);
        rasterizer_render_states_dirty = 1;
        return allocated;
    }
    // queued: the node matrices, lighting and function source must outlive the context, so the
    // first queued group of a model copies them into the rasterizer scratch pool
    if (!rasterizer_model_scratch_valid) {
        rasterizer_model_scratch_node_matrices =
            chimera__rasterizer_memory_alloc((const void *)(uintptr_t)context->node_matrices,
                                             (uint32_t)(context->node_count * 0x34));
        rasterizer_model_scratch_node_count = context->node_count;
        rasterizer_model_scratch_lighting = chimera__rasterizer_memory_alloc(&context->lighting, 0x74);
        rasterizer_model_scratch_function_source = chimera__rasterizer_memory_alloc(&context->change_colors, 8);
        rasterizer_model_scratch_valid = 1;
    }
    group->node_matrices = (uint32_t)(uintptr_t)rasterizer_model_scratch_node_matrices;
    group->lighting = (uint32_t)(uintptr_t)rasterizer_model_scratch_lighting;
    group->node_count = rasterizer_model_scratch_node_count;
    group->lighting_extra = (uint32_t)(uintptr_t)rasterizer_model_scratch_function_source;
    return allocated;
}

#if 0
Original Ghidra decompilation (0x52b180): (the second half, 0x52b340..0x52b52b, is the fake Ghidra function at 0x52b340; see python tools/pack.py 0x52b340)

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint * FUN_0052b180(uint param_1,undefined2 param_2,uint param_3,uint param_4,uint param_5,
                   uint param_6,uint param_7,uint *param_8)

{
  bool bVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint uVar5;
  uint *puVar6;
  short sVar7;
  float fVar8;
  char cVar9;
  undefined2 uVar10;
  undefined4 *in_EAX;
  uint uVar11;
  int iVar12;
  uint *puVar13;
  uint *puVar14;
  uint *puVar15;
  bool bVar16;
  uint local_18;
  uint *local_14;
  
  sVar7 = DAT_0071d1f8;
  puVar6 = DAT_0071d1f0;
  local_14 = (uint *)0x0;
  if ((DAT_006893ec == '\0') || (DAT_006893ed == '\0')) {
    return (uint *)0x0;
  }
  if (((param_1 == 0) || (*(short *)(param_1 + 0x24) != 4)) ||
     ((*(byte *)(param_1 + 0x28) & 8) == 0)) {
    bVar16 = false;
  }
  else {
    bVar16 = true;
  }
  if ((DAT_0071d1f8 == 1) &&
     (((param_1 == 0 || (*(short *)(param_1 + 0x24) != 4)) || (*(short *)(param_1 + 0x28) == 0)))) {
    bVar1 = false;
  }
  else {
    bVar1 = true;
  }
  if (bVar16) {
    if (in_EAX == (undefined4 *)0x0) {
      return (uint *)0x0;
    }
    *(undefined2 *)(in_EAX + 2) = 0xffff;
    *in_EAX = 0;
    in_EAX[1] = 0;
    return (uint *)0x0;
  }
  local_18 = *DAT_0071d1f0;
  if (bVar1) {
    cVar9 = FUN_0053fde0();
    if (cVar9 != '\0') {
      local_18 = local_18 | 3;
    }
    if ((local_18 & 2) != 0) {
      puVar13 = &DAT_006e1828;
      _DAT_006e18c0 = 0xffffffff;
      goto LAB_0052b273;
    }
  }
  if (((sVar7 == 1) && (param_1 != 0)) && (*(short *)(param_1 + 0x24) != 4)) {
    puVar13 = (uint *)transparent_geometry_group_allocate_secondary();
  }
  else {
    puVar13 = (uint *)transparent_geometry_group_allocate();
    local_14 = puVar13;
  }
  if (in_EAX != (undefined4 *)0x0) {
    uVar10 = transparent_geometry_group_index_from_pointer();
    *(undefined2 *)(in_EAX + 2) = uVar10;
    *in_EAX = puVar13 + 0x27;
    in_EAX[1] = (int)puVar13 + 0x9e;
  }
  if (puVar13 == (uint *)0x0) {
    if (DAT_0071d204 != '\0') {
      return local_14;
    }
    DAT_0071d204 = 1;
    return local_14;
  }
LAB_0052b273:
  *puVar13 = local_18;
  puVar13[1] = puVar6[1];
  uVar11 = DAT_0071d1a0;
  if ((local_18 & 0x100) == 0) {
    uVar11 = 0;
    puVar13[0x1a] = 0;
  }
  else {
    puVar13[0x1a] = DAT_0071d19c;
  }
  puVar13[0x1b] = uVar11;
  if ((short)puVar6[0x23] == 0) {
    puVar13[2] = 0;
    puVar13[0x1f] = *param_8;
    puVar13[0x20] = param_8[1];
    puVar13[0x21] = param_8[2];
  }
  else {
    puVar13[2] = puVar6[0x26];
    puVar13[0x1f] = puVar6[0x27];
    puVar13[0x20] = puVar6[0x28];
    puVar13[0x21] = puVar6[0x29];
  }
  *(undefined2 *)(puVar13 + 4) = param_2;
  puVar13[3] = param_1;
  puVar14 = puVar6 + 0x23;
  puVar15 = puVar13 + 5;
  for (iVar12 = 10; iVar12 != 0; iVar12 = iVar12 + -1) {
    *puVar15 = *puVar14;
    puVar14 = puVar14 + 1;
    puVar15 = puVar15 + 1;
  }
  puVar13[0x12] = param_3;
  puVar13[0x14] = param_5;
  puVar13[0x15] = param_7;
  puVar13[0x11] = param_4;
  puVar13[0x16] = param_6;
  puVar13[0x13] = 0;
  puVar13[0x17] = 0;
  fVar8 = DAT_007c1234;
  fVar2 = (float)puVar13[0x1f] - DAT_007c1228;
  fVar3 = DAT_007c123c * ((float)puVar13[0x21] - DAT_007c1230);
  fVar4 = DAT_007c1238 * ((float)puVar13[0x20] - DAT_007c122c);
  puVar13[0x22] = 0;
  puVar13[0x23] = 0;
  puVar13[0x24] = 0;
  puVar13[0x25] = 0;
  puVar13[0x1e] = (uint)-(fVar8 * fVar2 + fVar4 + fVar3);
  puVar13[0xf] = puVar6[0x31];
  bVar16 = DAT_0071d1f8 == 1;
  puVar13[0x10] = puVar6[0x32];
  *(undefined2 *)(puVar13 + 0x27) = 0xffff;
  *(undefined2 *)((int)puVar13 + 0x9e) = 0xffff;
  if ((bVar16) && (*(short *)(param_1 + 0x24) != 4)) {
    puVar13[0x28] = puVar6[0x26];
  }
  else {
    puVar13[0x28] = 0;
  }
  *(undefined1 *)((int)puVar13 + 0xa5) = DAT_007c0478;
  if ((local_18 & 2) != 0) {
    puVar13[0x18] = puVar6[2];
    *(short *)(puVar13 + 0x19) = (short)puVar6[3];
    puVar13[0x1c] = (uint)(puVar6 + 4);
    puVar13[0x1d] = (uint)(puVar6 + 0x21);
    DAT_006e1d58 = 0;
    DAT_0071d274 = 0;
    rasterizer_transparent_geometry_group_draw(puVar13,0);
    DAT_0069c74c = 1;
    return local_14;
  }
  if (DAT_0071d1f4 == '\0') {
    DAT_006e18d4 = chimera__rasterizer_memory_alloc();
    DAT_006e19c8 = (undefined2)puVar6[3];
    DAT_006e17e0 = chimera__rasterizer_memory_alloc();
    DAT_006e18d0 = chimera__rasterizer_memory_alloc();
    DAT_0071d1f4 = '\x01';
  }
  uVar10 = DAT_006e19c8;
  uVar11 = DAT_006e17e0;
  puVar13[0x18] = DAT_006e18d4;
  uVar5 = DAT_006e18d0;
  puVar13[0x1c] = uVar11;
  *(undefined2 *)(puVar13 + 0x19) = uVar10;
  puVar13[0x1d] = uVar5;
  return local_14;
}
#endif
