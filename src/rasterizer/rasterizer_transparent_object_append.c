// rasterizer_transparent_object_append  (Ghidra: FUN_0051c830; name from
// out/phase4/rasterizer_types_notes.md, "A transparent_geometry_group constructor")
// address 0x51c830, size 361 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/rasterizer.h transparent_geometry_group (0xa8 bytes; pool at 0x0071d14c,
// count at 0x0071d154, capacity k_rasterizer_maximum_transparent_groups, overflow flag at
// 0x0071d1ce) and its field list -- every dword offset below (0x00 flags, 0x0c shader, 0x10
// shader_permutation, 0x14 parameters.mode, 0x44 dynamic_index_slot, 0x48 index_buffer, 0x4c
// first_index, 0x50 primitive_count, 0x54 dynamic_vertex_slot, 0x58 vertex_buffer, 0x5c
// lightmap_bitmap, 0x60 node_matrices, 0x64 node_count, 0x70 lighting, 0x74 lighting_extra,
// 0x78 depth, 0x7c position, 0x9c/0x9e previous/next_group_index, 0xa0 attached_sort_key, 0xa5
// first_person) matches this constructor's writes exactly. `unaff_EDI` is read at +0x24 (int16)
// and +0x28 (byte), matching Shader.shader_type and "the first flags word of the derived
// shader" per the header's own Shader note, confirming EDI is a Shader tag pointer -- the depth
// bump of 0.25 when shader_type == 1 and that flag bit 0 is set matches
// transparent_geometry_group.depth's own comment ("+0.25 for some shaders").
// register convention: lightmap_bitmap, dynamic_index_slot, dynamic_vertex_slot,
// primitive_count, flags as the five stack parameters (in that call order); EDX = camera-space
// world position (real_point3d*, live-in), EDI = shader (Shader*, live-in).
// (ColorARGB is 4 floats/16 bytes, so the four zeroed dwords at group+0x88..0x94 are exactly
// `tint` -- confirmed against the sibling constructor rasterizer_transparent_geometry_group_new,
// 0x522300, which fills the same 16 bytes from a caller-supplied ColorARGB.)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220

extern transparent_geometry_group *transparent_geometry_groups;   // 0x0071d14c, 384 entries
extern int32_t transparent_geometry_group_count;                  // 0x0071d154
extern uint8_t transparent_geometry_group_overflow_a;              // 0x0071d1ce
extern uint8_t console_debug_toggle_689400;                        // 0x00689400, UNSURE meaning; gates this whole function

// Appends a new transparent_geometry_group built from a dynamic vertex/index cache submission
// (as opposed to rasterizer_transparent_geometry_group_new's static-tag submission), computing
// its depth sort key from the camera position/forward axis and bumping it by 0.25 for shaders
// whose type is 1 with flag bit 0 set. Sets the sticky overflow flag once if the pool is full.
// blam-cc: stack params as declared; EDX = world_position, EDI = shader
void rasterizer_transparent_object_append(uint32_t lightmap_bitmap, int32_t dynamic_index_slot,
                                           int32_t dynamic_vertex_slot, int32_t primitive_count,
                                           uint32_t flags, real_point3d *world_position,
                                           Shader *shader)
{
    transparent_geometry_group *group;
    float dx, dy, dz;

    if (console_debug_toggle_689400 == 0) {
        return;
    }

    dx = world_position->x - rasterizer_window.camera.position.x;
    dy = world_position->y - rasterizer_window.camera.position.y;
    dz = world_position->z - rasterizer_window.camera.position.z;

    if (transparent_geometry_group_count < k_rasterizer_maximum_transparent_groups) {
        transparent_geometry_groups[transparent_geometry_group_count].sorted_index =
            transparent_geometry_group_count;
        group = &transparent_geometry_groups[transparent_geometry_group_count];
        transparent_geometry_group_count = transparent_geometry_group_count + 1;

        group->flags = flags;
        group->dynamic_index_slot = dynamic_index_slot;
        group->primitive_count = primitive_count;
        group->dynamic_vertex_slot = dynamic_vertex_slot;
        group->lightmap_bitmap = lightmap_bitmap;
        group->object_index = 0;
        group->sort_key = 0;
        group->shader = (uint32_t)shader;
        group->shader_permutation = 0;
        group->parameters.mode = 0;
        group->index_buffer = 0;
        group->first_index = 0;
        group->vertex_buffer = 0;
        group->depth = -(rasterizer_window.camera.forward.i * dx + rasterizer_window.camera.forward.j * dy +
                          rasterizer_window.camera.forward.k * dz);
        group->position = *world_position;
        group->tint.alpha = 0.0f;
        group->tint.red = 0.0f;
        group->tint.green = 0.0f;
        group->tint.blue = 0.0f;
        group->base_map_v_scale = 1.0f;
        group->base_map_u_scale = 1.0f;
        group->previous_group_index = -1;
        group->next_group_index = -1;
        group->attached_sort_key = 0;
        group->first_person = 0;

        if (shader->shader_type == 1 && (*((uint8_t *)shader + 0x28) & 1) != 0) {
            group->depth = group->depth + 0.25f;
        }

        group->node_matrices = 0;
        group->node_count = 0;
        group->lighting = 0;
        group->lighting_extra = 0;
    } else if (transparent_geometry_group_overflow_a == 0) {
        transparent_geometry_group_overflow_a = 1;
    }
}

