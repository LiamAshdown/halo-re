// network_game_message_handle_handshake_forward  (Ghidra: FUN_004e25e0; named per this rewrite)
// address 0x4e25e0, size 74 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md: "Handles message type 0x13 by decoding it and
// forwarding to FUN_004e0590" -- network_client_connection_handshake_tick, already written in
// an earlier batch.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_client_connection_handshake_tick is called here with zero visible arguments,
// against its own file's (state, owner) signature; matching Ghidra literally.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e25e0: ESI server, EDX record, stack length; state (+4) 0, class 3; the decoded state goes to
// network_client_connection_handshake_tick (EAX state, ECX server).

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6


uint32_t network_game_message_handle_handshake_forward(network_server_globals *server, uint8_t *record, int32_t length)
{
    int32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        network_client_connection_handshake_tick((int16_t)body[0], server);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e25e0), from tools/pack.py 0x4e25e0:

undefined4 FUN_004e25e0(void)

{
  char cVar1;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if (*(short *)(unaff_ESI + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,in_EDX + 2,local_8,
                       local_c,3);
    if (cVar1 != '\0') {
      FUN_004e0590();
    }
  }
  return 1;
}
#endif
