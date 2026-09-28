// network_connection_initiate  (Ghidra: network_connection_initiate, already named)
// address 0x4d8cf0, size 470 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("Begins establishing a connection
// object to a given address/session, substituting the loopback address 127.0.0.1 when the
// target turns out to be the local machine"); connection+0xec4 matches types/networking.h's
// network_client_globals::unknown_ec4 exactly; connection+0xadc matches ::channel; the writes
// to connection+0xac4/0xac6 as two 16-bit halves of a dword whose low half is set to the literal
// 4 (k_network_address_size_ipv4) confirm connection+0xab4 is an s_network_address, which
// upgrades the network_connection_endpoint struct first introduced in
// network_connection_endpoint_set.c; the review pass folded it into types/networking.h.
// register convention: Ghidra's own parameter recovery -- connection in the first cdecl stack
// slot, param_2 (target address, 6 dwords / s_network_address + 1) in the second, param_3
// (9-dword session-info payload) in the third. // blam-cc: cdecl(connection, target, session_info)
// UNSURE: DAT_00718fa4 is not yet declared anywhere in types/networking.h; named here
// network_join_error_code from the -1-sentinel/default-to-7 pattern and from
// out/phase4/networking_functions.md's summary of 0x4d9ce0 ("defaulting the retry-limit field").
// It is read by several other functions in this same task batch (network_join_handshake_tick, network_join_connect_retry_tick,
// network_host_lobby_tick, network_game_client_update, network_host_channel_service_tick) with the same name.
// UNSURE: the boolean this function threads through (bVar11 in Ghidra, `is_local_connection`
// here) is only known by its two visible tests -- whether channel->endpoint exists and whether
// network_channel_attempt_connect(0x96640, 1) (address/mutex registration outside this task's range) succeeds --
// and by the fact the final return is that boolean zero-extended (see below). Named from the
// summary's "target turns out to be the local machine" framing, not independently confirmed.
// UNSURE: `network_address_to_string` and `network_channel_attempt_connect` are called with a Ghidra-elided
// argument and a Ghidra-elided implicit self-pointer respectively (the same register-carryover
// elision documented throughout this codebase). network_address_to_string's argument is
// reconstructed as connection's own endpoint address (the value about to be replaced), the only
// address in scope; network_channel_attempt_connect is shown with two literal integer arguments and needs no
// reconstruction.
// UNSURE: `network_connection_endpoint_set` (network_connection_endpoint_set) is called twice with no visible
// arguments. In the non-loopback branch the manual six-dword copy just above it already
// reproduces exactly what that function would do from `target`, so it is called with
// (target, connection) -- redundant with the manual copy, but both are preserved per the
// no-invented-behaviour rule. In the loopback branch there is no live source value except the
// loopback address just written directly into connection's own memory, so it is called with a
// pointer back into that same memory (a self-copy) rather than with `target` (which would
// overwrite the freshly-built loopback address with the caller's original, empty one). This is
// the best locally-consistent reconstruction, not a confirmed register trace.
// UNSURE: the four separate `inet_addr("127.0.0.1")` calls (rather than one call reused) are
// preserved exactly as Ghidra shows them, in case the compiler genuinely emitted four identical
// calls rather than caching the result.
// UNSURE: the real return value is `CONCAT31((int3)(fn_result >> 8), is_local_connection)` in
// Ghidra, i.e. the low byte of is_local_connection plus three garbage-looking bytes from
// network_connection_endpoint_set's result shifted down. Since that function always returns the
// constant 1, those three bytes are always zero in practice, so this is written as a plain
// zero-extended `(int32_t)is_local_connection` with no behaviour lost.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, owned by the timing/system module
extern int32_t network_game_socket_port; // 0x00698208, port of network_game_socket (0x006f14c4)
extern int16_t network_join_error_code; // 0x00718fa4, the pending join/disconnect error
                                        // string index; -1 means none. WORD-sized everywhere
                                        // (cmp/mov WORD PTR ds:0x718fa4), consumed and reset by
                                        // the main-menu display_error call at 0x4a9ff0.
