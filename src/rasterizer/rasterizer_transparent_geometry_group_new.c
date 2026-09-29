// rasterizer_transparent_geometry_group_new  (Ghidra: already named)
// address 0x522300, size 541 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase2/results/rasterizer_01.json ("Builds and submits a pooled transparent-
// geometry-group record (decal/particle style) with its blend, texture and camera-facing tint
// parameters."). Every dword offset matches transparent_geometry_group exactly (see
// rasterizer_transparent_object_append.c for the full offset table, verified here again field
// by field); this constructor additionally proves `tint` is the full 16-byte ColorARGB (it
// copies all four floats of the caller-supplied `tint` argument, or zero when NULL, into
// group+0x88..0x97) and fills `lighting` from chimera__rasterizer_memory_alloc's return value
// rather than leaving it NULL. The `mode == 2` (immediate) path additionally matches a
// ScenarioStructureBSPMaterial-style shader (shader_type == 8, a derived-shader flag bit 3 set)
// by drawing the group immediately and calling transparent_geometry_group_set_drawn_bit(1) (a
// transparent_geometry_group-bitmask setter per its own name) right after.
// register convention: shader, shader_permutation, lightmap_bitmap, dynamic_index_slot,
// first_index, primitive_count, vertex_buffer, tint and flags as the ten recognized parameters
// (lighting is the render_lighting copied into the group's scratch lighting, 0x522472); EAX = world position (real_point3d*, live-in).
// Phase 4 review: shader_is_decal takes the shader in ECX (the two sided bit of shader types 5..10)
//   and transparent_geometry_group_set_drawn_bit 0x515370 takes the group in EAX; both call
//   sites now pass them.
//   The final draw tests the local [esp+0x10] = flags & 2 saved at 0x522356.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220

extern uint8_t console_debug_toggle_6893fb; // 0x006893fb, gates this whole function
extern transparent_geometry_group transparent_geometry_group_immediate; // 0x006e0a70
extern int32_t transparent_geometry_group_count;   // 0x0071d154
extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c
extern uint8_t transparent_geometry_group_overflow_b; // 0x0071d1dc
extern uint8_t unknown_0071d276;                                    // 0x0071d276 UNSURE

extern uint8_t shader_is_decal(const Shader *shader); // 0x0053fde0
// blam-cc: EAX -> group, stack -> clear
extern void transparent_geometry_group_set_drawn_bit(transparent_geometry_group *group, uint8_t clear); // 0x515370
extern void *chimera__rasterizer_memory_alloc(const void *source, uint32_t size); // 0x00514560
extern void rasterizer_transparent_geometry_group_draw(transparent_geometry_group *group, uint8_t attached); // 0x00533850

// Allocates (or, for `flags` bit 1 already set, reuses the single static "immediate" record) a
// transparent_geometry_group, fills every field from the caller's arguments and the current
// camera position/forward axis, and either draws it right away (immediate groups, or
// ScenarioStructureBSPMaterial-style shaders with derived-shader flag bit 3 set) or leaves it
// queued for the frame's depth sort.
// blam-cc: params as declared; EAX = world_position
void rasterizer_transparent_geometry_group_new(Shader *shader, int16_t shader_permutation,
                                                 uint32_t lightmap_bitmap, uint32_t dynamic_index_slot,
                                                 uint32_t first_index, uint32_t primitive_count,
                                                 uint32_t vertex_buffer, ColorARGB *tint,
                                                 uint32_t lighting, uint32_t flags,
                                                 real_point3d *world_position)
{
    transparent_geometry_group *group;
    float dx, dy, dz;
    ColorARGB zero_tint;

    if (console_debug_toggle_6893fb == 0) {
        return;
    }

    dx = world_position->x - rasterizer_window.camera.position.x;
    dy = world_position->y - rasterizer_window.camera.position.y;
    dz = world_position->z - rasterizer_window.camera.position.z;

    if (tint != 0) {
        flags = flags | 1;
    }
    if (shader_is_decal(shader) != 0) {   // ECX = shader (0x522340)
        flags = flags | 7;
    }

    if ((flags & 2) == 0) {
        group = 0;
        if (transparent_geometry_group_count < k_rasterizer_maximum_transparent_groups) {
            group = &transparent_geometry_groups[transparent_geometry_group_count];
            group->sorted_index = transparent_geometry_group_count;
            transparent_geometry_group_count = transparent_geometry_group_count + 1;
        }
        if (group == 0) {
            if (transparent_geometry_group_overflow_b == 0) {
                transparent_geometry_group_overflow_b = 1;
            }
            return;
        }
    } else {
        group = &transparent_geometry_group_immediate;
        group->sorted_index = -1;
    }

    group->shader_permutation = shader_permutation;
    group->dynamic_index_slot = dynamic_index_slot;
    group->first_index = first_index;
    group->flags = flags;
    group->primitive_count = primitive_count;
    group->vertex_buffer = vertex_buffer;
    group->object_index = 0;
    group->sort_key = 0;
    group->shader = (uint32_t)shader;
    group->index_buffer = 0;          // +0x48 (0x5223dc)
    group->parameters.mode = 0;
    group->dynamic_vertex_slot = -1;
    group->lightmap_bitmap = lightmap_bitmap;

    group->depth = -(rasterizer_window.camera.forward.i * dx + rasterizer_window.camera.forward.j * dy +
                      rasterizer_window.camera.forward.k * dz);
    group->position = *world_position;

    if (tint == 0) {
        zero_tint.alpha = 0.0f;
        zero_tint.red = 0.0f;
        zero_tint.green = 0.0f;
        zero_tint.blue = 0.0f;
        tint = &zero_tint;
    }
    group->tint = *tint;

    group->base_map_u_scale = 1.0f;
    group->base_map_v_scale = 1.0f;
    group->previous_group_index = -1;
    group->next_group_index = -1;
    group->attached_sort_key = 0;
    group->first_person = 0;
    group->node_matrices = 0;
    group->node_count = 0;

    // 0x522472..0x5224a1: EAX = argument 8 (a render_lighting to copy, or NULL), ECX = 0x74
    group->lighting = (uint32_t)chimera__rasterizer_memory_alloc((const void *)lighting, 0x74);
    group->lighting_extra = 0;

    if (shader->shader_type == 8) {
        unknown_0071d276 = 1;
    }
    if (shader->shader_type == 8 && (*((uint8_t *)shader + 0x28) & 8) != 0) {
        group->flags = group->flags | 2;
        rasterizer_transparent_geometry_group_draw(group, 0);
        transparent_geometry_group_set_drawn_bit(group, 1); // EAX = group (0x5224ce), clears the drawn bit
        group->flags = group->flags & 0xfffffffd;
        return;
    }
    if ((flags & 2) != 0) {
        rasterizer_transparent_geometry_group_draw(group, 0);
    }
}

