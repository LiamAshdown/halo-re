// network_client_rejoin_check  (Ghidra: FUN_004de390; named per this rewrite)
// address 0x4de390, size 132 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Finds the channel-key entry matching the
// caller's key and parameter and, unless a follow-up check succeeds, flags DAT_0071c2de and
// calls FUN_004aa900 (likely to force a host handoff or disconnect)." The scanned array (client
// treated as short*, +0x669 shorts == byte +0xcd2) matches network_client->session.players[]'s
// machine_index/machine_player_index fields exactly (same evidence as
// network_game_session_reset.c and network_player_entry_add.c). network_session_info_packet_send and
// network_send_join_request_packet are already named by the batch covering 0x4d8a80..0x4d9340.
// The `if (&players[i] == NULL) return;` check is unreachable (client is already known non-NULL
// by that point) and is kept verbatim.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern network_client_globals *network_client; // 0x0071c2d8
extern uint8_t network_host_handoff_requested;  // 0x0071c2de

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, this batch
extern void network_session_info_packet_send(network_client_globals *client); // 0x4d9050, outside this batch
extern uint32_t network_send_join_request_packet(network_client_globals *client); // 0x4d9220, outside this batch
extern void chat_close(void); // 0x4aa900, this module

void network_client_rejoin_check(int8_t machine_player_index)
{
    int16_t own_id;
    int32_t i;
    network_player_entry *player;
    uint32_t send_result;

    if (network_client != 0) {
        own_id = *(int16_t *)network_client;
        if (own_id != -1) {
            for (i = 0; i < 0x10; i++) {
                player = &network_client->session.players[i];
                if (network_player_entry_validate(player) != 0 && (int32_t)player->machine_index == (int32_t)own_id &&
                    player->machine_player_index == machine_player_index) {
                    if (player == 0) {
                        return; // unreachable, kept verbatim
                    }
                    network_session_info_packet_send(network_client);
                    send_result = network_send_join_request_packet(network_client);
                    if ((char)send_result != 0) {
                        return;
                    }
                    network_host_handoff_requested = 1;
                    chat_close();
                    return;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4de390):

void FUN_004de390(short param_1)

{
  short sVar1;
  short *psVar2;
  char cVar3;
  uint uVar4;
  int iVar5;
  short *psVar6;

  psVar2 = DAT_0071c2d8;
  if ((DAT_0071c2d8 != (short *)0x0) && (sVar1 = *DAT_0071c2d8, sVar1 != -1)) {
    iVar5 = 0;
    psVar6 = DAT_0071c2d8 + 0x669;
    do {
      cVar3 = network_player_entry_validate();
      if (((cVar3 != '\0') && ((int)(char)*psVar6 == (int)sVar1)) &&
         (*(char *)((int)psVar6 + 1) == param_1)) {
        if (psVar2 + iVar5 * 0x10 + 0x65b == (short *)0x0) {
          return;
        }
        FUN_004d9050(psVar2);
        uVar4 = network_send_join_request_packet((int)DAT_0071c2d8);
        if ((char)uVar4 != '\0') {
          return;
        }
        DAT_0071c2de = 1;
        chat_close();
        return;
      }
      iVar5 = iVar5 + 1;
      psVar6 = psVar6 + 0x10;
    } while (iVar5 < 0x10);
  }
  return;
}
#endif