extern char *network_address_to_string(s_network_address *addr); // 0x440570
extern int16_t network_channel_attempt_connect(int32_t a, int32_t b); // 0x441f60


extern int32_t network_connection_endpoint_set(const uint32_t *source, network_client_globals *connection); // 0x4d8c50


// blam-cc: cdecl(connection, target, session_info)
int32_t network_connection_initiate(network_client_globals *connection, const uint32_t *target,
                                     const uint32_t *session_info)
{
    network_connection_attempt_state *attempt;
    network_connection_endpoint *endpoint;
    large_integer counter;
    int32_t started_ms;
    int32_t is_local_connection; // UNSURE name; see file header
    int16_t registration_result;
    uint32_t loopback_a, loopback_b, loopback_c, loopback_d;
    uint16_t port;
    int32_t i;

    connection->unknown_ec4 = 1;
    attempt = &connection->connect_attempt;
    attempt->unknown_00 = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    started_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    attempt->elapsed_counter = 0;
    attempt->unknown_0c = 0;
    attempt->started_ms = started_ms;
    for (i = 0; i < 9; i = i + 1) {
        attempt->session_info[i] = session_info[i];
    }

    endpoint = &connection->connection;
    is_local_connection = connection->channel->endpoint != 0;
    if (is_local_connection && connection->channel->endpoint != 0) {
        network_address_to_string(&endpoint->address); // UNSURE argument; see file header
        registration_result = network_channel_attempt_connect(0x96640, 1);
        if (registration_result == 0) {
            goto local_machine_check_done;
        }
        is_local_connection = 0;
    } else {
local_machine_check_done:
        if (is_local_connection) {
            connection->state = 1; // UNSURE: live connection-mode value, not padding
            goto rebuild_endpoint;
        }
    }
    if (network_join_error_code == -1) {
        network_join_error_code = 7;
    }
rebuild_endpoint:
    endpoint->address.ipv4 = 0;
    endpoint->address.ipv6_1 = 0;
    endpoint->address.ipv6_2 = 0;
    endpoint->address.ipv6_3 = 0;
    *(uint32_t *)&endpoint->address.size = 0; // clears size and port together (one dword)
    endpoint->unknown_14 = 0;
    endpoint->last_send_ms = 0;
    endpoint->message_count = 0;
    endpoint->retry_count = 0;
    endpoint->unknown_20 = 0;
    endpoint->ready = 0;
    endpoint->unknown_23 = 0;
    endpoint->control_block = 0;

    if ((int16_t)target[4] == 0) {
        // Target address is empty: substitute the loopback address 127.0.0.1.
        loopback_a = inet_addr("127.0.0.1");
        loopback_b = inet_addr("127.0.0.1");
        loopback_c = inet_addr("127.0.0.1");
        loopback_d = inet_addr("127.0.0.1");
        port = (uint16_t)network_game_socket_port;
        endpoint->address.ipv4 = (loopback_a & 0xff0000 | loopback_b >> 0x10) >> 8 |
                                 (loopback_c << 0x10 | loopback_d & 0xff00) << 8;
        endpoint->address.size = k_network_address_size_ipv4;
        endpoint->address.port = port;
        network_connection_endpoint_set((const uint32_t *)endpoint, connection); // UNSURE source
        return is_local_connection;
    }

    endpoint->address.ipv4 = target[0];
    endpoint->address.ipv6_1 = target[1];
    endpoint->address.ipv6_2 = target[2];
    endpoint->address.ipv6_3 = target[3];
    *(uint32_t *)&endpoint->address.size = target[4]; // size (low16) + port (high16) together
    endpoint->unknown_14 = target[5];
    network_connection_endpoint_set(target, connection);
    return is_local_connection;
}

