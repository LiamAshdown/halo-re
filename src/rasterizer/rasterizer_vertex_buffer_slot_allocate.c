// rasterizer_vertex_buffer_slot_allocate  (Ghidra: FUN_005305f0)
// address 0x5305f0, size 160 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/rasterizer_types_notes.md ("Vertex buffer slot allocator (0x007bf060
//   table)"); types/rasterizer.h's rasterizer_vertex_buffer_slot doc paragraph ("0x5305f0 writes
//   slot +0/+4/+8/+0xc from eax/ebx/edi/esi ..., returns the slot index plus one"), confirmed
//   against the raw disassembly (0x5305f0..0x53068f), which this rewrite follows for the register
//   roles Ghidra could not recover (`unaff_EBX/ESI/EDI`).
// register convention: EBX -> vertex_type, ESI -> fvf, EDI -> length.
// blam-cc: EBX -> vertex_type, ESI -> fvf, EDI -> length
// UNSURE: the free-slot search is a 4-way-unrolled scan in the original (checking slot[i-1] then
//   slot[i], slot[i+1], slot[i+2] before advancing by 4); rewritten here as a plain linear scan
//   for the first free slot, which the raw code visits in the exact same strictly-increasing
//   index order, so the result is identical.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots]; // 0x007bf060
extern int32_t rasterizer_vertex_buffer_slot_high_water; // 0x0071d258
extern int32_t rasterizer_vertex_buffer_slot_count;      // 0x0071d25c

extern void *rasterizer_dx9_create_vertex_buffer(int32_t vertex_type, uint32_t length, uint32_t fvf,
                                                  uint8_t not_dynamic); // 0x530570

// Looks up a free or matching entry in the vertex-buffer slot table, creates the buffer for it,
// and stores its parameters; returns the 1-based slot handle, or 0 on failure.
int32_t rasterizer_vertex_buffer_slot_allocate(int32_t vertex_type, uint32_t fvf, uint32_t length)
{
    void *buffer;
    int32_t index;

    if (rasterizer_vertex_buffer_slot_count == k_rasterizer_vertex_buffer_slots) {
        return 0;
    }
    buffer = rasterizer_dx9_create_vertex_buffer(vertex_type, length, fvf, 0);
    if (buffer == 0) {
        return 0;
    }

    if (rasterizer_vertex_buffer_slot_high_water == k_rasterizer_vertex_buffer_slots) {
        for (index = 0; index < k_rasterizer_vertex_buffer_slots; index++) {
            if (rasterizer_vertex_buffer_slots[index].hardware_buffer == 0) {
                break;
            }
        }
    } else {
        index = rasterizer_vertex_buffer_slot_high_water;
        rasterizer_vertex_buffer_slot_high_water++;
    }
    rasterizer_vertex_buffer_slot_count++;

    rasterizer_vertex_buffer_slots[index].hardware_buffer = (uint32_t)buffer;
    rasterizer_vertex_buffer_slots[index].vertex_type = vertex_type;
    rasterizer_vertex_buffer_slots[index].length = length;
    rasterizer_vertex_buffer_slots[index].fvf = fvf;
    return index + 1;
}

#if 0
Original Ghidra decompilation (0x5305f0):

int FUN_005305f0(void)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  undefined4 unaff_EBX;
  undefined4 unaff_ESI;
  undefined4 unaff_EDI;

  if ((DAT_0071d25c == 0x100) || (iVar1 = rasterizer_dx9_vertex_shader_create(), iVar1 == 0)) {
    return 0;
  }
  if (DAT_0071d258 == 0x100) {
    iVar3 = 0;
    piVar2 = &DAT_007bf074;
    do {
      if (piVar2[-5] == 0) break;
      if (*piVar2 == 0) {
        iVar3 = iVar3 + 1;
        break;
      }
      if (piVar2[5] == 0) {
        iVar3 = iVar3 + 2;
        break;
      }
      if (piVar2[10] == 0) {
        iVar3 = iVar3 + 3;
        break;
      }
      piVar2 = piVar2 + 0x14;
      iVar3 = iVar3 + 4;
    } while ((int)piVar2 < 0x7c0474);
  }
  else {
    iVar3 = DAT_0071d258;
    DAT_0071d258 = DAT_0071d258 + 1;
  }
  DAT_0071d25c = DAT_0071d25c + 1;
  (&DAT_007bf060)[iVar3 * 5] = iVar1;
  (&DAT_007bf064)[iVar3 * 5] = unaff_EBX;
  (&DAT_007bf068)[iVar3 * 5] = unaff_EDI;
  (&DAT_007bf06c)[iVar3 * 5] = unaff_ESI;
  return iVar3 + 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
