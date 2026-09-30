// network_game_message_handle_settings_relay  (Ghidra: FUN_004e24d0; named per this rewrite)
// address 0x4e24d0, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md: "Server-side handler for message type 0x10
// that decodes its payload and passes it to FUN_004df0e0" -- the already-written
// network_game_settings_broadcast_send. Follows the exact decode-then-forward shape shared by
// every sibling handler in this cluster (see network_game_process_incoming_message.c).
// register convention: ESI = server (implicit passthrough), EDX = buffer (implicit
// passthrough), matching every sibling handler in this batch.
//   // blam-cc: ESI -> server, EDX -> buffer
// UNSURE: network_game_settings_broadcast_send is called here with zero visible arguments in
// Ghidra's own decompile, against its own file's established (round, record) signature;
// declared and called with no arguments, matching Ghidra literally.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e24d0: ESI server, EDX record, stack length; state (+4) 0, class 3.

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


uint32_t network_game_message_handle_settings_relay(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        return network_game_settings_broadcast_send((uint32_t)server, body);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e24d0), from tools/pack.py 0x4e24d0:

undefined4 FUN_004e24d0(void)

{
  char cVar1;
  undefined4 uVar2;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [32];

  if (*(short *)(unaff_ESI + 4) == 0) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       local_28,3);
    if (cVar1 != '\0') {
      uVar2 = FUN_004df0e0();
      return uVar2;
    }
  }
  return 1;
}
#endif
