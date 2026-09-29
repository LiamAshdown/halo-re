// network_game_server_host_new  (Ghidra: network_game_server_host_new, already named)
// address 0x4dec40, size 338 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: types/networking.h's own comment cites this address: "network_game_server_host_new
// (0x4dec40) zeroes 0x284 dwords of 0x00861340, which is the size of network_server_globals."
// Every field this function sets after the zero (flags bit1, session.message_callback,
// session.difficulty, and the 16-entry machines[] init matching network_machine's
// channel/unknown_04/unknown_08/machine_id/flags/unknown_50/unknown_51/unknown_52/unknown_56/
// unknown_5c fields, including the header's own "unaligned in the original" note on
// unknown_52/unknown_56) matches types/networking.h exactly.
// UNSURE: the tail zeroing (host+0x9c4..0x9d4, +0x9f8/+0x9f9/+0x9fa) only partly lines up with
// the header's "unknown_9bc[0x3c]" catch-all array; kept as raw offsets into that array rather
// than asserting finer field boundaries the header doesn't declare.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

// FIXED 2026-09-28 (retail-independence loop): the session's message callback is the C
// network_session_reject_pending_connection_callback, not the literal retail address 0x4e1410.
extern int32_t network_session_reject_pending_connection_callback(void *unused, int32_t reject_code); // 0x4e1410

extern network_server_globals network_server_storage; // 0x00861340
extern uint8_t network_session_active2;                // 0x0071c2ec, UNSURE name; "network_server_active" per header
extern int32_t network_scenario_round_counter_a;                       // 0x00699f44, UNSURE identity
extern uint8_t network_scenario_round_counter_b;                        // 0x0071cc24, UNSURE identity
extern uint8_t unknown_00861d4e;                        // 0x00861d4e, UNSURE identity (beyond network_server_globals)
extern uint8_t unknown_00861d4f;                        // 0x00861d4f, UNSURE identity
extern int16_t pending_difficulty;                        // 0x00696564, UNSURE identity; seeds session.difficulty

extern void network_channels_open(void); // 0x441300, this module
extern network_channel *network_channel_new(uint32_t flags); // 0x4dc9b0, this batch
extern void network_game_session_reset(network_game_session *session); // 0x4de470, this batch
extern void network_game_server_host_dispose(network_server_globals *host); // 0x4deda0, this batch
extern char network_game_session_reset_defaults(void); // 0x4e1820, outside this batch, elided args
extern void * network_session_host_start(int32_t user_data); // 0x577850, outside this batch
extern void message_delta_protocol_initialize(void); // 0x4ec2f0, outside this batch
extern void network_stats_summary_log_open(void); // 0x440670, this module

