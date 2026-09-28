// network_stats_summary_log_open  (Ghidra: network_stats_summary_log_open, already named)
// address 0x440670, size 417 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "bandwidth statistics" section names every
// global here (debug_log_level, network_statistics_logging_enabled,
// network_summary_log_needs_open, network_summary_log_file, network_summary_stats); the
// literal strings "\Game Summary " and the tab-separated header pin the rest.
// register convention: __cdecl, no arguments.
//
// Two simplifications from the raw decompile, both verified to produce identical resulting
// bytes (same technique as src/math/random_seed_generate.c folding __allmul/__alldiv):
//  1. The decompiled code builds a literal "Gamespy Metrics\0" string on the stack right
//     before the path buffer purely so that a later `p - 1` pointer (shown by Ghidra as
//     "local_20c + 3") has somewhere valid to start a find-end-of-string scan from. That text
//     is never read as data (the path buffer that follows it is overwritten first, and every
//     later scan starts one byte before the CURRENT contents of the path buffer, not before
//     the "Gamespy Metrics" text) -- see out/phase2/networking/00.md around this address for
//     the raw byte assignments. It has no observable effect and is not reproduced here.
//  2. The three manual "walk to the NUL, then dword/byte-copy" loops that follow are ordinary
//     strcpy/strcat, and are written as such.
// UNSURE: join_game_server_browser_tick (foreign, > 0x4b80f0) is assumed to return the base log-directory path
// (its result is what gets a directory created for it, then has the log filename appended).
// UNSURE: FUN_00449210 (foreign) is used here as a zero-argument millisecond tick reader
// (QueryPerformanceCounter scaled by its frequency, same shape as random_seed_generate.c);
// note src/objects/object_nudge_position_by_velocity.c calls the same address with a visible
// pointer argument in a different context, which this rewrite does not attempt to reconcile.
// UNSURE: the exact contents of the fopen mode string at 0x0065fd30 (not captured by string
// extraction; assumed to be a plain text mode such as "w").
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

extern uint8_t debug_log_level;                  // 0x0087ac06, byte-wide (R01)
extern uint8_t network_statistics_logging_enabled; // 0x006f14b4
extern uint8_t network_summary_log_needs_open;    // 0x006869bc
extern void *network_summary_log_file;            // 0x006a6140, FILE *
extern network_summary_statistics network_summary_stats; // 0x0087bea0
extern char network_summary_log_mode_string[];    // 0x0065fd30, UNSURE: exact text unresolved

extern int32_t time_query_performance_counter_ms(void);        // foreign module, millisecond tick reader; see UNSURE
extern char *join_game_server_browser_tick(void);          // foreign module (> 0x4b80f0), log base directory path
extern char directory_create_recursive(char *path); // 0x449250, foreign module
// time(), localtime(), strftime() and fopen() come from <time.h>/<stdio.h> above; the retail
// binary calls the 32-bit-time-specific CRT entry points (_time32, localtime, _fsopen) for
// the same effect, per the _rand -> rand renaming precedent in src/math/random_seed_generate.c.

void network_stats_summary_log_open(void)
{
    char path_buf[0x208];
    char date_buf[0x104];
    time_t now;
    struct tm *tm_now;
    char *base_path;

    if (2 < debug_log_level && network_statistics_logging_enabled != 0) {
        if (network_summary_log_needs_open != 0) {
            time(&now);
            tm_now = localtime(&now);
            strftime(date_buf, 0x103, "%Y-%m-%d %H_%M_%S", tm_now);

            base_path = join_game_server_browser_tick();
            strcpy(path_buf, base_path);
            directory_create_recursive(path_buf);

            strcat(path_buf, "\\Game Summary ");
            strcat(path_buf, date_buf);
            strcat(path_buf, ".xls");

            network_summary_log_file = fopen(path_buf, network_summary_log_mode_string);
            fprintf((FILE *)network_summary_log_file,
                    "Map\tLength (seconds)\tAvg # Players\tPackets Sent\tPackets Received\t"
                    "Packets Sent/sec\tPackets Received/sec\tBytes Sent\tBytes Received\t"
                    "Bytes Sent/sec\tBytes Received/sec\tBits Sent/sec/conn\t"
                    "Bits Received/sec/conn\tBytes Sent/packet\tBytes Received/packet\n");
            network_summary_log_needs_open = 0;
        }
        network_summary_stats.bytes_sent = 0;
        network_summary_stats.bytes_received = 0;
        network_summary_stats.packets_sent = 0;
        network_summary_stats.packets_received = 0;
        network_summary_stats.player_count_total = 0;
        network_summary_stats.player_count_samples = 0;
        network_summary_stats.start_ms = time_query_performance_counter_ms();
    }
}

