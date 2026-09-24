// network_game_client_decode_join_finalize_message  (Ghidra: FUN_004dc090; renamed, no prior
// name)
// address 0x4dc090, size 130 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Decodes a join-handshake message (type
// 0xa) and forwards it to a secondary validation/processing routine, propagating its
// success/failure result"); the "secondary routine" is network_connection_finalize_join.c
// (0x4d9960, this task's batch), called with the client pointer.
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
extern int32_t network_connection_finalize_join(uint16_t *connection); // 0x4d9960, this batch

// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_client_decode_join_finalize_message(network_client_globals *client, const uint8_t *buffer,
    void *capacity, int32_t *expected_sequence)
{
    network_resolved_address sender;
    int16_t out_a, out_b;

    // FIXED in the review pass: 0x4dc090 is `mov eax,[esi+0xadc]`, i.e. client->channel;
    // the first draft's `(network_channel **)(client + 0x56e)` was pointer arithmetic on a
    // network_client_globals * and did not address anything real.
    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *expected_sequence) {
        return 1;
    }
    if (client->state == k_network_client_state_joining) {
        // UNSURE: decode destination is `&expected_sequence`; see
        // network_game_client_decode_player_config_value.c for the same idiom.
        if (data_packet_group_decode_packet(&network_game_messages_group, &expected_sequence,
                                             buffer + 2, &out_a, &out_b, 2) != 0) {
            return network_connection_finalize_join((uint16_t *)client);
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4dc090):

int FUN_004dc090(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int iVar2;
  ushort *unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if (local_18 != *param_3) {
    return 1;
  }
  if (unaff_ESI[0x76d] == 2) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                       &param_3,2);
    if (cVar1 != '\0') {
      iVar2 = network_connection_finalize_join(unaff_ESI);
      return iVar2;
    }
  }
  return 0;
}
#endif
