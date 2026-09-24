// rasterizer_geometry_part_draw  (Ghidra: rasterizer_geometry_part_draw, already named)
// address 0x533730, size 281 bytes
// name confidence: 0.5   rewrite confidence: 0.95
// evidence: raw disassembly (phase 4 review). Called by rasterizer_transparent_geometry_group_draw
//   0x533850 for each group of a mode 1 model key during the z pre-pass, with the group pushed.
//   The earlier file passed a DI "flag" through to draw_vertices: the binary pushes the constant 0
//   (the push edi at 0x53373d only saves the register), and it dropped every register argument of
//   the pre ps_1_1 helper 0x528ae0.
// What it does: sets the skinning palette, node parts and lighting constants of the group unless
//   it is immediate, applies the first person z range to mode 1 first person groups, draws the
//   vertices through vertex shader 0x0069e460 (declaration 4) or, below ps_1_1, through the fixed
//   function helper, and restores the z range.
// register convention: stack -> group.
// blam-cc: stack -> group

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern d3d_caps9 rasterizer_caps;                           // 0x007c10c0
extern uint32_t rasterizer_frustum_z_values[2];             // 0x0069c664
extern real_matrix4x3 *k_render_identity_matrix_ptr;        // 0x0069673c
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern uint32_t rasterizer_depth_prepass_vertex_shader;     // 0x0069e460 rasterizer_vertex_shaders[34].shader

// blam-cc: stack -> upload, EDI -> nodes
extern void chimera__rasterizer_set_model_skinning(uint8_t upload, rasterizer_node_matrices *nodes); // 0x518b40
// blam-cc: EAX -> node_part_count, ESI -> node_part_indices
extern void chimera__rasterizer_set_up_node_parts(int32_t node_part_count, uint8_t *node_part_indices); // 0x526cf0
extern void rasterizer_prepare_lighting_constants(render_lighting *lighting); // 0x518ce0
extern void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far); // 0x518f40
// blam-cc: ECX -> group, stack -> flag
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x533660
// blam-cc: EAX -> flags, ECX -> dynamic_vertex_slot, EDX -> vertex_buffer, EDI -> index_buffer, stack -> (dynamic_index_slot, primitive_count)
extern void rasterizer_geometry_draw_fixed_function(uint32_t flags, int32_t dynamic_vertex_slot,
                                                    rasterizer_vertex_buffer *vertex_buffer,
                                                    rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                                    int32_t primitive_count); // 0x528ae0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);

void rasterizer_geometry_part_draw(transparent_geometry_group *group)
{
    if ((group->flags & 2) == 0) {
        rasterizer_node_matrices nodes;

        if (group->node_matrices != 0 && group->node_count != 0) {
            nodes.matrices = group->node_matrices;
            nodes.node_count = group->node_count;
        } else {
            nodes.matrices = (uint32_t)(uintptr_t)k_render_identity_matrix_ptr;
            nodes.node_count = 1;
        }
        chimera__rasterizer_set_model_skinning((uint8_t)(~(uint8_t)(group->flags >> 8) & 1), &nodes);
        if (group->flags & 0x100) {
            chimera__rasterizer_set_up_node_parts(group->node_part_count, (uint8_t *)(uintptr_t)group->node_part_indices);
        }
        if (group->lighting != 0) {
            rasterizer_prepare_lighting_constants((render_lighting *)(uintptr_t)group->lighting);
        }
    }
    if ((int8_t)group->flags < 0 && group->parameters.mode == 1) {
        chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
    }
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        rasterizer_geometry_draw_fixed_function(group->flags, group->dynamic_vertex_slot,
                                                (rasterizer_vertex_buffer *)(uintptr_t)group->vertex_buffer,
                                                (rasterizer_index_buffer *)(uintptr_t)group->index_buffer,
                                                group->dynamic_index_slot, group->primitive_count);
    } else {
        ((d3d_call1_fn)(*(void ***)rasterizer_device)[0x15c / 4])(rasterizer_device,
                                                                  rasterizer_vertex_declarations[4].declaration);
        ((d3d_call1_fn)(*(void ***)rasterizer_device)[0x170 / 4])(rasterizer_device, rasterizer_depth_prepass_vertex_shader);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    }
    if ((int8_t)group->flags < 0 && group->parameters.mode == 1) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
}

#if 0
Original Ghidra decompilation (0x533730):

void __cdecl rasterizer_geometry_part_draw(uint *part)

{
  uint uVar1;
  char unaff_DI;
  
  uVar1 = *part;
  if ((uVar1 & 2) == 0) {
    chimera__rasterizer_set_model_skinning
              (CONCAT21((short)(uVar1 >> 0x10),~(byte)(uVar1 >> 8)) & 0xffffff01);
    if ((*part & 0x100) != 0) {
      chimera__rasterizer_set_up_node_parts();
    }
    if (part[0x1c] != 0) {
      FUN_00518ce0(part[0x1c]);
    }
  }
  if (((char)*part < '\0') && ((short)part[5] == 1)) {
    chimera__rasterizer_set_frustum_z_func(DAT_0069c664,DAT_0069c668);
  }
  if (DAT_007c118c < 0xffff0101) {
    FUN_00528ae0(part[0x11],part[0x14]);
  }
  else {
    (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1ac0);
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e460);
    rasterizer_transparent_geometry_group_draw_vertices(part,(void *)0x0,unaff_DI);
  }
  if (((char)*part < '\0') && ((short)part[5] == 1)) {
    chimera__rasterizer_set_frustum_z_func(0,0);
  }
  return;
}
#endif
