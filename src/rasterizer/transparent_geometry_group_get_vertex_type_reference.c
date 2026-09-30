// transparent_geometry_group_get_vertex_type_reference  (Ghidra: FUN_00515400, unnamed; named
// from its behaviour per out/phase4/rasterizer_functions.md's summary)
// address 0x515400, size 33 bytes
// name confidence: 0.35  rewrite confidence: 0.55
// evidence: reads group->vertex_buffer (+0x58) and group->dynamic_vertex_slot (+0x54), matching
//   transparent_geometry_group exactly (types/rasterizer.h); prefers the static vertex buffer's
//   vertex_type field, falling back to rasterizer_dynamic_vertex_slots[slot].vertex_type.
// register convention: group pointer in in_EDX. // blam-cc: EDX -> group

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8

// blam-cc: EDX -> group
// Returns a packed 0xffff0000 | vertex_type reference for `group`'s vertex source (its static
// vertex_buffer when set, else its dynamic_vertex_slot's type), or 0xffffffff if neither is set.
uint32_t transparent_geometry_group_get_vertex_type_reference(transparent_geometry_group *group)
{
    rasterizer_vertex_buffer *vb = (rasterizer_vertex_buffer *)group->vertex_buffer;

    if (vb != (rasterizer_vertex_buffer *)0) {
        return 0xffff0000u | (uint16_t)vb->type;
    }
    if (group->dynamic_vertex_slot != -1) {
        return 0xffff0000u | (uint16_t)rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    return 0xffffffffu;
}

#if 0
Original Ghidra decompilation (0x515400):

undefined4 FUN_00515400(void)

{
  undefined4 uVar1;
  int in_EDX;

  uVar1 = 0xffffffff;
  if (*(undefined2 **)(in_EDX + 0x58) != (undefined2 *)0x0) {
    return CONCAT22(0xffff,**(undefined2 **)(in_EDX + 0x58));
  }
  if (*(int *)(in_EDX + 0x54) != -1) {
    uVar1 = CONCAT22(0xffff,*(undefined2 *)(&DAT_006d99d8 + *(int *)(in_EDX + 0x54) * 0x10));
  }
  return uVar1;
}
#endif
