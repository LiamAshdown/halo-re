// network_game_message_decode_dispatch  (Ghidra: network_game_message_decode_dispatch, already
// named)
// address 0x4db6b0, size 336 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md summary ("The central switch that dispatches a
// decoded incoming network-game message to the correct per-type handler based on a type byte in
// the packet").
// register convention: the decoded record header arrives in EDX (in_EDX), a length/offset value
// in EDI (unaff_EDI). // blam-cc: EDX -> record, EDI -> record_length
// UNSURE (major): every one of the 18 per-type handlers is called with literally zero visible
// arguments in Ghidra's output, yet each one's own rewrite (see their individual files, all in
// this task's batch except the last five) needed real parameters (client, buffer, capacity,
// expected_sequence) reconstructed from ITS OWN body. None of those reconstructed signatures can
// be satisfied from what this dispatcher alone has in scope (`record`, `record_length`). Rather
// than force an incorrect strongly-typed call at every case, every handler is called here through
// a raw zero-argument function-pointer cast, which matches Ghidra's own literal `CALL` with
// whatever registers are already live -- the most honest representation of what this specific
// function's machine code actually does, at the cost of not type-checking the handlers'
// individually-reconstructed parameter lists against each other.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db6b0..0x4db800, jump tables
// 0x4db84c / 0x4db800): the client arrives in EAX, the record in EDX, its length in EDI and the sender address on
// the stack. A record whose first word has low bits 0 and bits 2-3 == 3 is dispatched on its last byte; every
// handler gets (client, record, length, sender) -- the binary passes the client in EAX only to those that read it,
// and the record in EDX to the first two -- and its result is returned (1 for anything else). The previous C
// called all eighteen handlers with no arguments.

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

typedef int32_t (*network_game_message_handler_proc)(network_client_globals *client, const void *record,
    int32_t record_length, const uint32_t *sender);

extern int32_t network_game_client_decode_beacon_reply(); // 0x4db9a0, EAX client, EDX record, stack length
extern int32_t network_game_client_decode_pong_reply(); // 0x4dba20, EAX client, EDX record, stack length, sender
extern int32_t network_game_decode_settings_request(); // 0x4dbc00
extern int32_t network_game_client_decode_join_accepted(); // 0x4dbcc0
extern int32_t network_game_client_decode_connect_rejected(); // 0x4dbd40, EAX client
extern int32_t network_game_client_decode_join_complete(); // 0x4dbdc0
extern int32_t network_game_client_decode_settings_or_ack(); // 0x4dbe50
extern int32_t network_game_client_decode_player_config_value(); // 0x4dbf30
extern int32_t network_game_client_decode_and_discard_join_message(); // 0x4dbfb0
extern int32_t network_game_client_decode_and_discard_ingame_message(); // 0x4dc020
extern int32_t network_game_client_decode_join_finalize_message(); // 0x4dc090
extern int32_t network_game_client_decode_join_finalize_ack(); // 0x4dc120
extern int32_t network_game_client_decode_state_update_chunk(); // 0x4dc190, EAX client
extern int32_t network_game_client_decode_player_join_chunk(); // 0x4dc240, EAX client
extern int32_t network_game_client_decode_player_slot_chunk(); // 0x4dc2e0
extern int32_t network_game_client_decode_sync_complete(); // 0x4dc3a0
extern int32_t network_game_message_decode_replicated_command(); // 0x4dc410, EAX client
extern int32_t network_game_message_decode_ingame_notification(); // 0x4dc4b0

