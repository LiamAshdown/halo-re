// network_game_any_team_empty  (Ghidra: FUN_004e0480, unnamed)
// address 0x4e0480, size 104 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md describes this as "Checks whether both teams
// (0 and 1) have at least one active, validated player, returning true only when the game is
// in team mode and both teams are populated." Traced literally, the polarity is the opposite:
// the function returns 1 as soon as it finds a team (0 or 1) with a zero count, and only
// returns 0 once both team counts are confirmed non-zero (or the game is not in team mode at
// all). in_EAX+0x140 is exactly network_server_globals::session.variant.teams
// (session at +0x008, game_variant at session+0x104, teams at game_variant+0x34); the
// per-entry field at +0x1e (unknown_1e) is read here as a 0/1 team index.
// register convention: EAX = server (network_server_globals *).
// blam-cc: EAX -> server
// UNSURE: entry->unknown_1e is read as a team index here, which is new information not
// reflected in that field's name in types/networking.h (left unrenamed; header not edited).
// UNSURE: EAX is re-read as the implicit "entry" argument to network_player_entry_is_valid on
// every loop iteration but this function's own body never shows an entry pointer being
// advanced; modelled here with an explicit `entry` walking the player table in lockstep with
// the byte pointer Ghidra does show, consistent with the analogous pattern in
// network_game_server_handoff_object_ownership.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_player_entry_is_valid(network_player_entry *entry);
    // blam-cc: EAX -> entry; 0x4de9f0, other module. The EAX convention is pinned by
    // network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
    // `lea eax,[esp+0x20]`), both immediately before the call.

// Returns 1 if the game is in team mode and either team 0 or team 1 currently has zero valid,
// active players; returns 0 if both teams are populated, or if the game is not in team mode.
uint32_t network_game_any_team_empty(network_server_globals *server)
{
    int16_t counts[2];
    network_player_entry *entry;
    int32_t i;

    if (server->session.variant.teams == 0) {
        return 0;
    }
    counts[0] = 0;
    counts[1] = 0;
    entry = server->session.players;
    for (i = 0; i < 16; i = i + 1) {
        char valid;
        int8_t team;

        valid = network_player_entry_is_valid(entry); // blam-cc: EAX -> entry
        if (valid != 0) {
            team = entry->unknown_1e;
            if (team >= 0 && team < 2) {
                counts[(int32_t)team] = counts[(int32_t)team] + 1;
            }
        }
        entry = entry + 1;
    }
    for (i = 0; i < 2; i = i + 1) {
        if (counts[i] == 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e0480):

undefined4 FUN_004e0480(void)

{
  char cVar1;
  int in_EAX;
  char *pcVar2;
  int iVar3;
  short local_4 [2];

  if (*(char *)(in_EAX + 0x140) != '\0') {
    local_4[0] = 0;
    local_4[1] = 0;
    pcVar2 = (char *)(in_EAX + 0x1c8);
    iVar3 = 0x10;
    do {
      cVar1 = FUN_004de9f0();
      if (((cVar1 != '\0') && (cVar1 = *pcVar2, -1 < cVar1)) && (cVar1 < '\x02')) {
        local_4[cVar1] = local_4[cVar1] + 1;
      }
      pcVar2 = pcVar2 + 0x20;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    iVar3 = 0;
    do {
      if (local_4[iVar3] == 0) {
        return 1;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < 2);
  }
  return 0;
}
#endif
