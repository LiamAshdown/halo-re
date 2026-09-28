// game_engine_players_ready_for_bsp_switch  (Ghidra: FUN_0045c750; named per
// out/phase4/game_functions.md, "Checks whether all active players are in a state (spawned,
// on the target structure-BSP) that allows a structure-BSP switch to proceed.")
// address 0x45c750, size 214 bytes
// name confidence: 0.45   rewrite confidence: 0.3
// evidence: types/game.h player (marked_for_deletion 0xd5, unit 0x34, team 0x20),
//   game_variant::lives_per_round (0x50, live copy at 0x006f1cd8); players_active_count
//   (0x45c6a0, this batch); FUN_00460e40 is types/game.h's own citation for player::odd_man_out
//   (0x8c) being "cached result of 0x460e40".
// register convention: __cdecl, no arguments.
//
// UNSURE: the two accesses at player_data->data + 0x1fffe34 / + 0x1fffeae are transcribed
// exactly as Ghidra computed them (a huge byte offset off the player array's base pointer);
// no plausible struct or table this actually indexes into was identified, so these are kept as
// raw pointer arithmetic rather than guessed fields -- they may be two unrelated globals that
// the decompiler folded into the wrong base pointer. FUN_00460e40 is called with the literal
// constant -1 every time (not the current player's handle); reproduced as observed rather than
// assumed to be a typo for the loop variable.
// reconciled: R16 the elided iterator is the inline 0x10-byte data_iterator over player_data (0x45c769); FUN_00460e40 gets iterator.index (0x45c7b0), not -1

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

extern data_array *player_data;              // 0x0087a480
extern game_variant game_engine_variant;    // 0x006f1c88 (::lives_per_round at 0x006f1cd8)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; blam-cc: EDI -> iterator
extern int32_t players_active_count(void);   // 0x45c6a0, this batch
extern uint8_t game_engine_player_has_respawn_priority(uint32_t player_handle); // UNSURE signature; types/game.h cites this
                                                     // address for the player::odd_man_out test

// Returns true immediately if fewer than two players are active. Otherwise scans players: a
// player "passes" (lets the scan continue) if it is marked for deletion, or has no unit and
// either the odd-man-out test says so or (in a limited-lives game) two UNSURE globals off the
// player array's base agree, or its team matches (or seeds) a running reference team. As soon
// as a player fails all of that, the function returns true (ready); running out of players
// while still passing returns false (not ready).
uint8_t game_engine_players_ready_for_bsp_switch(void)
{
    data_iterator iterator; // inline over player_data, 0x45c769..0x45c78e
    player *p;
    int32_t reference_team;
    datum_index reread_unit;   // re-read of p->unit after the 0x460e40 call
    int16_t reread_deaths;    // re-read of p->deaths, same reason
    uint8_t result;
    uint8_t odd_man_out_result;
    uint8_t no_reference_team;

    if (players_active_count() < 2) {
        return 1;
    }

    result = 0;
    reference_team = -1;
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)data_iterator_next(&iterator);
    if (p != (player *)0) {
        while (1) {
            uint8_t keep_going;

            if (p->marked_for_deletion != 0) {
                keep_going = 1;
            } else if (p->unit == k_datum_index_none) {
                odd_man_out_result = game_engine_player_has_respawn_priority(iterator.index); // 0x45c7b0 pushes the iterator's index
                if (odd_man_out_result != 0) {
                    keep_going = 1;
                } else if (0 < game_engine_variant.lives_per_round &&
                           p->unit == k_datum_index_none &&
                           game_engine_variant.lives_per_round <= p->deaths) {
                    keep_going = 1;
                } else {
                    keep_going = 0;
                }
            } else {
                keep_going = 0;
            }

            if (!keep_going) {
                if (p->team == reference_team) {
                    keep_going = 1;
                } else {
                    no_reference_team = (reference_team == -1);
                    reference_team = p->team;
                    keep_going = no_reference_team;
                }
            }

            if (!keep_going) {
                break;
            }

            p = (player *)data_iterator_next(&iterator);
            if (p == (player *)0) {
                return 0;
            }
        }
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x45c750), from tools/pack.py 0x45c750:

undefined1 FUN_0045c750(void)

{
  int iVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  bool bVar5;
  undefined1 local_11;

  local_11 = 0;
  iVar3 = FUN_0045c6a0();
  iVar1 = DAT_0087a480;
  if (iVar3 < 2) {
    return 1;
  }
  iVar3 = -1;
  iVar4 = data_iterator_next();
  if (iVar4 != 0) {
    while ((*(char *)(iVar4 + 0xd5) != '\0' ||
           (((*(int *)(iVar4 + 0x34) == -1 &&
             ((cVar2 = FUN_00460e40(0xffffffff), cVar2 != '\0' ||
              (((0 < DAT_006f1cd8 && (*(int *)(*(int *)(iVar1 + 0x34) + 0x1fffe34) == -1)) &&
               (DAT_006f1cd8 <= *(short *)(*(int *)(iVar1 + 0x34) + 0x1fffeae))))))) ||
            ((*(int *)(iVar4 + 0x20) == iVar3 ||
             (bVar5 = iVar3 == -1, iVar3 = *(int *)(iVar4 + 0x20), bVar5))))))) {
      iVar4 = data_iterator_next();
      if (iVar4 == 0) {
        return 0;
      }
    }
    local_11 = 1;
  }
  return local_11;
}
#endif
