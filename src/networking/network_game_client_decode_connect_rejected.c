// network_game_client_decode_connect_rejected  (Ghidra: FUN_004dbd40; renamed, no prior name)
// address 0x4dbd40, size 124 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes a connect-rejected / error
// notification from the server and triggers the client disconnect path with the given error
// code"). Same guard/decode shape as the rest of this handler cluster.
// register convention: client in EAX (in_EAX); stack -> buffer, capacity, expected_sequence.
// blam-cc: EAX -> client; stack -> buffer, capacity, expected_sequence
// UNSURE: `network_session_disconnect_with_error` (network_session_disconnect_with_error, this batch) is called here with
// no visible argument; reconstructed as taking the decoded record's first word (the only
// plausible source of "the given error code" per the summary).
// UNSURE: the real return value is `local_18 & 0xffffff00` in Ghidra (always 0, since local_18
// only ever holds a decode-success byte or the disconnect call's own boolean result by this
// point); modeled directly as 0.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the third argument is the record length (an int);
// the original reduces it by 2 in its own slot and passes its address (EAX) as data_packet_group_decode_packet's
// remaining length -- that argument was missing from the declaration, so every other argument was shifted by one.

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
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6
extern data_packet_group network_game_messages_group; // 0x006994f8
extern void network_session_disconnect_with_error(int16_t error_code); // 0x4d97e0, this batch

// blam-cc: EAX -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_client_decode_connect_rejected(network_client_globals *client, const uint8_t *buffer,
    int32_t length, const uint32_t *expected_sequence)
{
    network_resolved_address sender;
    uint32_t decoded_body[2];
    int16_t out_a;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence && client->state != 0 && client->state != 4) {
        uint16_t version_used;
        if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                             decoded_body, buffer + 2, &out_a, &version_used, 2) != 0) {
            network_session_disconnect_with_error((int16_t)decoded_body[0]); // 0x4dbdac: EAX = body[0]
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4dbd40):

uint FUN_004dbd40(int param_1,undefined4 param_2,uint *param_3)

{
  int in_EAX;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  uint local_18;

  FUN_004dd390();
  if (((local_18 == *param_3) && (*(short *)(in_EAX + 0xeda) != 0)) &&
     (*(short *)(in_EAX + 0xeda) != 4)) {
    local_18 = data_packet_group_decode_packet
                         (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                          &param_3,2);
    if ((char)local_18 != '\0') {
      local_18 = FUN_004d97e0();
    }
  }
  return local_18 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
