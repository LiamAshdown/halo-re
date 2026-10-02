// game_engine_koth_update_hill_occupancy_state  (Ghidra: game_engine_koth_update_hill_occupancy_
//   state, already named)
// address 0x46acb0, size 546 bytes
// name confidence: 0.5   rewrite confidence: 0.25
// evidence: types/game.h king_globals (hill_state/hill_ticks/occupant at 0x006b1050/54/58);
//   game_variant::teams aliased at 0x006f1cbc. The non-team branch's counting loop tests a
//   FIXED global (0x006c0f3f) on every iteration without ever deriving it from the iterator's
//   own element, and updates `local_14` (the eventual sole occupant) from `local_8`, which is
//   likewise set to -1 once before the loop and never reassigned inside it -- both are preserved
//   exactly as Ghidra shows them (see UNSURE) rather than "fixed", since this rewrite must not
//   invent behaviour the disassembly was not re-derived to confirm.
// register convention: no parameters.
// UNSURE: (1) DAT_006c0f3f (0x006c0f3f) is modeled as a plain global byte gate, checked
//   identically every loop iteration; this may be a genuine Ghidra mis-resolution of a
//   per-element field access that a future disassembly pass should re-derive. (2) `local_8`/
//   `occupant_candidate` is consequently always -1 in this transcription, matching Ghidra's own
//   rendering, not a corrected per-element occupant id.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (teams aliased 0x006f1cbc)
extern data_array *player_data;                     // 0x0087a480
extern uint8_t king_hill_single_occupant_flag;      // 0x006c0f3f, UNSURE identity, see header
extern king_globals king_hill_state_globals;        // 0x006b1050

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast

void game_engine_koth_update_hill_occupancy_state(void)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    if (current_game_engine == 0 || game_engine_variant.teams == 0) {
        int32_t count = 0;
        int32_t occupant_candidate = -1;
        int32_t occupant = -1; // UNSURE: never reassigned from occupant_candidate's actual value

        element = data_iterator_next(&iter);
        if (element != 0) {
            do {
                if (king_hill_single_occupant_flag != 0) {
                    count++;
                    occupant = occupant_candidate;
                }
                element = data_iterator_next(&iter);
            } while (element != 0);

            if (count > 1) {
                king_hill_state_globals.hill_state = _king_hill_contested;
                if (king_hill_state_globals.hill_ticks < 0x12d) {
                    king_hill_state_globals.hill_state = _king_hill_contested;
                    king_hill_state_globals.hill_ticks = 0;
                    king_hill_state_globals.occupant = (datum_index)0xffffffff;
                    return;
                }
                game_engine_queue_multiplayer_sound(0x27, 0xffffffff, 1); // 0x46ae57..0x46ae60
                king_hill_state_globals.hill_ticks = 0;
                king_hill_state_globals.occupant = (datum_index)0xffffffff;
                return;
            }
            if (count != 0) {
                if (king_hill_state_globals.hill_state == _king_hill_held &&
                    occupant == (int32_t)king_hill_state_globals.occupant) {
                    king_hill_state_globals.hill_ticks++;
                } else {
                    king_hill_state_globals.hill_ticks = 0;
                    king_hill_state_globals.occupant = (datum_index)occupant;
                }
                king_hill_state_globals.hill_state = _king_hill_held;
                goto check_streak;
            }
        }
        king_hill_state_globals.hill_state = _king_hill_empty;
        king_hill_state_globals.hill_ticks = 0;
        king_hill_state_globals.occupant = (datum_index)0xffffffff;
        return;
    }
    {
        int32_t team0_count = 0;
        int32_t team1_count = 0;
        int32_t new_state;

        element = data_iterator_next(&iter);
        if (element == 0) {
            king_hill_state_globals.hill_state = _king_hill_empty;
            king_hill_state_globals.hill_ticks = 0;
            return;
        }
        do {
            if (king_hill_single_occupant_flag != 0) {
                if (*(int32_t *)((uint8_t *)element + 0x20) == 0) {
                    team0_count++;
                } else {
                    team1_count++;
                }
            }
            element = data_iterator_next(&iter);
        } while (element != 0);

        if (team1_count == 0) {
            if (team0_count == 0) {
                king_hill_state_globals.hill_state = _king_hill_empty;
                king_hill_state_globals.hill_ticks = 0;
                return;
            }
            new_state = _king_hill_team_0;
            if (king_hill_state_globals.hill_state == _king_hill_team_0) {
                king_hill_state_globals.hill_ticks++;
                king_hill_state_globals.hill_state = new_state;
                goto check_streak;
            }
        } else {
            if (team0_count != 0) {
                king_hill_state_globals.hill_state = _king_hill_contested;
                if (king_hill_state_globals.hill_ticks > 300) {
                    game_engine_queue_multiplayer_sound(0x27, 0xffffffff, 1); // 0x46ad56..0x46ad60
                }
                king_hill_state_globals.hill_ticks = 0;
                return;
            }
            new_state = _king_hill_team_1;
            if (king_hill_state_globals.hill_state == _king_hill_team_1) {
                king_hill_state_globals.hill_ticks++;
                king_hill_state_globals.hill_state = new_state;
                goto check_streak;
            }
        }
        king_hill_state_globals.hill_ticks = 0;
        king_hill_state_globals.hill_state = new_state;
    }
