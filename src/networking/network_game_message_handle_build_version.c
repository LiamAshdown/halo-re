// network_game_message_handle_build_version  (Ghidra: FUN_004e2630; named per this rewrite)
// address 0x4e2630, size 100 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md's summary claims message types 0x14/0x25, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x15: FUN_004e2630();` -- the low-confidence (0.3) summary swapped this function's type
// number with FUN_004e26a0's (which the same switch shows at `case 0x14: case 0x25:`); the
// dispatcher's literal call table is trusted here instead. Forwards to
// network_machine_check_build_version (FUN_004dff20, already written: EAX -> remote_version,
// EDI -> machine).
// register convention: EAX = server (implicit passthrough), stack = buffer (a genuine
// parameter, matching Ghidra's own recovered `int param_1`).
//   // blam-cc: EAX -> server, stack -> buffer
// UNSURE: network_machine_check_build_version is called here with zero visible arguments,
// against its own file's (remote_version, machine) signature; matching Ghidra literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern void network_machine_check_build_version(void); // 0x4dff20, this module,
    // called here with no visible arguments (UNSURE, see header)

// blam-cc: EAX -> server, stack -> buffer
uint32_t network_game_message_handle_build_version(network_server_globals *server, uint8_t *buffer)
{
    uint8_t decoded_body[256];
    int16_t out_a, out_b;

    if (server->unknown_004 == 0) {
        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, &out_b, 3) != 0) {
            network_machine_check_build_version();
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2630), from tools/pack.py 0x4e2630:

undefined4 FUN_004e2630(int param_1)

{
  char cVar1;
  int in_EAX;
  undefined1 local_108 [4];
  undefined1 local_104 [4];
  undefined1 local_100 [256];

  if (*(short *)(in_EAX + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_100,param_1 + 2,local_108,
                       local_104,3);
    if (cVar1 != '\0') {
      FUN_004dff20();
    }
  }
  return 1;
}
#endif
