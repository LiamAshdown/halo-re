// rasterizer_geometry_draw_fixed_function  (Ghidra: FUN_00528ae0; the earlier rewrite called it
//   rasterizer_model_bind_render_target_pair)
// address 0x528ae0, size 251 bytes
// name confidence: 0.55   rewrite confidence: 0.95
// evidence: raw disassembly. Its caller rasterizer_geometry_part_draw 0x533730 uses it instead of
//   the vertex shader path on cards below ps_1_1, loading EAX = group flags, ECX = dynamic vertex
//   slot, EDX = vertex buffer, EDI = index buffer and pushing (dynamic_index_slot,
//   primitive_count). Nothing here touches a render target. Phase 4 review fixes: the copy of the
//   vertex buffer gets its type (word +0) set to 15 and its hardware buffer (+0x10) replaced by the
//   ProcessVertices result; the earlier file wrote +8 and +4.
// What it does: groups with the fixed function fog flag (0x200) draw their vertices as they are
//   with declaration 14 and no vertex shader; the rest are skinned by vertex shader 27 into a
//   processed copy (only when there is an index buffer, else the copy has no buffer) and drawn
//   with declaration 15.
// register convention: EAX -> flags, ECX -> dynamic_vertex_slot, EDX -> vertex_buffer,
//   EDI -> index_buffer, stack -> (dynamic_index_slot, primitive_count).
// blam-cc: EAX -> flags, ECX -> dynamic_vertex_slot, EDX -> vertex_buffer, EDI -> index_buffer, stack -> (dynamic_index_slot, primitive_count)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350

// blam-cc: stack -> (index_buffer, dynamic_index_slot, vertex_buffer), EAX -> primitive_count,
//   ECX -> first_primitive, EBX -> dynamic_vertex_slot
extern void rasterizer_dynamic_geometry_draw_dispatch(rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                                      rasterizer_vertex_buffer *vertex_buffer, int32_t primitive_count,
                                                      int32_t first_primitive, int32_t dynamic_vertex_slot); // 0x51c730
// blam-cc: ESI -> vertex_buffer
extern uint32_t rasterizer_dynamic_vertex_process_and_get_handle(rasterizer_vertex_buffer *vertex_buffer); // 0x51c790

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);

void rasterizer_geometry_draw_fixed_function(uint32_t flags, int32_t dynamic_vertex_slot,
                                             rasterizer_vertex_buffer *vertex_buffer,
                                             rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                             int32_t primitive_count)
{
    if (flags & 0x200) {
        ((d3d_call1_fn)(*(void ***)rasterizer_device)[0x170 / 4])(rasterizer_device, 0);
        ((d3d_call1_fn)(*(void ***)rasterizer_device)[0x15c / 4])(rasterizer_device,
                                                                  rasterizer_vertex_declarations[14].declaration);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, 0,
                                                  dynamic_vertex_slot);
    } else {
        rasterizer_vertex_buffer processed = *vertex_buffer;

        ((d3d_call1_fn)(*(void ***)rasterizer_device)[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[27].shader);
        ((d3d_call1_fn)(*(void ***)rasterizer_device)[0x15c / 4])(rasterizer_device,
                                                                  rasterizer_vertex_declarations[4].declaration);
        processed.hardware_buffer = index_buffer != NULL ? rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer) : 0;
        processed.type = _rasterizer_vertex_type_model_processed;
        ((d3d_call1_fn)(*(void ***)rasterizer_device)[0x170 / 4])(rasterizer_device, 0);
        ((d3d_call1_fn)(*(void ***)rasterizer_device)[0x15c / 4])(rasterizer_device,
                                                                  rasterizer_vertex_declarations[15].declaration);
        rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, &processed, primitive_count, 0,
                                                  dynamic_vertex_slot);
    }
}

#if 0
Original Ghidra decompilation (0x528ae0):

void FUN_00528ae0(void)

{
  uint in_EAX;
  int in_EDX;
  undefined4 unaff_EBP;
  int unaff_EDI;
  undefined4 local_c;
  
  if ((in_EAX & 0x200) == 0) {
    (**(code **)(*DAT_0071d174 + 0x170))();
    (**(code **)(*DAT_0071d174 + 0x15c))();
    if (unaff_EDI != 0) {
      FUN_0051c790();
    }
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174);
    (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1b44);
    rasterizer_dynamic_geometry_draw_dispatch(unaff_EDI,unaff_EBP,(int)&stack0xffffffcc);
    return;
  }
  (**(code **)(*DAT_0071d174 + 0x170))();
  (**(code **)(*DAT_0071d174 + 0x15c))();
  rasterizer_dynamic_geometry_draw_dispatch(unaff_EDI,local_c,in_EDX);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
