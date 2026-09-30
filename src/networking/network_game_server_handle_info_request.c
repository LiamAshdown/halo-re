// network_game_server_handle_info_request  (Ghidra: FUN_004e2700; named per this rewrite)
// address 0x4e2700, size 136 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
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
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2700: ESI server, EDI machine, EDX record, stack length; state (+4) 0 or 1, class 5. A machine whose
// connection (+0) has +0xa98 set, or any machine while the server's +0xa0f is clear, gets
// network_game_server_handle_client_join (stack server, machine) and 1; otherwise the full game info packet is
// built for it (0x4e0bd0) and its result returned. Returns 0 when the state or the decode fails.

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6


uint32_t network_game_server_handle_info_request(network_server_globals *server, network_machine *machine, uint8_t *record,
    int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;
    int16_t state = *(int16_t *)((uint8_t *)server + 4);
    uint8_t *connection;

    if ((state != 0 && state != 1) || data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) == 0) {
        return 0;
    }
    connection = machine != 0 ? *(uint8_t **)machine : 0;
    if ((connection != 0 && connection[0xa98] != 0) || *((uint8_t *)server + 0xa0f) == 0) {
        network_game_server_handle_client_join(0 /* UNSURE: a register pass-through */, server, machine, 1);
        return 1;
    }
    return (uint8_t)network_server_build_full_game_info_packet(machine);
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
