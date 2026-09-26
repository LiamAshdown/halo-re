// network_game_message_decode_ingame_notification  (Ghidra: FUN_004dc4b0; named per this rewrite)
// address 0x4dc4b0, size 166 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
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
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
    // 0x4d09d0, blam-cc: EAX -> remaining_length, stack -> group, decoded_body, buffer, out_type,
    //           out_version_used, expected_class
extern void chat_close(void); // 0x4aa900, this module's leave-game path

// blam-cc: ESI -> client, stack -> buffer, length, sender_address
// FIXED (step 1, objdump -d 0x4dc4b0..0x4dc555): stack (buffer, length, sender address); the decoder gets &length after
// the 2-byte header (the draft passed the length as a pointer plus two extra arguments). Another sender -> 1; not in
// game (state 4) -> 0; otherwise the (class 6) decode result, after defaulting +0xedc to 8 and, unless this machine is
// hosting with bit 2, raising the handoff flag and closing chat.
int32_t network_game_message_decode_ingame_notification(network_client_globals *client, const uint8_t *buffer,
    int32_t length, const uint32_t *sender_address)
{
    network_resolved_address sender;
    uint8_t decoded_body[32];
    int16_t out_type;
    uint16_t out_version;
    int32_t decoded = 0;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address) {
        return 1;
    }
    if (client->state != 4) {
        return 0;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 6) != 0) {
        decoded = 1;
    }
    if (client->unknown_edc == 0) {
        client->unknown_edc = 8;
    }
    if (network_server == 0 || ((network_server->flags >> 2) & 1) == 0) {
        network_host_handoff_requested = 1;
        chat_close();
    }
    return decoded;
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
