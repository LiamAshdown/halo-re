// network_game_message_decode_dispatch  (Ghidra: network_game_message_decode_dispatch, already
// named)
// address 0x4db6b0, size 336 bytes
// name confidence: 0.5   rewrite confidence: 0.3
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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern char network_game_client_decode_beacon_reply(network_client_globals *client, const uint8_t *buffer); // 0x4db9a0
extern char network_game_client_decode_pong_reply(network_client_globals *client, const uint8_t *buffer);   // 0x4dba20
extern int32_t network_game_decode_settings_request(network_client_globals *client, const uint8_t *buffer,
    void *capacity, const int32_t *expected_sequence); // 0x4dbc00
extern int32_t network_game_client_decode_join_accepted(network_client_globals *client, const uint8_t *buffer,
    void *capacity, const int32_t **expected_sequence); // 0x4dbcc0
extern int32_t network_game_client_decode_connect_rejected(network_client_globals *client, const uint8_t *buffer,
    void *capacity, const uint32_t **expected_sequence); // 0x4dbd40
extern int32_t network_game_client_decode_join_complete(network_client_globals *client, const uint8_t *buffer,
    void *capacity, const uint32_t **expected_sequence); // 0x4dbdc0
extern int32_t network_game_client_decode_settings_or_ack(network_client_globals *client, const uint8_t *buffer,
    void *capacity, const int32_t *expected_sequence); // 0x4dbe50
extern int32_t network_game_client_decode_player_config_value(network_client_globals *client,
    const uint8_t *buffer, void *capacity, int32_t *expected_sequence); // 0x4dbf30
extern int32_t network_game_client_decode_and_discard_join_message(network_client_globals *client,
    const uint8_t *buffer, void *capacity, int32_t *expected_sequence); // 0x4dbfb0
extern int32_t network_game_client_decode_and_discard_ingame_message(network_client_globals *client,
    const uint8_t *buffer, void *capacity, int32_t *expected_sequence); // 0x4dc020
extern int32_t network_game_client_decode_join_finalize_message(uint16_t *client, const uint8_t *buffer,
    void *capacity, int32_t *expected_sequence); // 0x4dc090
extern int32_t network_game_client_decode_join_finalize_ack(network_client_globals *client, const uint8_t *buffer,
    void *capacity, int32_t *expected_sequence); // 0x4dc120
extern char network_game_client_decode_state_update_chunk(network_client_globals *client,
    uint8_t *param_1, int16_t *param_2, int32_t *param_3); // 0x4dc190, already rewritten (by a
    // separate pass) with a 4-parameter reconstruction that likewise cannot be satisfied here.
extern char network_game_client_decode_player_join_chunk(void); // 0x4dc240, outside this task's range
extern char network_game_client_decode_player_slot_chunk(void); // 0x4dc2e0, outside this task's range
extern char network_game_client_decode_sync_complete(void); // 0x4dc3a0, outside this task's range
extern char network_game_message_decode_replicated_command(void); // 0x4dc410, outside this task's range
extern char network_game_message_decode_ingame_notification(void); // 0x4dc4b0, outside this task's range

// blam-cc: EDX -> record, EDI -> record_length
char network_game_message_decode_dispatch(uint16_t *record, int32_t record_length)
{
    char result;
    uint8_t type_byte;

    result = 1;
    if ((*record & 3) == 0 && ((*record >> 2) & 3) == 3) {
        type_byte = *((uint8_t *)record + record_length - 1);
        switch (type_byte) {
        case 2:
            return ((network_game_message_handler)network_game_client_decode_beacon_reply)();
        case 3:
            return ((network_game_message_handler)network_game_client_decode_pong_reply)();
        case 4:
            return (char)((int32_t (*)(void))network_game_decode_settings_request)();
        case 5:
            return (char)((int32_t (*)(void))network_game_client_decode_join_accepted)();
        case 6:
            return (char)((int32_t (*)(void))network_game_client_decode_connect_rejected)();
        case 7:
            return (char)((int32_t (*)(void))network_game_client_decode_join_complete)();
        case 8:
            return (char)((int32_t (*)(void))network_game_client_decode_settings_or_ack)();
        case 9:
            return (char)((int32_t (*)(void))network_game_client_decode_player_config_value)();
        case 10:
            return (char)((int32_t (*)(void))network_game_client_decode_join_finalize_message)();
        case 0xb:
            return (char)((int32_t (*)(void))network_game_client_decode_join_finalize_ack)();
        case 0xc:
            return (char)((int32_t (*)(void))network_game_client_decode_and_discard_join_message)();
        case 0xd:
            return (char)((int32_t (*)(void))network_game_client_decode_and_discard_ingame_message)();
        case 0x16:
            return ((network_game_message_handler)network_game_client_decode_state_update_chunk)();
        case 0x17:
            return network_game_client_decode_player_join_chunk();
        case 0x18:
            return network_game_client_decode_player_slot_chunk();
        case 0x19:
            return network_game_client_decode_sync_complete();
        case 0x21:
            return network_game_message_decode_replicated_command();
        case 0x22:
            return network_game_message_decode_ingame_notification();
        }
    }
    return result;
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
