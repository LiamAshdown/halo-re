// player_reset_after_unit_change  (Ghidra: FUN_00474e10; named per this rewrite)
// address 0x474e10, size 414 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x474e10..0x474fb0 (the local / non-local network branches were swapped).)
// evidence: out/phase4/game_functions.md ("Resets a player's per-tick state and local-player
//   control struct after its controlled unit changes, and updates the global all-players-
//   spawned flag"); types/game.h player (previous_unit +0x38, unit +0x34, local_player_index
//   +0x02, marked_for_deletion +0xd5), local_player_control (the whole 0x40-byte reset matches
//   game_engine_reset_player_look_state's own field list, done inline here instead of by
//   calling it), player_globals::no_player_has_a_unit (+0x10), update_server_queue (the
//   +0x34/+0x38 write matches queue.queue.write_index/read_index, at update_server_queues+
//   player_index*sizeof(update_server_queue)); player_update_queue / circular_queue
//   write_index/read_index (matching the +0x12c/+0x130, +0x17c/+0x180, +0x1dc/+0x1e0 pairs).
// objdump -d -M intel --start-address=0x474e10 --stop-address=0x474fb0 bin/halo.exe confirms
// param_1 is a genuine __cdecl stack parameter (unlike most of this batch) and every register.
// register convention: stack -> player_index.
//
// UNSURE: the two dwords at update_server_queue+8 and +0x14/+0x18 (this function's
// network_game_mode==2 branch) fall inside that struct's own unresolved unknown_02[] padding
// (types/game.h); kept as raw offsets. Likewise player+0x13c/+0x148/+0x14c: only three of the
// eight update_history.current[] slots are cleared here, exactly as the disassembly shows,
// not all eight.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>
#include <stdint.h>

extern data_array *player_data;                             // 0x0087a480
extern player_control_globals *player_control_globals_ptr;  // 0x006b145c
extern player_globals *local_player_globals;                // 0x0087a478
extern int16_t network_game_mode;                            // 0x00719720
extern data_array *update_server_queues;                     // 0x006f1d90
extern uint8_t *network_client;                               // 0x0071c2d8

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void player_remove(datum_index player_handle); // this batch, 0x473bb0, blam-cc: EAX -> player_handle
extern void player_update_history_free_all(void *queue); // 0x4e6f20, established elsewhere

