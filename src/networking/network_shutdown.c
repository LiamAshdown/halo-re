// network_shutdown  (Ghidra: network_shutdown, already named)
// address 0x4416e0, size 493 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("tears down the networking subsystem:
// closes channels, flushes and closes both statistics log files with final summary lines, and
// clears the initialized flag"); reuses every network_connection_statistics field name from
// out/phase4/networking_types_notes.md, confirming connection_id/connection_key double as an
// IPv4 address and port pair here (they are fed straight into the same address-formatting
// call, gt2AddressToString, that network_channels_open.c uses for socket addresses).
// register convention: __cdecl, no arguments.
// UNSURE: gt2CloseSocket is shown with zero visible arguments at both call sites here, but
// src/networking/network_channels_close.c's decompile of the same callee shows one socket
// argument; the value being tested by the surrounding `if` is passed explicitly here to match.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>

extern uint8_t network_winsock_initialized;    // 0x006869b8
extern int32_t network_query_socket;           // 0x006f14c8
extern int32_t network_game_socket;            // 0x006f14c4
extern void *network_summary_log_file;         // 0x006a6140, FILE *
extern void *network_connection_stats_log_file; // 0x006f14b8, FILE *
extern int32_t network_initialized_at_ms;      // 0x006f14c0
extern int32_t network_connection_stats_count; // 0x006f14bc
extern network_connection_statistics network_connection_stats[k_network_connection_stats_count]; // 0x0087bec0

extern void gt2CloseSocket(int32_t socket); // foreign GameSpy transport call, closes a socket
extern void gt2AddressToString(uint32_t address, uint16_t port, void *out_address); // 0x6148b0: fills a
    // 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8) // foreign
extern int32_t time_query_performance_counter_ms(void); // foreign module, millisecond tick reader

int32_t network_shutdown(void)
{
    int32_t i;
    int32_t total_sent;
    int32_t total_received;
    int32_t connection_duration_ms;
    int32_t now;
    uint32_t total_elapsed_sec;
    float total_sent_f;
    float connection_seconds;
    uint8_t address_buf[24];

    if (network_winsock_initialized == 0) {
        return 0xfffffffb;
    }
    if (network_query_socket != 0) {
        gt2CloseSocket(network_query_socket);
        network_query_socket = 0;
    }
    if (network_game_socket != 0) {
        gt2CloseSocket(network_game_socket);
        network_game_socket = 0;
    }
    if (network_summary_log_file != 0) {
        fclose((FILE *)network_summary_log_file);
    }
    if (network_connection_stats_log_file != 0) {
        fprintf((FILE *)network_connection_stats_log_file, "\n\n");
        total_sent = 0;
        total_received = 0;
        now = time_query_performance_counter_ms();
        total_elapsed_sec = (uint32_t)(now - network_initialized_at_ms) / 1000;
        for (i = 0; i < network_connection_stats_count; i++) {
            total_sent = total_sent + network_connection_stats[i].bytes_sent;
            total_received = total_received + network_connection_stats[i].bytes_received;
            connection_duration_ms = network_connection_stats[i].connected_duration_ms;
            if (network_connection_stats[i].active == 1) {
                connection_duration_ms = connection_duration_ms +
                    (now - network_connection_stats[i].active_since_ms);
            }
            gt2AddressToString((uint32_t)network_connection_stats[i].connection_id,
                         (uint16_t)network_connection_stats[i].connection_key, address_buf);
            // FIXED in the review pass: Ghidra dropped the sixth (%f) argument entirely,
            // because the binary does not push it -- it stores the x87 result straight into
            // the outgoing argument slot with `fstp QWORD PTR [esp+0x4]` at 0x441808, after
            // computing it at 0x4417c4..0x4417e6:
            //   fild [esp+0x18]           ; (double)bytes_sent, stashed at 0x4417ac
            //   ...  [esp+0x18] = seconds ; the /1000 quotient
            //   fild [esp+0x18]           ; (double)seconds
            //   jge / fadd ds:0x672bc0    ; +2^32 when the quotient came out negative
            //   fdivp st(1),st            ; bytes_sent / seconds
            connection_seconds = (float)((uint32_t)connection_duration_ms / 1000);
            if ((int32_t)((uint32_t)connection_duration_ms / 1000) < 0) {
                connection_seconds = connection_seconds + 4.2949673e+09f;
            }
            fprintf((FILE *)network_connection_stats_log_file,
                    "Connection [%d]  Live for[%d] seconds  Was address[%s]  Total Sent[%d]  "
                    "Total Received[%d]  Bytes sent per second[%f]\n",
                    i, (uint32_t)connection_duration_ms / 1000, address_buf,
                    network_connection_stats[i].bytes_sent,
                    network_connection_stats[i].bytes_received,
                    (double)((float)network_connection_stats[i].bytes_sent / connection_seconds));
        }
        total_sent_f = (float)total_sent;
        if (total_sent < 0) {
            total_sent_f = total_sent_f + 4.2949673e+09f; // 32-bit tick wraparound
        }
        fprintf((FILE *)network_connection_stats_log_file,
                "total data sent[%d]  total received[%d] total time in seconds[%d]  "
                "bytes sent per second[%f]\n",
                total_sent, total_received, total_elapsed_sec,
                (double)(total_sent_f / (float)total_elapsed_sec));
        fprintf((FILE *)network_connection_stats_log_file, "Log file closed\n");
        fclose((FILE *)network_connection_stats_log_file);
        network_connection_stats_log_file = 0;
    }
    network_winsock_initialized = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4416e0):

