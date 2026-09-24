// network_game_client_decode_join_complete  (Ghidra: network_game_client_decode_join_complete,
// already named)
// address 0x4dbdc0, size 141 bytes
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Decodes the server's final
// join-complete confirmation and transitions the client to the in-game connection state").
// Same guard/decode shape as the rest of this handler cluster; client->state = 4 on success
// matches this cluster's other functions treating state as a live connection-mode value.
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t join_ui_state; // 0x00718f8c

// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_client_decode_join_complete(network_client_globals *client, const uint8_t *buffer,
    void *capacity, const uint32_t **expected_sequence)
{
    network_resolved_address sender;
    uint32_t decoded_body[2];
    int16_t out_a;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == **expected_sequence) {
        if (client->state != 0 && client->state != 4) { // UNSURE: live connection-mode value
            if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                                 &out_a, (int16_t *)expected_sequence, 2) != 0) {
                join_ui_state = 9;
                client->state = 4;
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4dbdc0):

undefined4 network_game_client_decode_join_complete(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if (local_18 == *param_3) {
    if ((*(short *)(unaff_ESI + 0xeda) != 0) && (*(short *)(unaff_ESI + 0xeda) != 4)) {
      cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                         &param_3,2);
      if (cVar1 != '\0') {
        DAT_00718f8c = 9;
        *(undefined2 *)(unaff_ESI + 0xeda) = 4;
        return 1;
      }
    }
  }
  return 0;
}
#endif