#if 0
Original Ghidra decompilation (0x522300):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void rasterizer_transparent_geometry_group_new
               (uint param_1,undefined2 param_2,uint param_3,uint param_4,uint param_5,uint param_6,
               uint param_7,uint *param_8,undefined4 param_9,uint param_10)

{
  char cVar1;
  float *in_EAX;
  uint uVar2;
  uint *puVar3;
  float10 fVar4;
  float10 extraout_ST0;
  float10 extraout_ST1;
  uint local_10 [4];

  if (DAT_006893fb != '\0') {
    fVar4 = (float10)*in_EAX - (float10)DAT_007c1228;
    if (param_8 != (uint *)0x0) {
      param_10 = param_10 | 1;
    }
    cVar1 = FUN_0053fde0();
    if (cVar1 != '\0') {
      param_10 = param_10 | 7;
    }
    if ((param_10 & 2) == 0) {
      puVar3 = (uint *)0x0;
      if ((int)DAT_0071d154 < 0x180) {
        puVar3 = (uint *)(DAT_0071d154 * 0xa8 + DAT_0071d14c);
        puVar3[0x26] = DAT_0071d154;
        DAT_0071d154 = DAT_0071d154 + 1;
      }
      if (puVar3 == (uint *)0x0) {
        if (DAT_0071d1dc != '\0') {
          return;
        }
        DAT_0071d1dc = 1;
        return;
      }
    }
    else {
      puVar3 = &DAT_006e0a70;
      _DAT_006e0b08 = 0xffffffff;
    }
    *(undefined2 *)(puVar3 + 4) = param_2;
    puVar3[0x11] = param_4;
    puVar3[0x13] = param_5;
    *puVar3 = param_10;
    puVar3[0x14] = param_6;
    puVar3[0x16] = param_7;
    puVar3[1] = 0;
    puVar3[2] = 0;
    puVar3[3] = param_1;
    *(undefined2 *)(puVar3 + 5) = 0;
    puVar3[0x12] = 0;
    puVar3[0x15] = 0xffffffff;
    puVar3[0x17] = param_3;
    local_10[0] = 0;
    local_10[1] = 0;
    local_10[2] = 0;
    local_10[3] = 0;
    puVar3[0x1e] = (uint)(float)-((float10)DAT_007c1234 * fVar4 +
                                 (float10)DAT_007c1238 * extraout_ST1 +
                                 (float10)DAT_007c123c * extraout_ST0);
    puVar3[0x1f] = (uint)*in_EAX;
    puVar3[0x20] = (uint)in_EAX[1];
    puVar3[0x21] = (uint)in_EAX[2];
    if (param_8 == (uint *)0x0) {
      param_8 = local_10;
    }
    puVar3[0x22] = *param_8;
    puVar3[0x23] = param_8[1];
    puVar3[0x24] = param_8[2];
    puVar3[0x25] = param_8[3];
    puVar3[0x10] = 0x3f800000;
    puVar3[0xf] = 0x3f800000;
    *(undefined2 *)(puVar3 + 0x27) = 0xffff;
    *(undefined2 *)((int)puVar3 + 0x9e) = 0xffff;
    puVar3[0x28] = 0;
    *(undefined1 *)((int)puVar3 + 0xa5) = 0;
    puVar3[0x18] = 0;
    *(undefined2 *)(puVar3 + 0x19) = 0;
    uVar2 = chimera__rasterizer_memory_alloc();
    puVar3[0x1c] = uVar2;
    puVar3[0x1d] = 0;
    if ((*(short *)(param_1 + 0x24) == 8) &&
       (DAT_0071d276 = 1, (*(byte *)(param_1 + 0x28) & 8) != 0)) {
      *puVar3 = *puVar3 | 2;
      rasterizer_transparent_geometry_group_draw(puVar3,0);
      FUN_00515370(1);
      *puVar3 = *puVar3 & 0xfffffffd;
      return;
    }
    if ((param_10 & 2) != 0) {
      rasterizer_transparent_geometry_group_draw(puVar3,0);
      return;
    }
  }
  return;
}
#endif
