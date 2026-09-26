// network_game_client_decode_join_complete  (Ghidra: network_game_client_decode_join_complete,
// already named)
// address 0x4dbdc0, size 141 bytes
// name confidence: 0.55   rewrite confidence: 0.9 (step 1: rewritten from the disassembly; see the note above the function)
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
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
    // 0x4d09d0, blam-cc: EAX -> remaining_length, stack -> group, decoded_body, buffer, out_type,
    //           out_version_used, expected_class
extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t join_ui_state; // 0x00718f8c

// blam-cc: ESI -> client, stack -> buffer, length, sender_address
// FIXED (step 1, objdump -d 0x4dbdc0..0x4dbe4c): stack (buffer, length, sender address); the sender address is
// dereferenced once; the decoder gets &length after the 2-byte header. Returns 0 unless the join completed.
int32_t network_game_client_decode_join_complete(network_client_globals *client, const uint8_t *buffer,
    int32_t length, const uint32_t *sender_address)
{
    network_resolved_address sender;
    uint8_t decoded_body[16];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address || client->state == 0 || client->state == 4) {
        return 0;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 2) == 0) {
        return 0;
    }
    join_ui_state = 9;
    client->state = 4;
    return 1;
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
