// main_ensure_local_players  (Ghidra: FUN_004c8800; still unnamed -> renamed)
// address 0x4c8800, size 255 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/main_functions.md's summary ("Reassigns input-device (controller) slots
// to local players, rebuilding the player-to-device and device-to-player lookup tables") does
// NOT match the disassembly: there is no device-index table anywhere in this function, only
// player_globals::local_players and player::local_player_index (types/game.h). Renamed to
// describe what it actually does: called once per frame from the main loop (0x4c796d, inside
// the batch already committed as game_state_save_core) and once from main_level_transition_update
// (0x4c965b, this batch), it makes sure the right local player(s) exist for the current mode.
// Confirmed field-by-field against objdump -d -M intel bin/halo.exe at 0x4c8800..0x4c88fe:
// local_player_globals (0x0087a478, established in src/interface/display_error.c) and
// player_data (0x0087a480, established in src/game/player_new_network.c) are both POINTERS
// stored at those addresses, not the structs themselves (Ghidra's DAT_0087a478 + 4 arithmetic
// is pointer arithmetic on the pointer's value). local_player_count (0x006894b8, established in
// src/saved_games/game_state_build_header.c) is the loop bound.
// register convention: no register-passed arguments (Ghidra recognizes none, and no in_/unaff_
// phase 4 review (disassembly 0x4c8800..0x4c88fe): player_new_network EAX = -1 plus three stack
// arguments, the 0x200 player stride (sizeof(player) asserted with -m32) and the slot window 0..0 match.
// register reads appear in the disassembly either).
// UNSURE: FUN_00473780 (player_new_network, 0x473780, already committed in
// src/game/player_new_network.c) is called with a genuinely different, narrower 2-argument
// signature by src/networking/network_channel_key_open.c (`FUN_00473780(int32_t machine_index,
// int16_t machine_player_index)`); that file's guess is superseded by player_new_network.c's
// own objdump-confirmed 4-argument signature, which this file also uses (both call sites here
// push exactly 3 stack dwords plus EAX, matching it exactly).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "main.h"
#include "fn_game.h"
#include "fn_main.h"

extern main_globals main_globals_data; // 0x00719700
extern player_globals *local_player_globals; // 0x0087a478, foreign (game module)
extern data_array *player_data;              // 0x0087a480, foreign (game module)
extern int16_t local_player_count;           // 0x006894b8, foreign (saved_games module)


extern datum_index player_new_network(datum_index requested_index, uint32_t machine_index,
    int16_t local_player_index, uint16_t *identifier_record); // 0x473780, foreign (game module)

// Ensures the current mode's local player(s) exist and are correctly slotted:
//  - on the main menu (main_menu_scenario_loaded set), ensures exactly one local player at
//    local player slot 0, detaching whatever player previously occupied that slot;
//  - otherwise, for each of local_player_count local players, finds a free local-player slot
//    index and creates (or re-keys) a network player into it the same way, but only slot 0 is
//    ever actually wired up to player_globals::local_players (types/game.h pins
//    k_maximum_local_players at 1 for this build; a nonzero slot index from
//    local_player_find_free_slot_index is silently skipped, matching the "-1 < slot && slot < 1"
//    bounds check in the disassembly).
void main_ensure_local_players(void)
{
    if (main_globals_data.main_menu_scenario_loaded == 0) {
        int16_t i;
        int32_t slot;
        datum_index new_player;
        datum_index old_player;

        for (i = 0; i < local_player_count; i = i + 1) {
            slot = local_player_find_free_slot_index();
            new_player = player_new_network((datum_index)-1, 0, (int16_t)slot, 0);
            if (-1 < slot && slot < 1) {
                old_player = local_player_globals->local_players[slot];
                if (old_player != (datum_index)-1) {
                    player *old_p = (player *)((uint8_t *)player_data->data +
                                                (old_player & 0xffff) * sizeof(player));
                    old_p->local_player_index = -1;
                }
                local_player_globals->local_players[slot] = new_player;
                if (new_player != (datum_index)-1) {
                    player *new_p = (player *)((uint8_t *)player_data->data +
                                                (new_player & 0xffff) * sizeof(player));
                    new_p->local_player_index = (int16_t)slot;
                }
            }
        }
    } else {
        datum_index new_player;
        datum_index old_player;

        new_player = player_new_network((datum_index)-1, 0, 0, 0);
        old_player = local_player_globals->local_players[0];
        if (old_player != (datum_index)-1) {
            player *old_p = (player *)((uint8_t *)player_data->data +
                                        (old_player & 0xffff) * sizeof(player));
            old_p->local_player_index = -1;
        }
        local_player_globals->local_players[0] = new_player;
        if (new_player != (datum_index)-1) {
            player *new_p = (player *)((uint8_t *)player_data->data +
                                        (new_player & 0xffff) * sizeof(player));
            new_p->local_player_index = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4c8800):

void FUN_004c8800(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  undefined4 uVar6;
  short sVar7;
  int local_4;

  if (DAT_00719754._2_1_ == '\0') {
    local_4 = 0;
    if (0 < DAT_006894b8) {
      do {
        uVar6 = FUN_00473730();
        uVar5 = FUN_00473780(0,uVar6,0);
        iVar3 = DAT_0087a480;
        sVar7 = (short)uVar6;
        if ((-1 < sVar7) && (sVar7 < 1)) {
          puVar1 = (uint *)(DAT_0087a478 + 4 + sVar7 * 4);
          uVar2 = *puVar1;
          if (uVar2 != 0xffffffff) {
            *(undefined2 *)((uVar2 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) = 0xffff;
          }
          *puVar1 = uVar5;
          if (uVar5 != 0xffffffff) {
            *(short *)((uVar5 & 0xffff) * 0x200 + 2 + *(int *)(iVar3 + 0x34)) = sVar7;
          }
        }
        local_4 = local_4 + 1;
      } while (local_4 < DAT_006894b8);
    }
  }
  else {
    uVar5 = FUN_00473780(0,0,0);
    iVar4 = DAT_0087a480;
    iVar3 = DAT_0087a478;
    if (*(uint *)(DAT_0087a478 + 4) != 0xffffffff) {
      *(undefined2 *)
       ((*(uint *)(DAT_0087a478 + 4) & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) = 0xffff
      ;
    }
    *(uint *)(iVar3 + 4) = uVar5;
    if (uVar5 != 0xffffffff) {
      *(undefined2 *)((uVar5 & 0xffff) * 0x200 + 2 + *(int *)(iVar4 + 0x34)) = 0;
      return;
    }
  }
  return;
}
#endif
