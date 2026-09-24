// bit_vector_or  (Ghidra: FUN_004cb760; renamed, Blam-style, not previously named)
// address 0x4cb760, size 54 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/math_types_notes.md item 2 ("Bitwise-ORs one fixed-size bit array into
//   another, word by word... the three buffers addressed as base + two deltas. It is a
//   flags/bit-vector utility, not geometry; it defines no math type"). The pointer walked by
//   `puVar1` and the two delta-computed addresses are the same index applied to three distinct
//   arrays (a, b, dst); rewritten here as plain indexing.
// register convention: array `a` in EAX (in_EAX), bit count in CX/ECX (in_CX), array `b` in EDX
//   (in_EDX); dst as the recognized stack parameter (param_1).
//   // blam-cc: EAX -> a, ECX (low 16) -> bit_count, EDX -> b, stack -> dst

#include "tags.h"
#include "math.h"

// Bitwise-ORs one fixed-size bit array into another, word by word.
void bit_vector_or(uint32_t *a, int16_t bit_count, uint32_t *b, uint32_t *dst)
{
    int16_t dword_count;
    int16_t i;

    dword_count = (int16_t)((bit_count + 0x1f) >> 5);
    i = (int16_t)(dword_count - 1);
    while (-1 < i) {
        dst[i] = b[i] | a[i];
        i = (int16_t)(i - 1);
    }
}

#if 0
Original Ghidra decompilation (0x4cb760):

void FUN_004cb760(int param_1)

{
  int in_EAX;
  uint *puVar1;
  short in_CX;
  short sVar2;
  uint uVar3;
  int in_EDX;
  
  uVar3 = in_CX + 0x1f >> 5;
  sVar2 = (short)uVar3 + -1;
  if (-1 < sVar2) {
    puVar1 = (uint *)(in_EAX + sVar2 * 4);
    uVar3 = uVar3 & 0xffff;
    do {
      *(uint *)((param_1 - in_EAX) + (int)puVar1) =
           *(uint *)((in_EDX - in_EAX) + (int)puVar1) | *puVar1;
      puVar1 = puVar1 + -1;
      uVar3 = uVar3 - 1;
    } while (uVar3 != 0);
  }
  return;
}
#endif
