// network_game_client_decode_and_discard_ingame_message  (Ghidra: FUN_004dc020; renamed, no
// prior name)
// address 0x4dc020, size 99 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
// evidence: out/phase4/networking_functions.md summary ("Decodes and discards an in-game
// message type (0xd) using a different message-group index than the join-handshake handlers").
// Structurally identical to network_game_client_decode_and_discard_join_message.c except for
// the mode check (4, in-game, instead of 2) and the message-class argument (6 instead of 2).
// register convention: client in ESI (unaff_ESI); stack -> buffer, capacity, expected_sequence.
// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
    // 0x4d09d0, blam-cc: EAX -> remaining_length, stack -> group, decoded_body, buffer, out_type,
    //           out_version_used, expected_class
extern data_packet_group network_game_messages_group; // 0x006994f8

// blam-cc: ESI -> client, stack -> buffer, length, sender_address
// FIXED (step 1, objdump -d 0x4dc020..0x4dc082): stack (buffer, length, sender address); the decoder gets &length after
// the 2-byte header; the (class 6) message is decoded only to be dropped. Always returns 1.
int32_t network_game_client_decode_and_discard_ingame_message(network_client_globals *client, const uint8_t *buffer,
    int32_t length, const uint32_t *sender_address)
{
    network_resolved_address sender;
    uint8_t decoded_body[32];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 4) {
        length = length - 2;
        data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 6);
    }
    return 1;
}


#if 0
Original Ghidra decompilation (0x4dc020):

undefined4 FUN_004dc020(int param_1,undefined4 param_2,int *param_3)

{
  int unaff_ESI;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  FUN_004dd390();
  if ((local_18 == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 4)) {
    data_packet_group_decode_packet
              (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,&param_3,6)
    ;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
