// actor_update_path_if_needed  (Ghidra: actor_update_path_if_needed, renamed)
// address 0x4017b0, size 156 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: types/ai.h actor.needs_new_path (0x4c); phase-4 summary ("issues a pathfinding
//   request ... when its 'needs new path' flag is set").
// register convention: actor index in EAX (in_EAX, the sole real parameter); the two large
//   locals are stack scratch space this function owns and hands to its callees by pointer.
//   // blam-cc: EAX -> actor_index
// UNSURE: actor_select_firing_position and actor_claim_firing_position are outside this session's address range and have not
//   been rewritten yet, so their real parameter types are not established; the prototypes
//   below only mirror what this call site passes (an actor index, a small request block, a
//   much larger scratch buffer, and an output flag). The request/scratch buffer sizes below
//   are copied from Ghidra's stack layout as-is rather than mapped onto a named struct, since
//   attributing them to path_find_context (a plausible but unconfirmed match: it is 0x1008c
//   bytes against this buffer's reported 65684) belongs to whichever future session rewrites
//   0x413e50.
//   UNSURE: the returned flag is read from actor+0x280, which this header names danger_type
//   on strong evidence from two other functions (0x41ea60/0x41ec90); its use here as a
//   "path search finished" flag does not obviously match that name, so the field may be
//   double-booked between the two roles. Preserved as the same byte either way.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern data_array *actor_data; // 0x00880360

// 0x413e50, not yet rewritten: builds a path-find request from the actor's current state and
// destination into request_block and runs the search into scratch_context, reporting
// completion through *reached_exactly.
extern int16_t actor_select_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_select_firing_position at 0x413e50
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
// 0x414060, not yet rewritten: applies the search result (formation_point is
// request_block[0]) to the actor's queued movement.
extern int16_t actor_claim_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_claim_firing_position at 0x414060
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.

// If the actor's needs_new_path flag is set, issues a fresh pathfinding request (kind/
// formation selector 6) and applies the result to the actor's queued movement. Either way,
// returns whether actor+0x280 (danger_type; see UNSURE above) reads zero.
uint8_t actor_update_path_if_needed(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (a->needs_new_path != 0) {
        uint32_t request_block[16];
        uint8_t scratch_context[65684];
        uint8_t reached_exactly;
        // The decompiled function zeroes exactly 0x199 dwords (1636 bytes) starting right
        // after request_block, i.e. the tail of this stack frame up to (and a few bytes
        // into) scratch_context; it does not zero request_block itself before overwriting
        // its first word below.
        memset((uint8_t *)request_block + sizeof(request_block), 0, 0x199 * 4);
        request_block[1] = 6; // kind/formation selector consumed by actor_select_firing_position

        actor_select_firing_position(actor_index, request_block, scratch_context, &reached_exactly);
        actor_claim_firing_position(actor_index, request_block[0], scratch_context);
    }
    return a->danger_type == 0;
}

#if 0
Original Ghidra decompilation (0x4017b0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

bool FUN_004017b0(uint param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined1 uStack_10741;
  undefined4 auStack_10740 [16];
  undefined4 uStack_10700;
  undefined2 uStack_106fc;
  undefined1 auStack_10098 [65684];

  iVar1 = *(int *)(DAT_00880360 + 0x34);
  iVar3 = (param_1 & 0xffff) * 0x724;
  if (*(char *)(iVar3 + 0x4c + iVar1) != '\0') {
    puVar4 = &uStack_10700;
    for (iVar2 = 0x199; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    uStack_106fc = 6;
    FUN_00413e50(param_1,auStack_10740,auStack_10098,&uStack_10741);
    FUN_00414060(param_1,auStack_10740[0],auStack_10098);
  }
  return *(short *)(iVar3 + iVar1 + 0x280) == 0;
}
#endif
