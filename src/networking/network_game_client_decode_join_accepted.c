// network_game_client_decode_join_accepted  (Ghidra: network_game_client_decode_join_accepted,
// already named)
// address 0x4dbcc0, size 127 bytes
// name confidence: 0.55   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
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


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
    // 0x4d09d0, blam-cc: EAX -> remaining_length, stack -> group, decoded_body, buffer, out_type,
    //           out_version_used, expected_class
extern data_packet_group network_game_messages_group; // 0x006994f8
extern void network_session_player_join_notify(network_client_globals *client,
    const uint32_t *source); // 0x4d9700, this batch

// blam-cc: ESI -> client, stack -> buffer, length, sender_address
// FIXED (step 1, objdump -d 0x4dbcc0..0x4dbd3e): stack (buffer, length, sender address), the sender dereferenced once,
// &length after the header for the decoder; a decoded (class 2) acceptance goes to
// network_session_player_join_notify(EAX client, ECX &decoded). Returns 1 only then.
int32_t network_game_client_decode_join_accepted(network_client_globals *client, const uint8_t *buffer,
    int32_t length, const uint32_t *sender_address)
{
    network_resolved_address sender;
    uint32_t decoded_body[8];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address || client->state != 1) {
        return 0;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 2) == 0) {
        return 0;
    }
    network_session_player_join_notify(client, decoded_body);
    return 1;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
