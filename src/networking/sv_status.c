// sv_status  (Ghidra: sv_status, already named)
// address 0x4e2e50, size 121 bytes
// name confidence: 0.9   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md; CEA-pdb match on all three literal strings;
// types/networking.h network_server_globals::session.maximum_players at +0x1a5.
// register convention: __cdecl, no arguments.
// UNSURE (major): the format string has three conversions ("%s (%d / %d players)") but Ghidra's
// decompile shows only two variadic arguments (the map-name pointer and FUN_0045c6a0's single
// result); preserved literally as a two-argument call rather than inventing a third.
// UNSURE: the map-name argument is `network_build_string` (0x00719879), the same global
// src/networking/network_machine_check_build_version.c already names for the build/version
// string comparison -- an odd reuse for a message that documents itself as printing the current
// map, but this is the address the disassembly and functions.md both point at, and no separate
// "current map name" global exists anywhere in types/networking.h.
// UNSURE: FUN_0045c6a0 (foreign, < this module) is presumed to return a player-count-shaped
// value purely from how its result feeds the "%d / %d players" text.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int16_t network_game_mode; // 0x00719720, 2 == host
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_build_string[]; // 0x00719879 (UNSURE: reused here as a map name, see header)
extern game_engine_state game_engine_state_value; // 0x0087aa10

extern int32_t players_active_count(int32_t maximum_players); // foreign (UNSURE)
extern void *global_white_argb; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void *console_message_default_color; // 0x00685218, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: prints the current map and player count, and whether the game is ending, or
// reports that this is a server-only command.
void sv_status(void)
{
    if (network_game_mode == 2) {
        if (network_server != 0) {
            int32_t player_count_info = players_active_count((int32_t)network_server->session.maximum_players);
            chimera__console_out((ColorARGB *)0, (char *)"Dedicated server is running on map %s (%d / %d players)",
                                  network_build_string, player_count_info);
            if (game_engine_state_value == _game_engine_state_not_started) {
                chimera__console_out((ColorARGB *)0, (char *)"Use the 'sv_end_game' command to stop the game.");
                return;
            }
            chimera__console_out((ColorARGB *)console_message_default_color, (char *)"Game is ending...");
        }
        return;
    }
    chimera__console_out((ColorARGB *)global_white_argb, (char *)"%s is a server-only function!", "sv_status");
}

#if 0
Original Ghidra decompilation (0x4e2e50), from tools/pack.py 0x4e2e50:

void sv_status(void)

{
  undefined4 uVar1;

  if (DAT_00719720 == 2) {
    if (DAT_0071c2d4 != 0) {
      uVar1 = FUN_0045c6a0((int)*(char *)(DAT_0071c2d4 + 0x1a5));
      chimera__console_out
                ("Dedicated server is running on map %s (%d / %d players)",&DAT_00719879,uVar1);
      if (DAT_0087aa10 == 0) {
        chimera__console_out("Use the \'sv_end_game\' command to stop the game.");
        return;
      }
      chimera__console_out("Game is ending...");
      return;
    }
  }
  else {
    chimera__console_out("%s is a server-only function!","sv_status");
  }
  return;
}
#endif
