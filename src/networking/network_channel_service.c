// network_channel_service  (Ghidra: FUN_004dd110; named per this rewrite)
// address 0x4dd110, size 292 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Per-tick service routine for a network
// channel: updates timing/status flags, scans for and retransmits timed-out reliable messages,
// and dispatches to the receive or transmit path as needed." channel[1]=+0x004
// (last_activity_ms), channel[0x2a3]=+0xa8c (flags), channel[0xb]=+0x02c (in.empty),
// channel[0x158]=+0x560 (out.empty), channel[0x2a0]=+0xa80 (send_budget) all match
// types/networking.h's network_channel exactly.
// UNSURE: `timeout_ms` (in_EAX) -- the caller-supplied idle-timeout bound -- is an elided
// register argument with no further evidence of its source in this function's own body.
// register convention: timeout_ms in EAX (in_EAX), channel in EDI (unaff_EDI), plus ONE real
// cdecl stack argument that Ghidra dropped entirely: 0x4dd202 is `mov edx,[esp+0x18]` (the
// first stack parameter, since the frame is sub esp,8 + three pushes) and it is forwarded
// straight to network_channel_listen_service as its out-new-child pointer. Both in-module
// callers (0x4daef0, 0x4db100) push 0 for it.
// blam-cc: EAX -> timeout_ms, EDI -> channel, stack -> out_new_child

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern uint8_t network_channel_service_backoff_bypass; // 0x0071c2c8
extern int32_t unknown_00697ed8; // 0x00697ed8, UNSURE identity
extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t network_game_mode; // 0x00719720


// blam-cc: EAX -> timeout_ms, EDI -> channel
char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child)
{
    int32_t now_ms;
    uint32_t flags;

    now_ms = (int32_t)0; // placeholder assigned below via the QPC-derived millisecond helper pattern
    {
        // QueryPerformanceCounter-derived milliseconds, matching the pattern used throughout
        // this module (see network_channel_record_timestamp.c for the canonical form).
        extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
        large_integer counter;
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    }

    flags = channel->flags;
    channel->flags = flags & 0xffffffdf;
    if (timeout_ms != 0) {
        if ((uint32_t)(channel->last_activity_ms + 5000) < (uint32_t)now_ms) {
            channel->flags = (flags & 0xffffffdf) | k_network_channel_timed_out;
        }
        if ((uint32_t)now_ms <= (uint32_t)(channel->last_activity_ms + timeout_ms)) {
            goto after_timestamp;
        }
        if (network_channel_service_backoff_bypass == 0 &&
            (unknown_00697ed8 * 0x1e < game_time->game_time || network_game_mode == 1)) {
            return 0;
        }
    }
    channel->last_activity_ms = now_ms;
after_timestamp:
    network_channel_scan_retransmit_timeouts(channel);
    // FIXED in the review pass: 0x4dd1c5 pushes 1 for the in-stream flush and 0x4dd1dd
    // pushes 0 for the out-stream flush; the first draft passed 0 for both.
    if (channel->outgoing.empty == 0) {
        network_channel_stream_flush(&channel->outgoing, channel, 1);
    }
    if (channel->retransmit.empty == 0) {
        network_channel_stream_flush(&channel->retransmit, channel, 0);
    }
    channel->send_budget = 0xe0;
    if (channel->flags & k_network_channel_listening) {
        return network_channel_listen_service(channel, out_new_child);
    }
    if (channel->flags & (k_network_channel_client | k_network_channel_transmit_pending)) {
        return network_channel_transmit(channel);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dd110):

char FUN_004dd110(void)

{
  uint uVar1;
  char cVar2;
  int in_EAX;
  uint uVar3;
  int *unaff_EDI;
  undefined8 uVar4;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar4 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar3 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
  uVar1 = unaff_EDI[0x2a3];
  unaff_EDI[0x2a3] = uVar1 & 0xffffffdf;
  if (in_EAX != 0) {
    if (unaff_EDI[1] + 5000U < uVar3) {
      unaff_EDI[0x2a3] = uVar1 & 0xffffffdf | 0x20;
    }
    if (uVar3 <= (uint)(unaff_EDI[1] + in_EAX)) goto LAB_004dd1b5;
    if ((DAT_0071c2c8 == '\0') &&
       ((DAT_00697ed8 * 0x1e < *(int *)(DAT_006f1d6c + 0xc) || (DAT_00719720 == 1)))) {
      return '\0';
    }
  }
  unaff_EDI[1] = uVar3;
LAB_004dd1b5:
  network_channel_scan_retransmit_timeouts((int)unaff_EDI);
  if ((char)unaff_EDI[0xb] == '\0') {
    FUN_004ddb60();
  }
  if ((char)unaff_EDI[0x158] == '\0') {
    FUN_004ddb60();
  }
  unaff_EDI[0x2a0] = 0xe0;
  if ((unaff_EDI[0x2a3] & 1U) != 0) {
    cVar2 = FUN_004dd4e0();
    return cVar2;
  }
  if ((unaff_EDI[0x2a3] & 6U) != 0) {
    cVar2 = network_channel_transmit(unaff_EDI);
    return cVar2;
  }
  return '\x01';
}
#endif
