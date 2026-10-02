// network_game_message_handle_settings_relay_role2  (Ghidra: FUN_004e28d0; named per this rewrite)
// address 0x4e28d0, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x1e, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x23: FUN_004e28d0();` -- the dispatcher's literal call table is trusted here instead
// of the low-confidence (0.3) summary. This is the third of three role-gated forwarders to
// network_game_settings_broadcast_send (roles 0, 1 and 2 at message types 0x10, 0x1d and 0x23
// respectively), gated on role == 2 and decode class 7.
// register convention: ESI = context (implicit passthrough), EDX = buffer (implicit
// passthrough).
//   // blam-cc: ESI -> context, EDX -> buffer
// UNSURE: network_game_settings_broadcast_send is called here with zero visible arguments;
// matching Ghidra literally.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e28d0: ESI server, EDX record, stack length; state (+4) 2, class 7.

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6

extern uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record); // 0x4df0e0, stack (server, record)

uint32_t network_game_message_handle_settings_relay_role2(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 2 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 7) != 0) {
        return network_game_settings_broadcast_send((uint32_t)server, body);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e28d0), from tools/pack.py 0x4e28d0:

undefined4 FUN_004e28d0(void)

{
  char cVar1;
  undefined4 uVar2;
  int in_EDX;
  int unaff_ESI;
  undefined1 local_28 [4];
  undefined1 local_24 [4];
  undefined1 local_20 [32];

  if (*(short *)(unaff_ESI + 4) == 2) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_20,in_EDX + 2,local_24,
                       local_28,7);
    if (cVar1 != '\0') {
      uVar2 = FUN_004df0e0();
      return uVar2;
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
