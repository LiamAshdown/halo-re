// rasterizer_dynamic_vertex_cache_reserve  (Ghidra: FUN_0051bdd0, house name
// rasterizer_dynamic_index_cache_reserve; per out/phase4/rasterizer_types_notes.md the Ghidra
// name is swapped with FUN_0051bd60 -- this one reserves VERTICES, not indices)
// address 0x51bdd0, size 106 bytes
// name confidence: 0.7   rewrite confidence: 0.75
// evidence: out/phase4/rasterizer_types_notes.md "Misnamed, misattributed or not-a-function"
// table; types/rasterizer.h rasterizer_dynamic_vertex_cache (0x006d98e8, stride 0x0c) and
// rasterizer_dynamic_vertex_slot (0x006d99d8, stride 0x10, count at 0x006dd9d8).
// register convention: vertex type in the low 16 bits of EAX, count in ESI.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8
extern int32_t rasterizer_dynamic_vertex_slot_count;                                    // 0x006dd9d8
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern uint8_t rasterizer_dynamic_vertex_overflow;                                      // 0x0071d1cd

// Reserves `count` vertices out of the per vertex type dynamic cache `vertex_type` and records the
// reservation as a new entry in the dynamic vertex slot table. Returns the new slot's index, or
// -1 if count is not positive or the reservation would exceed the cache's remaining capacity or
// the slot table (in which case the sticky overflow flag is set once).
// blam-cc: AX = vertex_type, ESI = count
int32_t rasterizer_dynamic_vertex_cache_reserve(int16_t vertex_type, int32_t count)
{
    rasterizer_dynamic_vertex_cache *cache;
    int32_t slot_index;

    if (0 < count) {
        cache = &rasterizer_dynamic_vertex_caches[vertex_type];
        if (cache->used < cache->capacity - count && rasterizer_dynamic_vertex_slot_count < k_rasterizer_dynamic_vertex_slots - 1) {
            slot_index = rasterizer_dynamic_vertex_slot_count;
            rasterizer_dynamic_vertex_slots[slot_index].vertex_type = vertex_type;
            rasterizer_dynamic_vertex_slots[slot_index].first_vertex = cache->used;
            rasterizer_dynamic_vertex_slots[slot_index].vertex_count = count;
            cache->used = cache->used + count;
            rasterizer_dynamic_vertex_slot_count = rasterizer_dynamic_vertex_slot_count + 1;
            return slot_index;
        }
        if (rasterizer_dynamic_vertex_overflow == 0) {
            rasterizer_dynamic_vertex_overflow = 1;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x51bdd0):

int FUN_0051bdd0(void)

{
  int *piVar1;
  short in_AX;
  int iVar2;
  int iVar3;
  int unaff_ESI;

  if (0 < unaff_ESI) {
    piVar1 = &DAT_006d98e8 + in_AX * 3;
    if ((*piVar1 < (&DAT_006d98ec)[in_AX * 3] - unaff_ESI) && (DAT_006dd9d8 < 0x3ff)) {
      iVar2 = DAT_006dd9d8;
      iVar3 = DAT_006dd9d8 * 0x10;
      *(short *)(&DAT_006d99d8 + iVar3) = in_AX;
      *(int *)(&DAT_006d99dc + iVar3) = *piVar1;
      *(int *)(&DAT_006d99e0 + iVar3) = unaff_ESI;
      *piVar1 = *piVar1 + unaff_ESI;
      DAT_006dd9d8 = DAT_006dd9d8 + 1;
      return iVar2;
    }
    if (DAT_0071d1cd == '\0') {
      DAT_0071d1cd = '\x01';
    }
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
