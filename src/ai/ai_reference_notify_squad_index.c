// ai_reference_notify_squad_index  (Ghidra: ai_reference_notify_squad_index; named for this rewrite)
// address 0x432c20, size 82 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (REWRITTEN from objdump 0x432c20..0x432c71; the ai_erase worker)
// evidence: decodes just the squad sub-index byte of a packed ai reference (kind 2) and
// forwards it to ai_release_actors_filtered unconditionally (both the kind==1 and the fallthrough path
// call it with the same value); ai_release_actors_filtered is outside this rewrite's address range.
// register convention: Ghidra fully resolved the parameter.
//   // blam-cc: stack -> packed_reference (EAX, per sibling convention; not independently
//   confirmed with objdump for this file)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_release_actors_filtered(datum_index encounter_index, int32_t platoon_index, int32_t squad_index,
    uint8_t is_dead); // 0x42ab00, EAX, EDI, stack, BL

// REWRITTEN from objdump. hs ai_erase (0x47d30f) lands here. A packed ai reference is the encounter in the low
//   word, the kind in the top two bits and the sub-index in bits 16..23: kind 1 = platoon, kind 2 = squad. The
//   binary calls ai_release_actors_filtered(EAX = ref & 0xffff, EDI = kind 1 ? sub : -1, stack = kind 2 ? sub
//   : -1, BL = 0). The draft passed only the squad value, which for a whole-encounter reference was -1 and
//   so hit the release-every-actor path: every ai_erase deleted all actors in the level.
// blam-cc: stack -> packed_reference
void ai_reference_notify_squad_index(uint32_t packed_reference)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        uint32_t kind = packed_reference >> 0x1e;
        int32_t sub_index = (int32_t)((packed_reference >> 0x10) & 0xff);
        int32_t squad_index = (kind == 2) ? sub_index : -1;
        int32_t platoon_index = (kind == 1) ? sub_index : -1;

        ai_release_actors_filtered((datum_index)(packed_reference & 0xffff), platoon_index, squad_index, 0);
    }
}

#if 0
Original Ghidra decompilation (0x432c20):

void FUN_00432c20(uint param_1)

{
  uint uVar1;

  if (param_1 != 0xffffffff) {
    if (param_1 >> 0x1e == 2) {
      uVar1 = param_1 >> 0x10 & 0xff;
    }
    else {
      uVar1 = 0xffffffff;
    }
    if (param_1 >> 0x1e == 1) {
      FUN_0042ab00(uVar1);
      return;
    }
    FUN_0042ab00(uVar1);
  }
  return;
}
#endif
