// network_client_identity_tick  (Ghidra: FUN_004db310; renamed, no prior name)
// address 0x4db310, size 422 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Periodic bookkeeping for the local
// client's machine/session identity: expires a stale pending-close flag, then either refreshes
// or re-establishes the client's connection identity depending on whether [acting as host]").
// client+0xee4/+0xee8/+0xeec/+0xef0/+0xef4 match network_client_globals::timer
// (a network_client_timer_record, folded into types/networking.h by the review pass), and
// client+0xef8..+0xf0c is network_client_globals::server_address.
// register convention: __cdecl, single stack parameter `client`.
// // blam-cc: stack -> client
// UNSURE: `network_channel_service_close_if_disconnected`, `network_client_globals_create`, `network_client_globals_dispose` are called with no visible arguments
// and are not in this task's range; declared exactly as shown.
// UNSURE: `datum_get` is called with no visible argument; reconstructed as taking the local
// player's datum index (`network_local_player_index_table[1]`, the same table read in
// network_connection_finalize_join.c), the only value in scope that makes semantic sense here.
// UNSURE: `DAT_0071c2c1` is documented in types/networking.h only as "copied into
// network_game_session::unknown_3ac"; named `network_session_unknown_3ac_source` here.
// UNSURE: client+0xaf0 (the wcsncpy source) is the same still-unresolved offset already used in
// network_game_settings_packet_send.c (client+0xae0+0x10); accessed the same way, via a raw
// offset, not a named field.

#include "tags.h"
#include "memory.h"
#include <wchar.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter);
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern int32_t network_channel_service_close_if_disconnected(network_channel *channel); // 0x4dd3f0
extern network_server_globals *network_server; // 0x0071c2d4
extern network_client_globals *network_client; // 0x0071c2d8
extern uint8_t *network_local_player_index_table; // 0x0087a478, per network_connection_finalize_join.c
extern data_array *player_data; // 0x0087a480
extern void *datum_get(datum_index index); // 0x4d0680
extern void network_client_globals_dispose(void); // 0x4dde70, not in this batch
extern void network_client_globals_create(void); // 0x4dde50, not in this batch
extern uint8_t network_session_unknown_3ac_source; // 0x0071c2c1, UNSURE name; see file header
extern void network_client_begin_connect(const wchar_t *name); // 0x4dc8d0, this batch (not yet rewritten at time of writing)
extern wchar_t *_wcsncpy(wchar_t *dest, const wchar_t *source, int32_t count);