#if 0
Original Ghidra decompilation (0x4d8cf0):

int __cdecl network_connection_initiate(int connection,uint *param_2,uint *param_3)

{
  undefined2 uVar1;
  int iVar2;
  short sVar3;
  undefined4 uVar4;
  ulong uVar5;
  ulong uVar6;
  ulong uVar7;
  ulong uVar8;
  int iVar9;
  uint *puVar10;
  bool bVar11;
  undefined8 uVar12;
  LARGE_INTEGER local_8;

  iVar2 = connection;
  *(undefined4 *)(connection + 0xec4) = 1;
  *(undefined4 *)(connection + 0xae0) = 0;
  QueryPerformanceCounter(&local_8);
  uVar12 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar4 = __alldiv(uVar12,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(connection + 0xae8) = 0;
  *(undefined1 *)(connection + 0xaec) = 0;
  *(undefined4 *)(connection + 0xae4) = uVar4;
  puVar10 = (uint *)(connection + 0xaee);
  for (iVar9 = 9; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar10 = *param_3;
    param_3 = param_3 + 1;
    puVar10 = puVar10 + 1;
  }
  bVar11 = **(int **)(connection + 0xadc) != 0;
  if ((bVar11) && (**(int **)(connection + 0xadc) != 0)) {
    network_address_to_string();
    sVar3 = FUN_00441f60(0x96640,1);
    if (sVar3 == 0) goto LAB_004d8e78;
    connection._0_1_ = false;
    bVar11 = connection._0_1_;
  }
  else {
LAB_004d8e78:
    if (bVar11) {
      *(undefined2 *)(connection + 0xeda) = 1;
      connection._0_1_ = bVar11;
      goto LAB_004d8dc6;
    }
  }
  connection._0_1_ = bVar11;
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 7;
  }
LAB_004d8dc6:
  puVar10 = (uint *)(iVar2 + 0xab4);
  *puVar10 = 0;
  *(undefined4 *)(iVar2 + 0xab8) = 0;
  *(undefined4 *)(iVar2 + 0xabc) = 0;
  *(undefined4 *)(iVar2 + 0xac0) = 0;
  *(undefined4 *)(iVar2 + 0xac4) = 0;
  *(undefined4 *)(iVar2 + 0xac8) = 0;
  *(undefined4 *)(iVar2 + 0xacc) = 0;
  *(undefined4 *)(iVar2 + 0xad0) = 0;
  *(undefined4 *)(iVar2 + 0xad4) = 0;
  *(undefined4 *)(iVar2 + 0xad8) = 0;
  if ((short)param_2[4] == 0) {
    uVar5 = inet_addr("127.0.0.1");
    uVar6 = inet_addr("127.0.0.1");
    uVar7 = inet_addr("127.0.0.1");
    uVar8 = inet_addr("127.0.0.1");
    uVar1 = (undefined2)DAT_00698208;
    *puVar10 = (uVar5 & 0xff0000 | uVar6 >> 0x10) >> 8 | (uVar7 << 0x10 | uVar8 & 0xff00) << 8;
    *(undefined2 *)(iVar2 + 0xac4) = 4;
    *(undefined2 *)(iVar2 + 0xac6) = uVar1;
    uVar4 = FUN_004d8c50();
    return CONCAT31((int3)((uint)uVar4 >> 8),connection._0_1_);
  }
  *puVar10 = *param_2;
  *(uint *)(iVar2 + 0xab8) = param_2[1];
  *(uint *)(iVar2 + 0xabc) = param_2[2];
  *(uint *)(iVar2 + 0xac0) = param_2[3];
  *(uint *)(iVar2 + 0xac4) = param_2[4];
  *(uint *)(iVar2 + 0xac8) = param_2[5];
  uVar4 = FUN_004d8c50();
  return CONCAT31((int3)((uint)uVar4 >> 8),connection._0_1_);
}
#endif
