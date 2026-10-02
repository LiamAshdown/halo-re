// rasterizer_dynamic_index_cache_reserve  (Ghidra: FUN_0051bd60, house name
// rasterizer_dynamic_vertex_cache_reserve; per out/phase4/rasterizer_types_notes.md the Ghidra
// name is swapped with FUN_0051bdd0 -- this one reserves INDICES, not vertices)
// address 0x51bd60, size 109 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: out/phase4/rasterizer_types_notes.md "Misnamed, misattributed or not-a-function"
// table; types/rasterizer.h rasterizer_dynamic_index_slot (0x006dd9e0, stride 0x0c matches the
// "iVar1 * 0xc" stride here), rasterizer_dynamic_index_count (0x006e09e4) and
// rasterizer_dynamic_index_slot_count (0x006e09e0); k_rasterizer_dynamic_index_budget (0x8000)
// and k_rasterizer_dynamic_vertex_slots (0x400, shared by both dynamic slot tables).
// register convention: count in EDX (only register argument the callee reads).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t rasterizer_dynamic_index_count;                                          // 0x006e09e4
extern int32_t rasterizer_dynamic_index_slot_count;                                     // 0x006e09e0
extern rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006dd9e0
extern uint8_t rasterizer_dynamic_index_overflow;                                       // 0x0071d1cc

// Reserves `count` indices out of the shared per frame dynamic index budget and records the
// reservation as a new entry in the dynamic index slot table. Returns the new slot's index, or
// -1 if count is not positive or the reservation would exceed the per frame index budget or the
// slot table (in which case the sticky overflow flag is set once).
// blam-cc: EDX = count
int32_t rasterizer_dynamic_index_cache_reserve(int32_t count)
{
    int32_t slot_index;

    slot_index = rasterizer_dynamic_index_slot_count;
    if (0 < count) {
        if (rasterizer_dynamic_index_count < k_rasterizer_dynamic_index_budget - count &&
            rasterizer_dynamic_index_slot_count < k_rasterizer_dynamic_vertex_slots - 1) {
            rasterizer_dynamic_index_slots[rasterizer_dynamic_index_slot_count].first_index =
                rasterizer_dynamic_index_count;
            rasterizer_dynamic_index_slots[slot_index].index_count = count;
            rasterizer_dynamic_index_count = rasterizer_dynamic_index_count + count;
            rasterizer_dynamic_index_slot_count = rasterizer_dynamic_index_slot_count + 1;
            return slot_index;
        }
        if (rasterizer_dynamic_index_overflow == 0) {
            rasterizer_dynamic_index_overflow = 1;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x51bd60):

int FUN_0051bd60(void)

{
  int iVar1;
  int in_EDX;

  iVar1 = DAT_006e09e0;
  if (0 < in_EDX) {
    if ((DAT_006e09e4 < 0x8000 - in_EDX) && (DAT_006e09e0 < 0x3ff)) {
      *(int *)(&DAT_006dd9e0 + DAT_006e09e0 * 0xc) = DAT_006e09e4;
      *(int *)(&DAT_006dd9e4 + iVar1 * 0xc) = in_EDX;
      DAT_006e09e4 = DAT_006e09e4 + in_EDX;
      DAT_006e09e0 = DAT_006e09e0 + 1;
      return iVar1;
    }
    if (DAT_0071d1cc == '\0') {
      DAT_0071d1cc = '\x01';
    }
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
