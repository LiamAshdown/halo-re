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
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original; the separate write-only iter_signature local is folded into it

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI

// blam-cc: EBX -> new_team, ESI -> target_team_index_desired
// Finds the first live player whose team_index_desired equals `target_team_index_desired` and
// sets both its team and team_index to `new_team`.
void player_set_team_by_color(uint8_t new_team, int8_t target_team_index_desired)
{
    data_iterator player_iter;
    void *player_element;

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
