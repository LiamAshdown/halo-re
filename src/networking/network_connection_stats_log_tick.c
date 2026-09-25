// network_connection_stats_log_tick  (Ghidra: network_connection_stats_log_tick, already named)
// address 0x440d80, size 660 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/networking_types_notes.md "network_connection_statistics (0x44)": the
// header names interval_packets_sent/interval_bytes_sent/interval_reliable_bytes_sent/
// interval_resend_bytes_sent directly from this function's own log header and %d group, and
// the player-count accumulation ("network_client + 0xb14 or network_server + 8", reading
// +0x1a0) is the cross-check that pinned network_game_session::player_count.
// register convention: __cdecl, no arguments.
//
// Same two foldings as network_stats_summary_log_open.c (identical shape, same technique as
// src/math/random_seed_generate.c folding __allmul/__alldiv):
//  1. the "gamespy Metrics" stack scratch text is dead (never read) and is not reproduced.
//  2. the manual find-end-of-string / dword-copy loops are ordinary strcpy/strcat.
// UNSURE: same foreign-function notes as network_stats_summary_log_open.c apply to
// join_game_server_browser_tick, FUN_00449210 and the fopen mode string at 0x0065fd30.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

extern uint8_t debug_log_level;                   // 0x0087ac06, byte-wide (R01)
extern uint8_t network_statistics_logging_enabled; // 0x006f14b4
extern uint8_t network_connection_log_needs_open;  // 0x006869bd
extern void *network_connection_stats_log_file;    // 0x006f14b8, FILE *
extern int32_t network_connection_log_last_row_ms; // 0x006a4038
extern int32_t network_connection_log_start_ms;    // 0x006a8148
extern int32_t network_connection_stats_count;     // 0x006f14bc
extern network_connection_statistics network_connection_stats[k_network_connection_stats_count]; // 0x0087bec0
extern network_summary_statistics network_summary_stats; // 0x0087bea0
extern network_client_globals *network_client; // 0x0071c2d8
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_summary_log_mode_string[]; // 0x0065fd30, shared by both stats logs

extern int32_t time_query_performance_counter_ms(void); // foreign module, millisecond tick reader
extern char *join_game_server_browser_tick(void);   // foreign module (> 0x4b80f0), log base directory path
extern char directory_create_recursive(char *path); // 0x449250, foreign module

void network_connection_stats_log_tick(void)
{
    char path_buf[0x208];
    char date_buf[0x104];
    time_t now_time;
    struct tm *tm_now;
    char *base_path;
    int32_t now;
    int32_t i;
    network_game_session *session;
    uint8_t control_char;

    if (2 < debug_log_level && network_statistics_logging_enabled == 1) {
        now = time_query_performance_counter_ms();
        if (network_connection_log_needs_open == 1) {
            network_connection_log_needs_open = 0;
            network_connection_log_last_row_ms = now;
            network_connection_log_start_ms = now;

            time(&now_time);
            tm_now = localtime(&now_time);
            strftime(date_buf, 0x103, "%Y-%m-%d %H_%M_%S", tm_now);

            base_path = join_game_server_browser_tick();
            strcpy(path_buf, base_path);
            directory_create_recursive(path_buf);

            strcat(path_buf, "\\gamespy ");
            strcat(path_buf, date_buf);
            strcat(path_buf, ".xls");

            network_connection_stats_log_file = fopen(path_buf, network_summary_log_mode_string);
            fprintf((FILE *)network_connection_stats_log_file,
                    "\tEach connection has five columns (see headers below). One empty column "
                    "separates each connection. Note: resend traffic is considered unreliable.\n");
            fprintf((FILE *)network_connection_stats_log_file,
                    "Time\tPackets Sent\tTotal Sent\tReliable Sent\tUnreliable Sent\tResends Sent\n");
        }
        if (100 < (uint32_t)(now - network_connection_log_last_row_ms)) {
            network_connection_log_last_row_ms = now;
            fprintf((FILE *)network_connection_stats_log_file, "%d",
                    (uint32_t)(now - network_connection_log_start_ms) / 1000);
            if (0 < network_connection_stats_count) {
                fprintf((FILE *)network_connection_stats_log_file, "\t");
                for (i = 0; i < network_connection_stats_count; i++) {
                    control_char = (i != network_connection_stats_count - 1) ? 9 : 0;
                    fprintf((FILE *)network_connection_stats_log_file, "%d\t%d\t%d\t%d\t%d\t%c",
                            network_connection_stats[i].interval_packets_sent,
                            network_connection_stats[i].interval_bytes_sent,
                            network_connection_stats[i].interval_reliable_bytes_sent,
                            network_connection_stats[i].interval_bytes_sent -
                                network_connection_stats[i].interval_reliable_bytes_sent,
                            network_connection_stats[i].interval_resend_bytes_sent,
                            control_char);
                    network_connection_stats[i].interval_packets_sent = 0;
                    network_connection_stats[i].interval_bytes_sent = 0;
                    network_connection_stats[i].interval_reliable_bytes_sent = 0;
                    network_connection_stats[i].interval_resend_bytes_sent = 0;
                    network_connection_stats[i].interval_bytes_received = 0;

                    session = 0;
                    if (network_client != 0) {
                        session = &network_client->session;
                    } else if (network_server != 0) {
                        session = &network_server->session;
                    }
                    if (session != 0) {
                        network_summary_stats.player_count_samples += 1;
                        network_summary_stats.player_count_total += session->player_count;
                    }
                }
            }
            fprintf((FILE *)network_connection_stats_log_file, "\n");
        }
    }
}

