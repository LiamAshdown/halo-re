// random_get_table_point  (Ghidra: random_get_table_point, already named)
// address 0x473560, size 78 bytes
// name confidence: 0.7   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md ("Advances the global PRNG and returns a random point
// from a precomputed vector3d table, used elsewhere for respawn placement jitter"); types/math.h
// random_seed_global (0x00719cd0), real_point3d.
// register convention: output point pointer in EAX (Ghidra's `in_EAX`).
//   // blam-cc: EAX -> out

// CORRECTED (phase 4 review): the Blam random-index idiom is
//   movsx ecx,<count> ; shr eax,0x10 ; imul eax,ecx ; shr eax,0x10 ; movsx <idx>,ax
// so the seed's high half is used ZERO-extended (shr, no movsx) and the product is shifted
// down logically. Casting (seed >> 16) to int16_t first, as this file did, makes the index
// negative for half of all seeds, which silently disables the pick.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern random_seed random_seed_global;      // 0x00719cd0
extern real_point3d *random_point_table; // 0x006b7af4, a POINTER (0x473590 loads it, then indexes)
extern int16_t random_point_table_count;   // 0x006b7af8 (read with movsx from a word)

// blam-cc: EAX -> out
// Advances the global LCG PRNG and writes a random entry of random_point_table into *out.
void random_get_table_point(real_point3d *out)
{
    int16_t index;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    index = (int16_t)(((random_seed_global >> 16) *
                       (uint32_t)(int32_t)(int16_t)random_point_table_count) >> 16);
    *out = random_point_table[index];
}

#if 0
Original Ghidra decompilation (0x473560), from tools/pack.py 0x473560:

void random_get_table_point(void)

{
  undefined4 *puVar1;
  undefined4 *in_EAX;

  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  puVar1 = (undefined4 *)
           (DAT_006b7af4 + (short)((random_seed_global >> 0x10) * (int)DAT_006b7af8 >> 0x10) * 0xc);
  *in_EAX = *puVar1;
  in_EAX[1] = puVar1[1];
  in_EAX[2] = puVar1[2];
  return;
}
#endif
