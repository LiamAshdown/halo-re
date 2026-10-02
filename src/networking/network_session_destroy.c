// network_session_destroy  (Ghidra: network_session_destroy, already named)
// address 0x4d8b70, size 64 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/networking.h "network_client_globals (0x4d8a80 network_session_create,
// 0x4d8b70 destroy)"; the two offsets this function writes (+0xf48, +0xadc) are exactly
// update_history and channel, matching out/phase4/networking_types_notes.md's
// "network_session_destroy (0x4d8b70) frees the +0xf48 list and deletes the +0xadc channel".
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void message_delta_parameters_protocol_dump_to_config_file(void); // 0x4ec330
extern void player_update_history_destroy(player_update_history *history); // 0x4e6b10, blam-cc: EBX -> history
extern void network_channel_delete(network_channel *channel); // 0x4dcae0
extern void network_stats_summary_log_write(void); // 0x440820
extern uint8_t network_session_active; // 0x0071c2c2

void network_session_destroy(network_client_globals *client) // blam-cc: EAX -> client
{
    message_delta_parameters_protocol_dump_to_config_file();
    if (client != 0) {
        player_update_history_destroy((player_update_history *)client->update_history);
        client->update_history = 0;
        if (client->channel != 0) {
            network_channel_delete(client->channel);
        }
        network_session_active = 0;
    }
    network_stats_summary_log_write();
}

#if 0
Original Ghidra decompilation (0x4d8b70):

void network_session_destroy(void)

{
  int in_EAX;

  message_delta_parameters_protocol_dump_to_config_file();
  if (in_EAX != 0) {
    FUN_004e6b10();
    *(undefined4 *)(in_EAX + 0xf48) = 0;
    if (*(int **)(in_EAX + 0xadc) != (int *)0x0) {
      network_channel_delete(*(int **)(in_EAX + 0xadc));
    }
    DAT_0071c2c2 = 0;
  }
  network_stats_summary_log_write();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
