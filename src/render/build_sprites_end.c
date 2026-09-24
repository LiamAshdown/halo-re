// build_sprites_end  (Ghidra: FUN_00511620; CEA build_sprites_end(data), hint only)
// address 0x511620, size 222 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: objdump -d -M intel 0x511620..0x5116fd, re-traced in the phase-4 review.
//   - ESI = build_sprite_data. centroid (+0x14) *= 1 / sprite_count (0 when there are none),
//     then matrix4x3_transform_point 0x4cbde0 (EAX = EDX = &centroid, stack view_to_world
//     0x007c31ac of render_frustum_global) takes it back to world space.
//   - per group: the vertex range is unlocked through the dynamic vertex cache of its vertex type
//     (rasterizer_dynamic_vertex_slots[slot].vertex_type (0x006d99d8, stride 0x10) ->
//     rasterizer_dynamic_vertex_caches[type].buffer_handle (0x006d98f0 = 0x006d98e8 + 8, stride
//     0xc) -> the IDirect3DVertexBuffer9 at 0x007bf04c + handle * 0x14, i.e.
//     rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer; Unlock is vtable +0x30);
//   - then, for a non empty group of a world space build, rasterizer_transparent_object_append
//     0x51c830 (EDX = &centroid, EDI = data->shader, stack (group bitmap, -4, vertex slot,
//     quad_count * 2, ((flags & 2) << 6) | 0x20)).
//   - finally flag bit 2 is cleared.
// review fix (phase-4 gate): the first draft indexed the two rasterizer tables with the wrong
//   strides (a uint16_t array times 3 for the 12 byte cache entries, and handle * 10 for the
//   0x14 byte buffer slots) and called the draw without its EDX / EDI arguments; both fixed.
// register convention: ESI = data (build_sprite_data*).
//   // blam-cc: ESI -> data
// UNSURE: the -4 index slot (the shared quad index list, by the look of it) is passed through
//   as a literal.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include <stdint.h> // uintptr_t

extern render_frustum render_frustum_global; // 0x007c3168, this module
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots];
    // 0x006d99d8, rasterizer module
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count];
    // 0x006d98e8, rasterizer module
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots];
    // 0x007bf060, rasterizer module (handle h addresses slot h - 1)

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m);
    // 0x4cbde0, math module; blam-cc: EAX -> out, EDX -> point, stack -> m
extern void rasterizer_transparent_object_append(uint32_t lightmap_bitmap, int32_t dynamic_index_slot,
    int32_t dynamic_vertex_slot, int32_t primitive_count, uint32_t flags,
    real_point3d *world_position, Shader *shader);
    // 0x51c830, rasterizer module; blam-cc: stack params as declared; EDX = world_position,
    // EDI = shader

typedef int32_t (*d3d_unlock_fn)(void *self);

// Averages data's accumulated view-space sprite origins back into its centroid (now in world
// space), then unlocks and queues each of data's groups (screen space builds are not queued),
// finally clearing build_sprite_data_flags bit 2.
void build_sprites_end(build_sprite_data *data) // blam-cc: ESI -> data
{
    real scale;
    int16_t i;

    if (data->sprite_count == 0) {
        scale = 0.0f;
    } else {
        scale = 1.0f / (real)data->sprite_count;
    }
    data->centroid.x = scale * data->centroid.x;
    data->centroid.y = scale * data->centroid.y;
    data->centroid.z = scale * data->centroid.z;
    matrix4x3_transform_point(&data->centroid, &data->centroid, &render_frustum_global.view_to_world);

    for (i = 0; i < data->group_count; i++) {
        build_sprite_group *group = &data->groups[i];

        if (group->vertex_slot != -1) {
            int16_t vertex_type = rasterizer_dynamic_vertex_slots[group->vertex_slot].vertex_type;
            int32_t handle = rasterizer_dynamic_vertex_caches[vertex_type].buffer_handle;

            if (handle != 0) {
                void *buffer = (void *)(uintptr_t)rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer;

                ((d3d_unlock_fn)(*(void ***)buffer)[0x30 / 4])(buffer);
            }
        }

        if (group->quad_count != 0 && (data->flags & _build_sprite_data_screen_space_bit) == 0) {
            rasterizer_transparent_object_append(group->bitmap, -4, group->vertex_slot,
                                                 (int32_t)group->quad_count * 2,
                                                 ((data->flags & 0xff & 2) << 6) | 0x20,
                                                 &data->centroid,
                                                 (Shader *)(uintptr_t)data->shader);
        }
    }

    data->flags = data->flags & ~(uint32_t)_build_sprite_data_flag_2_bit;
}

#if 0
Original Ghidra decompilation (0x511620):

void FUN_00511620(void)

{
  undefined4 *puVar1;
  int iVar2;
  float fVar3;
  short sVar4;
  int unaff_ESI;

  if (*(short *)(unaff_ESI + 0xc) == 0) {
    fVar3 = 0.0;
  }
  else {
    fVar3 = 1.0 / (float)(int)*(short *)(unaff_ESI + 0xc);
  }
  *(float *)(unaff_ESI + 0x14) = fVar3 * *(float *)(unaff_ESI + 0x14);
  *(float *)(unaff_ESI + 0x18) = fVar3 * *(float *)(unaff_ESI + 0x18);
  *(float *)(unaff_ESI + 0x1c) = fVar3 * *(float *)(unaff_ESI + 0x1c);
  matrix4x3_transform_point(&DAT_007c31ac);
  sVar4 = 0;
  if (0 < *(short *)(unaff_ESI + 0x20)) {
    do {
      iVar2 = *(int *)(sVar4 * 0x10 + 0x24 + unaff_ESI);
      puVar1 = (undefined4 *)(sVar4 * 0x10 + 0x24 + unaff_ESI);
      if ((iVar2 != -1) && ((&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iVar2 * 0x10) * 3] != 0)) {
        (**(code **)(**(int **)(&DAT_007bf04c +
                               (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iVar2 * 0x10) * 3] * 10) +
                    0x30))(*(int **)(&DAT_007bf04c +
                                    (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iVar2 * 0x10) * 3] *
                                    10));
      }
      if ((*(short *)(puVar1 + 2) != 0) && ((*(uint *)(unaff_ESI + 0x10) & 1) == 0)) {
        FUN_0051c830(puVar1[3],0xfffffffc,*puVar1,(int)*(short *)(puVar1 + 2) << 1,
                     (*(uint *)(unaff_ESI + 0x10) & 2) << 6 | 0x20);
      }
      sVar4 = sVar4 + 1;
    } while (sVar4 < *(short *)(unaff_ESI + 0x20));
  }
  *(uint *)(unaff_ESI + 0x10) = *(uint *)(unaff_ESI + 0x10) & 0xfffffffb;
  return;
}
#endif
