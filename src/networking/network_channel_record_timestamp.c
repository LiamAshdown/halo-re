// network_channel_record_timestamp  (Ghidra: network_channel_record_timestamp, already named)
// address 0x4dd930, size 66 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md: "Records the current time (in milliseconds)
// into a channel's timestamp field, used elsewhere for timeout comparisons." channel+4 matches
// types/networking.h's network_channel.last_activity_ms exactly.

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

void network_channel_record_timestamp(network_channel *channel)
{
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    channel->last_activity_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
}

#if 0
Original Ghidra decompilation (0x4dd930):

void __cdecl network_channel_record_timestamp(int channel)

{
  undefined4 uVar1;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(channel + 4) = uVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
