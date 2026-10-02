// game_engine_players_ready_for_bsp_switch_strict  (Ghidra: FUN_0045c830; named per
// out/phase4/game_functions.md, "More thorough variant of the BSP-switch readiness check that
// also detects disagreement between players about which structure-BSP to switch to.")
// address 0x45c830, size 426 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// evidence: shares its per-player predicate and the 0x1fffe34/0x1fffeae UNSURE offsets with
// game_engine_players_ready_for_bsp_switch (0x45c750, this batch); types/game.h player
// (marked_for_deletion 0xd5, unit 0x34, team 0x20), game_variant::lives_per_round (0x50, live
// copy at 0x006f1cd8) and game_variant::unknown_40 (live copy at 0x006f1cc8).
// register convention: __cdecl, no arguments.
//
// The control flow is reproduced exactly, including the two redundant re-calls to players_active_count()/re-reads of player_data
// in the <2-players branch and the first-seen-team tracking in the main loop (ebp keeps the first team it saw).
// reconciled: R16 the elided iterators are the inline 0x10-byte data_iterator over player_data (0x45c856, 0x45c8ec); FUN_00460e40 gets iterator.index (0x45c942), not -1

// RESOLVED (phase 4 review): the two "player_data->data + 0x1fffe34 / + 0x1fffeae" accesses are
// NOT real offsets. objdump of 0x45c7ca..0x45c7e8 shows
//     mov edx,[ebx+0x34]        ; player_data->data
//     and esi,0xffff            ; esi = iterator.index (the current player handle)
//     shl esi,9                 ; * sizeof(player) == 0x200
//     mov ecx,[esi+edx*1+0x34]  ; player_at(handle)->unit
//     add esi,edx
//     movsx ecx,WORD [esi+0xae] ; player_at(handle)->deaths
// Ghidra constant-folded the wildcard handle 0xffff into the displacement:
//     (0xffff << 9) + 0x34 == 0x1fffe34   and   (0xffff << 9) + 0xae == 0x1fffeae.
// So the test is simply "this player has no unit and has used up its lives". The re-read goes
// through the handle rather than the element pointer the iterator already returned because the
// call to 0x460e40 above it could have changed the record; it is written below as a re-read of
// the same player for that reason.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;              // 0x0087a480
extern game_variant game_engine_variant;    // 0x006f1c88 (::lives_per_round at 0x006f1cd8,
                                             //             ::unknown_40 at 0x006f1cc8)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; blam-cc: EDI -> iterator
extern int32_t players_active_count(void);   // 0x45c6a0, this batch
extern uint8_t game_engine_player_has_respawn_priority(uint32_t player_handle); // 0x460e40, stack player handle; see
                                                     // game_engine_players_ready_for_bsp_switch.c

