// network_channel_service_light  (Ghidra: FUN_004dd240; named per this rewrite)
// address 0x4dd240, size 225 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "A lighter-weight per-tick channel service
// routine, updating timing/status flags and dispatching to receive/transmit without the
// retransmit-timeout scan performed by the full channel service routine." Identical to
// network_channel_service.c (0x4dd110) except it omits the
// network_channel_scan_retransmit_timeouts call and the two network_channel_stream_flush flush calls; see that
// file for the field/offset evidence, which is identical here.
// register convention: timeout_ms in EAX (in_EAX), channel in ESI (unaff_ESI). blam-cc:
// EAX -> timeout_ms, ESI -> channel

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
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc


// FIXED in the review pass: like 0x4dd110, this function has a third, stack-passed
// argument Ghidra dropped -- 0x4dd2ef is `mov edx,[esp+0x18]`, forwarded to
// network_channel_listen_service.
// blam-cc: EAX -> timeout_ms, ESI -> channel, stack -> out_new_child
char network_channel_service_light(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child)
{
    large_integer counter;
    int32_t now_ms;
    uint32_t flags;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

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
    if (channel->flags & k_network_channel_listening) {
        return network_channel_listen_service(channel, out_new_child);
    }
    if (channel->flags & (k_network_channel_client | k_network_channel_transmit_pending)) {
        return network_channel_transmit(channel);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dd240):

char FUN_004dd240(void)

{
  uint uVar1;
  char cVar2;
  int in_EAX;
  uint uVar3;
  int *unaff_ESI;
  undefined8 uVar4;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar4 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar3 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
  uVar1 = unaff_ESI[0x2a3];
  unaff_ESI[0x2a3] = uVar1 & 0xffffffdf;
  if (in_EAX != 0) {
    if (unaff_ESI[1] + 5000U < uVar3) {
      unaff_ESI[0x2a3] = uVar1 & 0xffffffdf | 0x20;
    }
    if (uVar3 <= (uint)(unaff_ESI[1] + in_EAX)) goto LAB_004dd2e5;
    if ((DAT_0071c2c8 == '\0') &&
       ((DAT_00697ed8 * 0x1e < *(int *)(DAT_006f1d6c + 0xc) || (DAT_00719720 == 1)))) {
      return '\0';
    }
  }
  unaff_ESI[1] = uVar3;
LAB_004dd2e5:
  if ((unaff_ESI[0x2a3] & 1U) != 0) {
    cVar2 = FUN_004dd4e0();
    return cVar2;
  }
  if ((unaff_ESI[0x2a3] & 6U) != 0) {
    cVar2 = network_channel_transmit(unaff_ESI);
    return cVar2;
  }
  return '\x01';
}
#endif
