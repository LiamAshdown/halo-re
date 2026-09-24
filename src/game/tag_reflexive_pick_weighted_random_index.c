// tag_reflexive_pick_weighted_random_index  (Ghidra: FUN_0045f720; named per
// out/phase4/game_functions.md)
// address 0x45f720, size 145 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Advances the PRNG and picks a weighted-random
// permutation/index from a tag reflexive block"); types/cache.h tag_instance; the global LCG
// constants (0x19660d / 0x3c6ef35f) match types/game.h's random_seed_global comment exactly.
// register convention: tag id in EAX (in_EAX).
//   // blam-cc: EAX -> tag_id
// UNSURE: the reflexive element layout (stride 0x54, payload at +0x30) is not attributed to any
// named tag struct here; each element's "weight" is read through __ftol with no visible operand
// (an x87-stack value this decompilation cannot recover), exactly like random_advance_draws.

#include "tags.h"
#include "math.h"
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global; // 0x00719cd0, types/math.h

extern int32_t random_advance_draws(int32_t *count); // 0x45f6e0, this batch
extern int32_t __ftol(void); // 0x6391b4, MSVC runtime; UNSURE: real argument is on the x87 stack

// blam-cc: EAX -> tag_id
// Reads a TagReflexive header out of tag `tag_id`'s data, advances the global
// PRNG, then scans the reflexive (stride 0x54, per-element weight at +0x30 read through the same
// FPU-stack idiom as random_advance_draws) for the first negative weight, returning that
// element's own value; returns -1 if every element's weight was non-negative.
int32_t tag_reflexive_pick_weighted_random_index(datum_index tag_id)
{
    TagReflexive *reflexive; // types/tags.h; the tag's data begins with one reflexive header
    int32_t i;

    reflexive = (TagReflexive *)tag_instances[tag_id & 0xffff].data;

    random_advance_draws((int32_t *)&reflexive->count); // UNSURE: argument not visible at this call site

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;

    for (i = 0; i < (int32_t)reflexive->count; i++) {
        if (__ftol() < 0) {
            return *(int32_t *)((uint8_t *)reflexive->pointer + i * 0x54 + 0x30);
        }
    }

    return -1;
}

#if 0
Original Ghidra decompilation (0x45f720), from tools/pack.py 0x45f720:

undefined4 FUN_0045f720(void)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;

  piVar1 = *(int **)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *piVar1;
  FUN_0045f6e0();
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  iVar3 = piVar1[1];
  iVar5 = 0;
  if (0 < iVar2) {
    do {
      iVar4 = __ftol();
      if (iVar4 < 0) {
        return *(undefined4 *)(iVar5 * 0x54 + 0x30 + iVar3);
      }
      iVar5 = iVar5 + 1;
    } while (iVar5 < iVar2);
  }
  return 0xffffffff;
}
#endif