uint8_t game_engine_players_ready_for_bsp_switch_strict(void)
{
    data_iterator iterator; // inline over player_data, 0x45c856..0x45c87a and 0x45c8ec..0x45c91b
    player *p;
    uint8_t result;
    int32_t lives_cached;
    datum_index reread_unit;   // re-read of p->unit after the 0x460e40 call
    int16_t reread_deaths;    // re-read of p->deaths, same reason

    if (players_active_count() < 2) {
        result = 1;
        // redundant re-call, preserved from the original (0x45c84c).
        if (players_active_count() == 1) {
            iterator.data = player_data;
            iterator.next_index = 0;
            iterator.index = k_datum_index_none;
            iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
            p = (player *)data_iterator_next(&iterator);
            lives_cached = game_engine_variant.lives_per_round;
            while (p != (player *)0) {
                reread_unit = p->unit;
                reread_deaths = p->deaths;
                if (lives_cached < 1 || reread_unit != -1 || reread_deaths < lives_cached) {
                    result = 1;
                } else {
                    result = 0;
                }
                p = (player *)data_iterator_next(&iterator);
            }
        }
        return result;
    }

    if (0 < game_engine_variant.lives_per_round || game_engine_variant.odd_man_out != 0) {
        int32_t spawned_reference_team;
        uint8_t disagreement_found;
        int32_t previous_team;
        int32_t this_team;
        int32_t carry_team;
        uint8_t counts;
        uint8_t has_spawned_reference_team;

        spawned_reference_team = -1;
        disagreement_found = 0;
        iterator.data = player_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        p = (player *)data_iterator_next(&iterator);
        previous_team = -1;
        if (p != (player *)0) {
            do {
                if (p->marked_for_deletion == 0 && p->unit == k_datum_index_none &&
                    (game_engine_player_has_respawn_priority(iterator.index) != 0 || // 0x45c942 pushes the iterator's index
                     (0 < game_engine_variant.lives_per_round &&
                      (reread_unit = p->unit,
                       reread_unit == k_datum_index_none) &&
                      (reread_deaths = p->deaths,
                       game_engine_variant.lives_per_round <= reread_deaths)))) {
                    counts = 0;
                } else {
                    counts = 1;
                }

                this_team = p->team;
                carry_team = this_team;
                if (previous_team != -1) {
                    carry_team = previous_team;
                    if (previous_team != this_team) {
                        disagreement_found = 1;
                    }
                }

                if (counts && this_team != spawned_reference_team) {
                    has_spawned_reference_team = (spawned_reference_team != -1);
                    spawned_reference_team = this_team;
                    if (has_spawned_reference_team) {
                        return 1;
                    }
                }

                p = (player *)data_iterator_next(&iterator);
                previous_team = carry_team;
            } while (p != (player *)0);

            if (disagreement_found) {
                return 0;
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x45c830), from tools/pack.py 0x45c830:

undefined1 FUN_0045c830(void)

{
  bool bVar1;
  int iVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  undefined1 uVar6;
  int iVar7;
  bool bVar8;
  int local_14;

  iVar4 = FUN_0045c6a0();
  iVar2 = DAT_0087a480;
  if (iVar4 < 2) {
    uVar6 = 1;
    iVar4 = FUN_0045c6a0();
    iVar2 = DAT_0087a480;
    if (iVar4 == 1) {
      iVar5 = data_iterator_next();
      iVar4 = DAT_006f1cd8;
      while (iVar5 != 0) {
        if (((iVar4 < 1) || (*(int *)(*(int *)(iVar2 + 0x34) + 0x1fffe34) != -1)) ||
           (*(short *)(*(int *)(iVar2 + 0x34) + 0x1fffeae) < iVar4)) {
          uVar6 = 1;
        }
        else {
          uVar6 = 0;
        }
        iVar5 = data_iterator_next();
      }
    }
    return uVar6;
  }
  if ((0 < DAT_006f1cd8) || (DAT_006f1cc8 != '\0')) {
    local_14 = -1;
    bVar1 = false;
    iVar5 = data_iterator_next();
    iVar4 = -1;
    if (iVar5 != 0) {
      do {
        if (((*(char *)(iVar5 + 0xd5) == '\0') && (*(int *)(iVar5 + 0x34) == -1)) &&
           ((cVar3 = FUN_00460e40(0xffffffff), cVar3 != '\0' ||
            (((0 < DAT_006f1cd8 && (*(int *)(*(int *)(iVar2 + 0x34) + 0x1fffe34) == -1)) &&
             (DAT_006f1cd8 <= *(short *)(*(int *)(iVar2 + 0x34) + 0x1fffeae))))))) {
          bVar8 = false;
        }
        else {
          bVar8 = true;
        }
        iVar5 = *(int *)(iVar5 + 0x20);
        iVar7 = iVar5;
        if ((iVar4 != -1) && (iVar7 = iVar4, iVar4 != iVar5)) {
          bVar1 = true;
        }
        if (((bVar8) && (iVar5 != local_14)) && (bVar8 = local_14 != -1, local_14 = iVar5, bVar8)) {
          return 1;
        }
        iVar5 = data_iterator_next();
        iVar4 = iVar7;
      } while (iVar5 != 0);
      if (bVar1) {
        return 0;
      }
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
