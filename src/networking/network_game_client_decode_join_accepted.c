// network_game_client_decode_join_accepted  (Ghidra: network_game_client_decode_join_accepted,
// already named)
// address 0x4dbcc0, size 127 bytes
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes the server's join-accepted
// message, assigning the local client its player slot index and advancing the connection to the
// next handshake state"). Same guard/decode shape as network_game_decode_settings_request.c.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `data_packet_group_decode_packet`'s 5th argument is literally `&param_3` in Ghidra
// (the address of the caller's own `expected_sequence` pointer parameter, i.e. a
// pointer-to-pointer) rather than a fresh local buffer -- preserved exactly, though it looks
// like it could be a decompiler artifact rather than intentional.
// UNSURE: `network_channel_remote_address_or_default` and `network_session_player_join_notify` are both
// called with no visible arguments beyond what Ghidra shows; reconstructed per the established
// conventions of this cluster.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern data_packet_group network_game_messages_group; // 0x006994f8
extern void network_session_player_join_notify(network_client_globals *client,
    const uint32_t *source); // 0x4d9700, this batch

// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_client_decode_join_accepted(network_client_globals *client, const uint8_t *buffer,
    void *capacity, const int32_t **expected_sequence)
{
    network_resolved_address sender;
    uint32_t decoded_body[2];
    int16_t out_a;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == **expected_sequence && client->state == 1) { // UNSURE: mode field
        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, (int16_t *)expected_sequence, 2) != 0) {
            network_session_player_join_notify(client, decoded_body); // UNSURE argument
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4dbcc0):

undefined4 network_game_client_decode_join_accepted(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_24 [4];
  undefined1 local_20 [8];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,param_1 + 2,local_24,
                       &param_3,2);
    if (cVar1 != '\0') {
      FUN_004d9700();
      return 1;
    }
  }
  return 0;
}
#endif
