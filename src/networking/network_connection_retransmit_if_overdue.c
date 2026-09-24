// network_connection_retransmit_if_overdue  (Ghidra: FUN_004d93b0; renamed, no prior name)
// address 0x4d93b0, size 80 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Checks whether a connection's last
// outgoing data has gone unacknowledged past its deadline and, if so, resends it and bumps the
// retry counter"). client+0xad6/+0xab4/+0xad8 match network_client_globals::connection
// (a network_connection_endpoint in types/networking.h since the review pass)
// (ready flag, address.ipv4, control_block); this function additionally shows +0xad2 as a live
// int16 retry counter (inside what those files call the raw `unknown_1c` dword, at its upper
// half) and +0xad4 as a live int16 (their `unknown_20[2]`, matching that field's size exactly).
// register convention: sender address pointer in ECX (in_ECX), client in ESI (unaff_ESI),
// deadline in EDI (unaff_EDI). // blam-cc: ECX -> sender_address, ESI -> client, EDI -> deadline_ms
// UNSURE: none of the three callees are in this task's address range (0x449210 is cseries'
// current-time helper, already declared this way in network_client_connect_progress_percent.c;
// 0x4ed310/0x4ed350 are message-delta code); their signatures are reconstructed from this call
// site alone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t time_query_performance_counter_ms(void); // 0x449210, cseries: current time in milliseconds (QPC-based)
extern void message_delta_sample_record_and_append(void *control_block); // 0x4ed310, not in this batch
extern int16_t message_delta_sample_ring_buffer_average(void); // 0x4ed350, not in this batch


// blam-cc: ECX -> sender_address, ESI -> client, EDI -> deadline_ms
void network_connection_retransmit_if_overdue(const uint32_t *sender_address,
    network_client_globals *client, uint32_t deadline_ms)
{
    network_connection_endpoint *endpoint;
    uint32_t now;
    int16_t doubled;

    endpoint = &client->connection;
    if (endpoint->ready != 0 && endpoint->address.ipv4 == *sender_address) {
        now = time_query_performance_counter_ms();
        if (deadline_ms <= now) {
            endpoint->retry_count = endpoint->retry_count + 1;
            message_delta_sample_record_and_append(endpoint->control_block);
            doubled = message_delta_sample_ring_buffer_average();
            endpoint->unknown_20 = doubled << 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d93b0):

void FUN_004d93b0(void)

{
  short sVar1;
  uint uVar2;
  int *in_ECX;
  int unaff_ESI;
  uint unaff_EDI;

  if ((*(char *)(unaff_ESI + 0xad6) != '\0') && (*(int *)(unaff_ESI + 0xab4) == *in_ECX)) {
    uVar2 = FUN_00449210();
    if (unaff_EDI <= uVar2) {
      *(short *)(unaff_ESI + 0xad2) = *(short *)(unaff_ESI + 0xad2) + 1;
      FUN_004ed310(*(undefined4 *)(unaff_ESI + 0xad8));
      sVar1 = FUN_004ed350();
      *(short *)(unaff_ESI + 0xad4) = sVar1 << 1;
    }
  }
  return;
}
#endif
