// network_game_server_handle_info_request  (Ghidra: FUN_004e2700; named per this rewrite)
// address 0x4e2700, size 136 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Handles message type 0x1a, sending a full
// server-info reply to not-yet-established machines or otherwise forwarding to FUN_004dfc90"
// -- network_game_server_handle_client_join, already written. `*unaff_EDI + 0xa98` matches
// network_channel::connected via network_machine::channel (offset 0), and
// `unaff_ESI + 0xa0f` matches network_server_globals::game_over, exactly as in the type-0xe and
// type-0xf handlers.
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough), EDI = machine (implicit passthrough).
//   // blam-cc: ESI -> server, EDX -> buffer, EDI -> machine
// UNSURE: both network_server_build_full_game_info_packet and
// network_game_server_handle_client_join are called here with zero visible arguments, against
// their own files' non-empty signatures; matching Ghidra literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern char network_server_build_full_game_info_packet(void); // 0x4e0bd0, this module,
    // called here with no visible arguments (UNSURE, see header)
extern void network_game_server_handle_client_join(void); // 0x4dfc90, this module,
    // called here with no visible arguments (UNSURE, see header)

// blam-cc: ESI -> server, EDX -> buffer, EDI -> machine
uint32_t network_game_server_handle_info_request(network_server_globals *server, uint8_t *buffer, network_machine *machine)
{
    uint8_t decoded_body[4];
    int16_t out_a, out_b;

    if (server->unknown_004 != 0 && server->unknown_004 != 1) {
        return 0;
    }
    if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                         &out_a, &out_b, 5) == 0) {
        return 0;
    }
    if ((machine == 0 || machine->channel == 0 || machine->channel->connected == 0) &&
        server->game_over != 0) {
        return network_server_build_full_game_info_packet();
    }
    network_game_server_handle_client_join();
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2700), from tools/pack.py 0x4e2700:

undefined4 FUN_004e2700(void)

{
  char cVar1;
  undefined4 uVar2;
  int in_EDX;
  int unaff_ESI;
  int *unaff_EDI;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if ((*(short *)(unaff_ESI + 4) == 0) || (*(short *)(unaff_ESI + 4) == 1)) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,in_EDX + 2,local_8,
                       local_c,5);
    if (cVar1 != '\0') {
      if ((((unaff_EDI == (int *)0x0) || (*unaff_EDI == 0)) ||
          (*(char *)(*unaff_EDI + 0xa98) == '\0')) && (*(char *)(unaff_ESI + 0xa0f) != '\0')) {
        uVar2 = FUN_004e0bd0();
        return uVar2;
      }
      FUN_004dfc90();
      return 1;
    }
  }
  return 0;
}
#endif
