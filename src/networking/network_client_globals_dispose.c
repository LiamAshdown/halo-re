// network_client_globals_dispose  (Ghidra: FUN_004dde70; named per this rewrite)
// address 0x4dde70, size 89 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Tears down the network-game globals
// (DAT_0071c2d8): releases its connection sub-allocation, resets related counters/flags and
// frees the globals themselves." client->update_history (+0xf48) and client->channel (+0xadc)
// match types/networking.h's network_client_globals exactly; network_session_active
// (0x0071c2c2) matches the header's own documented name for that address.
// UNSURE: player_update_history_destroy (outside this batch's range) is called with no visible argument;
// reconstructed as (network_client->update_history), matching player_update_history's own
// "0x4e6b10 destroy" citation in types/networking.h's comment on that type.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern network_client_globals *network_client; // 0x0071c2d8
extern uint8_t network_session_active;         // 0x0071c2c2
extern uint8_t network_host_handoff_requested;  // 0x0071c2de


extern void player_update_history_destroy(void *update_history); // 0x4e6b10, outside this batch, elided arg


void network_client_globals_dispose(void)
{
    if (network_client != 0) {
        message_delta_parameters_protocol_dump_to_config_file();
        player_update_history_destroy(network_client->update_history);
        network_client->update_history = 0;
        if (network_client->channel != 0) {
            network_channel_delete(network_client->channel);
        }
        network_session_active = 0;
        network_stats_summary_log_write();
        network_client = 0;
    }
    network_host_handoff_requested = 0;
}

#if 0
Original Ghidra decompilation (0x4dde70):

void FUN_004dde70(void)

{
  int iVar1;

  iVar1 = DAT_0071c2d8;
  if (DAT_0071c2d8 != 0) {
    message_delta_parameters_protocol_dump_to_config_file();
    if (iVar1 != 0) {
      FUN_004e6b10();
      *(undefined4 *)(iVar1 + 0xf48) = 0;
      if (*(int **)(iVar1 + 0xadc) != (int *)0x0) {
        network_channel_delete(*(int **)(iVar1 + 0xadc));
      }
      DAT_0071c2c2 = 0;
    }
    network_stats_summary_log_write();
    DAT_0071c2d8 = 0;
  }
  DAT_0071c2de = 0;
  return;
}
#endif
