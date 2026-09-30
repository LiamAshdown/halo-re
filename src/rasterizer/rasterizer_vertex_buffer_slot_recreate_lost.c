// rasterizer_vertex_buffer_slot_recreate_lost  (Ghidra: FUN_00530690)
// address 0x530690, size 69 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/rasterizer_types_notes.md ("Recreates lost vertex buffers after a device
//   reset"); types/rasterizer.h's rasterizer_vertex_buffer_slot doc paragraph ("FUN_00530690
//   recreates every buffer whose length is set but whose +0x10 byte is clear").
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots]; // 0x007bf060
extern int32_t rasterizer_vertex_buffer_slot_high_water; // 0x0071d258


// Re-creates the Direct3D vertex buffer for any vertex-buffer-slot entry that is marked in-use
// (length nonzero) but has lost its compiled buffer handle (managed byte clear), e.g. after a
// device reset drops default-pool resources.
void rasterizer_vertex_buffer_slot_recreate_lost(void)
{
    int32_t i;

    for (i = 0; i < rasterizer_vertex_buffer_slot_high_water; i++) {
        rasterizer_vertex_buffer_slot *slot = &rasterizer_vertex_buffer_slots[i];
        if (slot->length != 0 && slot->managed == 0) {
            slot->hardware_buffer = (uint32_t)rasterizer_dx9_create_vertex_buffer(slot->vertex_type, slot->length,
                                                                                  slot->fvf, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x530690):

void FUN_00530690(void)

{
  undefined4 uVar1;
  char *pcVar2;
  uint uVar3;

  uVar3 = 0;
  if (DAT_0071d258 != 0) {
    pcVar2 = &DAT_007bf070;
    do {
      if ((*(int *)(pcVar2 + -8) != 0) && (*pcVar2 == '\0')) {
        uVar1 = rasterizer_dx9_vertex_shader_create
                          (*(int *)(pcVar2 + -8),*(undefined4 *)(pcVar2 + -4),0);
        *(undefined4 *)(pcVar2 + -0x10) = uVar1;
      }
      uVar3 = uVar3 + 1;
      pcVar2 = pcVar2 + 0x14;
    } while (uVar3 < DAT_0071d258);
  }
  return;
}
#endif
