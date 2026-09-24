// network_game_client_decode_player_config_value  (Ghidra: FUN_004dbf30; renamed, no prior name)
// address 0x4dbf30, size 117 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes a small per-player
// configuration value from the server and stores it directly into the connection context during
// the join handshake"). client+0xed8 matches types/networking.h's
// network_client_globals::unknown_ed8 exactly.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `data_packet_group_decode_packet`'s 2nd argument (the decode destination) is literally
// `&param_3` in Ghidra -- the address of the caller's own `expected_sequence` pointer parameter
// -- and the decoded value is then read back from that same location. Preserved exactly, though
// it looks like it could be a decompiler artifact rather than intentional; modeled here by
// decoding into a local and writing that local's address into `*expected_sequence_slot`, which
// reproduces the same "decode result ends up reachable through the expected_sequence parameter"
// shape.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern data_packet_group network_game_messages_group; // 0x006994f8

// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_client_decode_player_config_value(network_client_globals *client,
    const uint8_t *buffer, void *capacity, int32_t *expected_sequence)
{
    network_resolved_address sender;
    int16_t out_a, out_b;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence && client->state == 2) { // UNSURE: mode field
        // UNSURE: decode destination is `&expected_sequence` (the address of this function's own
        // copy of that pointer parameter), matching Ghidra's literal `&param_3`; see file header.
        if (data_packet_group_decode_packet(&network_game_messages_group,
                                             &expected_sequence, buffer + 2, &out_a, &out_b, 2) != 0) {
            client->unknown_ed8 = *(int16_t *)&expected_sequence;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dbf30):

undefined4 FUN_004dbf30(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 2)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,&param_3,param_1 + 2,local_1c,
                       local_20,2);
    if (cVar1 != '\0') {
      *(undefined2 *)(unaff_ESI + 0xed8) = param_3._0_2_;
    }
  }
  return 1;
}
#endif