#if 0
Original Ghidra decompilation (0x440670):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl network_stats_summary_log_open(void)

{
  char cVar1;
  undefined4 *puVar2;
  tm *_Tm;
  char *pcVar3;
  uint uVar4;
  uint uVar5;
  char *pcVar6;
  undefined4 *puVar7;
  undefined8 local_21c;
  undefined4 local_214;
  undefined4 local_210;
  undefined1 local_20c [4];
  char local_208 [4];
  undefined1 local_204 [10];
  char local_1fa [246];
  char local_104 [260];

  if ((2 < DAT_0087ac06) && (DAT_006f14b4 != '\0')) {
    if (DAT_006869bc != '\0') {
      local_214 = 0x20797073;
      local_21c._4_4_ = 0x656d6147;
      local_210 = 0x7274654d;
      local_20c = (undefined1  [4])&DAT_00736369;
      FID_conflict___time32((__time32_t *)&local_21c);
      _Tm = _localtime(&local_21c);
      _strftime(local_104,0x103,"%Y-%m-%d %H_%M_%S",_Tm);
      pcVar3 = (char *)FUN_004e40a0();
      pcVar6 = local_208;
      do {
        cVar1 = *pcVar3;
        pcVar3 = pcVar3 + 1;
        *pcVar6 = cVar1;
        pcVar6 = pcVar6 + 1;
      } while (cVar1 != '\0');
      directory_create_recursive(local_208);
      pcVar6 = local_20c + 3;
      do {
        pcVar3 = pcVar6;
        pcVar6 = pcVar3 + 1;
      } while (pcVar3[1] != '\0');
      builtin_strncpy(pcVar3 + 1,"\\Game Summary ",0xf);
      pcVar6 = local_104;
      do {
        cVar1 = *pcVar6;
        pcVar6 = pcVar6 + 1;
      } while (cVar1 != '\0');
      uVar4 = (int)pcVar6 - (int)local_104;
      pcVar6 = local_20c + 3;
      do {
        pcVar3 = pcVar6 + 1;
        pcVar6 = pcVar6 + 1;
      } while (*pcVar3 != '\0');
      pcVar3 = local_104;
      for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined4 *)pcVar6 = *(undefined4 *)pcVar3;
        pcVar3 = pcVar3 + 4;
        pcVar6 = pcVar6 + 4;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *pcVar6 = *pcVar3;
        pcVar3 = pcVar3 + 1;
        pcVar6 = pcVar6 + 1;
      }
      puVar2 = (undefined4 *)(local_20c + 3);
      do {
        puVar7 = puVar2;
        puVar2 = (undefined4 *)((int)puVar7 + 1);
      } while (*(char *)((int)puVar7 + 1) != '\0');
      *(undefined4 *)((int)puVar7 + 1) = 0x736c782e;
      *(undefined1 *)((int)puVar7 + 5) = 0;
      DAT_006a6140 = (FILE *)FUN_00624186(local_208,&DAT_0065fd30);
      _fprintf(DAT_006a6140,
               "Map\tLength (seconds)\tAvg # Players\tPackets Sent\tPackets Received\tPackets Sent/sec\tPackets Received/sec\tBytes Sent\tBytes Received\tBytes Sent/sec\tBytes Received/sec\tBits Sent/sec/conn\tBits Received/sec/conn\tBytes Sent/packet\tBytes Received/packet\n"
              );
      DAT_006869bc = '\0';
    }
    _DAT_0087bea0 = 0;
    DAT_0087bea4 = 0;
    DAT_0087bea8 = 0;
    DAT_0087beac = 0;
    DAT_0087beb0 = 0;
    _DAT_0087beb4 = 0;
    _DAT_0087beb8 = 0;
    _DAT_0087bea0 = FUN_00449210();
  }
  return;
}
#endif
