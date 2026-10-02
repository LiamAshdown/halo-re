// ai_platoon_range_clear_unknown_00  (Ghidra: ai_platoon_range_clear_unknown_00; named for this rewrite)
// address 0x433200, size 105 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: same platoon-range expansion as ai_platoon_range_has_available (0x433180, this
// batch); clears encounter_platoon_state.unknown_00 across the range.
// register convention: matches every ai_reference_expand_to_platoon_range consumer.
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4
extern data_array *encounter_data;                        // 0x008802c8

extern void ai_reference_expand_to_platoon_range(uint32_t packed_reference, ai_reference_platoon_range *out_range); // 0x432420, this batch

// blam-cc: EAX -> packed_reference
void ai_platoon_range_clear_unknown_00(uint32_t packed_reference)
{
    ai_reference_platoon_range range;

    if (packed_reference == (uint32_t)k_datum_index_none) {
        return;
    }

    ai_reference_expand_to_platoon_range(packed_reference, &range);

    while (range.encounter_index != -1 && range.platoon_start <= range.platoon_end) {
        encounter *enc = &((encounter *)encounter_data->data)[range.encounter_index & 0xffff];
        encounter_platoon_state *state =
            &encounter_platoon_states[(int16_t)(enc->first_platoon + (int16_t)range.platoon_start)];
        range.platoon_start = range.platoon_start + 1;
        if (state == 0) {
            return;
        }
        state->defending = 0;
    }
}

#if 0
Original Ghidra decompilation (0x433200):

void FUN_00433200(void)

{
  int iVar1;
  int iVar2;
  int in_EAX;
  undefined1 *puVar3;
  uint local_c;
  int local_8;
  int local_4;

  if (in_EAX != -1) {
    FUN_00432420();
    iVar2 = DAT_008802c8;
    iVar1 = DAT_008802c4;
    while ((local_c != 0xffffffff && (local_8 <= local_4))) {
      puVar3 = (undefined1 *)
               ((short)(*(short *)((local_c & 0xffff) * 0x6c + 8 + *(int *)(iVar2 + 0x34)) +
                       (short)local_8) * 0x10 + iVar1);
      local_8 = local_8 + 1;
      if (puVar3 == (undefined1 *)0x0) {
        return;
      }
      *puVar3 = 0;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
