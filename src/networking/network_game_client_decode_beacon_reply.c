// network_game_client_decode_beacon_reply  (Ghidra: network_game_client_decode_beacon_reply,
// already named)
// address 0x4db9a0, size 119 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes an incoming game-announcement
// ('beacon') message received while idle and adds it to the discovered-games search list").
// register convention: client in EAX (in_EAX), the incoming buffer in EDX (in_EDX).
// blam-cc: EAX -> client, EDX -> buffer
// UNSURE (major): `network_game_search_results_add_or_update` (0x4da7d0, this task's batch) is
// called with a single stack argument, `client + 4` -- but that function's own decompiled body
// (a genuine stack parameter plus a register `announcement`) takes the results-table pointer on
// the stack and the announcement in a register. That means client+4 is the results-table
// argument, which is new evidence that types/networking.h's undeclared 9-slot
// network_game_search_entry table lives at client+4 (inside the still-unresolved
// unknown_002[0xada] block) -- worth a header update in a future pass. The announcement
// argument itself is not visibly set at this call site; reconstructed as `buffer`, the only
// address in scope that could be the decoded announcement payload.
// UNSURE: `data_packet_group_decode_packet` is called here with only 6 of its fuller,
// separately-reconstructed 8-argument signature (see
// network_game_client_decode_state_update_chunk.c); called here with a matching 6-argument local
// prototype instead of forcing the mismatch, per the same reasoning documented in
// network_send_join_request_packet.c for data_packet_group_encode_packet.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t network_game_search_results_add_or_update(network_game_search_entry *results,
    const uint8_t *announcement); // 0x4da7d0, this batch

// blam-cc: EAX -> client, EDX -> buffer
int32_t network_game_client_decode_beacon_reply(network_client_globals *client, const uint8_t *buffer)
{
    uint8_t decoded_body[368];
    int16_t out_a, out_b;

    if (client->state == 0) { // UNSURE: live connection-mode value, not padding
        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, &out_b, 1) != 0) {
            network_game_search_results_add_or_update(
                (network_game_search_entry *)((uint8_t *)client + 4), buffer); // UNSURE argument
        }
        return 1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4db9a0):

undefined4 network_game_client_decode_beacon_reply(void)

{
  char cVar1;
  int in_EAX;
  int in_EDX;
  undefined1 local_178 [4];
  undefined1 local_174 [4];
  undefined1 local_170 [368];

  if (*(short *)(in_EAX + 0xeda) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_170,in_EDX + 2,local_178,
                       local_174,1);
    if (cVar1 != '\0') {
      network_game_search_results_add_or_update(in_EAX + 4);
    }
    return 1;
  }
  return 1;
}
#endif
