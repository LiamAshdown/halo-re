// network_game_message_handle_join_finalize_ack_role2  (Ghidra: FUN_004e2930; named per this
// rewrite)
// address 0x4e2930, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x23, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x24: FUN_004e2930();` ('$' is 0x24, not 0x23) -- the dispatcher's literal call table
// is trusted here instead of the low-confidence (0.3) summary. The cleared bit
// (machine->flags &= ~0x04) is exactly the bit network_game_server_handle_client_join.c sets
// (`machine->flags |= 0x04`) while a join is pending, so this is that join's client-side
// finalize acknowledgement.
// register convention: ECX = context (role, checked against 2), ESI = machine (the flags
// clear), stack = buffer.
//   // blam-cc: ECX -> context, ESI -> machine, stack -> buffer
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2930: ECX server, ESI machine, stack (record, length); state (+4) 2, class 7; then the machine's flag
// byte +0x0e loses bit 4.

#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6

uint32_t network_game_message_handle_join_finalize_ack_role2(network_server_globals *server, network_machine *machine,
    uint8_t *record, int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 2 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 7) != 0) {
        *((uint8_t *)machine + 0xe) &= 0xfb;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2930), from tools/pack.py 0x4e2930:

undefined4 FUN_004e2930(int param_1)

{
  char cVar1;
  int in_ECX;
  int unaff_ESI;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if (*(short *)(in_ECX + 4) == 2) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,param_1 + 2,local_8,
                       local_c,7);
    if (cVar1 != '\0') {
      *(byte *)(unaff_ESI + 0xe) = *(byte *)(unaff_ESI + 0xe) & 0xfb;
      return 1;
    }
  }
  return 1;
}
#endif
