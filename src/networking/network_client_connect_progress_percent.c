// network_client_connect_progress_percent  (Ghidra: FUN_004d8c10; renamed, no prior name)
// address 0x4d8c10, size 52 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Computes the connection-attempt
// progress as a percentage of the configured connect timeout, for display while in the
// 'connecting' state"); the sole caller (network_client_update_dispatch) assigns its result straight through,
// consistent with the mode check against client+0xeda == 1 (the "waiting to join" state per
// src/networking/network_client_state_dispatch.c case 1).
// register convention: the client pointer arrives in ESI (unaff_ESI), the output percentage
// slot in EDI (unaff_EDI). // blam-cc: ESI -> client, EDI -> out_percent
// UNSURE: the function's *return value* is client->state (the connection-mode field, see
// network_client_state_dispatch.c's note on that field's real meaning), not the percentage --
// the percentage is written only through *out_percent when the mode is 1. Preserved exactly;
// this looks intentional (callers care about the mode transition, and separately snapshot the
// percentage through the pointer for UI text) rather than a decompiler artifact.
// UNSURE: client+0xae4 (byte offset 4 into the still-unresolved network_client_globals
// unknown_ae0[0x34] block) is read here as an int32 millisecond timestamp -- almost certainly
// "connect attempt started at", based on the elapsed-time-over-timeout arithmetic -- but is left
// as a raw offset into that block rather than a new named field, since types/networking.h has
// not resolved it and this task must not redefine that header.
// UNSURE: network_connect_timeout_ms (0x006894ac) is not yet declared in types/networking.h;
// named here from its role as the divisor of an elapsed-ms-times-100 percentage, also read by
// network_channel_attempt_connect and network_join_connect_retry_tick in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t time_query_performance_counter_ms(void); // 0x449210, cseries: current time in milliseconds (QPC-based)
extern int32_t network_connect_timeout_ms; // 0x006894ac

int16_t network_client_connect_progress_percent(network_client_globals *client, int16_t *out_percent)
    // blam-cc: ESI -> client, EDI -> out_percent
{
    int32_t now;

    if (out_percent != 0) {
        *out_percent = 0;
        if (client->state == k_network_client_state_connecting) {
            now = time_query_performance_counter_ms();
            *out_percent = (int16_t)(((uint32_t)(now - client->connect_attempt.started_ms) * 100)
                                      / network_connect_timeout_ms);
        }
    }
    return client->state;
}

#if 0
Original Ghidra decompilation (0x4d8c10):

undefined2 FUN_004d8c10(void)

{
  int iVar1;
  int unaff_ESI;
  undefined2 *unaff_EDI;

  if ((unaff_EDI != (undefined2 *)0x0) && (*unaff_EDI = 0, *(short *)(unaff_ESI + 0xeda) == 1)) {
    iVar1 = FUN_00449210();
    *unaff_EDI = (short)((uint)((iVar1 - *(int *)(unaff_ESI + 0xae4)) * 100) / DAT_006894ac);
  }
  return *(undefined2 *)(unaff_ESI + 0xeda);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
