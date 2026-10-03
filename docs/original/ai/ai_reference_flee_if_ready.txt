// ai_reference_flee_if_ready  (Ghidra: ai_reference_flee_if_ready; named for this rewrite)
// address 0x434d90, size 91 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: for every actor a packed ai reference names, switches it into mode 0xb
// (types/ai.h _actor_mode_flee) via actor_set_mode (already established) if a readiness
// predicate (actor_squad_action_status_broadcast, outside this rewrite's range) is satisfied. Mirrors
// ai_unit_flee_if_ready (0x434df0, this batch), which calls the same predicate with an
// explicit second argument this call site does not visibly pass; guessed as 0 here.
// register convention: Ghidra fully resolved neither the packed reference (assumed EAX,
// per every sibling in this cluster) nor actor_squad_action_status_broadcast's second argument.
//   // blam-cc: EAX -> packed_reference, EBX -> readiness_param
// FIXED (register inputs, objdump): EBX carries actor_squad_action_status_broadcast's second
//   argument (read at 0x434db5, `push ebx`, pushed ahead of `push edi; call 0x407140`, i.e. the
//   2nd/rightmost parameter in cdecl order); the sibling ai_unit_flee_if_ready.c names this same
//   callee argument `readiness_param`. It was guessed as a literal 0 here; now threaded through.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern int32_t actor_squad_action_status_broadcast(uint32_t actor_index, int16_t command_list_index, int16_t *record); // 0x407140, stack, ESI record
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, this module

// blam-cc: EAX -> packed_reference, EBX -> readiness_param
void ai_reference_flee_if_ready(uint32_t packed_reference, uint32_t readiness_param)
{
    ai_reference_actor_iterator iterator;
    actor *a;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        // 0x434db1..0x434dd2: the mode record is built in place (ESI) and then handed to actor_set_mode
        uint8_t mode_data[0x84];

        if (actor_squad_action_status_broadcast(iterator.actor_index, (int16_t)readiness_param,
                (int16_t *)mode_data) != 0) {
            actor_set_mode(iterator.actor_index, _actor_mode_flee, mode_data);
        }
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434d90):

void FUN_00434d90(void)

{
  char cVar1;
  int iVar2;
  undefined4 local_8c;
  undefined1 local_84 [132];

  FUN_00432650();
  iVar2 = FUN_004326d0();
  while (iVar2 != 0) {
    cVar1 = FUN_00407140(local_8c);
    if (cVar1 != '\0') {
      actor_set_mode(local_8c,0xb,local_84);
    }
    iVar2 = FUN_004326d0();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
