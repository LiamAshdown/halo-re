// network_join_handshake_tick  (Ghidra: FUN_004daa20; renamed, no prior name)
// address 0x4daa20, size 348 bytes
// name confidence: 0.4   rewrite confidence: 0.25 (LOW -- unresolved stack-frame overlap; see
// UNSURE notes)
// evidence: out/phase4/networking_functions.md summary ("Per-tick update that drives the
// client's join-a-server handshake: services the transport, and once a specific waiting
// sub-state is reached builds and sends the address/name connect request"). network_server+4
// matches network_server_globals::unknown_004 ("tested against 0 and 2 by host_dispose" per
// types/networking.h); network_server+0x9fc matches ::password exactly.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE (major): by strict byte-offset arithmetic on Ghidra's own declared locals, the
// loopback address this function computes (into what Ghidra calls local_170/local_160/local_15e)
// does NOT overlap the 6-dword buffer (local_134) actually passed as
// network_connection_initiate's `target` argument -- that buffer is separately, fully zeroed and
// never touched again, meaning `target` reaches network_connection_initiate as all-zero,
// triggering that function's own loopback substitution. The computed loopback address therefore
// appears to go unread by anything in this function's own body. The only call between its
// computation and network_connection_initiate is `network_debug_fill_canary_buffer()` (zero visible arguments, not
// in this task's range), so it is the most likely real consumer; not confirmed. Preserved
// exactly (the writes are made, in the same order, whether or not they are the "real" target).
// FIXED in the review pass: `network_channel_service` is network_channel_service and its three arguments
// are all recoverable from the disassembly (see the call site comment below).
// UNSURE: `network_debug_fill_canary_buffer` is still called with no reconstructed argument.
// UNSURE: `local_a` (written 1) falls, by the same byte-offset arithmetic, inside the tail of
// the local_134 array rather than in the 9-dword session_info range read by
// network_connection_initiate; preserved as a write into the same combined scratch buffer used
// for local_134, at its own computed offset, rather than folded into session_info.

#include "tags.h"
#include "memory.h"
#include <wchar.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter);
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern network_server_globals *network_server; // 0x0071c2d4
extern uint32_t network_local_address; // 0x006869b0
extern int32_t network_game_socket_port; // 0x00698208
extern int16_t network_join_error_code; // 0x00718fa4, WORD-sized
extern char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child); // 0x4dd110
extern int32_t network_game_process_incoming_messages(network_client_globals *client); // 0x4db180, this batch
extern void network_connection_send_keepalive(network_client_globals *client); // 0x4d9400
extern void network_debug_fill_canary_buffer(void); // 0x4e0790, UNSURE argument; not in this batch
extern wchar_t *_wcsncpy(wchar_t *dest, const wchar_t *source, int32_t count);
extern int32_t network_connection_initiate(network_client_globals *connection, const uint32_t *target,
    const uint32_t *session_info); // 0x4d8cf0

// blam-cc: EAX -> client
uint32_t network_join_handshake_tick(network_client_globals *client)
{
    // TYPES-GAP: one shared buffer; see file header for the offset reasoning.
    uint8_t frame[400];
    network_channel *channel;
    large_integer counter;
    int32_t now_ms;
    uint32_t result;
    uint32_t *fill;
    int32_t i;
    int32_t loopback_ip;

    QueryPerformanceCounter(&counter);
    channel = client->channel;
    QueryPerformanceCounter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    channel->last_activity_ms = now_ms;

    result = 1;
    if (network_server == 0) {
        // 0x4dab42: edi = client->channel; 0x4dab48: push 0; 0x4dab4a: eax = 0x1388 (5000 ms).
        result = (uint32_t)network_channel_service(client->channel, 5000, 0);
        if ((char)result == 0) {
            goto retry_limit_check;
        }
        result = network_game_process_incoming_messages(client);
        if (result) {
            network_connection_send_keepalive(client);
        }
    } else if (network_server->unknown_004 == 1) {
        *(int32_t *)(frame + 72) = 0;
        *(int32_t *)(frame + 76) = 0;
        *(int32_t *)(frame + 80) = 0;
        *(int32_t *)(frame + 84) = 0;
        fill = (uint32_t *)(frame + 88);
        for (i = 0x48; i != 0; i = i - 1) {
            *fill = 0;
            fill = fill + 1;
        }

        loopback_ip = network_local_address;
        if (network_local_address == 0) {
            loopback_ip = 0x7f000001;
        }
        *(int32_t *)(frame + 12) = loopback_ip;
        *(int16_t *)(frame + 28) = 4;
        *(int16_t *)(frame + 30) = (int16_t)network_game_socket_port;
        *(int16_t *)(frame + 370) = 1;
        _wcsncpy((wchar_t *)(frame + 38), (const wchar_t *)network_server->password, 8);
        *(int16_t *)(frame + 54) = 0;

        network_debug_fill_canary_buffer(); // UNSURE argument; see file header

        result = (uint32_t)network_connection_initiate(client, (const uint32_t *)(frame + 72),
                                                         (const uint32_t *)(frame + 36));
        if ((char)result == 0) {
            goto retry_limit_check;
        }
    }
    return result;

retry_limit_check:
    if (network_join_error_code != -1) {
        return result;
    }
    network_join_error_code = 7;
    return result;
}

#if 0
Original Ghidra decompilation (0x4daa20):

uint FUN_004daa20(void)

{
  int iVar1;
  bool bVar2;
  int in_EAX;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  uint *puVar6;
  bool bVar7;
  undefined8 uVar8;
  LARGE_INTEGER local_17c;
  int local_170;
  undefined2 local_160;
  undefined2 local_15e;
  undefined4 local_158;
  undefined2 local_146;
  uint local_134 [74];
  undefined2 local_a;

  QueryPerformanceCounter(&local_17c);
  iVar5 = *(int *)(in_EAX + 0xadc);
  QueryPerformanceCounter(&local_17c);
  uVar8 = __allmul(local_17c.s.LowPart,local_17c.s.HighPart,1000,0);
  uVar3 = __alldiv(uVar8,DAT_006ac8f8,DAT_006ac8fc);
  iVar1 = DAT_0071c2d4;
  bVar7 = DAT_0071c2d4 == 0;
  *(undefined4 *)(iVar5 + 4) = uVar3;
  bVar2 = true;
  if (bVar7) {
    uVar4 = FUN_004dd110(0);
    if ((char)uVar4 == '\0') {
LAB_004dab24:
      if (DAT_00718fa4 != -1) {
        return uVar4;
      }
      DAT_00718fa4 = 7;
      return uVar4;
    }
    bVar2 = network_game_process_incoming_messages(in_EAX);
    if (bVar2) {
      FUN_004d9400();
    }
  }
  else if (*(short *)(iVar1 + 4) == 1) {
    local_134[1] = 0;
    local_134[2] = 0;
    local_134[3] = 0;
    local_134[0] = 0;
    puVar6 = local_134 + 4;
    for (iVar5 = 0x48; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    local_170 = DAT_006869b0;
    if (DAT_006869b0 == 0) {
      local_170 = 0x7f000001;
    }
    local_15e = (undefined2)DAT_00698208;
    local_160 = 4;
    local_a = 1;
    _wcsncpy((wchar_t *)((int)&local_158 + 2),(wchar_t *)(iVar1 + 0x9fc),8);
    local_146 = 0;
    FUN_004e0790();
    uVar4 = network_connection_initiate(in_EAX,local_134,&local_158);
    if ((char)uVar4 == '\0') goto LAB_004dab24;
  }
  return (uint)bVar2;
}
#endif
