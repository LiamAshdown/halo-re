// network_game_message_handle_handshake_forward  (Ghidra: FUN_004e25e0; named per this rewrite)
// address 0x4e25e0, size 74 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Handles message type 0x13 by decoding it and
// forwarding to FUN_004e0590" -- network_client_connection_handshake_tick, already written in
// an earlier batch.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_client_connection_handshake_tick is called here with zero visible arguments,
// against its own file's (state, owner) signature; matching Ghidra literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern void network_client_connection_handshake_tick(void); // 0x4e0590, this module,
    // called here with no visible arguments (UNSURE, see header)

// blam-cc: ESI -> server, EDX -> buffer
uint32_t network_game_message_handle_handshake_forward(network_server_globals *server, uint8_t *buffer)
{
    uint8_t decoded_body[4];
    int16_t out_a, out_b;

    if (server->unknown_004 == 0) {
        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, &out_b, 3) != 0) {
            network_client_connection_handshake_tick();
        }
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
