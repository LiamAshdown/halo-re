// ai_reference_actor_iterator_new  (Ghidra: ai_reference_actor_iterator_new; named for this rewrite)
// address 0x432650, size 121 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/ai.h already documents this exact function ("the pair at 0x432650 (new;
// ECX -> iterator, stack -> packed ai reference)"), matching the ai_reference_actor_iterator
// struct there. Decodes the packed reference the same way as its siblings in this batch
// (ai_reference_expand_to_platoon_range, ai_reference_squad_iterator_new): kind 1 is a
// platoon reference (filter stored at +0x08), kind 2 a squad reference (filter at +0x04),
// kind 0 a plain encounter (no filter, both -1). Confirmed by objdump (bin/halo.exe
// 0x432650..0x4326cf) that the cursor half (+0xc/+0x10/+0x14) is filled by a tail call into
// ai_reference_actor_iterator_init_cursor (0x4369f0, this batch) with ECX advanced by 0xc.
// register convention: confirmed by objdump and the existing header comment: ECX ->
// out_iterator, stack -> packed_reference (EDX briefly holds it at entry, `mov edx,
// [esp+0x4]`, then it is moved to EAX; there is no other stack argument once outside the
// prologue).
//   // blam-cc: ECX -> out_iterator, stack -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern Scenario *global_scenario; // 0x00746f8c
extern ai_globals *ai_globals_ptr; // 0x00880354

extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor); // 0x4369f0, this batch

// blam-cc: ECX -> out_iterator, stack -> packed_reference
// Initializes out_iterator to walk every actor named by a packed ai reference: every actor
// of one squad or platoon within the reference's encounter, or every actor of the whole
// encounter for a plain encounter reference. Sets the encounter index to none (which
// ai_reference_actor_iterator_init_cursor then reads as "walk the global unassigned-actor
// list") on any validation failure or unrecognized reference kind.
void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator)
{
    int32_t encounter_index = (int32_t)(packed_reference & 0xffff);
    int32_t *field0 = (int32_t *)(out_iterator->unknown_00 + 0x00);
    int32_t *squad_filter = (int32_t *)(out_iterator->unknown_00 + 0x04);
    int32_t *platoon_filter = (int32_t *)(out_iterator->unknown_00 + 0x08);

    *field0 = encounter_index;

    if (global_scenario == 0 || ai_globals_ptr->actors_valid == 0 ||
        encounter_index >= global_scenario->encounters.count) {
        *field0 = -1;
        return;
    }

    {
        uint32_t kind = packed_reference >> 0x1e;
        *platoon_filter = -1;
        *squad_filter = -1;

        if (kind != 0) {
            if (kind == 1) {
                *platoon_filter = (int8_t)(packed_reference >> 0x10);
            } else if (kind == 2) {
                *squad_filter = (int8_t)(packed_reference >> 0x10);
            } else {
                *field0 = -1;
                return;
            }
        }
    }

    ai_reference_actor_iterator_init_cursor(encounter_index, (datum_index *)((uint8_t *)out_iterator + 0xc));
        // the original: add ecx,0xc; jmp 0x4369f0 (the cursor is the iterator's last 12 bytes)
}

#if 0
Original Ghidra decompilation (0x432650):

void FUN_00432650(uint param_1)

{
  int iVar1;
  uint uVar2;
  uint *in_ECX;
  uint uVar3;
  bool bVar4;

  iVar1 = global_scenario;
  uVar2 = param_1 & 0xffff;
  bVar4 = global_scenario == 0;
  *in_ECX = uVar2;
  if (((bVar4) || (*(char *)(DAT_00880354 + 1) == '\0')) || (*(int *)(iVar1 + 0x42c) <= (int)uVar2))
  {
    *in_ECX = 0xffffffff;
  }
  else {
    uVar3 = param_1 >> 0x1e;
    in_ECX[2] = 0xffffffff;
    in_ECX[1] = 0xffffffff;
    if (uVar3 != 0) {
      if (uVar3 == 1) {
        in_ECX[2] = (uint)param_1._2_1_;
      }
      else {
        if (uVar3 != 2) {
          *in_ECX = 0xffffffff;
          return;
        }
        in_ECX[1] = (uint)param_1._2_1_;
      }
    }
    if (uVar2 != 0xffffffff) {
      FUN_004369f0();
      return;
    }
  }
  return;
}
#endif