/* WARNING: Removing unreachable block (ram,0x004417e0) */
/* WARNING: Removing unreachable block (ram,0x00441868) */

undefined4 network_shutdown(void)

{
  float fVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  char *pcVar8;
  int local_30;
  undefined1 local_20 [28];

  iVar6 = 0;
  if (DAT_006869b8 != '\0') {
    if (DAT_006f14c8 != 0) {
      FUN_00614860();
      DAT_006f14c8 = 0;
    }
    if (DAT_006f14c4 != 0) {
      FUN_00614860();
      DAT_006f14c4 = 0;
    }
    if (DAT_006a6140 != (FILE *)0x0) {
      _fclose(DAT_006a6140);
    }
    if (DAT_006f14b8 != (FILE *)0x0) {
      _fprintf(DAT_006f14b8,"\n\n");
      iVar7 = 0;
      local_30 = 0;
      iVar5 = 0;
      iVar2 = FUN_00449210();
      uVar4 = (uint)(iVar2 - DAT_006f14c0) / 1000;
      if (0 < DAT_006f14bc) {
        pcVar8 = &DAT_0087bec8;
        do {
          iVar7 = local_30 + *(int *)(pcVar8 + 0xc);
          iVar5 = iVar5 + *(int *)(pcVar8 + 0x10);
          uVar3 = *(uint *)(pcVar8 + -8);
          if (*pcVar8 == '\x01') {
            uVar3 = uVar3 + (iVar2 - *(int *)(pcVar8 + -4));
          }
          FUN_006148b0(*(undefined4 *)(pcVar8 + 4),*(undefined2 *)(pcVar8 + 8),local_20);
          _fprintf(DAT_006f14b8,
                   "Connection [%d]  Live for[%d] seconds  Was address[%s]  Total Sent[%d]  Total Received[%d]  Bytes sent per second[%f]\n"
                   ,iVar6,uVar3 / 1000,local_20,*(undefined4 *)(pcVar8 + 0xc),
                   *(undefined4 *)(pcVar8 + 0x10));
          iVar6 = iVar6 + 1;
          pcVar8 = pcVar8 + 0x44;
          local_30 = iVar7;
        } while (iVar6 < DAT_006f14bc);
      }
      fVar1 = (float)iVar7;
      if (iVar7 < 0) {
        fVar1 = fVar1 + 4.2949673e+09;
      }
      _fprintf(DAT_006f14b8,
               "total data sent[%d]  total received[%d] total time in seconds[%d]  bytes sent per second[%f]\n"
               ,iVar7,iVar5,uVar4,(double)(fVar1 / (float)uVar4));
      _fprintf(DAT_006f14b8,"Log file closed\n");
      _fclose(DAT_006f14b8);
      DAT_006f14b8 = (FILE *)0x0;
    }
    DAT_006869b8 = 0;
    return 0;
  }
  return 0xfffffffb;
}
#endif