#if 0
Original Ghidra decompilation (0x440d80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl network_connection_stats_log_tick(void)

{
  char cVar1;
  undefined4 *puVar2;
  int iVar3;
  tm *_Tm;
  char *pcVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  char *pcVar8;
  int *piVar9;
  undefined4 *puVar10;
  bool bVar11;
  undefined8 local_21c;
  undefined4 local_214;
  undefined4 local_210;
  undefined1 local_20c [4];
  char local_208 [4];
  undefined1 local_204 [256];
  char local_104 [260];

  if ((2 < DAT_0087ac06) && (DAT_006f14b4 == '\x01')) {
    iVar3 = FUN_00449210();
    if (DAT_006869bd == '\x01') {
      local_214 = 0x20797073;
      local_21c._4_4_ = 0x656d6147;
      local_210 = 0x7274654d;
      local_20c = (undefined1  [4])&DAT_00736369;
      DAT_006869bd = '\0';
      DAT_006a4038 = iVar3;
      DAT_006a8148 = iVar3;
      FID_conflict___time32((__time32_t *)&local_21c);
      _Tm = _localtime(&local_21c);
      _strftime(local_104,0x103,"%Y-%m-%d %H_%M_%S",_Tm);
      pcVar4 = (char *)FUN_004e40a0();
      pcVar8 = local_208;
      do {
        cVar1 = *pcVar4;
        pcVar4 = pcVar4 + 1;
        *pcVar8 = cVar1;
        pcVar8 = pcVar8 + 1;
      } while (cVar1 != '\0');
      directory_create_recursive(local_208);
      pcVar8 = local_20c + 3;
      do {
        pcVar4 = pcVar8;
        pcVar8 = pcVar4 + 1;
      } while (pcVar4[1] != '\0');
      builtin_strncpy(pcVar4 + 1,"\\gamespy ",10);
      pcVar8 = local_104;
      do {
        cVar1 = *pcVar8;
        pcVar8 = pcVar8 + 1;
      } while (cVar1 != '\0');
      uVar5 = (int)pcVar8 - (int)local_104;
      pcVar8 = local_20c + 3;
      do {
        pcVar4 = pcVar8 + 1;
        pcVar8 = pcVar8 + 1;
      } while (*pcVar4 != '\0');
      pcVar4 = local_104;
      for (uVar7 = uVar5 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
        *(undefined4 *)pcVar8 = *(undefined4 *)pcVar4;
        pcVar4 = pcVar4 + 4;
        pcVar8 = pcVar8 + 4;
      }
      for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *pcVar8 = *pcVar4;
        pcVar4 = pcVar4 + 1;
        pcVar8 = pcVar8 + 1;
      }
      puVar2 = (undefined4 *)(local_20c + 3);
      do {
        puVar10 = puVar2;
        puVar2 = (undefined4 *)((int)puVar10 + 1);
      } while (*(char *)((int)puVar10 + 1) != '\0');
      *(undefined4 *)((int)puVar10 + 1) = 0x736c782e;
      *(undefined1 *)((int)puVar10 + 5) = 0;
      DAT_006f14b8 = (FILE *)FUN_00624186(local_208,&DAT_0065fd30);
      _fprintf(DAT_006f14b8,
               "\tEach connection has five columns (see headers below). One empty column separates each connection. Note: resend traffic is considered unreliable.\n"
              );
      _fprintf(DAT_006f14b8,
               "Time\tPackets Sent\tTotal Sent\tReliable Sent\tUnreliable Sent\tResends Sent\n");
    }
    if (100 < (uint)(iVar3 - DAT_006a4038)) {
      DAT_006a4038 = iVar3;
      _fprintf(DAT_006f14b8,"%d",(uint)(iVar3 - DAT_006a8148) / 1000);
      if (0 < DAT_006f14bc) {
        _fprintf(DAT_006f14b8,"\t");
        iVar3 = 0;
        if (0 < DAT_006f14bc) {
          piVar9 = &DAT_0087bee4;
          do {
            _fprintf(DAT_006f14b8,"%d\t%d\t%d\t%d\t%d\t%c",piVar9[6],*piVar9,piVar9[2],
                     *piVar9 - piVar9[2],piVar9[3],-(uint)(DAT_006f14bc + -1 != iVar3) & 9);
            iVar6 = DAT_0071c2d8;
            bVar11 = DAT_0071c2d8 == 0;
            piVar9[6] = 0;
            *piVar9 = 0;
            piVar9[2] = 0;
            piVar9[3] = 0;
            piVar9[1] = 0;
            if (bVar11) {
              if (DAT_0071c2d4 != 0) {
                iVar6 = DAT_0071c2d4 + 8;
                goto LAB_00440fcd;
              }
            }
            else {
              iVar6 = iVar6 + 0xb14;
LAB_00440fcd:
              if (iVar6 != 0) {
                _DAT_0087beb8 = _DAT_0087beb8 + 1;
                _DAT_0087beb4 = _DAT_0087beb4 + *(short *)(iVar6 + 0x1a0);
              }
            }
            iVar3 = iVar3 + 1;
            piVar9 = piVar9 + 0x11;
          } while (iVar3 < DAT_006f14bc);
        }
      }
      _fprintf(DAT_006f14b8,"\n");
    }
  }
  return;
}
#endif
