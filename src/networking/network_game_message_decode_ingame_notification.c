// network_game_message_decode_ingame_notification  (Ghidra: FUN_004dc4b0; named per this rewrite)
// address 0x4dc4b0, size 166 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Decodes an in-game notification and, unless a
// particular game-engine state flag is already set, triggers the client disconnect/leave path --
// consistent with a game-over or host-shutdown notice." Matches the code: once decoded (packet
// class 6, only while client->state == 4), it defaults client->unknown_edc to 8 if still zero,
// then leaves the game (network_host_handoff_requested = 1, chat_close()) unless there is a
// network_server AND its flags bit 2 is set.
// register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
// shared ESI->client, param_2->remaining_length reconstruction, and
// network_channel_remote_address_or_default.c for the local decode-result record it fills.
// UNSURE: network_server_globals.flags documents bit2 as "stats logging" in types/networking.h,
// which would make "skip disconnect while stats logging is on" an odd rule; this rewrite keeps
// the raw bit test against the header's declared field rather than asserting a different meaning.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_server_globals *network_server;      // 0x0071c2d4
extern uint8_t network_host_handoff_requested;       // 0x0071c2de
extern data_packet_group network_game_messages_group; // 0x006994f8


extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, byte_stream *input,
    uint16_t *out_version_used, int16_t expected_class); // 0x4d09d0
extern void chat_close(void); // 0x4aa900, this module's leave-game path

// blam-cc: ESI -> client (unaff_ESI)
int32_t network_game_message_decode_ingame_notification(network_client_globals *client, uint8_t *param_1,
    int16_t *param_2, int32_t *param_3)
{
    int32_t decoded;
    network_resolved_address sender;
    int16_t out_type;
    byte_stream input;
    uint32_t decoded_body[2];

    decoded = 0;
    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *param_3) {
        if (client->state == 4) {
            decoded = data_packet_group_decode_packet(param_2, &network_game_messages_group,
                decoded_body, param_1 + 2, &out_type, &input, 0, 6) != 0;
            if (client->unknown_edc == 0) {
                client->unknown_edc = 8;
            }
            if (network_server == 0 || ((network_server->flags >> 2) & 1) == 0) {
                network_host_handoff_requested = 1;
                chat_close();
            }
        }
        return decoded;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dc4b0):

bool FUN_004dc4b0(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  int unaff_ESI;
  bool bVar2;
  undefined1 local_20 [4];
  undefined1 local_1c [4];
  int local_18;

  bVar2 = false;
  FUN_004dd390();
  if (local_18 == *param_3) {
    if (*(short *)(unaff_ESI + 0xeda) == 4) {
      cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,local_1c,param_1 + 2,local_20,
                         &param_3,6);
      bVar2 = cVar1 != '\0';
      if (*(short *)(unaff_ESI + 0xedc) == 0) {
        *(undefined2 *)(unaff_ESI + 0xedc) = 8;
      }
      if ((DAT_0071c2d4 == 0) || ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)) {
        DAT_0071c2de = 1;
        chat_close();
      }
    }
    return bVar2;
  }
  return true;
}
#endif
