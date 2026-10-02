// game_engine_find_first_eligible_player_on_team  (Ghidra: FUN_0045c9e0; named per
// out/phase4/game_functions.md, "Finds the next eligible (alive, unit-bearing) entry in the
// current data iteration and reports whether it belongs to team param_1.")
// address 0x45c9e0, size 207 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: shares its per-player "eligible" predicate with
// game_engine_players_ready_for_bsp_switch.c (0x45c750) and the _strict variant (0x45c830);
// types/game.h player (marked_for_deletion 0xd5, unit 0x34, team 0x20),
// game_variant::lives_per_round (live copy at 0x006f1cd8).
// register convention: __cdecl; the team to match is a genuine stack parameter (Ghidra's own
//   "FUN_0045c9e0(int param_1)").
//
// UNSURE: same 0x1fffe34/0x1fffeae caveat as the sibling functions above; and, like them, this
// is a low-confidence function (0.3) whose overall purpose is inferred only from
// out/phase4/game_functions.md, not independently re-derived here.
// reconciled: R16 the elided iterator is the inline 0x10-byte data_iterator over player_data (0x45c9f5); FUN_00460e40 gets iterator.index (0x45ca3f), not -1

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
extern game_variant game_engine_variant;    // 0x006f1c88 (::lives_per_round at 0x006f1cd8)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; blam-cc: EDI -> iterator
extern int32_t players_active_count(void);   // 0x45c6a0, this batch
extern uint8_t game_engine_player_has_respawn_priority(uint32_t player_handle); // UNSURE signature; see
                                                     // game_engine_players_ready_for_bsp_switch.c

// If fewer than two players are active, returns true unconditionally. Otherwise walks
// player_data with an inline data_iterator (0x45c9f5..0x45ca19) past every "eligible" player (marked for deletion, or dead but
// exempted by the odd-man-out test or the limited-lives check) and reports whether the first
// non-eligible player found belongs to `team`. Returns false if the iterator is exhausted
// before such a player is found.
uint8_t game_engine_find_first_eligible_player_on_team(int32_t team)
{
    data_iterator iterator;
    player *p;
    datum_index reread_unit;   // re-read of p->unit after the 0x460e40 call
    int16_t reread_deaths;    // re-read of p->deaths, same reason
    uint8_t skip;

    if (players_active_count() < 2) {
        return 1;
    }

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)data_iterator_next(&iterator);
    if (p == (player *)0) {
        return 0;
    }

    do {
        if (p->marked_for_deletion != 0) {
            skip = 1;
        } else if (p->unit == k_datum_index_none) {
            skip = (game_engine_player_has_respawn_priority(iterator.index) != 0) || // 0x45ca3f pushes the iterator's index
                   (0 < game_engine_variant.lives_per_round &&
                    (reread_unit = p->unit,
                     reread_unit == k_datum_index_none) &&
                    (reread_deaths = p->deaths,
                     game_engine_variant.lives_per_round <= reread_deaths));
        } else {
            skip = 0;
        }

        if (!skip) {
            break;
        }

        p = (player *)data_iterator_next(&iterator);
        if (p == (player *)0) {
            return 0;
        }
    } while (1);

    return p->team == team;
}

#if 0
Original Ghidra decompilation (0x45c9e0), from tools/pack.py 0x45c9e0:

undefined4 FUN_0045c9e0(int param_1)

{
  int iVar1;
  char cVar2;
  int iVar3;

  iVar3 = FUN_0045c6a0();
  iVar1 = DAT_0087a480;
  if (iVar3 < 2) {
    return 1;
  }
  iVar3 = data_iterator_next();
  if (iVar3 == 0) {
    return 0;
  }
  while ((*(char *)(iVar3 + 0xd5) != '\0' ||
         ((*(int *)(iVar3 + 0x34) == -1 &&
          ((cVar2 = FUN_00460e40(0xffffffff), cVar2 != '\0' ||
           (((0 < DAT_006f1cd8 && (*(int *)(*(int *)(iVar1 + 0x34) + 0x1fffe34) == -1)) &&
            (DAT_006f1cd8 <= *(short *)(*(int *)(iVar1 + 0x34) + 0x1fffeae)))))))))) {
    iVar3 = data_iterator_next();
    if (iVar3 == 0) {
      return 0;
    }
  }
  if (*(int *)(iVar3 + 0x20) != param_1) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
