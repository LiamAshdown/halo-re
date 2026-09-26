// network_game_client_decode_player_slot_chunk  (Ghidra: FUN_004dc2e0; named per this rewrite)
// address 0x4dc2e0, size 183 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes a third variant of synchronization-phase
// data chunk, accepted across a wider range of connection states." On success it calls
// network_session_player_table_index_apply, whose own summary ("Finds the player slot matching a given machine id and
// records its assigned table index (e.g. team or score-table slot) for that player") gives this
// handler its name. Accepted while client->state is 2, 3 or 4.
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// shared EAX/ESI->client, param_2->remaining_length reconstruction, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills. Here the
// client pointer arrives via ESI (unaff_ESI) rather than EAX, still the sole blam-cc argument.
// The trailing "else if (state == 4) return 1" is unreachable in the original -- state == 4 is
// already covered by the first branch's condition -- and is kept verbatim rather than removed,
// since preserving control flow exactly takes priority over simplifying apparently-dead code
// Ghidra may be reporting faithfully from the real binary.

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
extern uint8_t network_session_player_table_index_apply(network_client_globals *client, int32_t param_2); // 0x4d9190, elided
    // register args unresolved; the visible params are this function's own best-guess mapping
extern void network_disconnect_notify_dropped_machines(void); // 0x4d9340, elided register args unresolved

// blam-cc: ESI -> client (unaff_ESI)
char network_game_client_decode_player_slot_chunk(network_client_globals *client, uint8_t *param_1,
    int16_t *param_2, int32_t *param_3)
{
    char result;
    network_resolved_address sender;
    int16_t state;
    int16_t out_type;
    byte_stream input;
    uint32_t decoded_body[9]; // opaque destination record

    result = 0;
    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    state = client->state;
    if (state == 3 || state == 4 || state == 2) {
        if (data_packet_group_decode_packet(param_2, &network_game_messages_group, decoded_body,
                param_1 + 2, &out_type, &input, 0, 4) != 0) {
            result = network_session_player_table_index_apply(client, 0); // UNSURE: this call site shows zero visible
                // arguments even though network_session_player_table_index_apply's own decompile declares two ordinary stack
                // parameters; its second (a table-index value written into a per-player record)
                // could not be reconstructed from this function's own decompilation, so 0 is a
                // placeholder, not an observed value
            if (result != 0) {
                return result;
            }
        }
    } else if (state == 4) {
        return 1; // unreachable, kept verbatim -- see header note
    }
    network_disconnect_notify_dropped_machines();
    return result;
}

#if 0
Original Ghidra decompilation (0x4dc2e0):

char FUN_004dc2e0(int param_1,undefined4 param_2,int *param_3)

{
  short sVar1;
  char cVar2;
  int unaff_ESI;
  char local_29;
  undefined1 local_28 [4];
  int local_24 [9];

  local_29 = '\0';
  FUN_004dd390();
  if (local_24[0] != *param_3) {
    return '\x01';
  }
  sVar1 = *(short *)(unaff_ESI + 0xeda);
  if (((sVar1 == 3) || (sVar1 == 4)) || (sVar1 == 2)) {
    cVar2 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_24,param_1 + 2,local_28,
                       &param_3,4);
    if ((cVar2 != '\0') && (local_29 = FUN_004d9190(), local_29 != '\0')) {
      return local_29;
    }
  }
  else if (sVar1 == 4) {
    return '\x01';
  }
  FUN_004d9340();
  return local_29;
}
#endif
