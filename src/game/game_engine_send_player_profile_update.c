// game_engine_send_player_profile_update  (Ghidra: FUN_00467010; named per
// out/phase4/game_functions.md: "Packages a player-profile buffer as network message type 0x15
// and enqueues it for sending/delivery.")
// address 0x467010, size 175 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: same hash_table_get / message_delta_encode_message / network_session_send_to_machine
//   trio, and the same "the encode call takes the address of a stack slot that itself holds a
//   pointer" decompiler artifact, as the already-committed game_engine_notify_kill_event.c
//   (0x4608d0) and game_engine_dispatch_item_pickup_event.c (0x45f850) -- modeled the same way
//   here, as one small opaque payload rather than reproducing Ghidra's exact (and, per its own
//   `items`/`local_8` aliasing, likely lossy) pointer arithmetic. network_message_scratch
//   (0x00871de0) is the same shared scratch buffer those two functions already name.
// register convention: VERIFIED against the disassembly of this function's only caller
//   (0x466ee0, this batch): EAX carries a pointer whose truthiness alone reaches this function
//   (Ghidra never dereferences in_EAX, only tests it against 0), EDX carries a commit/broadcast
//   selector, and the caller also loads ECX with a spare pointer (&cache[slot].kills) that this
//   function's own decompiled body never reads -- kept out of the signature since it has no
//   observable effect here.
//   // blam-cc: EAX -> has_payload, EDX -> broadcast_mode, stack -> (profile_tail, target)
// UNSURE: this function's own name; the true wire layout `message_delta_encode_message` expects
//   (modeled as a raw pointer to `profile_tail`, per the sibling files' TYPES-GAP note); why
//   EAX/ECX are passed at all given how little of them the decompiled body consumes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0

extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type,
    uint32_t unknown_2, void **fields, uint32_t unknown_4, uint32_t unknown_5,
    uint8_t unknown_6); // 0x4ec940; blam-cc: EAX -> destination buffer,
    // EDX -> destination size, then the seven stack arguments. Returns the encoded bit
    // length in EAX. `fields` is a pointer TO a pointer to the field block.
extern void network_session_broadcast_to_flagged(uint32_t unknown_0, void *unknown_1, uint32_t unknown_2, uint32_t unknown_3,
    uint32_t unknown_4, uint32_t unknown_5); // 0x4e1a80, not in this batch (matches other callers)
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1,
    int32_t length, uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5,
    uint32_t unknown_6); // 0x4e1930

// blam-cc: EAX -> has_payload, EDX -> broadcast_mode, stack -> (profile_tail, target)
// Encodes `profile_tail` as network message type 0x15 and either broadcasts it (target == -1)
// or sends it to one machine, when `broadcast_mode` selects the "commit" wire shape and the
// encode produces a payload. UNSURE: see header -- `has_payload` and the alternate (non-commit)
// wire shape are reproduced only at the level Ghidra's own decompile supports.
void game_engine_send_player_profile_update(void *has_payload, void *profile_tail, int32_t target)
{
    int32_t encoded_size;
    void *payload_ptr;

    payload_ptr = (has_payload != (void *)0) ? profile_tail : (void *)0;

    encoded_size = message_delta_encode_message(1, 0x15, (uint32_t)payload_ptr, &payload_ptr,
                                                 (uint32_t)profile_tail, 1, 0);
    if (encoded_size > 0) {
        if (target == -1) {
            network_session_broadcast_to_flagged(1, network_message_scratch, 1, 0, 0, 3); // UNSURE: args 3-6 elided by Ghidra here
        } else {
            network_session_send_to_machine(1, network_message_scratch, encoded_size, 1, 0, 0, 3);
        }
    }
}

#if 0
Original Ghidra decompilation (0x467010), from tools/pack.py 0x467010:

void FUN_00467010(void *param_1,int param_2)

{
  int in_EAX;
  uint changed_offset;
  int iVar1;
  int in_EDX;
  void **items;
  undefined1 *type_offset;
  undefined1 local_c [4];
  void *local_8;
  void *local_4;

  items = &local_8;
  if (in_EDX != 0) {
    local_4 = param_1;
    changed_offset = -(uint)(in_EAX != 0) & (uint)items;
    type_offset = local_c;
    items = &local_4;
  }
  else {
    type_offset = (undefined1 *)0x0;
    changed_offset = -(uint)(in_EAX != 0) & (uint)local_c;
  }
  iVar1 = message_delta_encode_message
                    ((uint)(in_EDX != 0),0x15,changed_offset,items,(int)type_offset,1,'\0');
  if (0 < iVar1) {
    if (param_2 == -1) {
      FUN_004e1a80(1,&DAT_00871de0);
      return;
    }
    network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,3);
  }
  return;
}
#endif
