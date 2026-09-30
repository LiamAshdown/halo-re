// network_game_server_load_scenario  (Ghidra: FUN_004e0720, unnamed)
// address 0x4e0720, size 106 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Loads the requested scenario for the host
// (via FUN_004de6d0), resetting per-round counters first, and optionally logs the operation
// to the debug log file when verbose host logging is enabled." DAT_0071c2d4 is
// types/networking.h's `network_server`; +6 is ::flags (stats-logging bit, same test as in
// other functions in this batch); DAT_0087ac06/006f14b4/006a6140/00719879 are all named
// globals from that header (debug_log_level, network_statistics_logging_enabled,
// network_summary_log_file, network_build_string).
// register convention: none; every operand is a fixed global.
// UNSURE: DAT_00699f44 and DAT_0071cc24 have no established names elsewhere in this module;
// declared here as generic per-round counters reset before every scenario load.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <stdio.h>

extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t network_scenario_round_counter_a; // 0x00699f44 (UNSURE name)
extern int32_t network_scenario_round_counter_b; // 0x0071cc24 (UNSURE name)
extern uint8_t debug_log_level;          // 0x0087ac06, byte-wide (R01)
extern uint8_t network_statistics_logging_enabled; // 0x006f14b4
extern FILE *network_summary_log_file; // 0x006a6140
extern char network_build_string[]; // 0x00719879


// Resets the two per-round counters and loads the pending scenario for `network_server`'s
// session, optionally logging the build string to the summary log when verbose,
// statistics-enabled logging is active.
char network_game_server_load_scenario(void)
{
    network_server_globals *server;
    char ok;

    server = network_server;
    network_scenario_round_counter_a = 0;
    network_scenario_round_counter_b = 0;
    ok = network_game_scenario_load_request(&server->session);
    if ((server->flags >> 2 & 1) != 0 && debug_log_level > 2 &&
        network_statistics_logging_enabled != 0 && network_summary_log_file != 0) {
        fprintf(network_summary_log_file, "%s\t", network_build_string);
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x4e0720):

char FUN_004e0720(void)

{
  int iVar1;
  char cVar2;

  iVar1 = DAT_0071c2d4;
  DAT_00699f44 = 0;
  DAT_0071cc24 = 0;
  cVar2 = network_game_scenario_load_request(DAT_0071c2d4 + 8);
  if (((((*(byte *)(iVar1 + 6) >> 2 & 1) != 0) && (2 < DAT_0087ac06)) && (DAT_006f14b4 != '\0')) &&
     (DAT_006a6140 != (FILE *)0x0)) {
    _fprintf(DAT_006a6140,"%s\t",&DAT_00719879);
  }
  return cVar2;
}
#endif
