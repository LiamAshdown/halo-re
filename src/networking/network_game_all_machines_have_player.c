// network_game_all_machines_have_player  (Ghidra: FUN_004e04f0, unnamed)
// address 0x4e04f0, size 145 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Verifies that every team/channel key stored
// at param_1+0x3c4 has a corresponding valid, active player-name entry, used as a
// synchronisation gate before starting a round." param_1+0x3c4 and +0x1c6 match
// network_server_globals::machines[0].machine_id and ::session.players[0].machine_index
// exactly.
// register convention: stack = server (network_server_globals *).
// blam-cc: stack -> server
// UNSURE: as in network_game_any_team_empty.c, network_player_entry_is_valid's EAX argument
// has no visible source in this function; modelled as an explicit `entry` walking the player
// table in lockstep with the byte pointer Ghidra does show.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, blam-cc: EAX -> entry
    // blam-cc: EAX -> entry; 0x4de9f0, other module. The EAX convention is pinned by
    // network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
    // `lea eax,[esp+0x20]`), both immediately before the call.

// For every one of the 16 machine slots with a connected (0..15) machine_id, requires at least
// one valid player-table entry whose machine_index matches it. Returns 0 as soon as a
// connected machine has no matching player; returns 1 once all 16 slots have been checked.
uint32_t network_game_all_machines_have_player(network_server_globals *server)
{
    int32_t machine_index;

    for (machine_index = 0; machine_index < 16; machine_index = machine_index + 1) {
        int16_t machine_id;

        machine_id = server->machines[machine_index].machine_id;
        if (machine_id >= 0 && machine_id < 16) {
            char found;
            network_player_entry *entry;
            int32_t i;

            found = 0;
            entry = server->session.players;
            for (i = 0; i < 16; i = i + 1) {
                char valid;

                valid = network_player_entry_validate(entry); // blam-cc: EAX -> entry
                if (valid != 0 && entry->machine_index == (int8_t)machine_id) {
                    found = 1;
                }
                entry = entry + 1;
            }
            if (!found) {
                return 0;
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e04f0):

uint FUN_004e04f0(int param_1)

{
  short sVar1;
  bool bVar2;
  uint uVar3;
  char *pcVar4;
  int iVar5;
  short *local_8;
  int local_4;

  local_8 = (short *)(param_1 + 0x3c4);
  local_4 = 0;
  do {
    sVar1 = *local_8;
    if ((-1 < sVar1) && (sVar1 < 0x10)) {
      bVar2 = false;
      pcVar4 = (char *)(param_1 + 0x1c6);
      iVar5 = 0x10;
      do {
        uVar3 = FUN_004de9f0();
        if (((char)uVar3 != '\0') && (*pcVar4 == sVar1)) {
          bVar2 = true;
        }
        pcVar4 = pcVar4 + 0x20;
        iVar5 = iVar5 + -1;
      } while (iVar5 != 0);
      if (!bVar2) {
        return uVar3 & 0xffffff00;
      }
    }
    local_4 = local_4 + 1;
    local_8 = local_8 + 0x30;
    if (0xf < local_4) {
      return CONCAT31((int3)((uint)local_4 >> 8),1);
    }
  } while( true );
}
#endif
