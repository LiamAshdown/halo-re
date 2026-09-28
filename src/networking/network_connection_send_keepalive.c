// network_connection_send_keepalive  (Ghidra: FUN_004d9400; renamed, no prior name)
// address 0x4d9400, size 178 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Periodically sends a keepalive packet
// on an idle connection once more than three seconds have elapsed since the last send").
// Confirms and refines network_client_globals::connection, shared with
// network_connection_endpoint_set.c / network_connection_initiate.c /
// network_connection_retransmit_if_overdue.c: what those files call the raw `unknown_18` dword
// (0xacc) is a live millisecond timestamp here, and what network_connection_retransmit_if_overdue.c
// calls `unknown_1c_lo` (0xad0) is a live int16 counter this function increments.
// register convention: the client pointer arrives in ESI (unaff_ESI). // blam-cc: ESI -> client
// UNSURE: `network_prepare_challenge_packet` is called here with no visible argument at all
// (unlike its call site in network_session_info_packet_send.c, which has an 8-dword local built
// immediately beforehand); modeled here as taking no visible source, since none is ever
// constructed in this function's own body.
// UNSURE: `network_channel_reliable_pool_store` (0x4dcdb0, outside this task's range -- types/networking.h names it as
// the reliable-retransmit-pool "store" step) is reconstructed purely from this call site's
// literal argument shapes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter);
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, owned by the timing/system module
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern void network_channel_reliable_pool_store(network_channel *channel, void *message, uint8_t *out_flag,
    int32_t priority); // 0x4dcdb0, not in this batch


// blam-cc: ESI -> client
void network_connection_send_keepalive(network_client_globals *client)
{
    large_integer counter;
    uint32_t now_ms;
    network_connection_endpoint *endpoint;
    int32_t *challenge;
    uint32_t challenge_payload[4]; // UNSURE: the caller-frame scratch EDX points at; its
                                   //         contents are not visible in the decompilation
    uint8_t out_flag;

    QueryPerformanceCounter(&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / performance_frequency);

    endpoint = &client->connection;
    if (endpoint->ready == 1 && 3000 < (int32_t)(now_ms - endpoint->last_send_ms)) {
        // 0x4d945b: eax = 1 (message type); 0x4d9457: edx = &now_ms scratch.
        challenge = (int32_t *)network_prepare_challenge_packet(1, challenge_payload);
        if (challenge != 0) {
            out_flag = 0;
            if ((client->channel->flags & 1) == 0) {
                network_channel_reliable_pool_store(client->channel, challenge, &out_flag, 0);
            }
            endpoint->message_count = endpoint->message_count + 1;
            endpoint->last_send_ms = now_ms;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d9400):

void FUN_004d9400(void)

{
  DWORD DVar1;
  int iVar2;
  int unaff_ESI;
  undefined8 uVar3;
  undefined1 local_9;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  DVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  if ((*(char *)(unaff_ESI + 0xad6) == '\x01') && (3000 < DVar1 - *(int *)(unaff_ESI + 0xacc))) {
    local_8.s.LowPart = DVar1;
    iVar2 = network_prepare_challenge_packet();
    if (iVar2 != 0) {
      local_9 = 0;
      if ((*(byte *)(*(int *)(unaff_ESI + 0xadc) + 0xa8c) & 1) == 0) {
        FUN_004dcdb0(*(int *)(unaff_ESI + 0xadc),iVar2,&local_9,0);
      }
      *(short *)(unaff_ESI + 0xad0) = *(short *)(unaff_ESI + 0xad0) + 1;
      *(DWORD *)(unaff_ESI + 0xacc) = DVar1;
    }
  }
  return;
}
#endif
