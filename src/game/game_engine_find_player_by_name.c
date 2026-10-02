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
// FIXED (register inputs, objdump): EBX is a genuine live-in (string_convert_ascii_to_unicode's
// own EAX/EBX/EDI convention shows EBX -> source, never set locally here before the 0x47344a
// call); the extern's stale single-argument prototype was also corrected to match the callee's
// real 3-parameter signature. No callers exist yet in this build, so this is safe to change.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t network_game_mode; // 0x00719720
extern data_array *player_data;    // 0x0087a480

extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source); // 0x557990, EAX dst, EDI capacity, EBX source
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI

// blam-cc: EBX -> source_name
// UNSURE: see header -- the comparison result is computed but never used anywhere Ghidra shows.
void game_engine_find_player_by_name(char *source_name)
{
    if (network_game_mode == 2) {
        wchar_t name[1024];
        data_iterator iter;
        void *element;

        string_convert_ascii_to_unicode((uint16_t *)name, 0x800, source_name); // 0x473441: EDI = 0x800

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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
