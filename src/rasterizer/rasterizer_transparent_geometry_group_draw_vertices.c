// rasterizer_transparent_geometry_group_draw_vertices  (Ghidra: already named)
// address 0x533660, size 202 bytes
// name confidence: 0.75   rewrite confidence: 0.95
// evidence: raw disassembly (phase 4 review). Field offsets +0x44 dynamic_index_slot, +0x48
//   index_buffer, +0x4c first_index, +0x50 primitive_count, +0x54 dynamic_vertex_slot, +0x58
//   vertex_buffer of transparent_geometry_group. The earlier file called the static/static and
//   static index paths with the wrong arguments (the dynamic vertex slot instead of the primitive
//   count and the buffers, the first index instead of the primitive count).
// What it does: picks the draw path from which buffers the group has: static indices and
//   vertices 0x51c5f0, static indices with dynamic vertices 0x51c490, dynamic indices with static
//   vertices 0x51c1c0 (or 0x51c310 with the lightmap stream vertex_buffer + 0x14 when flag is
//   set), dynamic indices and vertices 0x51c090, and for a negative dynamic index slot the non
//   indexed draw 0x51bec0 of primitive kind -slot (3 and 4 divide the primitive count by kind - 2).
// register convention: ECX -> group, stack -> flag (tested as a byte).
// blam-cc: ECX -> group, stack -> flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX -> vertex_buffer, EDI -> index_buffer, stack -> primitive_count
extern void rasterizer_dynamic_geometry_chain_draw(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                   rasterizer_index_buffer *index_buffer); // 0x51c5f0
extern void rasterizer_dynamic_vertex_draw_indexed(rasterizer_index_buffer *index_buffer, int32_t primitive_count,
                                                   int32_t dynamic_vertex_slot); // 0x51c490
extern void rasterizer_dynamic_index_cache_draw(int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count,
                                                int32_t dynamic_vertex_slot); // 0x51c090
extern void rasterizer_dynamic_vertex_draw(int32_t first_primitive, int32_t primitive_count, int32_t dynamic_vertex_slot,
                                           int16_t primitive_kind); // 0x51bec0
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                       int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0
// blam-cc: EAX -> primitive_count, EDI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive, second_stream)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                        int32_t dynamic_index_slot, int32_t first_primitive,
                                                                        rasterizer_vertex_buffer *second_stream); // 0x51c310

void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag)
{
    rasterizer_index_buffer *index_buffer = (rasterizer_index_buffer *)(uintptr_t)group->index_buffer;
    rasterizer_vertex_buffer *vertex_buffer = (rasterizer_vertex_buffer *)(uintptr_t)group->vertex_buffer;

    if (index_buffer != NULL) {
        if (vertex_buffer != NULL) {
            rasterizer_dynamic_geometry_chain_draw(group->primitive_count, vertex_buffer, index_buffer);
        } else {
            rasterizer_dynamic_vertex_draw_indexed(index_buffer, group->primitive_count, group->dynamic_vertex_slot);
        }
        return;
    }
    if (vertex_buffer != NULL) {
        if (flag) {
            chimera__rasterizer_draw_dynamic_triangles_static_vertices2(group->primitive_count, vertex_buffer,
                                                                        group->dynamic_index_slot, group->first_index,
                                                                        vertex_buffer + 1);   // +0x14
        } else {
            chimera__rasterizer_draw_dynamic_triangles_static_vertices(group->primitive_count, vertex_buffer,
                                                                       group->dynamic_index_slot, group->first_index);
        }
        return;
    }
    if (group->dynamic_index_slot >= 0) {
        rasterizer_dynamic_index_cache_draw(group->dynamic_index_slot, group->first_index, group->primitive_count,
                                            group->dynamic_vertex_slot);
    } else {
        int16_t kind = (int16_t)-(int16_t)group->dynamic_index_slot;
        int16_t count;

        if (kind == 3 || kind == 4) {
            count = (int16_t)(group->primitive_count / (kind - 2));
        } else {
            count = 1;
        }
        rasterizer_dynamic_vertex_draw(0, count, group->dynamic_vertex_slot, kind);
    }
}

#if 0
Original Ghidra decompilation (0x533660):

void __thiscall
rasterizer_transparent_geometry_group_draw_vertices
          (void *this,void *geometry_group,char draw_dynamic)

{
  short sVar1;
  short sVar2;
  
  if (*(int *)((int)this + 0x48) != 0) {
    if (*(int *)((int)this + 0x58) != 0) {
      FUN_0051c5f0(*(undefined4 *)((int)this + 0x50));
      return;
    }
    FUN_0051c490(*(int *)((int)this + 0x48),*(undefined4 *)((int)this + 0x50),
                 *(undefined4 *)((int)this + 0x54));
    return;
  }
  if (*(int *)((int)this + 0x58) == 0) {
    if (-1 < *(int *)((int)this + 0x44)) {
      FUN_0051c090(*(int *)((int)this + 0x44),*(undefined4 *)((int)this + 0x4c),
                   *(undefined4 *)((int)this + 0x50),*(undefined4 *)((int)this + 0x54));
      return;
    }
    sVar2 = -*(short *)((int)this + 0x44);
    if ((sVar2 == 3) || (*(short *)((int)this + 0x44) == -4)) {
      sVar1 = (short)(*(int *)((int)this + 0x50) / (sVar2 + -2));
    }
    else {
      sVar1 = 1;
    }
    FUN_0051bec0(0,(int)sVar1,*(undefined4 *)((int)this + 0x54),sVar2);
    return;
  }
  if ((char)geometry_group != '\0') {
    chimera__rasterizer_draw_dynamic_triangles_static_vertices2
              (*(undefined4 *)((int)this + 0x44),*(undefined4 *)((int)this + 0x4c),
               *(int *)((int)this + 0x58) + 0x14);
    return;
  }
  chimera__rasterizer_draw_dynamic_triangles_static_vertices
            (*(undefined4 *)((int)this + 0x44),*(undefined4 *)((int)this + 0x4c));
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
