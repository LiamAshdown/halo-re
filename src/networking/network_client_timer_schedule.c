// network_client_timer_schedule  (Ghidra: FUN_004d9ed0; renamed, no prior name)
// address 0x4d9ed0, size 122 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Schedules a delayed network event/timer
// to fire after a given number of milliseconds"). The five fields written (byte, dword, byte,
// dword, dword at client+0xee4/0xee8/0xeec/0xef0/0xef4) are the first five elements of
// network_client_globals::timer, a network_client_timer_record that the review pass folded
// into types/networking.h (it occupies the first five dwords of the zeroed run at +0xee4).
// register convention: delay in milliseconds in ECX (param_1, per Ghidra's own recovery),
// context/callback value in EDX (param_2), client in ESI (unaff_ESI).
// // blam-cc: ECX -> delay_ms, EDX -> context, ESI -> client
// UNSURE: `network_channel_remote_address_or_default` is `network_channel_remote_address_or_default` (0x4dd390, same address, rewritten
// outside this task's range); called here with no visible argument, reconstructed as taking
// `client`, matching that function's own single-parameter signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter);
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module


// blam-cc: ECX -> delay_ms, EDX -> context, ESI -> client
void network_client_timer_schedule(int32_t delay_ms, int32_t context, network_client_globals *client)
{
    network_client_timer_record *timer;
    large_integer counter;
    int32_t now_ms;

    QueryPerformanceCounter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    timer = &client->timer;
    timer->deadline_ms = now_ms + delay_ms;
    timer->context = context;
    timer->active = 1;
    timer->triggered = 0;
    timer->retrigger_ms = 0;

    // FIXED in the review pass: 0x4d9f1d/0x4d9f23 load EAX = client->channel and
    // ECX = client+0xef8, so this call resolves the server's address into the client's own
    // server_address record; it is not an argument-less call.
    network_channel_remote_address_or_default(client->channel, &client->server_address);
}

#if 0
Original Ghidra decompilation (0x4d9ed0):

void FUN_004d9ed0(int param_1,undefined4 param_2)

{
  int iVar1;
  int unaff_ESI;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *(int *)(unaff_ESI + 0xee8) = iVar1 + param_1;
  *(undefined4 *)(unaff_ESI + 0xef0) = param_2;
  *(undefined1 *)(unaff_ESI + 0xee4) = 1;
  *(undefined1 *)(unaff_ESI + 0xeec) = 0;
  *(undefined4 *)(unaff_ESI + 0xef4) = 0;
  FUN_004dd390();
  return;
}
#endif
