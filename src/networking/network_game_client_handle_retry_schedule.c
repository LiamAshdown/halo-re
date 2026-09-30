// network_game_client_handle_retry_schedule  (Ghidra: FUN_004e2870; named per this rewrite)
// address 0x4e2870, size 83 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
// evidence: out/phase4/networking_functions.md's summary claims message type 0x1d, but
// network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
// `case 0x1e: FUN_004e2870();` (case 0x1d instead reaches FUN_004e2810, the settings relay) --
// the dispatcher's literal call table is trusted here instead of the low-confidence (0.3)
// summary. Forwards to network_machine_timer_start; the client-side counterpart of the
// FUN_004e2630 server handler, gated on role == 1 and decode class 5.
// register convention: EAX = context (implicit passthrough), ESI = machine (implicit
// passthrough, needed only for the network_machine_timer_start call), stack = buffer.
//   // blam-cc: EAX -> context, ESI -> machine, stack -> buffer
// UNSURE: `machine` is not read anywhere in this function's own body; inferred purely from
// network_machine_timer_start's own established ESI convention, per the same reasoning as
// network_game_message_handle_retry_schedule.c.
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
// 0x4e2870: EAX server, ESI machine, stack (record, length); state (+4) 1, class 5; then the machine's
// timer starts (network_machine_timer_start: ESI machine, stack 0).

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


uint32_t network_game_client_handle_retry_schedule(network_server_globals *server, network_machine *machine, uint8_t *record,
    int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 1 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) != 0) {
        network_machine_timer_start(machine, 0);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e2870), from tools/pack.py 0x4e2870:

undefined4 FUN_004e2870(int param_1)

{
  char cVar1;
  int in_EAX;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  if (*(short *)(in_EAX + 4) == 1) {
    cVar1 = data_packet_group_decode_packet
                      (&PTR_s_network_game_messages_group_006994f8,local_4,param_1 + 2,local_8,
                       local_c,5);
    if (cVar1 != '\0') {
      FUN_004df090(0);
    }
  }
  return 1;
}
#endif
