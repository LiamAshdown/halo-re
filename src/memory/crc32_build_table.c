// crc32_build_table  (Ghidra: crc32_build_table, already named)
// address 0x4d0330, size 53 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: out/phase4/memory_types_notes.md "crc32"; standard reversed CRC-32 polynomial
// 0xedb88320, 256 entries, matches types/memory.h crc32_table.
// register convention: destination table pointer in EDX (in_EDX).

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"

#define CRC32_POLYNOMIAL 0xedb88320u

// blam-cc: table in EDX
// Generates the standard 256-entry reversed CRC-32 lookup table (polynomial 0xedb88320) into
// `table`.
void crc32_build_table(crc32_table *table)
{
    uint32_t seed;
    uint32_t value;
    int32_t bit;
    int32_t index;

    seed = 0;
    index = 0x100;
    while (index != 0) {
        bit = 8;
        value = seed;
        while (bit != 0) {
            if ((value & 1) == 0) {
                value = value >> 1;
            } else {
                value = (value >> 1) ^ CRC32_POLYNOMIAL;
            }
            bit = bit - 1;
        }
        table->entries[seed] = value;
        seed = seed + 1;
        index = index - 1;
    }
}

#if 0
Original Ghidra decompilation (0x4d0330):

void crc32_build_table(void)

{
  uint uVar1;
  uint uVar2;
  uint *in_EDX;
  int iVar3;
  int iVar4;

  uVar2 = 0;
  iVar4 = 0x100;
  do {
    iVar3 = 8;
    uVar1 = uVar2;
    do {
      if ((uVar1 & 1) == 0) {
        uVar1 = uVar1 >> 1;
      }
      else {
        uVar1 = uVar1 >> 1 ^ 0xedb88320;
      }
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    *in_EDX = uVar1;
    uVar2 = uVar2 + 1;
    in_EDX = in_EDX + 1;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  return;
}
#endif