#if 0
Original Ghidra decompilation (0x51c830):

void FUN_0051c830(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  undefined4 *puVar5;
  float *in_EDX;
  int unaff_EDI;

  iVar4 = DAT_0071d14c;
  if (DAT_00689400 != '\0') {
    fVar1 = *in_EDX - DAT_007c1228;
    fVar3 = in_EDX[1] - DAT_007c122c;
    fVar2 = in_EDX[2] - DAT_007c1230;
    if (DAT_0071d154 < 0x180) {
      *(int *)(DAT_0071d154 * 0xa8 + 0x98 + DAT_0071d14c) = DAT_0071d154;
      puVar5 = (undefined4 *)(DAT_0071d154 * 0xa8 + iVar4);
      DAT_0071d154 = DAT_0071d154 + 1;
      *puVar5 = param_5;
      puVar5[0x11] = param_2;
      puVar5[0x14] = param_4;
      puVar5[0x15] = param_3;
      puVar5[0x17] = param_1;
      puVar5[1] = 0;
      puVar5[2] = 0;
      puVar5[3] = unaff_EDI;
      *(undefined2 *)(puVar5 + 4) = 0;
      *(undefined2 *)(puVar5 + 5) = 0;
      puVar5[0x12] = 0;
      puVar5[0x13] = 0;
      puVar5[0x16] = 0;
      puVar5[0x1e] = -(DAT_007c1234 * fVar1 + DAT_007c1238 * fVar3 + DAT_007c123c * fVar2);
      puVar5[0x1f] = *in_EDX;
      puVar5[0x20] = in_EDX[1];
      puVar5[0x21] = in_EDX[2];
      puVar5[0x22] = 0;
      puVar5[0x23] = 0;
      puVar5[0x24] = 0;
      puVar5[0x25] = 0;
      puVar5[0x10] = 0x3f800000;
      puVar5[0xf] = 0x3f800000;
      *(undefined2 *)(puVar5 + 0x27) = 0xffff;
      *(undefined2 *)((int)puVar5 + 0x9e) = 0xffff;
      puVar5[0x28] = 0;
      *(undefined1 *)((int)puVar5 + 0xa5) = 0;
      if ((*(short *)(unaff_EDI + 0x24) == 1) && ((*(byte *)(unaff_EDI + 0x28) & 1) != 0)) {
        puVar5[0x1e] = (float)puVar5[0x1e] + 0.25;
      }
      puVar5[0x18] = 0;
      *(undefined2 *)(puVar5 + 0x19) = 0;
      puVar5[0x1c] = 0;
      puVar5[0x1d] = 0;
      return;
    }
    if (DAT_0071d1ce == '\0') {
      DAT_0071d1ce = '\x01';
    }
  }
  return;
}
#endif
