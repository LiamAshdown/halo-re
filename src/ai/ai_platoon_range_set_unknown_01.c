// ai_platoon_range_set_unknown_01  (Ghidra: ai_platoon_range_set_unknown_01; named for this rewrite)
// address 0x4332e0, size 106 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: same platoon-range walk as ai_platoon_range_clear_unknown_00 (0x433200, this
// batch), setting the first byte of encounter_platoon_state.unknown_01[3] (offset +1) to 1.
// register convention: matches every ai_reference_expand_to_platoon_range consumer.
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4
extern data_array *encounter_data;                        // 0x008802c8

extern void ai_reference_expand_to_platoon_range(uint32_t packed_reference, ai_reference_platoon_range *out_range); // 0x432420, this batch

// blam-cc: EAX -> packed_reference
void ai_platoon_range_set_unknown_01(uint32_t packed_reference)
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
        state->maneuvering = 1;
    }
}

#if 0
Original Ghidra decompilation (0x4332e0):

void FUN_004332e0(void)

{
  int iVar1;
  int iVar2;
  int in_EAX;
  int iVar3;
  uint local_c;
  int local_8;
  int local_4;

  if (in_EAX != -1) {
    FUN_00432420();
    iVar2 = DAT_008802c8;
    iVar1 = DAT_008802c4;
    while ((local_c != 0xffffffff && (local_8 <= local_4))) {
      iVar3 = (short)(*(short *)((local_c & 0xffff) * 0x6c + 8 + *(int *)(iVar2 + 0x34)) +
                     (short)local_8) * 0x10 + iVar1;
      local_8 = local_8 + 1;
      if (iVar3 == 0) {
        return;
      }
      *(undefined1 *)(iVar3 + 1) = 1;
    }
  }
  return;
}
#endif
