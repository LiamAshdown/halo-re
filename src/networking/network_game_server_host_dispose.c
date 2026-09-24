// network_game_server_host_dispose  (Ghidra: network_game_server_host_dispose, already named)
// address 0x4deda0, size 284 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Tears down the network host globals block
// passed in param_1: disposes each of its 16 team/player sub-entries, frees its allocation,
// zeroes the structure, and shuts down the associated transport connection." host->flags bit2
// (+6, stats logging), machines[16] at +0x3b8 stride 0x60 with channel/machine_id/flags, and the
// final 0x284-dword (0xa10-byte) zero of the whole network_server_globals all match
// types/networking.h exactly.
// UNSURE: network_channel_service's channel argument (network_channel_service's own `channel` parameter) is
// elided at this call site; reconstructed as the current machine's channel, matching the loop's
// own subject.
// register convention: fully recovered cdecl (host is the only parameter).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#include <string.h>

extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t unknown_0069fdfc;   // 0x0069fdfc, the rcon/console connection id per header
extern uint8_t network_session_active2; // 0x0071c2ec, UNSURE name
extern void *master_server_object; // 0x00722a20
extern int32_t master_server_state; // 0x00722a18

extern void FUN_0061b3f0(int32_t connection_id); // outside this batch, foreign
extern void message_delta_parameters_protocol_dump_to_config_file(void); // 0x4ec330, outside this batch
extern void network_stats_summary_log_write(void); // 0x440820, this module
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0
extern char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child); // 0x4dd110
extern void network_channel_delete(network_channel *channel); // 0x4dcae0, this batch
extern void network_session_host_update(void); // 0x577940, outside this batch
extern void FUN_0061b760(void); // outside this batch, foreign
extern void FUN_00616c40(void *object); // outside this batch, foreign

void network_game_server_host_dispose(network_server_globals *host)
{
    int32_t i;
    network_machine *machine;
    int32_t challenge_packet;
    int32_t message_type;
    uint32_t challenge_payload[4]; // UNSURE: the caller-frame scratch EDX points at

    FUN_0061b3f0(unknown_0069fdfc);
    if (((*(uint8_t *)((uint8_t *)host + 6) >> 2) & 1) != 0) {
        message_delta_parameters_protocol_dump_to_config_file();
        network_stats_summary_log_write();
    }
    if ((host->unknown_004 == 0 || host->unknown_004 == 2)) {
        message_type = (host->unknown_004 == 2) ? 0x22 : 0x0b;
        // 0x4dedd7/0x4dedde select the message type (0x22 when unknown_004 == 2, 0x0b when
        // it is 0); 0x4dede3: edx = &payload scratch.
        challenge_packet = (int32_t)network_prepare_challenge_packet(message_type, challenge_payload);
        if (challenge_packet != 0) {
            network_session_broadcast_to_all(network_server, 0, (void *)(uintptr_t)challenge_packet,
                1, 0, 1, 3);
        }
    }
    for (i = 0; i < 16; i++) {
        machine = &host->machines[i];
        if (machine->machine_id != -1 && (machine->channel->flags & k_network_channel_dead) == 0) {
            // FIXED in the review pass: 0x4dee3b sets EAX = 0x3a98 (15000 ms) before the
            // call and pushes 0 as the out-new-child argument; the first draft read Ghidra's
            // single visible `0` as the timeout and passed 0, which skips the whole
            // idle/back-off branch inside the service routine.
            network_channel_service(machine->channel, 15000, 0);
        }
    }
    if (host->listen_channel != 0) {
        network_channel_delete(host->listen_channel);
    }
    memset(host, 0, sizeof(*host));
    network_session_active2 = 0;
    if (master_server_object != 0) {
        if (master_server_state != 2) {
            master_server_state = 2;
        }
        network_session_host_update();
        unknown_0069fdfc = -1;
        FUN_0061b760();
        FUN_00616c40(master_server_object);
        master_server_object = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4deda0):

void __cdecl network_game_server_host_dispose(int *param_1)

{
  int iVar1;
  int *piVar2;

  FUN_0061b3f0(DAT_0069fdfc);
  if ((*(byte *)((int)param_1 + 6) >> 2 & 1) != 0) {
    message_delta_parameters_protocol_dump_to_config_file();
    network_stats_summary_log_write();
  }
  if ((((short)param_1[1] == 0) || ((short)param_1[1] == 2)) &&
     (iVar1 = network_prepare_challenge_packet(), iVar1 != 0)) {
    FUN_004e19c0(0,iVar1,1,0,1,3);
  }
  piVar2 = param_1 + 0xee;
  iVar1 = 0x10;
  do {
    if (((short)piVar2[3] != -1) && ((~(byte)(*(uint *)(*piVar2 + 0xa8c) >> 4) & 1) != 0)) {
      FUN_004dd110(0);
    }
    piVar2 = piVar2 + 0x18;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  if ((int *)*param_1 != (int *)0x0) {
    network_channel_delete((int *)*param_1);
  }
  for (iVar1 = 0x284; iVar1 != 0; iVar1 = iVar1 + -1) {
    *param_1 = 0;
    param_1 = param_1 + 1;
  }
  DAT_0071c2ec = 0;
  if (DAT_00722a20 != 0) {
    if (DAT_00722a18 != 2) {
      DAT_00722a18 = 2;
    }
    FUN_00577940();
    DAT_0069fdfc = 0xffffffff;
    FUN_0061b760();
    FUN_00616c40(DAT_00722a20);
    DAT_00722a20 = 0;
  }
  return;
}
#endif
