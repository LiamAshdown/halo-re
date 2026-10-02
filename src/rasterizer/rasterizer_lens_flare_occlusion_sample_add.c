// rasterizer_lens_flare_occlusion_sample_add  (Ghidra: FUN_00536ff0)
// address 0x536ff0, size 314 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase4/rasterizer_types_notes.md ("A transparent_geometry_group constructor for
//   callback groups (shader NULL, procedure at +0x48)"); functions.md summary ("Registers one
//   occlusion-test sample point (camera-relative offset plus caller-supplied ids) for a lens
//   flare/light source to be tested this frame, up to a fixed capacity of 384"); every field
//   offset matches transparent_geometry_group exactly (checked field by field against
//   types/rasterizer.h), and transparent_geometry_group_overflow_d (0x0071d27c) is already
//   documented there as this function's own overflow flag.
// register convention: EAX -> procedure (the callback, stored as index_buffer -- NULL means "do
//   nothing"), EDX -> position, stack -> (id_1, id_2).
// blam-cc: EAX -> procedure, EDX -> position, stack -> (id_1, id_2)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c
extern int32_t transparent_geometry_group_count; // 0x0071d154
extern uint8_t transparent_geometry_group_overflow_d; // 0x0071d27c
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220

// Registers one occlusion-test sample point (camera-relative offset plus caller-supplied ids) for
// a lens flare/light source to be tested this frame, up to a fixed capacity of 384. `procedure`
// is stored as the group's callback (a NULL-shader transparent_geometry_group), so it doubles as
// the "do nothing" guard.
void rasterizer_lens_flare_occlusion_sample_add(void *procedure, const real_point3d *position, uint32_t id_1,
                                                uint32_t id_2)
{
    int32_t index;
    transparent_geometry_group *group;
    float dx, dy, dz;

    if (procedure == 0) {
        return;
    }
    if (transparent_geometry_group_count >= k_rasterizer_maximum_transparent_groups) {
        if (transparent_geometry_group_overflow_d == 0) {
            transparent_geometry_group_overflow_d = 1;
        }
        return;
    }

    index = transparent_geometry_group_count;
    transparent_geometry_groups[index].sorted_index = index;
    group = &transparent_geometry_groups[index];
    transparent_geometry_group_count++;

    dx = position->x - rasterizer_window.camera.position.x;
    dy = position->y - rasterizer_window.camera.position.y;
    dz = position->z - rasterizer_window.camera.position.z;

    group->dynamic_index_slot = -1;
    group->index_buffer = (uint32_t)procedure;
    group->first_index = (int32_t)id_1;
    group->primitive_count = (int32_t)id_2;
    group->dynamic_vertex_slot = -1;

    group->flags = 0;
    group->object_index = 0;
    group->sort_key = 0;
    group->shader = 0;
    group->shader_permutation = 0;
    group->unknown_12 = 0;
    group->vertex_buffer = 0;
    group->lightmap_bitmap = 0;

    group->depth = -(rasterizer_window.camera.forward.i * dx + rasterizer_window.camera.forward.k * dz +
                     rasterizer_window.camera.forward.j * dy);
    group->position.x = position->x;
    group->position.y = position->y;
    group->position.z = position->z;
    group->tint.alpha = 0.0f;
    group->tint.red = 0.0f;
    group->tint.green = 0.0f;
    group->tint.blue = 0.0f;
    group->previous_group_index = -1;
    group->next_group_index = -1;

    group->base_map_v_scale = 1.0f;
    group->base_map_u_scale = 1.0f;
    group->parent_sort_key = 0;
    group->first_person = 0;
    group->node_matrices = 0;
    group->node_count = 0;
    group->lighting = 0;
    group->lighting_extra = 0;
}

#if 0
Original Ghidra decompilation (0x536ff0):

void FUN_00536ff0(undefined4 param_1,undefined4 param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int in_EAX;
  undefined4 *puVar5;
  float *in_EDX;

  iVar4 = DAT_0071d14c;
  if (in_EAX != 0) {
    if (DAT_0071d154 < 0x180) {
      *(int *)(DAT_0071d154 * 0xa8 + 0x98 + DAT_0071d14c) = DAT_0071d154;
      puVar5 = (undefined4 *)(DAT_0071d154 * 0xa8 + iVar4);
      DAT_0071d154 = DAT_0071d154 + 1;
      fVar1 = *in_EDX - DAT_007c1228;
      fVar2 = in_EDX[1] - DAT_007c122c;
      fVar3 = in_EDX[2] - DAT_007c1230;
      puVar5[0x12] = in_EAX;
      puVar5[0x13] = param_1;
      puVar5[0x14] = param_2;
      puVar5[0x11] = 0xffffffff;
      puVar5[0x15] = 0xffffffff;
      *puVar5 = 0;
      puVar5[1] = 0;
      puVar5[2] = 0;
      puVar5[3] = 0;
      *(undefined2 *)(puVar5 + 4) = 0;
      *(undefined2 *)(puVar5 + 5) = 0;
      puVar5[0x16] = 0;
      puVar5[0x17] = 0;
      puVar5[0x1e] = -(DAT_007c1234 * fVar1 + DAT_007c123c * fVar3 + DAT_007c1238 * fVar2);
      puVar5[0x1f] = *in_EDX;
      puVar5[0x20] = in_EDX[1];
      puVar5[0x21] = in_EDX[2];
      puVar5[0x22] = 0;
      puVar5[0x23] = 0;
      puVar5[0x24] = 0;
      puVar5[0x25] = 0;
      *(undefined2 *)(puVar5 + 0x27) = 0xffff;
      *(undefined2 *)((int)puVar5 + 0x9e) = 0xffff;
      puVar5[0x10] = 0x3f800000;
      puVar5[0xf] = 0x3f800000;
      puVar5[0x28] = 0;
      *(undefined1 *)((int)puVar5 + 0xa5) = 0;
      puVar5[0x18] = 0;
      *(undefined2 *)(puVar5 + 0x19) = 0;
      puVar5[0x1c] = 0;
      puVar5[0x1d] = 0;
      return;
    }
    if (DAT_0071d27c == '\0') {
      DAT_0071d27c = '\x01';
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
