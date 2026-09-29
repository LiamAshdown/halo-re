// squad_members_request_order  (Ghidra: squad_members_request_order, already named)
// address 0x435630, size 69 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: for every actor a packed ai reference names, calls actor_process_order_request
// (already established) with a caller-supplied order code, gated to the valid order-code
// range 0..11.
// register convention: Ghidra could not resolve the order code parameter at all.
//   // blam-cc: EAX -> packed_reference, SI -> order_code

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code); // 0x409ea0, this module

// blam-cc: EAX -> packed_reference, SI -> order_code
void squad_members_request_order(uint32_t packed_reference, int16_t order_code)
{
    if (order_code >= 0 && order_code < 0xc) {
        ai_reference_actor_iterator iterator;
        actor *a;

        ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            actor_process_order_request(iterator.actor_index, (uint16_t)order_code);
            a = ai_reference_actor_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x435630):

void squad_members_request_order(void)

{
  int iVar1;
  short unaff_SI;
  undefined4 local_8;

  if ((-1 < unaff_SI) && (unaff_SI < 0xc)) {
    FUN_00432650();
    iVar1 = FUN_004326d0();
    while (iVar1 != 0) {
      actor_process_order_request(local_8);
      iVar1 = FUN_004326d0();
    }
  }
  return;
}
#endif