// blam-cc: stack -> client
int32_t network_client_identity_tick(network_client_globals *client)
{
    large_integer counter;
    uint32_t now_ms;
    wchar_t name[10];
    int32_t timer_tail[6];
    int32_t local_player_id;
    int32_t i;

    QueryPerformanceCounter(&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / performance_frequency);

    // FIXED in the review pass: 0x4db358 is `cmp BYTE PTR [edi],0x0` with edi = client+0xee4,
    // so the armed flag is a byte, not a dword; and 0x4db365 loads client->channel into EAX
    // immediately before `call 0x4dd3f0`, so the elided argument is the channel.
    if (client->timer.active != 0 && (uint32_t)client->timer.deadline_ms <= now_ms) {
        int32_t result = network_channel_service_close_if_disconnected(client->channel);
        client->timer.active = 0;
        client->timer.deadline_ms = 0;
        client->timer.triggered = 1;
        client->timer.retrigger_ms = client->timer.context + (int32_t)now_ms;
        return result;
    }

    if (client->timer.triggered != 0 && (uint32_t)client->timer.retrigger_ms <= now_ms &&
        (client->channel->endpoint->flags & 0x40) != 0) {
        for (i = 0; i < 6; i = i + 1) {
            timer_tail[i] = ((int32_t *)&client->server_address)[i]; // 0x4db3cc: lea eax,[ebp+0xef8]
        }
        local_player_id = -1;
        // client+0xaf0 lands two bytes into connect_attempt.session_info, which the binary
        // reads as UTF-16 text; kept as a raw offset because the field is unaligned there.
        _wcsncpy(name, (const wchar_t *)((uint8_t *)client + 0xaf0), 8);

        if (network_server == 0) {
            if (*(int32_t *)(network_local_player_index_table + 4) != -1) {
                void *player = datum_get(*(datum_index *)(network_local_player_index_table + 4)); // UNSURE argument
                if (player != 0) {
                    local_player_id = *(int32_t *)((uint8_t *)player + 0x20);
                }
            }
        } else {
            local_player_id = client->unknown_f10;
        }

        network_client_globals_dispose();
        if (network_server != 0) {
            network_client_globals_create();
            network_client->unknown_f10 = local_player_id;
            return 1;
        }

        network_session_unknown_3ac_source = 1;
        network_client_begin_connect(name);
        // Zeroes the whole 11-dword run at +0xee4 (the timer record plus unknown_ef8) plus
        // unknown_f10, matching Ghidra's literal "pcVar5 = &client->unknown_ee4[0]".
        for (i = 0; i < 5; i = i + 1) {
            ((int32_t *)&client->timer)[i] = 0;
        }
        for (i = 0; i < 6; i = i + 1) {
            ((int32_t *)&client->server_address)[i] = 0;
        }
        client->unknown_f10 = 0;
        client->unknown_f10 = -1; // overwrites the zero just written, per Ghidra's own order
        network_client->unknown_f10 = local_player_id;
        network_session_unknown_3ac_source = 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4db310):

undefined4 FUN_004db310(int param_1)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  char *pcVar5;
  undefined8 uVar6;
  LARGE_INTEGER local_34;
  wchar_t local_2c [10];
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  QueryPerformanceCounter(&local_34);
  uVar6 = __allmul(local_34.s.LowPart,local_34.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar6,DAT_006ac8f8,DAT_006ac8fc);
  pcVar5 = (char *)(param_1 + 0xee4);
  if ((*pcVar5 != '\0') && (*(uint *)(param_1 + 0xee8) <= uVar1)) {
    uVar2 = FUN_004dd3f0();
    *pcVar5 = '\0';
    *(undefined4 *)(param_1 + 0xee8) = 0;
    *(undefined1 *)(param_1 + 0xeec) = 1;
    *(uint *)(param_1 + 0xef4) = *(int *)(param_1 + 0xef0) + uVar1;
    return uVar2;
  }
  if ((*(char *)(param_1 + 0xeec) != '\0') &&
     ((*(uint *)(param_1 + 0xef4) <= uVar1 &&
      ((*(byte *)(**(int **)(param_1 + 0xadc) + 0xc) & 0x40) != 0)))) {
    local_18 = *(undefined4 *)(param_1 + 0xef8);
    local_14 = *(undefined4 *)(param_1 + 0xefc);
    local_10 = *(undefined4 *)(param_1 + 0xf00);
    local_c = *(undefined4 *)(param_1 + 0xf04);
    local_8 = *(undefined4 *)(param_1 + 0xf08);
    local_4 = *(undefined4 *)(param_1 + 0xf0c);
    uVar2 = 0xffffffff;
    _wcsncpy(local_2c,(wchar_t *)(param_1 + 0xaf0),8);
    if (DAT_0071c2d4 == 0) {
      if (*(int *)(DAT_0087a478 + 4) != -1) {
        iVar3 = datum_get();
        if (iVar3 != 0) {
          uVar2 = *(undefined4 *)(iVar3 + 0x20);
        }
      }
    }
    else {
      uVar2 = *(undefined4 *)(param_1 + 0xf10);
    }
    FUN_004dde70();
    if (DAT_0071c2d4 != 0) {
      FUN_004dde50();
      *(undefined4 *)(DAT_0071c2d8 + 0xf10) = uVar2;
      return 1;
    }
    DAT_0071c2c1 = 1;
    FUN_004dc8d0(local_2c);
    iVar3 = DAT_0071c2d8;
    for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
      pcVar5[0] = '\0';
      pcVar5[1] = '\0';
      pcVar5[2] = '\0';
      pcVar5[3] = '\0';
      pcVar5 = pcVar5 + 4;
    }
    *(undefined4 *)(param_1 + 0xf10) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0xf10) = uVar2;
    DAT_0071c2c1 = 0;
  }
  return 1;
}
#endif
