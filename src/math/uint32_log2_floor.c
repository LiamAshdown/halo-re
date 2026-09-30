// uint32_log2_floor  (Ghidra: uint32_log2_floor, already named)
// address 0x4cb740, size 22 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/math_functions.md ("Returns the bit-index of the highest set bit of its
//   (register-passed) integer argument"); shift-until-1 loop is a textbook floor(log2(x)) with
//   the 0 case returning 0.
// register convention: value in ECX (in_ECX), no stack arguments.
//   // blam-cc: ECX -> value

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// Returns the bit-index of the highest set bit of its argument (0 for an input of 0).
int32_t uint32_log2_floor(uint32_t value)
{
    int32_t result;

    result = 0;
    if (value != 0) {
        while (value != 1) {
            result = result + 1;
            value = value >> 1;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4cb740):

int uint32_log2_floor(void)

{
  int iVar1;
  uint in_ECX;
  
  iVar1 = 0;
  if (in_ECX != 0) {
    for (; in_ECX != 1; in_ECX = in_ECX >> 1) {
      iVar1 = iVar1 + 1;
    }
  }
  return iVar1;
}
#endif