check_streak:
    if (king_hill_state_globals.hill_ticks == 300) {
        game_engine_queue_multiplayer_sound(0x28, 0xffffffff, 1); // 0x46aea3..0x46aeac
    }
}

#if 0
Original Ghidra decompilation (0x46acb0), from tools/pack.py 0x46acb0:

void __cdecl game_engine_koth_update_hill_occupancy_state(void)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  int local_14;
  int local_8;

  if ((DAT_006f1d20 == 0) || (DAT_006f1cbc == '\0')) {
    iVar3 = 0;
    local_8 = -1;
    iVar1 = data_iterator_next();
    if (iVar1 != 0) {
      do {
        if (DAT_006c0f3f != '\0') {
          iVar3 = iVar3 + 1;
          local_14 = local_8;
        }
        iVar1 = data_iterator_next();
      } while (iVar1 != 0);
      if (1 < iVar3) {
        DAT_006b1050 = 4;
        if (DAT_006b1054 < 0x12d) {
          DAT_006b1050 = 4;
          DAT_006b1054 = 0;
          DAT_006b1058 = 0xffffffff;
          return;
        }
        game_engine_queue_multiplayer_sound(1);
        DAT_006b1054 = 0;
        DAT_006b1058 = 0xffffffff;
        return;
      }
      if (iVar3 != 0) {
        if ((DAT_006b1050 == 1) && (local_14 == DAT_006b1058)) {
          DAT_006b1054 = DAT_006b1054 + 1;
        }
        else {
          DAT_006b1054 = 0;
          DAT_006b1058 = local_14;
        }
        DAT_006b1050 = 1;
        goto joined_r0x0046ada2;
      }
    }
    DAT_006b1050 = 0;
    DAT_006b1054 = 0;
    DAT_006b1058 = 0xffffffff;
    return;
  }
  iVar4 = 0;
  iVar3 = 0;
  iVar1 = data_iterator_next();
  if (iVar1 == 0) {
    DAT_006b1050 = 0;
    DAT_006b1054 = 0;
    return;
  }
  do {
    if (DAT_006c0f3f != '\0') {
      if (*(int *)(iVar1 + 0x20) == 0) {
        iVar4 = iVar4 + 1;
      }
      else {
        iVar3 = iVar3 + 1;
      }
    }
    iVar1 = data_iterator_next();
  } while (iVar1 != 0);
  if (iVar3 == 0) {
    if (iVar4 == 0) {
      DAT_006b1050 = 0;
      DAT_006b1054 = 0;
      return;
    }
    uVar2 = 2;
    if (DAT_006b1050 == 2) {
      DAT_006b1054 = DAT_006b1054 + 1;
      DAT_006b1050 = uVar2;
      goto joined_r0x0046ada2;
    }
  }
  else {
    if (iVar4 != 0) {
      DAT_006b1050 = 4;
      if (300 < DAT_006b1054) {
        game_engine_queue_multiplayer_sound(1);
      }
      DAT_006b1054 = 0;
      return;
    }
    uVar2 = 3;
    if (DAT_006b1050 == 3) {
      DAT_006b1054 = DAT_006b1054 + 1;
      DAT_006b1050 = uVar2;
      goto joined_r0x0046ada2;
    }
  }
  DAT_006b1054 = 0;
  DAT_006b1050 = uVar2;
joined_r0x0046ada2:
  if (DAT_006b1054 == 300) {
    game_engine_queue_multiplayer_sound(1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