void *network_game_server_host_new(void)
{
    network_server_globals *host;
    int32_t i;
    network_machine *machine;

    memset(&network_server_storage, 0, sizeof(network_server_storage));
    host = &network_server_storage;
    network_session_active2 = 1;
    network_scenario_round_counter_a = 0;
    network_scenario_round_counter_b = 0;
    unknown_00861d4e = 0;
    unknown_00861d4f = 0;
    network_channels_open();
    host->listen_channel = network_channel_new(k_network_channel_listening);
    if (host->listen_channel != 0) {
        host->flags = host->flags | 2;
        host->state = 0;
        network_game_session_reset(&host->session);
        host->session.message_callback = (void *)network_session_reject_pending_connection_callback; // 0x4e1410
        host->session.difficulty = pending_difficulty;
        *(int32_t *)((uint8_t *)host + 0x3b0) = -1; // UNSURE: lands within session.unknown_3a2[10]
        for (i = 0; i < 16; i++) {
            machine = &host->machines[i];
            machine->channel = 0;
            machine->unknown_04 = 0;
            machine->unknown_08 = 0;
            machine->machine_id = -1;
            machine->flags = 0;
            machine->unknown_0f = 0;
            *(uint32_t *)((uint8_t *)machine + 0x52) = 0; // unknown_52, unaligned per header
            *(uint32_t *)((uint8_t *)machine + 0x56) = 0; // unknown_56, unaligned per header
            machine->unknown_5c = -1;
            machine->unknown_50 = 0;
            machine->unknown_51 = 0;
        }
        host->update_tick_count = 0;
        *(uint32_t *)&host->unknown_9bc[0x08] = 0; // 0x9c4
        *(uint32_t *)&host->unknown_9bc[0x0c] = 0; // 0x9c8
        *(uint32_t *)&host->unknown_9bc[0x10] = 0; // 0x9cc
        *(uint32_t *)&host->unknown_9bc[0x00] = 0; // 0x9c0 -- UNSURE: see header re 0x9bc range
        *(uint32_t *)&host->unknown_9bc[0x14] = 0; // 0x9d0
        host->scenario_announcement_sent = 0;
        host->unknown_9fa = 0;
        host->pending_machine_finalize = 0;
        *(int32_t *)((uint8_t *)host + 0x3b0) = *(int32_t *)((uint8_t *)host + 0x3b0) + 1; // see above
        *(uint32_t *)&host->unknown_9bc[0x18] = 0; // 0x9d4
        if (network_game_session_reset_defaults() != 0) {
            network_session_host_start(0);
            goto done;
        }
    }
    network_game_server_host_dispose(host);
    host = 0;
done:
    if (host != 0 && ((*((uint8_t *)host + 6) >> 2) & 1) != 0) {
        message_delta_protocol_initialize();
        network_stats_summary_log_open();
    }
    return host;
}

#if 0
Original Ghidra decompilation (0x4dec40):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __cdecl network_game_server_host_new(void)

{
  char cVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 *puVar4;

  puVar4 = &DAT_00861340;
  puVar2 = &DAT_00861340;
  for (iVar3 = 0x284; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  DAT_0071c2ec = 1;
  DAT_00699f44 = 0;
  DAT_0071cc24 = 0;
  DAT_00861d4e = 0;
  DAT_00861d4f = 0;
  network_channels_open();
  DAT_00861340 = network_channel_new(1);
  if (DAT_00861340 != (int *)0x0) {
    DAT_00861344._2_1_ = DAT_00861344._2_1_ | 2;
    DAT_00861344._0_2_ = 0;
    puVar2 = &DAT_00861348;
    for (iVar3 = 0xec; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    DAT_00861340[2] = (int)&LAB_004e1410;
    network_channel_table_initialize();
    _DAT_008614e6 = DAT_00696564;
    DAT_008616f0 = -1;
    puVar2 = &DAT_008616fc;
    do {
      puVar2[-1] = 0;
      *puVar2 = 0;
      puVar2[1] = 0;
      *(undefined2 *)(puVar2 + 2) = 0xffff;
      *(undefined2 *)((int)puVar2 + 10) = 0;
      *(undefined4 *)((int)puVar2 + 0x4e) = 0;
      *(undefined4 *)((int)puVar2 + 0x52) = 0;
      puVar2[0x16] = 0xffffffff;
      *(undefined1 *)(puVar2 + 0x13) = 0;
      *(undefined1 *)((int)puVar2 + 0x4d) = 0;
      puVar2 = puVar2 + 0x18;
    } while ((int)puVar2 < 0x861cfc);
    _DAT_00861d08 = 0;
    _DAT_00861d0c = 0;
    _DAT_00861d10 = 0;
    _DAT_00861cf8 = 0;
    _DAT_00861d04 = 0;
    DAT_00861d39 = 0;
    DAT_00861d3a = 0;
    DAT_00861d38 = 0;
    DAT_008616f0 = DAT_008616f0 + 1;
    _DAT_00861d14 = 0;
    cVar1 = FUN_004e1820();
    if (cVar1 != '\0') {
      FUN_00577850(0);
      goto LAB_004ded77;
    }
  }
  network_game_server_host_dispose((int *)&DAT_00861340);
  puVar4 = (undefined4 *)0x0;
LAB_004ded77:
  if ((*(byte *)((int)puVar4 + 6) >> 2 & 1) != 0) {
    message_delta_protocol_initialize();
    network_stats_summary_log_open();
  }
  return puVar4;
}
#endif
