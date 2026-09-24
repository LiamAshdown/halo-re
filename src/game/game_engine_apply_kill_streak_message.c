// game_engine_apply_kill_streak_message  (Ghidra: FUN_00479b40; renamed -- the network handler
// for player_notify_kill_streak_update.c's message type 0xe)
// address 0x479b40, size 82 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x479b40
//   --stop-address=0x479ba0), which is far more precise than Ghidra's own pseudocode here: the
//   decoded message is {int32 machine_id at +0x00, int16 slot at +0x08, int16 amount at +0x0a},
//   machine_id is resolved to a player handle through machine_table+0x28 (the same array
//   game_engine_handle_kill_feed_network_event.c already established, stride 4), and the
//   resolved handle is passed to player_add_kill_streak.c in EBX exactly as that function's own
//   header documents. message_delta_decode_compound_field/message_delta_decode_compound_field_staged's (event, out_values)/(event) signatures are the
//   ones established in game_engine_handle_kill_feed_network_event.c.
// register convention: a message envelope in EAX (in_EAX). That is the only argument.
//   // blam-cc: EAX -> envelope
// UNSURE: bytes 4..7 of the decoded message (between machine_id and slot) are never read here;
//   kept as unnamed padding.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint8_t *machine_table; // 0x00687558, +0x28 array, stride 4, machine id -> player handle

extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590, blam-cc: EAX -> event,
    // ECX -> out_values
extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670, blam-cc: EAX -> event
extern uint8_t player_add_kill_streak(int32_t slot, int16_t amount, uint32_t player_handle); // this batch, 0x479ba0

// blam-cc: EAX -> envelope
// Decodes an incoming kill-streak-update message and applies it via player_add_kill_streak,
// resolving the message's machine id into a player handle through machine_table+0x28 (or -1 if
// the machine id is 0). A message whose first indirect dword is non-zero is handed to
// message_delta_decode_compound_field_staged instead (the replay path). Always returns 0.
uint8_t game_engine_apply_kill_streak_message(int32_t **envelope)
{
    struct { int32_t machine_id; uint8_t unknown_04[4]; int16_t slot; int16_t amount; } decoded;

    if (**envelope != 0) {
        message_delta_decode_compound_field_staged(envelope);
        return 0;
    }
    if (message_delta_decode_compound_field(envelope, &decoded) == 0) {
        return 0;
    }

    {
        uint32_t player_handle = 0xffffffff;
        if (decoded.machine_id != 0) {
            player_handle = *(uint32_t *)(*(uint8_t **)(machine_table + 0x28) + decoded.machine_id * 4);
        }
        player_add_kill_streak(decoded.slot, decoded.amount, player_handle);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x479b40), from tools/pack.py 0x479b40:

uint FUN_00479b40(void)

{
  undefined4 *in_EAX;
  uint uVar1;
  undefined2 uStack_8;
  undefined2 local_6;
  undefined2 uStack_4;

  if (*(int *)*in_EAX == 0) {
    uVar1 = FUN_004ec590();
    if ((char)uVar1 != '\0') {
      uVar1 = FUN_00479ba0(CONCAT22(local_6,uStack_8),CONCAT22(uStack_4,local_6));
      return uVar1 & 0xffffff00;
    }
  }
  else {
    uVar1 = FUN_004ec670();
  }
  return uVar1 & 0xffffff00;
}
#endif
