// game_engine_handle_kill_feed_network_event  (Ghidra: FUN_004609d0; renamed per its summary)
// address 0x4609d0, size 89 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md ("Handles an incoming or locally-generated kill-feed
// network event by resolving the killer and forwarding it to the kill-feed message system");
// this batch's chimera__kill_feed (0x460a30, called here with its own 4 stack arguments plus a
// forwarded EDI recipient, matching every other caller in this batch).
// register convention: a message pointer in EAX (in_EAX). That is the ONLY argument.
//   // blam-cc: EAX -> message
// CORRECTED (phase 4 review, objdump -d -M intel --start-address=0x4609d0 --stop-address=0x460a2a):
// the first pass modelled Ghidra's uninitialized local_c / local_8 / local_4 as three extra
// forwarded parameters, and the EDI recipient as a fourth. All four are wrong:
//   - `lea ecx,[esp]` right before the message_delta_decode_compound_field call points ECX at a three-dword stack block,
//     and EAX still holds `message`, so message_delta_decode_compound_field(message, decoded) DECODES those three
//     dwords -- they are outputs, not inputs. local_c/local_8/local_4 are decoded[0..2].
//   - EDI is loaded at 0x4609f6 with `mov edi,[edx+4]` where EDX = player_globals
//     (0x0087a478), i.e. player_globals->local_players[0]. The recipient is the local player,
//     not a caller-supplied handle.
// UNSURE: message_delta_decode_compound_field / message_delta_decode_compound_field_staged identities (a network message-delta decode pair), and the
// machine_table+0x28 machine-id -> player-handle array.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "fn_game.h"

extern network_id_table *machine_table;
extern player_globals *local_player_globals; // 0x0087a478

extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590, blam-cc:
    // EAX -> event, ECX -> out_values; UNSURE identity (a network message-delta decode)
extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670, blam-cc: EAX -> event; UNSURE identity


// blam-cc: EAX -> message
// Decodes an incoming kill-feed network message and shows it to the local player, resolving the
// killer machine id in the message into a player handle through machine_table+0x28. A message
// whose first indirect dword is non-zero is handed to message_delta_decode_compound_field_staged instead (the replay path).
void game_engine_handle_kill_feed_network_event(int32_t **message)
{
    int32_t decoded[3]; // [0] killer machine id, [1] message_type, [2] subject

    if (**message == 0) {
        if (message_delta_decode_compound_field(message, decoded) != 0) {
            datum_index killer = (datum_index)0xffffffff;
            if (decoded[0] != 0) {
                killer = *(datum_index *)(*(uint8_t **)&machine_table->handles + decoded[0] * 4);
            }
            chimera__kill_feed(local_player_globals->local_players[0], (int32_t)killer,
                                (uint32_t)decoded[1], (datum_index)decoded[2], 0);
            return;
        }
    } else {
        message_delta_decode_compound_field_staged(message); // objdump 0x460a20: EAX == message
    }
}

#if 0
Original Ghidra decompilation (0x4609d0), from tools/pack.py 0x4609d0:

void FUN_004609d0(void)

{
  char cVar1;
  undefined4 *in_EAX;
  undefined4 uVar2;
  int local_c;
  undefined4 local_8;
  undefined4 local_4;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 != '\0') {
      uVar2 = 0xffffffff;
      if (local_c != 0) {
        uVar2 = *(undefined4 *)(*(int *)(PTR_DAT_00687558 + 0x28) + local_c * 4);
      }
      chimera__kill_feed(uVar2,local_8,local_4,0);
      return;
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
