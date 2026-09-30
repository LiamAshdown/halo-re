// network_server_notify_or_resend_challenge  (Ghidra: FUN_004e0af0)
// address 0x4e0af0, size 150 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x4e0af0..0x4e0b85: CX reason, EDI machine, stack server. For a machine whose
//   channel is connected (+0xa98): the chat close deadline (0x00718fa4, when unset) becomes reason + 0x2b, the host
//   hand-off flag is set, chat closes; returns 1. Otherwise a type 6 packet carrying the reason goes to the machine
//   (reliable, 3) and the machine timer restarts for 1000 ms; returns whether the send worked (0 when no packet was
//   built). The server argument was missing.
// blam-cc: CX -> reason, EDI -> machine, stack -> server

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <string.h>
#include <wchar.h>

extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, blam-cc: EAX type, EDX payload
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t status_bit, void *data,
    uint32_t bits, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, blam-cc: EAX machine_id, ESI server
extern int16_t network_join_error_code; // 0x00718fa4
extern uint8_t network_host_handoff_requested; // 0x0071c2de
extern void chat_close(void); // 0x4aa900


uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine, network_server_globals *server)
{
    int32_t payload = reason;
    uint16_t *packet;
    uint8_t ok = 1;

    if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
        if (network_join_error_code == -1) {
            network_join_error_code = (int16_t)(reason + 0x2b);
        }
        network_host_handoff_requested = 1;
        chat_close();
        return 1;
    }
    packet = network_prepare_challenge_packet(6, &payload);
    if (packet == 0 || network_session_send_to_machine(machine->machine_id, server, 0, packet, (uint32_t)(*packet >> 4) << 3,
                                                        1, 1, 0, 3) == 0) {
        ok = 0;
    }
    network_machine_timer_start(machine, 1000);
    return ok;
}

#if 0
Original Ghidra decompilation (0x4e0af0):

undefined1 FUN_004e0af0(void)

{
  char cVar1;
  ushort *puVar2;
  short in_CX;
  undefined1 uVar3;
  int *unaff_EDI;

  uVar3 = 1;
  if (((unaff_EDI != (int *)0x0) && (*unaff_EDI != 0)) && (*(char *)(*unaff_EDI + 0xa98) != '\0')) {
    if (DAT_00718fa4 == -1) {
      DAT_00718fa4 = in_CX + 0x2b;
    }
    DAT_0071c2de = 1;
    chat_close();
    return 1;
  }
  puVar2 = (ushort *)network_prepare_challenge_packet();
  if (puVar2 != (ushort *)0x0) {
    cVar1 = network_session_send_to_machine(0,puVar2,(uint)(*puVar2 >> 4) << 3,1,1,0,3);
    if (cVar1 != '\0') goto LAB_004e0b4d;
  }
  uVar3 = 0;
LAB_004e0b4d:
  FUN_004df090(1000);
  return uVar3;
}
#endif
