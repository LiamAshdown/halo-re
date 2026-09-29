// squad_members_assign_team_and_request_order  (Ghidra: squad_members_assign_team_and_request_order, already named)
// address 0x435590, size 147 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: the phase-2 name matches the code: it writes SI into actor.unknown_62 for every
//   actor a packed ai reference names and, for members that have no pending burst
//   (actor.alert_level == 0) and whose current mode's grade is 0, 1 or 2, re-requests the
//   default order through actor_process_order_request (0x409ea0, already rewritten).
//   The grade comes from actor_mode_definitions[actor.mode].grade, the int16 at
//   0x00655254 + mode * 0x38 + 4 that types/ai.h already documents.
// register convention: SI -> the value written (Ghidra's unaff_SI), EAX -> packed reference
//   (consumed by ai_reference_actor_iterator_new, which Ghidra shows argument-less here).
//   // blam-cc: SI -> value, EAX -> packed_reference
//
// UNSURE: actor.unknown_62 is "0x435420 sets 2" in types/ai.h and nothing else reads it in
// this module, so "team" in the phase-2 name is not supported by anything here; the value is
// simply whatever the caller left in SI, range-checked to 0..11.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data;                             // 0x00880360
extern actor_mode_definition actor_mode_definitions[16];   // 0x00655254

extern void ai_reference_actor_iterator_new(uint32_t packed_reference,
    ai_reference_actor_iterator *out_iterator);                             // 0x432650
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0
extern void actor_process_order_request(datum_index actor_index, uint32_t order); // 0x409ea0

// blam-cc: SI -> value, EAX -> packed_reference
void squad_members_assign_team_and_request_order(uint32_t packed_reference, int16_t value)
{
    ai_reference_actor_iterator iterator;
    actor *a;
    int16_t grade;

    if (value < 0 || 0xc <= value) {
        return;
    }

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        grade = actor_mode_definitions[
            ((actor *)actor_data->data)[iterator.actor_index & 0xffff].mode].combat_grade;
        a->unknown_62 = value;
        if (a->alert_level == 0 && (grade == 0 || grade == 1 || grade == 2)) {
            actor_process_order_request(iterator.actor_index, 0xffffffff);
        }
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x435590):

void squad_members_assign_team_and_request_order(void)

{
  short sVar1;
  int iVar2;
  short unaff_SI;
  uint local_8;

  if ((-1 < unaff_SI) && (unaff_SI < 0xc)) {
    FUN_00432650();
    iVar2 = FUN_004326d0();
    while (iVar2 != 0) {
      sVar1 = *(short *)(&DAT_00655258 +
                        *(short *)((local_8 & 0xffff) * 0x724 + 0x6c + *(int *)(DAT_00880360 + 0x34)
                                  ) * 0x38);
      *(short *)(iVar2 + 0x62) = unaff_SI;
      if ((*(short *)(iVar2 + 0x6e) == 0) && (((sVar1 == 0 || (sVar1 == 1)) || (sVar1 == 2)))) {
        actor_process_order_request(local_8,0xffffffff);
      }
      iVar2 = FUN_004326d0();
    }
  }
  return;
}
#endif
