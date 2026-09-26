// network_stats_summary_log_write  (Ghidra: network_stats_summary_log_write, already named)
// address 0x440820, size 605 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "bandwidth statistics" section; the fourteen
// "%f\t"/"%d\t" writes match, in order, the fourteen columns of the "Game Summary" header
// (network_stats_summary_log_open.c) that follow "Map" -- that column is written by this
// function's caller, not here.
// register convention: __cdecl, no arguments.
// UNSURE: Ghidra's decompile shows the first fprintf call ("%f\t" for the "Length (seconds)"
// column) with eight extra trailing arguments (fVar4, fVar2, fVar5, fVar6, fVar7, fVar8,
// fVar9, fVar10) that duplicate values the next eight individually-shown fprintf calls
// already print on their own with matching formats; keeping both would print 15 values for a
// 14-column, one-"%f\t"-per-call log row. Treated as a decompiler call-folding artifact and
// not reproduced -- each value is written exactly once, in column order.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>
extern int _fflush(void *file);                                          // game CRT
extern int _fprintf(void *file, const char *format, ...);                // game CRT

extern uint8_t debug_log_level;                   // 0x0087ac06, byte-wide (R01)
extern uint8_t network_statistics_logging_enabled; // 0x006f14b4
extern void *network_summary_log_file;             // 0x006a6140, FILE *
extern network_summary_statistics network_summary_stats; // 0x0087bea0

extern int32_t time_query_performance_counter_ms(void); // foreign module, millisecond tick reader; see sibling file

void network_stats_summary_log_write(void)
{
    int32_t now;
    float elapsed_ms;
    float elapsed_sec_inv;
    float avg_players;
    float avg_players_inv;
    float bytes_sent_rate;
    float bytes_received_rate;
    float packets_sent_rate;
    float packets_received_rate;
    float bytes_received_f;
    float bytes_sent_per_packet;
    float bytes_received_per_packet;

    if (2 < debug_log_level && network_statistics_logging_enabled != 0 &&
        network_summary_log_file != 0) {
        now = time_query_performance_counter_ms();
        elapsed_ms = (float)(now - network_summary_stats.start_ms);
        if (now - network_summary_stats.start_ms < 0) {
            elapsed_ms = elapsed_ms + 4.2949673e+09f; // 32-bit tick wraparound
        }

        avg_players = (float)network_summary_stats.player_count_total /
                      (float)network_summary_stats.player_count_samples;
        elapsed_sec_inv = 1.0f / (elapsed_ms * 0.001f);
        packets_sent_rate = elapsed_sec_inv * (float)network_summary_stats.packets_sent;
        packets_received_rate = (float)network_summary_stats.packets_received * elapsed_sec_inv;
        bytes_sent_rate = (float)network_summary_stats.bytes_sent * elapsed_sec_inv;
        bytes_received_f = (float)network_summary_stats.bytes_received;
        bytes_received_rate = bytes_received_f * elapsed_sec_inv;
        bytes_sent_per_packet = (float)network_summary_stats.bytes_sent /
                                (float)network_summary_stats.packets_sent;
        bytes_received_per_packet = bytes_received_f / (float)network_summary_stats.packets_received;

        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)(elapsed_ms * 0.001f));
        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)avg_players);
        _fprintf((FILE *)network_summary_log_file, "%d\t", network_summary_stats.packets_sent);
        _fprintf((FILE *)network_summary_log_file, "%d\t", network_summary_stats.packets_received);
        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)packets_sent_rate);
        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)packets_received_rate);
        _fprintf((FILE *)network_summary_log_file, "%d\t", network_summary_stats.bytes_sent);
        _fprintf((FILE *)network_summary_log_file, "%d\t", network_summary_stats.bytes_received);
        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)bytes_sent_rate);
        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)bytes_received_rate);

        avg_players_inv = 1.0f / avg_players;
        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)(avg_players_inv * bytes_sent_rate * 8.0f));
        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)(avg_players_inv * bytes_received_rate * 8.0f));
        _fprintf((FILE *)network_summary_log_file, "%f\t", (double)bytes_sent_per_packet);
        _fprintf((FILE *)network_summary_log_file, "%f\n", (double)bytes_received_per_packet);
        _fflush((FILE *)network_summary_log_file);
    }
}

#if 0
Original Ghidra decompilation (0x440820):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl network_stats_summary_log_write(void)

{
  float fVar1;
  float fVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;

  if (((2 < DAT_0087ac06) && (DAT_006f14b4 != '\0')) && (DAT_006a6140 != (FILE *)0x0)) {
    iVar3 = FUN_00449210();
    fVar1 = (float)(iVar3 - _DAT_0087bea0);
    if (iVar3 - _DAT_0087bea0 < 0) {
      fVar1 = fVar1 + 4.2949673e+09;
    }
    fVar6 = (float)_DAT_0087beb4 / (float)_DAT_0087beb8;
    fVar8 = 1.0 / (fVar1 * 0.001);
    fVar9 = fVar8 * (float)DAT_0087beac;
    fVar10 = (float)DAT_0087beb0 * fVar8;
    fVar7 = (float)DAT_0087bea4 * fVar8;
    fVar2 = (float)DAT_0087bea8;
    fVar8 = fVar2 * fVar8;
    fVar4 = (float)DAT_0087bea4 / (float)DAT_0087beac;
    fVar5 = fVar2 / (float)DAT_0087beb0;
    _fprintf(DAT_006a6140,"%f\t",(double)(fVar1 * 0.001),fVar4,fVar2,fVar5,fVar6,fVar7,fVar8,fVar9,
             fVar10);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar6);
    _fprintf(DAT_006a6140,"%d\t",DAT_0087beac);
    _fprintf(DAT_006a6140,"%d\t",DAT_0087beb0);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar9);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar10);
    _fprintf(DAT_006a6140,"%d\t",DAT_0087bea4);
    _fprintf(DAT_006a6140,"%d\t",DAT_0087bea8);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar7);
    _fprintf(DAT_006a6140,"%f\t",(double)fVar8);
    fVar6 = 1.0 / fVar6;
    _fprintf(DAT_006a6140,"%f\t",(double)(fVar6 * fVar7 * 8.0));
    _fprintf(DAT_006a6140,"%f\t",(double)(fVar6 * fVar8 * 8.0));
    _fprintf(DAT_006a6140,"%f\t",(double)fVar4);
    _fprintf(DAT_006a6140,"%f\n",(double)fVar5);
    _fflush(DAT_006a6140);
  }
  return;
}
#endif
