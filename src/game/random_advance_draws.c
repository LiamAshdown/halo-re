// random_advance_draws  (Ghidra: FUN_0045f6e0; named per out/phase4/game_functions.md)
// address 0x45f6e0, size 53 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Advances the game's random number generator by a
// caller-specified number of draws").
// register convention: int *count in EAX (in_EAX).
//   // blam-cc: EAX -> count
// UNSURE: __ftol (the MSVC runtime float-to-int truncation helper) takes its argument on the x87
// FPU stack, which Ghidra shows with no operand at all; this function's actual purpose is almost
// certainly to pop and discard `*count - 1` FPU values a caller left pushed (one per "draw") and
// return the last one truncated to an int, but that stack-level detail cannot be expressed in
// portable C. Transcribed as a literal, argument-less, `count`-times loop matching the
// decompilation; correctness depends on whatever the real calling convention around __ftol does.

#include "tags.h"

extern int32_t __ftol(void); // 0x6391b4, MSVC runtime; UNSURE: real argument is on the x87 stack

// REWRITTEN from objdump 0x45f6e0..0x45f71c: EAX is the tag's first reflexive (count at +0, elements at +4, stride
// 0x54). The running total starts at 0 and, for each element, becomes __ftol(total + element weight at +0x20) -- the
// truncation happens after every add. Returns the total (0 for an empty block). The name predates this reading.
// blam-cc: EAX -> reflexive
int32_t random_advance_draws(TagReflexive *reflexive)
{
    int32_t count = (int32_t)reflexive->count;
    uint8_t *element = (uint8_t *)reflexive->pointer;
    int32_t total = 0;
    int32_t i;

    for (i = 0; i < count; i++) {
        total = (int32_t)((float)total + *(float *)(element + i * 0x54 + 0x20)); // fild / fadd / __ftol
    }
    return total;
}

#if 0
Original Ghidra decompilation (0x45f6e0), from tools/pack.py 0x45f6e0:

undefined4 FUN_0045f6e0(void)

{
  int *in_EAX;
  undefined4 uVar1;
  int iVar2;

  iVar2 = *in_EAX;
  if (iVar2 < 1) {
    return 0;
  }
  do {
    uVar1 = __ftol();
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return uVar1;
}
#endif
