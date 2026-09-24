// network_session_send_to_machine  (Ghidra: network_session_send_to_machine, already named)
// address 0x4e1930, size 137 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Sends a prepared packet to the single machine
// identified by id, skipping machines that are in the process of disconnecting." param_1+0x3c4
// matches network_server_globals::machines[0].machine_id; the found machine's channel is
// checked against ::connected (+0xa98) exactly as in every other send path in this batch.
// register convention: EAX = machine_id (int32_t), ESI = server (network_server_globals *),
// stack = param_1 (unused), data, param_3 (unused), reliable, unknown_a, force (char),
// priority.
// blam-cc: EAX -> machine_id, ESI -> server, stack -> (unused, data, unused, reliable,
// unknown_a, force, priority)
// UNSURE: param_1 and param_3 are genuinely unused stack parameters in the original (only
// param_2, param_4, param_5, param_7 are forwarded to network_channel_queue_message, plus a literal 1 in
// param_3's slot). This batch's other files that call this function pass their arguments
// positionally as if `machine_id` were an ordinary leading stack argument (matching how every
// call site's constant literal doubles as both the discarded param_1 and, presumably, EAX);
// that mismatch with this function's own true ABI is a known inconsistency in this batch, not
// resolved here.
// UNSURE: network_channel_queue_message's real parameter meaning is inferred purely from this call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint32_t network_channel_queue_message(void *data, void *out_status, int32_t one, uint32_t param_4,
    uint32_t param_5, uint32_t param_7); // 0x4dce40, other module (UNSURE)

// Finds the machine slot whose machine_id matches `machine_id` and, if it has a live channel
// that is either connected or `force` is set, forwards the send through network_channel_queue_message.
uint32_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server,
    uint32_t param_1, void *data, uint32_t param_3, uint32_t reliable, uint32_t unknown_a,
    char force, uint32_t priority)
{
    int32_t i;
    uint32_t result;

    (void)param_1;
    (void)param_3;
    result = (uint32_t)machine_id & 0xffffff00;

    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].machine_id == machine_id) {
            network_channel *channel;
            uint8_t status;

            channel = server->machines[i].channel;
            if (channel != 0 && (channel->connected == 1 || force != 0)) {
                result = network_channel_queue_message(data, &status, 1, reliable, unknown_a, priority);
            }
            return result;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4e1930):

uint network_session_send_to_machine
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5,char param_6,undefined4 param_7)

{
  int *piVar1;
  uint in_EAX;
  uint uVar2;
  int iVar3;
  short *psVar4;
  int unaff_ESI;
  undefined1 local_1;

  uVar2 = in_EAX & 0xffffff00;
  iVar3 = 0;
  psVar4 = (short *)(unaff_ESI + 0x3c4);
  do {
    if ((int)*psVar4 == in_EAX) {
      piVar1 = (int *)(iVar3 * 0x60 + 0x3b8 + unaff_ESI);
      if ((((piVar1 != (int *)0x0) && (iVar3 = *piVar1, iVar3 != 0)) &&
          ((*(char *)(iVar3 + 0xa98) != '\x01' || (param_6 != '\0')))) && (iVar3 != 0)) {
        uVar2 = FUN_004dce40(param_2,&local_1,1,param_4,param_5,param_7);
      }
      return uVar2;
    }
    iVar3 = iVar3 + 1;
    psVar4 = psVar4 + 0x30;
  } while (iVar3 < 0x10);
  return uVar2;
}
#endif
