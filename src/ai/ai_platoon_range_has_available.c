// ai_platoon_range_has_available  (Ghidra: ai_platoon_range_has_available; named for this rewrite)
// address 0x433180, size 125 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: expands a packed ai reference to a platoon range (ai_reference_expand_to_
// platoon_range, 0x432420, this batch; Ghidra's own decompile drops the out-parameter
// entirely, leaving the three locals uninitialized-looking, but they are exactly that
// struct's three fields by position) and scans encounter_platoon_states[encounter.
// first_platoon + i].unknown_00 (ScenarioPlatoon.flags bit 2) over the range. The loop
// structure means it returns 1 the moment it finds an entry with unknown_00 == 0 (i.e. an
// "available" platoon), not when it finds one that is set -- the phase-4 summary has this
// inverted.
// register convention: Ghidra fully resolved the parameter.
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

// TYPES-GAP: mirrors ai_reference_expand_to_platoon_range.c's local struct of the same name.
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4


extern data_array *encounter_data; // 0x008802c8

// blam-cc: EAX -> packed_reference
uint8_t ai_platoon_range_has_available(uint32_t packed_reference)
{
    ai_reference_platoon_range range;

    if (packed_reference == (uint32_t)k_datum_index_none) {
        return 0;
    }

    ai_reference_expand_to_platoon_range(packed_reference, &range);

    for (;;) {
        encounter *enc;
        encounter_platoon_state *state;

        if (range.encounter_index == -1 || range.platoon_end < range.platoon_start) {
            return 0;
        }

        enc = &((encounter *)encounter_data->data)[range.encounter_index & 0xffff];
        state = &encounter_platoon_states[(int16_t)(enc->first_platoon + (int16_t)range.platoon_start)];
        range.platoon_start = range.platoon_start + 1;
        if (state == 0) {
            return 0;
        }
        if (state->defending == 0) {
            return 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x433180):

undefined4 FUN_00433180(void)

{
  int in_EAX;
  char *pcVar1;
  uint local_c;
  int local_8;
  int local_4;

  if (in_EAX == -1) {
    return 0;
  }
  FUN_00432420();
  do {
    if ((local_c == 0xffffffff) || (local_4 < local_8)) {
      return 0;
    }
    pcVar1 = (char *)((short)(*(short *)((local_c & 0xffff) * 0x6c + 8 +
                                        *(int *)(DAT_008802c8 + 0x34)) + (short)local_8) * 0x10 +
                     DAT_008802c4);
    local_8 = local_8 + 1;
    if (pcVar1 == (char *)0x0) {
      return 0;
    }
  } while (*pcVar1 != '\0');
  return 1;
}
#endif
