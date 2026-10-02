// network_game_decode_settings_request  (Ghidra: FUN_004dbc00; renamed, no prior name)
// address 0x4dbc00, size 177 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Decodes an early-handshake server/map
// identification message and compares it against locally cached data, advancing the loading UI
// state on a mismatch"); forwards the decoded record straight into
// network_game_settings_packet_send.c (0x4d94c0, same task batch), which builds and sends the
// full settings/map-data reply -- consistent with this being the *host's* handler for a
// newly-joined client's settings request. Follows the same guard/decode shape as
// network_game_client_decode_state_update_chunk.c (0x4dc190, same address family).
// register convention: client in ESI (unaff_ESI); param_1/param_2/param_3 are ordinary stack
// parameters per Ghidra's own recovery. // blam-cc: ESI -> client; stack -> buffer, capacity,
// expected_sequence
// UNSURE: `network_channel_remote_address_or_default` and `network_game_settings_packet_send` are both called
// with no visible arguments beyond what Ghidra shows; reconstructed per the established
// conventions of this cluster (client + a fresh decode_result for the guard; client + the
// decoded record for the settings-send call).
// UNSURE: `DAT_0069b350` is not declared in types/networking.h; named generically from its
// boolean-cast source.

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
extern uint32_t message_delta_vector3d_mode; // 0x0069b350, UNSURE name
extern void network_game_settings_packet_send(network_client_globals *client, const uint8_t *request); // 0x4d94c0

// blam-cc: ESI -> client; stack -> buffer, capacity, expected_sequence
int32_t network_game_decode_settings_request(network_client_globals *client, const uint8_t *buffer,
                                              int32_t length, const int32_t *expected_sequence)
{
    network_resolved_address sender;
    uint8_t decoded_body[0x94];   // the engine-version byte is +0x08 (0x4dbc7c)
    int16_t out_a, out_b;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence && client->state == 1) { // UNSURE: mode field
        if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                             decoded_body, buffer + 2, &out_a, (uint16_t *)&out_b, 2) != 0) {
            message_delta_vector3d_mode = (decoded_body[8] == 1);
            network_game_settings_packet_send(client, decoded_body); // 0x4dbc87: stack body, EBX client
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4dbc00):

undefined4 FUN_004dbc00(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  undefined1 local_b4 [4];
  undefined1 local_b0 [4];
  int local_ac;
  undefined1 local_94 [8];
  char local_8c;

  FUN_004dd390();
  if ((local_ac == *param_3) && (*(short *)(unaff_ESI + 0xeda) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_94,param_1 + 2,local_b4,
                       local_b0,2);
    if (cVar1 != '\0') {
      DAT_0069b350 = (uint)(local_8c == '\x01');
      FUN_004d94c0(local_94);
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
