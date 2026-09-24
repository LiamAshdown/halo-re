// network_server_notify_or_resend_challenge  (Ghidra: FUN_004e0af0, unnamed)
// address 0x4e0af0, size 150 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Either notifies that a machine's connection
// is already established or (re)sends a prepared challenge/info packet to it and arms a
// 1000ms retry." `*machine + 0xa98` matches network_channel::connected exactly (machine's
// first field is its channel pointer).
// register convention: CX = value (int16_t), EDI = machine (network_machine *, may be NULL).
// blam-cc: CX -> value, EDI -> machine
// UNSURE: DAT_00718fa4's meaning (a cached "value + 0x2b", latched only once) is not
// established elsewhere in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_chat_close_deadline; // 0x00718fa4 (UNSURE name)
extern uint8_t network_host_handoff_requested; // 0x0071c2de
extern void chat_close(void); // other module, already named
extern void *network_prepare_challenge_packet(void); // 0x4deaf0, this module
extern char network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority); // 0x4e1930, this batch
extern void network_machine_timer_start(int32_t duration_ms); // 0x4df090, this module family (UNSURE name)

// If `machine` already has an established channel, requests a host handoff and closes chat,
// reporting success immediately. Otherwise (re)sends the prepared challenge/info packet to
// machine 0 and arms a 1000ms retry via network_machine_timer_start regardless of outcome.
uint8_t network_server_notify_or_resend_challenge(int16_t value, network_machine *machine)
{
    uint16_t *packet;
    uint8_t ok;

    ok = 1;
    if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
        if (network_chat_close_deadline == -1) {
            network_chat_close_deadline = value + 0x2b;
        }
        network_host_handoff_requested = 1;
        chat_close();
        return 1;
    }
    packet = (uint16_t *)network_prepare_challenge_packet();
    if (packet != 0) {
        char sent;

        sent = network_session_send_to_machine(0, packet, (int32_t)(*packet >> 4) << 3, 1, 1, 0, 3);
        if (sent != 0) {
            network_machine_timer_start(1000);
            return ok;
        }
    }
    ok = 0;
    network_machine_timer_start(1000);
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