// Called after a player's controlled unit has just changed. Rolls the old unit into
// previous_unit and clears unit; if the player is a local player, resets its whole
// local_player_control record to defaults (unit -1, weapon/grenade/zoom -1, nameplate -1, pitch
// clamps, everything else zeroed). Recomputes player_globals::no_player_has_a_unit by scanning
// every player. If this player is now marked for deletion, removes it outright and returns.
// Otherwise, while hosting, clears this player's server update-queue write/read cursors; while a
// non-local player and there is a network client, frees its update-history queues; while a local
// player and this machine is a network client, clears the three embedded queues' write/read
// cursors and three of the update_history current-record slots.
void player_reset_after_unit_change(uint32_t player_index)
{
    player *plr;
    data_iterator iter;
    player *scan;

    plr = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    plr->previous_unit = plr->unit;
    plr->unit = (datum_index)-1;

    if (plr->local_player_index != -1) {
        local_player_control *look =
            &player_control_globals_ptr->local_players[plr->local_player_index];

        memset(look, 0, sizeof(*look));
        look->unit = (datum_index)-1;
        look->desired_weapon_index = -1;
        look->desired_grenade_index = -1;
        look->desired_zoom_level = -1;
        look->autolevelling_active = 0;
        look->nameplate_target = (datum_index)-1;
        look->pitch_maximum = 1.4906585f;
        look->pitch_minimum = -1.4906585f;
        look->suppressed_buttons = 0;
        look->suppressed_until_released = 0;
    }

    local_player_globals->no_player_has_a_unit = 1;
    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    scan = (player *)data_iterator_next(&iter);
    while (scan != (player *)0) {
        if (scan->unit != (datum_index)-1) {
            local_player_globals->no_player_has_a_unit = 0;
        }
        scan = (player *)data_iterator_next(&iter);
    }

    if (plr->marked_for_deletion == 1) {
        player_remove(player_index);
        return;
    }

    if (network_game_mode == 2) {
        update_server_queue *entry =
            &((update_server_queue *)update_server_queues->data)[player_index];
        plr->unknown_f4 = (datum_index)-1;
        entry->queue.queue.read_index = 0;
        entry->queue.queue.write_index = 0;
        *(int32_t *)((uint8_t *)entry + 8) = 0;   // UNSURE: unknown_02 padding
        *(int32_t *)((uint8_t *)entry + 0x14) = 0; // UNSURE: unknown_02 padding
        *(int32_t *)((uint8_t *)entry + 0x18) = 0; // UNSURE: unknown_02 padding
    }

    // 0x474f42: a LOCAL player frees the client's history queues; a non-local one on a client clears its cursors.
    // FIXED 2026-09-28: the draft had the two branches swapped.
    if (plr->local_player_index != -1) {
        if (network_client != (uint8_t *)0) {
            player_update_history_free_all(*(void **)(network_client + 0xf48));
        }
    } else if (network_game_mode == 1) {
        plr->update_history.queue.read_index = 0;
        plr->update_history.queue.write_index = 0;
        plr->position_updates.read_index = 0;
        plr->position_updates.write_index = 0;
        plr->vehicle_updates.read_index = 0;
        plr->vehicle_updates.write_index = 0;
        plr->update_history.current[0] = 0;
        plr->update_history.current[3] = 0;
        plr->update_history.current[4] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x474e10), from tools/pack.py 0x474e10:

void FUN_00474e10(uint param_1)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;

  iVar3 = (param_1 & 0xffff) * 0x200;
  iVar2 = *(int *)(DAT_0087a480 + 0x34) + iVar3;
  *(undefined4 *)(iVar2 + 0x38) = *(undefined4 *)(*(int *)(DAT_0087a480 + 0x34) + 0x34 + iVar3);
  *(undefined4 *)(iVar2 + 0x34) = 0xffffffff;
  if (*(short *)(iVar2 + 2) != -1) {
    puVar1 = (undefined4 *)(*(short *)(iVar2 + 2) * 0x40 + 0x10 + DAT_006b145c);
    puVar4 = puVar1;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *puVar1 = 0xffffffff;
    *(undefined2 *)(puVar1 + 8) = 0xffff;
    *(undefined2 *)((int)puVar1 + 0x22) = 0xffff;
    *(undefined2 *)(puVar1 + 9) = 0xffff;
    *(undefined1 *)((int)puVar1 + 0x26) = 0;
    puVar1[10] = 0xffffffff;
    puVar1[0xf] = 0x3fbf0243;
    puVar1[0xe] = 0xbfbf0243;
    *(undefined2 *)(puVar1 + 2) = 0;
    *(undefined2 *)((int)puVar1 + 10) = 0;
  }
  *(undefined1 *)(DAT_0087a478 + 0x10) = 1;
  iVar2 = data_iterator_next();
  while (iVar2 != 0) {
    if (*(int *)(iVar2 + 0x34) != -1) {
      *(undefined1 *)(DAT_0087a478 + 0x10) = 0;
    }
    iVar2 = data_iterator_next();
  }
  iVar2 = *(int *)(DAT_0087a480 + 0x34) + iVar3;
  if (*(char *)(*(int *)(DAT_0087a480 + 0x34) + 0xd5 + iVar3) == '\x01') {
    player_remove();
    return;
  }
  if (DAT_00719720 == 2) {
    *(undefined4 *)(iVar2 + 0xf4) = 0xffffffff;
    iVar3 = (param_1 & 0xffff) * 100 + *(int *)(DAT_006f1d90 + 0x34);
    *(undefined4 *)(iVar3 + 0x38) = 0;
    *(undefined4 *)(iVar3 + 0x34) = 0;
    *(undefined4 *)(iVar3 + 8) = 0;
    *(undefined4 *)(iVar3 + 0x14) = 0;
    *(undefined4 *)(iVar3 + 0x18) = 0;
  }
  if (*(short *)(iVar2 + 2) == -1) {
    if (DAT_00719720 == 1) {
      *(undefined4 *)(iVar2 + 0x130) = 0;
      *(undefined4 *)(iVar2 + 300) = 0;
      *(undefined4 *)(iVar2 + 0x180) = 0;
      *(undefined4 *)(iVar2 + 0x17c) = 0;
      *(undefined4 *)(iVar2 + 0x1e0) = 0;
      *(undefined4 *)(iVar2 + 0x1dc) = 0;
      *(undefined4 *)(iVar2 + 0x13c) = 0;
      *(undefined4 *)(iVar2 + 0x148) = 0;
      *(undefined4 *)(iVar2 + 0x14c) = 0;
    }
  }
  else if (DAT_0071c2d8 != 0) {
    player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
    return;
  }
  return;
}
#endif
