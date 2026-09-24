// network_game_client_decode_pong_reply  (Ghidra: network_game_client_decode_pong_reply, already
// named)
// address 0x4dba20, size 114 bytes
// name confidence: 0.55   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Decodes an incoming ping-reply (pong)
// message and updates the connection's round-trip-time/ping statistics").
// register convention: client in EAX (in_EAX), the incoming buffer in EDX (in_EDX).
// blam-cc: EAX -> client, EDX -> buffer
// UNSURE: `network_connection_retransmit_if_overdue` (network_connection_retransmit_if_overdue.c, this batch) is called with
// a single argument here, `local_4` (an uninitialized 4-byte decode-output local); that
// function's own signature takes three parameters (sender_address, client, deadline_ms) in
// ECX/ESI/EDI. Reconstructed as passing `&decoded_body`-derived data is not evidenced either;
// left calling that function's first parameter slot with the decoded record pointer and 0 for
// the rest, flagged as the weakest link in this file.
// UNSURE: `data_packet_group_decode_packet`'s argument count mismatch is the same as
// network_game_client_decode_beacon_reply.c; see that file's header note.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern data_packet_group network_game_messages_group; // 0x006994f8
extern void network_connection_retransmit_if_overdue(const uint32_t *sender_address,
    network_client_globals *client, uint32_t deadline_ms); // 0x4d93b0, this batch

// blam-cc: EAX -> client, EDX -> buffer
int32_t network_game_client_decode_pong_reply(network_client_globals *client, const uint8_t *buffer)
{
    uint32_t decoded_body;
    int16_t out_a, out_b;

    if (client->state != 0 && client->state != 3) { // UNSURE: live connection-mode value, not padding
        return 1;
    }
    if (data_packet_group_decode_packet(&network_game_messages_group, &decoded_body, buffer + 2,
                                         &out_a, &out_b, 1) != 0) {
        network_connection_retransmit_if_overdue(&decoded_body, client, 0); // UNSURE argument
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dba20):

undefined4 network_game_client_decode_pong_reply(void)

{
  char cVar1;
  int in_EAX;
  int in_EDX;
  undefined1 local_10 [4];
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined4 local_4;

  if ((*(short *)(in_EAX + 0xeda) != 0) && (*(short *)(in_EAX + 0xeda) != 3)) {
    return 1;
  }
  cVar1 = data_packet_group_decode_packet
                    (&PTR_s_network_game_messages_group_006994f8,local_8,in_EDX + 2,local_c,local_10
                     ,1);
  if (cVar1 != '\0') {
    FUN_004d93b0(local_4);
  }
  return 1;
}
#endif
