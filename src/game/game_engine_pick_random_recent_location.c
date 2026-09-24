// game_engine_pick_random_recent_location  (Ghidra: FUN_0046a1b0; named per this rewrite)
// address 0x46a1b0, size 126 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md ("Picks a pseudo-random entry from a small fixed list
//   that differs from a given value, using an inline LCG random-number generator"); the LCG
//   constants (0x19660d, 0x3c6ef35f) match random_seed_global's own update in
//   types/game.h ("global 0x00719cd0: uint32_t random_seed_global  seed = seed * 0x19660d +
//   0x3c6ef35f"). The table (0x006b1070, int16, count at 0x006b106c) is written elsewhere
//   outside this function's own range (no writer found in 0x468010..0x46efe0); read-only here.
// register convention: value to avoid in the stack parameter (Ghidra's own param_1); a fallback
//   return value in unaff_ECX.
//   // blam-cc: stack -> exclude_value, ECX -> fallback

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern random_seed random_seed_global;      // 0x00719cd0
extern int16_t game_engine_recent_location_count; // 0x006b106c, UNSURE exact identity
extern int16_t game_engine_recent_location_table[]; // 0x006b1070, UNSURE exact size/identity

// blam-cc: stack -> exclude_value, ECX -> fallback
// Advances the shared LCG once, then scans the small recent-location table starting from a
// pseudo-random offset for the first entry that differs from `exclude_value`, wrapping around;
// returns `fallback` if the table is empty or every entry matches `exclude_value`.
int32_t game_engine_pick_random_recent_location(int32_t exclude_value, int32_t fallback)
{
    int16_t i;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;

    if (game_engine_recent_location_count < 1) {
        return fallback;
    }

    for (i = 0; i < game_engine_recent_location_count; i++) {
        int16_t slot = (int16_t)(((int32_t)i +
            (int16_t)(((int32_t)(random_seed_global >> 0x10) * (int32_t)game_engine_recent_location_count) >> 0x10))
            % (int32_t)game_engine_recent_location_count);
        if (exclude_value != (int32_t)game_engine_recent_location_table[slot]) {
            return (int32_t)game_engine_recent_location_table[slot];
        }
    }
    return fallback;
}

#if 0
Original Ghidra decompilation (0x46a1b0), from tools/pack.py 0x46a1b0:

int FUN_0046a1b0(int param_1)

{
  short sVar1;
  int in_ECX;
  short sVar2;

  DAT_00719cd0 = DAT_00719cd0 * 0x19660d + 0x3c6ef35f;
  sVar1 = 0;
  if (DAT_006b106c < 1) {
    return in_ECX;
  }
  do {
    sVar2 = (short)(((int)sVar1 + (int)(short)((DAT_00719cd0 >> 0x10) * (int)DAT_006b106c >> 0x10))
                   % (int)DAT_006b106c);
    if (param_1 != (short)(&DAT_006b1070)[sVar2]) {
      return (int)(short)(&DAT_006b1070)[sVar2];
    }
    sVar1 = sVar1 + 1;
  } while (sVar1 < DAT_006b106c);
  return in_ECX;
}
#endif
