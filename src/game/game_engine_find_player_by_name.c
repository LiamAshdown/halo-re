// game_engine_find_player_by_name  (Ghidra: FUN_00473430; renamed, no established name)
// address 0x473430, size 118 bytes
// name confidence: 0.25   rewrite confidence: 0.15
// evidence: out/phase4/game_functions.md ("Server-side helper that scans all players comparing
// names, likely for a console/RCON player-lookup command"); types/game.h player::name (+0x04).
// UNSURE: string_convert_ascii_to_unicode (fills the 1024-wchar_t local buffer this function then compares every
// player's name against) is not in this batch and is called with no visible arguments; and the
// per-player wcscmp result is never inspected anywhere in Ghidra's own rendering (no branch
// consumes it), which strongly suggests this function is itself incomplete/truncated in the
// original decompile (compare the confirmed-incomplete cases in game_engine_player_select_
// random_target.c and the two update_*dispose.c files in this batch) rather than genuinely a
// no-op scan. Transcribed literally; callers=0 in this build.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include <stdint.h>

extern int16_t network_game_mode; // 0x00719720
extern data_array *player_data;    // 0x0087a480

extern void string_convert_ascii_to_unicode(wchar_t *out); // 0x557990, not in this batch; blam-cc: EDI -> out (UNSURE)
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI

// UNSURE: see header -- the comparison result is computed but never used anywhere Ghidra shows.
void game_engine_find_player_by_name(void)
{
    if (network_game_mode == 2) {
        wchar_t name[1024];
        data_iterator iter;
        void *element;

        string_convert_ascii_to_unicode(name);

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = k_datum_index_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
        element = data_iterator_next(&iter);
        while (element != 0) {
            wcscmp((wchar_t *)((uint8_t *)element + 4), name);
            element = data_iterator_next(&iter);
        }
    }
}

#if 0
Original Ghidra decompilation (0x473430), from tools/pack.py 0x473430:

void FUN_00473430(void)

{
  int iVar1;
  wchar_t local_800 [1024];

  if (DAT_00719720 == 2) {
    FUN_00557990();
    iVar1 = data_iterator_next();
    while (iVar1 != 0) {
      _wcscmp((wchar_t *)(iVar1 + 4),local_800);
      iVar1 = data_iterator_next();
    }
  }
  return;
}
#endif
