// network_game_client_decode_sync_complete  (Ghidra: FUN_004dc3a0; named per this rewrite)
// address 0x4dc3a0, size 108 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Decodes the final synchronization message of the
// join handshake and transitions the connection into the fully-connected/in-game state." Matches
// the code exactly: on a matching sequence and client->state == 3, it decodes the (unused)
// payload and forces state to 4, always reporting the message consumed (1).
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// shared EAX/ESI->client, param_2->remaining_length reconstruction, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, byte_stream *input,
    uint16_t *out_version_used, int16_t expected_class); // 0x4d09d0

// blam-cc: ESI -> client (unaff_ESI)
int32_t network_game_client_decode_sync_complete(network_client_globals *client, uint8_t *param_1,
    int16_t *param_2, int32_t *param_3)
{
    network_resolved_address sender;
    int16_t out_type;
    byte_stream input;
    uint32_t decoded_body;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *param_3 && client->state == 3) {
        data_packet_group_decode_packet(param_2, &network_game_messages_group, &decoded_body,
            param_1 + 2, &out_type, &input, 0, 4);
        client->state = 4;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dc3a0):

undefined4 FUN_004dc3a0(int param_1,undefined4 param_2,int *param_3)

{
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 3)) {
    data_packet_group_decode_packet
              (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,&param_3,4)
    ;
    *(undefined2 *)(unaff_ESI + 0xeda) = 4;
  }
  return 1;
}
#endif
