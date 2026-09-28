// network_session_broadcast_to_flagged  (Ghidra: FUN_004e1a80, unnamed)
// address 0x4e1a80, size 193 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Broadcasts a prepared packet only to machines
// whose table entry has an extra qualifying flag bit set, on top of being established."
// Identical structure to network_session_broadcast_to_all.c, with an added flags-bit-0x04
// test.
// register convention: ECX = server (network_server_globals *), stack = status_bit, data,
// immediate, flush_after, force (char), unused.
// blam-cc: EAX -> body_bit_count, ECX -> server, stack -> status_bit, data, immediate, flush_after, force, unused (see FIXED below)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_channel_queue_message(network_channel *channel, uint32_t header_value, uint32_t body_value,
    int32_t header_bit_count, char immediate, char flush_after, int32_t body_bit_count); // 0x4dce40, EDI channel, EBX body bits

// Same qualifying test as network_session_broadcast_to_all, plus flags bit 0x04 (not one of
// the enumerated network_machine_flags).
// FIXED (objdump 0x4e1a8d, 0x4e1aef..0x4e1b13): the original also takes EAX -- the message length in bits, which it
// forwards in EBX to network_channel_queue_message -- and queues on each qualifying machine's channel (EDI) with
// stack (data, &status, 1, immediate, flush_after); the sixth push (unused) is not read by the callee.
// blam-cc: EAX -> body_bit_count, ECX -> server, stack -> status_bit, data, immediate, flush_after, force, unused
char network_session_broadcast_to_flagged(int32_t body_bit_count, network_server_globals *server, int32_t status_bit,
    void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused)
{
    char ok;
    int32_t i;

    ok = 1;
    for (i = 0; i < 16; i = i + 1) {
        network_machine *machine;
        network_channel *channel;
        char connected;
        uint8_t flags;

        machine = &server->machines[i];
        channel = machine->channel;
        connected = (channel != 0) ? channel->connected : 0;
        flags = machine->flags;

        if ((flags & 0x02) != 0 && (flags & 0x04) != 0 &&
            (connected != 1 || force != 0) &&
            channel != 0 &&
            (channel->flags & 0x10) == 0) { // not k_network_channel_dead
            uint8_t status;
            char sent;

            status = (uint8_t)(status_bit != 0);
            sent = network_channel_queue_message(channel, (uint32_t)data, (uint32_t)&status, 1, (char)immediate,
                                                 (char)flush_after, body_bit_count);
            if (sent == 0) {
                ok = 0;
            }
        }
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x4e1a80):

undefined1
FUN_004e1a80(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,char param_5,
            undefined4 param_6)

{
  byte bVar1;
  char cVar2;
  int iVar3;
  int in_ECX;
  int *piVar4;
  undefined1 local_6;
  undefined1 local_5;
  int local_4;

  local_6 = 1;
  piVar4 = (int *)(in_ECX + 0x3b8);
  local_4 = 0x10;
  do {
    if (piVar4 == (int *)0x0) {
      iVar3 = 0;
    }
    else {
      iVar3 = *piVar4;
    }
    cVar2 = '\0';
    if (iVar3 != 0) {
      cVar2 = *(char *)(iVar3 + 0xa98);
    }
    bVar1 = (byte)*(undefined2 *)((int)piVar4 + 0xe);
    if (((((bVar1 >> 1 & 1) != 0) && ((bVar1 >> 2 & 1) != 0)) &&
        ((cVar2 != '\x01' || (param_5 != '\0')))) &&
       ((*piVar4 != 0 && ((~(byte)(*(uint *)(*piVar4 + 0xa8c) >> 4) & 1) != 0)))) {
      local_5 = param_1 != 0;
      cVar2 = FUN_004dce40(param_2,&local_5,1,param_3,param_4,param_6);
      if (cVar2 == '\0') {
        local_6 = 0;
      }
    }
    piVar4 = piVar4 + 0x18;
    local_4 = local_4 + -1;
  } while (local_4 != 0);
  return local_6;
}
#endif
