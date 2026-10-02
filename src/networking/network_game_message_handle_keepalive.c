// network_game_message_handle_keepalive  (Ghidra: FUN_004e2110; named per this rewrite)
// address 0x4e2110, size 177 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Handles message type 1 by building and sending
// a timestamped acknowledgement/challenge packet back to the sender's channel." Called from
// network_game_process_incoming_message's own `case 1: ... FUN_004e2110(&local_2c);` with the
// just-decoded record. `*channel + 0xa8c` matches network_channel::flags exactly.
// register convention: EAX = channel (network_channel **, may be NULL), stack = record (the
// just-decoded message body). Confirmed by disassembly (objdump -d -M intel, bin/halo.exe),
// which also recovers the two under-attributed calls Ghidra's own decompile dropped:
//   4e2170: mov eax,0x3            ; network_prepare_challenge_packet(message_type=3,
//   4e216c: lea edx,[esp+0x14]     ;   payload=&{echoed_value, timestamp_ms})
//   4e2175: call 0x4deaf0
// register convention: EAX -> channel, stack -> record.
//   // blam-cc: EAX -> channel, stack -> record
// UNSURE: network_channel_reliable_pool_store is called here with only 4 of its fuller, separately-reconstructed
// 6-argument signature (see network_channel_queue_message.c); called here with a matching
// 4-argument local prototype instead of forcing the mismatch, per the same reasoning documented
// in network_send_join_request_packet.c for data_packet_group_encode_packet.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0
extern void network_channel_reliable_pool_store(network_channel *channel, uint16_t *packet, uint8_t *reliable_flag,
    int32_t priority); // 0x4dcdb0, this module, fuller signature in network_channel_queue_message.c

// blam-cc: EAX -> channel, stack -> record
// Builds a timestamped acknowledgement (echoing `record`'s first dword plus the current
// millisecond clock) as message type 3 and, when `channel` is not itself in listening mode,
// queues it for reliable send.
uint32_t network_game_message_handle_keepalive(network_channel **channel, int32_t *record)
{
    network_channel *chan;
    large_integer counter;
    struct {
        int32_t echoed_value;
        int32_t timestamp_ms;
    } payload;
    uint16_t *packet;

    if (channel == 0) {
        return 0;
    }
    chan = *channel;
    if (chan == 0) {
        return 0;
    }
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    payload.echoed_value = *record;
    payload.timestamp_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    packet = network_prepare_challenge_packet(3, &payload);
    if (packet != 0) {
        uint8_t reliable_flag = 0;
        if ((chan->flags & 1) == 0) {
            network_channel_reliable_pool_store(chan, packet, &reliable_flag, 1);
        }
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e2110), from tools/pack.py 0x4e2110:

undefined4 FUN_004e2110(undefined4 *param_1)

{
  int iVar1;
  int *in_EAX;
  int iVar2;
  undefined8 uVar3;
  undefined1 local_11;
  LARGE_INTEGER local_10;
  undefined4 local_8;
  undefined4 local_4;

  if (in_EAX == (int *)0x0) {
    return 0;
  }
  iVar1 = *in_EAX;
  if (iVar1 != 0) {
    QueryPerformanceCounter(&local_10);
    local_8 = *param_1;
    uVar3 = __allmul(local_10.s.LowPart,local_10.s.HighPart,1000,0);
    local_4 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
    iVar2 = network_prepare_challenge_packet();
    if (iVar2 != 0) {
      local_11 = 0;
      if ((*(byte *)(iVar1 + 0xa8c) & 1) == 0) {
        FUN_004dcdb0(iVar1,iVar2,&local_11,1);
      }
      return 1;
    }
  }
  return 0;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirming network_prepare_challenge_packet's
real arguments:
  4e216c: lea edx,[esp+0x14]     ; edx = &{echoed_value, timestamp_ms}
  4e2170: mov eax,0x3            ; eax = message_type = 3
  4e2175: call 0x4deaf0
  4e217e: test BYTE PTR [esi+0xa8c],0x1
  4e2189: jne 0x4e21aa           ; flags bit0 set -> skip FUN_004dcdb0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
