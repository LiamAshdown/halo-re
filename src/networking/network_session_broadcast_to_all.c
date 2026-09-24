// network_session_broadcast_to_all  (Ghidra: FUN_004e19c0, unnamed)
// address 0x4e19c0, size 183 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Broadcasts a prepared packet to every
// established machine in the session's machine table." Channel dead-bit and ::connected tests
// match every other send path in this batch exactly.
// register convention: ECX = server (network_server_globals *), stack = param_1, data,
// param_3, param_4, force (char), param_6.
// blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6
// UNSURE: the per-machine gate tests flags bit 0x02 (k_network_machine_pending per
// types/networking.h), not bit 0x01 (k_network_machine_established) as the "every established
// machine" summary would suggest; preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_channel_queue_message(void *data, void *out_status, int32_t one, uint32_t param_3,
    uint32_t param_4, uint32_t param_6); // 0x4dce40, other module (UNSURE)

// Sends `data` through network_channel_queue_message to every machine slot whose flags bit 0x02 is set and
// whose channel is alive (not k_network_channel_dead) and either connected or `force` is set.
// Returns false if any qualifying send fails.
char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, char force, int32_t param_6)
{
    char ok;
    int32_t i;

    ok = 1;
    for (i = 0; i < 16; i = i + 1) {
        network_machine *machine;
        network_channel *channel;
        char connected;

        machine = &server->machines[i];
        channel = machine->channel;
        connected = (channel != 0) ? channel->connected : 0;

        if ((machine->flags & 0x02) != 0 &&
            (connected != 1 || force != 0) &&
            channel != 0 &&
            (channel->flags & 0x10) == 0) { // not k_network_channel_dead
            uint8_t status;
            char sent;

            status = (uint8_t)(param_1 != 0);
            sent = network_channel_queue_message(data, &status, 1, param_3, param_4, param_6);
            if (sent == 0) {
                ok = 0;
            }
        }
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x4e19c0):

undefined1
FUN_004e19c0(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,char param_5,
            undefined4 param_6)

{
  char cVar1;
  int iVar2;
  int in_ECX;
  int *piVar3;
  undefined1 local_6;
  undefined1 local_5;
  int local_4;

  local_6 = 1;
  piVar3 = (int *)(in_ECX + 0x3b8);
  local_4 = 0x10;
  do {
    if (piVar3 == (int *)0x0) {
      iVar2 = 0;
    }
    else {
      iVar2 = *piVar3;
    }
    cVar1 = '\0';
    if (iVar2 != 0) {
      cVar1 = *(char *)(iVar2 + 0xa98);
    }
    if ((((*(byte *)((int)piVar3 + 0xe) >> 1 & 1) != 0) &&
        (((cVar1 != '\x01' || (param_5 != '\0')) && (*piVar3 != 0)))) &&
       ((~(byte)(*(uint *)(*piVar3 + 0xa8c) >> 4) & 1) != 0)) {
      local_5 = param_1 != 0;
      cVar1 = FUN_004dce40(param_2,&local_5,1,param_3,param_4,param_6);
      if (cVar1 == '\0') {
        local_6 = 0;
      }
    }
    piVar3 = piVar3 + 0x18;
    local_4 = local_4 + -1;
  } while (local_4 != 0);
  return local_6;
}
#endif
