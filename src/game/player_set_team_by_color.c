// player_set_team_by_color  (Ghidra: FUN_00470630; renamed, no established name)
// address 0x470630, size 89 bytes
// name confidence: 0.25   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md ("Finds the active player/object matching a given color
// id and reassigns its bucket/team index"); types/game.h player (team +0x20, team_index +0x66,
// team_index_desired +0x67).
// register convention: fully reconstructed against
//   objdump -d -M intel --start-address=0x470630 --stop-address=0x470689 bin/halo.exe
// since Ghidra shows both operands as unbound registers (`unaff_BL`, `unaff_ESI`), matching the
// same pattern as player_customization_slot_set.c.
//   // blam-cc: EBX -> new_team, ESI -> target_team_index_desired
// UNSURE: "color" in the auto-generated summary is inferred only from context; the field actually
// matched is player::team_index_desired.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI

// blam-cc: EBX -> new_team, ESI -> target_team_index_desired
// Finds the first live player whose team_index_desired equals `target_team_index_desired` and
// sets both its team and team_index to `new_team`.
void player_set_team_by_color(uint8_t new_team, int8_t target_team_index_desired)
{
    data_iterator player_iter;
    void *player_element;
    int32_t iter_signature; // UNSURE: write-only "iter" scratch value, see
                             // game_engine_player_select_random_target.c; never read back.

    iter_signature = (int32_t)(intptr_t)player_data ^ 0x69746572;
    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_element = data_iterator_next(&player_iter);
    while (player_element != 0) {
        player *p = (player *)player_element;

        if (p->team_index_desired == target_team_index_desired) {
            p->team = new_team;
            p->team_index = (int8_t)new_team;
            break;
        }
        player_element = data_iterator_next(&player_iter);
    }
    (void)iter_signature;
}

#if 0
Original Ghidra decompilation (0x470630), from tools/pack.py 0x470630:

void FUN_00470630(void)

{
  int iVar1;
  byte unaff_BL;
  int unaff_ESI;

  iVar1 = data_iterator_next();
  if (iVar1 != 0) {
    while (*(char *)(iVar1 + 0x67) != unaff_ESI) {
      iVar1 = data_iterator_next();
      if (iVar1 == 0) {
        return;
      }
    }
    *(uint *)(iVar1 + 0x20) = (uint)unaff_BL;
    *(byte *)(iVar1 + 0x66) = unaff_BL;
  }
  return;
}
#endif
