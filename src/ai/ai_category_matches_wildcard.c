// ai_category_matches_wildcard  (Ghidra: ai_category_matches_wildcard; named for this rewrite)
// address 0x433ba0, size 199 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: compares two small category/type values with a "1 means wildcard" rule (if
// either value is 1, it adopts the other's value instead of being compared literally), with
// values 2 and 5 given an extra symmetric special case, and reports the boolean result to
// team_pair_override_add (outside this rewrite's range). Matches the phase-4 summary structurally.
// register convention: Ghidra recognized param_1 as an ordinary parameter and left the
// second value in AX.
//   // blam-cc: AX -> other_category, stack -> category

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void team_pair_override_add(uint32_t matched); // 0x45be50, outside this rewrite's range, UNSURE signature

// blam-cc: AX -> other_category, stack -> category
void ai_category_matches_wildcard(int16_t category, int16_t other_category)
{
    int16_t resolved;
    uint8_t special_case_hit = 0;
    uint32_t matched;

    if (category == -1 || other_category == -1) {
        return;
    }

    resolved = other_category;
    if (category != 1) {
        resolved = -1;
        if (other_category == 1) {
            resolved = category;
        }
    }

    if (((resolved == 2 || resolved == 5) && (special_case_hit = 1, other_category == resolved)) || special_case_hit) {
        matched = (category == resolved) ? 1 : 0;
    } else {
        matched = 0;
    }

    team_pair_override_add(matched);
}

#if 0
Original Ghidra decompilation (0x433ba0):

void FUN_00433ba0(short param_1)

{
  bool bVar1;
  short in_AX;
  short sVar2;
  undefined4 uVar3;

  if ((param_1 != -1) && (in_AX != -1)) {
    bVar1 = false;
    sVar2 = in_AX;
    if ((param_1 != 1) && (sVar2 = -1, in_AX == 1)) {
      sVar2 = param_1;
    }
    if (((((sVar2 == 2) || (sVar2 == 5)) && (bVar1 = true, in_AX == sVar2)) || (bVar1)) &&
       (param_1 == sVar2)) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
    FUN_0045be50(uVar3);
  }
  return;
}
#endif