char network_game_message_decode_dispatch(network_client_globals *client, uint16_t *record, int32_t record_length,
    const uint32_t *sender)
{
    network_game_message_handler_proc handler;

    if ((*record & 3) != 0 || ((*record >> 2) & 3) != 3) {
        return 1;
    }
    switch (*((uint8_t *)record + record_length - 1)) {
    case 0x02: handler = (network_game_message_handler_proc)network_game_client_decode_beacon_reply; break;
    case 0x03: handler = (network_game_message_handler_proc)network_game_client_decode_pong_reply; break;
    case 0x04: handler = (network_game_message_handler_proc)network_game_decode_settings_request; break;
    case 0x05: handler = (network_game_message_handler_proc)network_game_client_decode_join_accepted; break;
    case 0x06: handler = (network_game_message_handler_proc)network_game_client_decode_connect_rejected; break;
    case 0x07: handler = (network_game_message_handler_proc)network_game_client_decode_join_complete; break;
    case 0x08: handler = (network_game_message_handler_proc)network_game_client_decode_settings_or_ack; break;
    case 0x09: handler = (network_game_message_handler_proc)network_game_client_decode_player_config_value; break;
    case 0x0a: handler = (network_game_message_handler_proc)network_game_client_decode_join_finalize_message; break;
    case 0x0b: handler = (network_game_message_handler_proc)network_game_client_decode_join_finalize_ack; break;
    case 0x0c: handler = (network_game_message_handler_proc)network_game_client_decode_and_discard_join_message; break;
    case 0x0d: handler = (network_game_message_handler_proc)network_game_client_decode_and_discard_ingame_message; break;
    case 0x16: handler = (network_game_message_handler_proc)network_game_client_decode_state_update_chunk; break;
    case 0x17: handler = (network_game_message_handler_proc)network_game_client_decode_player_join_chunk; break;
    case 0x18: handler = (network_game_message_handler_proc)network_game_client_decode_player_slot_chunk; break;
    case 0x19: handler = (network_game_message_handler_proc)network_game_client_decode_sync_complete; break;
    case 0x21: handler = (network_game_message_handler_proc)network_game_message_decode_replicated_command; break;
    case 0x22: handler = (network_game_message_handler_proc)network_game_message_decode_ingame_notification; break;
    default: return 1;
    }
    return (char)handler(client, record, record_length, sender);
}

#if 0
Original Ghidra decompilation (0x4db6b0):

undefined4 network_game_message_decode_dispatch(void)

{
  undefined4 uVar1;
  byte bVar2;
  ushort *in_EDX;
  int unaff_EDI;

  uVar1 = 1;
  if ((((*in_EDX & 3) == 0) && (bVar2 = (byte)*in_EDX >> 2 & 3, bVar2 != 1)) && (bVar2 == 3)) {
    switch(*(undefined1 *)((int)in_EDX + unaff_EDI + -1)) {
    case 2:
      uVar1 = network_game_client_decode_beacon_reply();
      return uVar1;
    case 3:
      uVar1 = network_game_client_decode_pong_reply();
      return uVar1;
    case 4:
      uVar1 = FUN_004dbc00();
      return uVar1;
    case 5:
      uVar1 = network_game_client_decode_join_accepted();
      return uVar1;
    case 6:
      uVar1 = FUN_004dbd40();
      return uVar1;
    case 7:
      uVar1 = network_game_client_decode_join_complete();
      return uVar1;
    case 8:
      uVar1 = FUN_004dbe50();
      return uVar1;
    case 9:
      uVar1 = FUN_004dbf30();
      return uVar1;
    case 10:
      uVar1 = FUN_004dc090();
      return uVar1;
    case 0xb:
      uVar1 = FUN_004dc120();
      return uVar1;
    case 0xc:
      uVar1 = FUN_004dbfb0();
      return uVar1;
    case 0xd:
      uVar1 = FUN_004dc020();
      return uVar1;
    case 0x16:
      uVar1 = FUN_004dc190();
      return uVar1;
    case 0x17:
      uVar1 = FUN_004dc240();
      return uVar1;
    case 0x18:
      uVar1 = FUN_004dc2e0();
      return uVar1;
    case 0x19:
      uVar1 = FUN_004dc3a0();
      return uVar1;
    case 0x21:
      uVar1 = FUN_004dc410();
      return uVar1;
    case 0x22:
      uVar1 = FUN_004dc4b0();
    }
  }
  return uVar1;
}
#endif
