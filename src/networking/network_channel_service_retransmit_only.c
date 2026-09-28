// network_channel_service_retransmit_only  (Ghidra: FUN_004dd330; named per this rewrite)
// address 0x4dd330, size 89 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Runs the retransmit-timeout scan and
// pending-flush bookkeeping for a channel without performing an actual receive or transmit
// pass." Same in.empty/out.empty/send_budget fields as network_channel_service.c. The Ghidra
// decompile's CONCAT31(uVar1,1) return packs garbage high bytes from extraout_EAX/_var with a
// fixed low byte of 1; this rewrite just returns 1.
// register convention: channel in EDI (unaff_EDI). blam-cc: EDI -> channel

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern void network_channel_scan_retransmit_timeouts(network_channel *channel); // 0x4dd9d0, this batch
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this batch

// blam-cc: EDI -> channel
int32_t network_channel_service_retransmit_only(network_channel *channel)
{
    large_integer counter; // UNSURE: result unused beyond the QPC call's own side effects

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    network_channel_scan_retransmit_timeouts(channel);
    if (channel->outgoing.empty == 0) {
        network_channel_stream_flush(&channel->outgoing, channel, 1); // FIXED: 0x4dd34f pushes 1
    }
    if (channel->retransmit.empty == 0) {
        network_channel_stream_flush(&channel->retransmit, channel, 0);
    }
    channel->send_budget = 0xe0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dd330):

undefined4 FUN_004dd330(void)

{
  undefined4 extraout_EAX;
  undefined3 uVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int unaff_EDI;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  network_channel_scan_retransmit_timeouts(unaff_EDI);
  uVar1 = (undefined3)((uint)extraout_EAX >> 8);
  if (*(char *)(unaff_EDI + 0x2c) == '\0') {
    FUN_004ddb60();
    uVar1 = extraout_var;
  }
  if (*(char *)(unaff_EDI + 0x560) == '\0') {
    FUN_004ddb60();
    uVar1 = extraout_var_00;
  }
  *(undefined4 *)(unaff_EDI + 0xa80) = 0xe0;
  return CONCAT31(uVar1,1);
}
#endif
