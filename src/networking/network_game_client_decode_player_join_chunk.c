// network_game_client_decode_player_join_chunk  (Ghidra: FUN_004dc240; named per this rewrite)
// address 0x4dc240, size 156 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes another synchronization-phase data
// chunk during the join handshake, aborting via the shared cleanup routine on failure." On
// success it calls network_player_join_finalize (0x4d9e30, already named, "Creates the
// game-object datum for a player once their connection has fully joined"), which gives this
// handler its name. Accepted only while client->state == 3; when it is 4 the message is
// silently treated as consumed with no decode attempt.
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// general EAX->client, param_2->remaining_length reconstruction this whole cluster shares, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills.
// UNSURE: network_player_join_finalize's own decompile (0x4d9e30) shows it taking an elided
// EAX (a network_player_entry *) and EDI (client, viewed as ushort * for word-indexed access at
// +0x58a == byte +0xb14 and +0x76d == byte +0xeda, both matching this cluster's client offsets)
// with no arguments visible at ITS call sites either -- the same Ghidra-drops-dead-looking-loads
// problem as network_channel_remote_address_or_default. Passing just `client` here is a lower bound on what
// the original call needed; the player-entry argument could not be reconstructed from this
// function's own decompilation and is left out, which is the single largest risk to this file's
// rewrite confidence.

// FIXED 2026-09-28 (networking call audit, from the disassembly): the dispatcher (0x4db6b0) passes (client, record,
// length, sender) -- the length is an int, which the original reduces by 2 in its own argument slot and hands to
// data_packet_group_decode_packet by address (EAX) as the remaining length; that call takes 7 arguments (remaining,
// group, body, record + 2, out_type, out_version_used, expected class), not 8 (the extra one made the class 0);
// network_disconnect_notify_dropped_machines gets the client (EBX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6
extern char network_player_join_finalize(network_client_globals *client, void *entry); // EDI client, EAX entry // 0x4d9e30, elided
    // register args only partially resolved -- see header UNSURE note
extern void network_disconnect_notify_dropped_machines(network_client_globals *client); // 0x4d9340, elided register args unresolved

// blam-cc: EAX -> client
char network_game_client_decode_player_join_chunk(network_client_globals *client, uint8_t *param_1,
    int32_t param_2, int32_t *param_3)
{
    char result;
    network_resolved_address sender;
    int16_t out_type;
    byte_stream input;
    uint32_t decoded_body[8]; // opaque destination record

    result = 0;
    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    if (client->state == 3) {
        if (data_packet_group_decode_packet((param_2 -= 2, (int16_t *)&param_2), &network_game_messages_group,
                decoded_body, param_1 + 2, &out_type, (uint16_t *)&input, 4) != 0) {
            result = network_player_join_finalize(client, decoded_body); // 0x4dc2ac: EAX = the decoded body
            if (result != 0) {
                return result;
            }
        }
    } else if (client->state == 4) {
        return 1;
    }
    network_disconnect_notify_dropped_machines(client);
    return result;
}

#if 0
Original Ghidra decompilation (0x4dc240):

char FUN_004dc240(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int in_EAX;
  char local_25;
  undefined1 local_24 [4];
  int local_20 [8];

  local_25 = '\0';
  FUN_004dd390();
  if (local_20[0] != *param_3) {
    return '\x01';
  }
  if (*(short *)(in_EAX + 0xeda) == 3) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,param_1 + 2,local_24,
                       &param_3,4);
    if ((cVar1 != '\0') && (local_25 = network_player_join_finalize(), local_25 != '\0')) {
      return local_25;
    }
  }
  else if (*(short *)(in_EAX + 0xeda) == 4) {
    return '\x01';
  }
  FUN_004d9340();
  return local_25;
}
#endif
