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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global; // 0x00719cd0, types/math.h

extern int32_t random_advance_draws(TagReflexive *reflexive); // 0x45f6e0, EAX reflexive: the sum of the element weights
extern int32_t __ftol(void); // 0x6391b4, MSVC runtime; UNSURE: real argument is on the x87 stack

// REWRITTEN from objdump 0x45f720..0x45f7b0: total = the weight sum (16 bits used); a draw r = ((seed >> 16) * total)
// >> 16 as int16; walking the elements, r = __ftol(r - weight) and the first element taking it below zero wins,
// returning its dword at +0x30; -1 when none does.
// blam-cc: EAX -> tag_id
int32_t tag_reflexive_pick_weighted_random_index(datum_index tag_id)
{
    TagReflexive *reflexive = (TagReflexive *)tag_instances[tag_id & 0xffff].data;
    int32_t count = (int32_t)reflexive->count;
    int16_t total = (int16_t)random_advance_draws(reflexive);
    uint8_t *element;
    int32_t remaining;
    int32_t i;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    remaining = (int16_t)(((random_seed_global >> 0x10) * (uint32_t)(int32_t)total) >> 0x10);
    element = (uint8_t *)reflexive->pointer;
    for (i = 0; i < count; i++) {
        remaining = (int32_t)((float)remaining - *(float *)(element + i * 0x54 + 0x20)); // fild / fsub / __ftol
        if (remaining < 0) {
            return *(int32_t *)(element + i * 0x54 + 0x30);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
