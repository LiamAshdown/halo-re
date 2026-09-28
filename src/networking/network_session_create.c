// network_session_create  (Ghidra: network_session_create, already named)
// address 0x4d8a80, size 236 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/networking.h "network_client_globals (0x4d8a80 network_session_create,
// 0x4d8b70 destroy)" -- every DAT_ global in this function is network_client_storage
// (0x00872de0) plus a fixed delta, matched field by field against that struct; the
// out/phase4/networking_types_notes.md "network_client_globals (0xf4c)" paragraph confirms the
// same deltas and confirms +0xb14 is the session network_game_session_reset receives.
// register convention: none (void); Ghidra shows no incoming registers.
// UNSURE: the final 13-dword zero of unknown_f14 runs unconditionally, even on the
// network_channel_new(2) failure path where the local base pointer has just been set to 0 --
// Ghidra's "puVar2 + 0xf14" byte arithmetic on a 0 puVar2 means that path would write
// through raw address 0xf14. Preserved exactly (see the task's no-invented-behaviour rule);
// this is presumably a real, effectively unreachable bug in the retail binary (network_channel_new
// failing here needs a socket-creation failure), not a decompiler artifact -- the two loops and
// the intervening if/else are still standard structured control flow either way.
// UNSURE: the 12-dword zero loop starting at unknown_ee4 writes one dword past the declared
// 11-element array, into unknown_f10; the very next statement then overwrites unknown_f10 with
// -1. Both are preserved exactly, in the original order.
// UNSURE: the calls to network_game_session_reset and network_session_destroy carry no
// visible arguments in the Ghidra output (their real parameters arrive in EDX/EAX respectively,
// per those functions' own signatures); network_client_storage (the client argument) is what
// every other evidence in this file's neighbourhood says is live in that register at the call
// site, so it is passed explicitly here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes);
extern uint8_t network_session_active; // 0x0071c2c2
extern network_client_globals network_client_storage; // 0x00872de0
extern void message_delta_protocol_initialize(void); // 0x4ec2f0
extern network_channel *network_channel_new(uint32_t flags); // 0x4dc9b0
extern void network_session_destroy(network_client_globals *client); // 0x4d8b70, blam-cc: EAX -> client
extern void network_game_session_reset(network_game_session *session); // 0x4de470, blam-cc: EDX -> session
extern void network_stats_summary_log_open(void); // 0x440670

network_client_globals *network_session_create(void)
{
    network_client_globals *client;
    player_update_history *history;
    int32_t *run;
    int32_t i;

    client = &network_client_storage;
    network_session_active = 1;
    message_delta_protocol_initialize();

    history = (player_update_history *)GlobalAlloc(0, 0x2c);
    client->update_history = history;
    history->next_update_id = 0;
    history->head = 0;
    history->tail = 0;
    for (i = 0; i < 8; i = i + 1) {
        history->unknown_0c[i] = 0;
    }

    client->channel = network_channel_new(2);
    if (client->channel == 0) {
        network_session_destroy(client);
        client = 0;
    } else {
        network_game_session_reset(&client->session);
        client->unknown_ede = client->unknown_ede & 0xfff9;
        client->unknown_000 = 0xffff;
        client->state = 0;
        client->unknown_edc = 0;
        client->unknown_ec8 = 0;
        client->unknown_ecc = 0;
        client->unknown_ed0 = 0;
        client->unknown_ee1 = 0;
        client->unknown_ed8 = 0xffff;
        client->unknown_ee0 = 0;
        // The 12-dword run at +0xee4: the timer record (5 dwords), unknown_ef8[6] and
        // unknown_f10, which the next statement then sets to -1.
        run = (int32_t *)&client->timer;
        for (i = 12; i != 0; i = i - 1) {
            *run = 0;
            run = run + 1;
        }
        client->unknown_f10 = -1;
    }

    // Unconditional: runs even when client was just set to 0 above (see file header UNSURE).
    run = (int32_t *)((uint8_t *)client + 0xf14);
    for (i = 13; i != 0; i = i - 1) {
        *run = 0;
        run = run + 1;
    }
    network_stats_summary_log_open();
    return client;
}

#if 0
Original Ghidra decompilation (0x4d8a80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined * network_session_create(void)

{
  int iVar1;
  undefined *puVar2;
  undefined4 *puVar3;

  puVar2 = &DAT_00872de0;
  DAT_0071c2c2 = 1;
  message_delta_protocol_initialize();
  _DAT_00873d28 = GlobalAlloc(0,0x2c);
  *_DAT_00873d28 = 0;
  _DAT_00873d28[1] = 0;
  _DAT_00873d28[2] = 0;
  _DAT_00873d28[3] = 0;
  _DAT_00873d28[4] = 0;
  _DAT_00873d28[5] = 0;
  _DAT_00873d28[6] = 0;
  _DAT_00873d28[7] = 0;
  _DAT_00873d28[8] = 0;
  _DAT_00873d28[9] = 0;
  _DAT_00873d28[10] = 0;
  _DAT_008738bc = network_channel_new(2);
  if (_DAT_008738bc == (int *)0x0) {
    network_session_destroy();
    puVar2 = (undefined *)0x0;
  }
  else {
    network_channel_table_initialize();
    DAT_00873cbe = DAT_00873cbe & 0xfff9;
    _DAT_00872de0 = 0xffff;
    _DAT_00873cba = 0;
    _DAT_00873cbc = 0;
    _DAT_00873ca8 = 0;
    _DAT_00873cac = 0;
    _DAT_00873cb0 = 0;
    DAT_00873cc1 = 0;
    _DAT_00873cb8 = 0xffff;
    DAT_00873cc0 = 0;
    puVar3 = &DAT_00873cc4;
    for (iVar1 = 0xc; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    _DAT_00873cf0 = 0xffffffff;
  }
  puVar3 = (undefined4 *)(puVar2 + 0xf14);
  for (iVar1 = 0xd; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  network_stats_summary_log_open();
  return puVar2;
}
#endif
