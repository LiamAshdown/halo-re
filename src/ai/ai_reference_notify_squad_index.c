// ai_reference_notify_squad_index  (Ghidra: ai_reference_notify_squad_index; named for this rewrite)
// address 0x432c20, size 82 bytes
// name confidence: 0.3   rewrite confidence: 0.5
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

extern void ai_release_actors_filtered(uint32_t squad_index_or_none); // 0x42ab00, outside this rewrite's range, UNSURE signature

// blam-cc: stack -> packed_reference
// Decodes just the squad sub-index of a packed ai reference (or none, for a platoon or
// plain encounter reference) and passes it to ai_release_actors_filtered, unconditionally either way.
void ai_reference_notify_squad_index(uint32_t packed_reference)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        uint32_t squad_index = (packed_reference >> 0x1e == 2) ? ((packed_reference >> 0x10) & 0xff)
                                                                 : (uint32_t)k_datum_index_none;
        ai_release_actors_filtered(squad_index);
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
