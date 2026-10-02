// network_clear_player_ready_flags  (Ghidra: FUN_004a5ad0, renamed)
// renamed from FUN_004a5ad0 in the naming pass
// address 0x4a5ad0, size 239 bytes, callers=0 in this build
// name confidence: 0.25   rewrite confidence: 0.15
// evidence: functions.md: "Walks the network session's player slots clearing a per-player flag
// (likely 'ready'/'loaded') based on team and game-state checks."
// register convention: none (void).
// UNSURE (significant): almost every field offset here (network_client+0xeda as a status byte,
// +0xb14 as a 16-entry player table with an 0x20-byte stride, network_server+6 bit 2, the local
// `local_5`/`(&local_5)[...]` byte-array indexing trick) is preserved exactly as Ghidra shows,
// without asserting field names against types/networking.h, since this function's own record
// layout could not be confidently cross-referenced in the time available this session.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_client_globals *network_client; // 0x0071c2d8
extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t local_team_00714dd8;              // 0x00714dd8, TYPES-GAP
extern int16_t network_game_mode;                // 0x00719720

extern int32_t time_query_performance_counter_ms(void); // 0x449210, current time in milliseconds
extern void network_game_settings_ack_send(void *client, int32_t unknown); // 0x4d9f50, UNSURE args
extern uint8_t network_player_entry_validate(void); // 0x4de9f0, UNSURE args

void network_clear_player_ready_flags(void)
{
    int16_t *client = (int16_t *)network_client;
    int16_t *status;
    int16_t *player;
    uint8_t local_ready_flags[16]; // Ghidra's `local_5` used as a byte array via pointer offset
    int32_t i;

    if (client == (int16_t *)0) {
        return;
    }

    status = client + 0x76d;
    if (client[0x76d] == 1) {
        time_query_performance_counter_ms();
    }

    if (*status != 2) {
        return;
    }

    if (network_server == (network_server_globals *)0) {
        player = (client == (int16_t *)0) ? (int16_t *)0 : client + 0x58a;
    } else {
        player = (int16_t *)((uint8_t *)network_server + 8);
    }
    local_ready_flags[0] = local_team_00714dd8;
    player = player + 0xd1;

    for (i = 0x10; i != 0; i--) {
        if (network_player_entry_validate() != 0) {
            if (player == (int16_t *)0 || network_player_entry_validate() == 0) {
                if (network_game_mode != 3 || *((int8_t *)player + 0x1c) == 0) {
                    goto clear_flag;
                }
            } else if ((network_server == (network_server_globals *)0 ||
                        (((*((uint8_t *)network_server + 6) >> 2) & 1) == 0)) &&
                       *client != -1 && *client == (int16_t)*((int8_t *)player + 0x1c)) {
            clear_flag:
                local_ready_flags[*((int8_t *)player + 0x1d)] = 0;
            }
        }
        player = player + 0x10;
    }

    if (local_ready_flags[0] != 0) {
        network_game_settings_ack_send(network_client, 0);
    }
}

#if 0
Original Ghidra decompilation (0x4a5ad0):

void FUN_004a5ad0(void)

{
  int iVar1;
  short *psVar2;
  char cVar3;
  short *psVar4;
  int iVar5;
  char local_5;
  short *local_4;

  local_4 = DAT_0071c2d8;
  if (DAT_0071c2d8 != (short *)0x0) {
    psVar4 = DAT_0071c2d8 + 0x76d;
    if (DAT_0071c2d8[0x76d] == 1) {
      FUN_00449210();
    }
    psVar2 = DAT_0071c2d8;
    iVar1 = DAT_0071c2d4;
    if (*psVar4 == 2) {
      if (DAT_0071c2d4 == 0) {
        if (DAT_0071c2d8 == (short *)0x0) {
          psVar4 = (short *)0x0;
        }
        else {
          psVar4 = DAT_0071c2d8 + 0x58a;
        }
      }
      else {
        psVar4 = (short *)(DAT_0071c2d4 + 8);
      }
      local_5 = DAT_00714dd8;
      psVar4 = psVar4 + 0xd1;
      iVar5 = 0x10;
      do {
        cVar3 = FUN_004de9f0();
        if (cVar3 != '\0') {
          if ((psVar4 == (short *)0x0) || (cVar3 = FUN_004de9f0(), cVar3 == '\0')) {
            if ((DAT_00719720 != 3) || ((char)psVar4[0xe] == '\0')) goto LAB_004a5b91;
          }
          else if ((((iVar1 == 0) || ((*(byte *)(iVar1 + 6) >> 2 & 1) == 0)) && (*psVar2 != -1)) &&
                  ((int)*psVar2 == (int)(char)psVar4[0xe])) {
LAB_004a5b91:
            (&local_5)[*(char *)((int)psVar4 + 0x1d)] = '\0';
          }
        }
        psVar4 = psVar4 + 0x10;
        iVar5 = iVar5 + -1;
      } while (iVar5 != 0);
      if (local_5 != '\0') {
        FUN_004d9f50(local_4,0);
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
