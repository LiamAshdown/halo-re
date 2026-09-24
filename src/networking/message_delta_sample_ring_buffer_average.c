// message_delta_sample_ring_buffer_average  (Ghidra: FUN_004ed350; named per this rewrite)
// address 0x4ed350, size 55 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: out/phase2/networking/07.md decompilation: sums the dword at ring+0x18+i*0x14 (entry
// index 3 of a 5-dword, 20-byte-stride record starting at ring+0xc) for the first `count`
// entries using an unsigned add-with-carry 64-bit accumulator, then divides by count via the
// CRT's signed 64-bit division helper (__alldiv). message_delta_sample_ring_buffer_append.c
// documents the same ring buffer's layout and append side.
// register convention: ring buffer object in ECX (in_ECX).
// blam-cc: ECX -> ring
// UNSURE: what the sampled quantity (entry index 3) represents, and why
// message_delta_sample_ring_buffer_append averages entry index 4 instead when it recomputes its
// own cached average after every append.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"



// blam-cc: ECX -> ring
// Averages entry field index 3 (offset 0xc) of the first `count` entries, using a 64-bit
// accumulator (summed as raw 32-bit patterns with carry, matching the original's unsigned
// add-with-carry then signed-divide) so up to 30 samples cannot lose precision. Returns 0 if the
// buffer is empty.
int32_t message_delta_sample_ring_buffer_average(message_delta_sample_ring_buffer *ring)
{
    int32_t count;
    uint64_t sum;
    int32_t i;

    count = ring->count;
    sum = 0;
    if (0 < count) {
        for (i = 0; i < count; i++) {
            sum = sum + (uint32_t)ring->entries[i][3];
        }
    }
    if (count == 0) {
        return 0;
    }
    return (int32_t)((int64_t)sum / count);
}

#if 0
Original Ghidra decompilation (0x4ed350):

undefined4 FUN_004ed350(void)

{
  int iVar1;
  undefined4 uVar2;
  int in_ECX;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  bool bVar7;

  iVar1 = *(int *)(in_ECX + 4);
  uVar5 = 0;
  iVar6 = 0;
  if (0 < iVar1) {
    puVar3 = (uint *)(in_ECX + 0x18);
    iVar4 = iVar1;
    do {
      bVar7 = CARRY4(uVar5,*puVar3);
      uVar5 = uVar5 + *puVar3;
      iVar6 = iVar6 + (uint)bVar7;
      puVar3 = puVar3 + 5;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  if (iVar1 != 0) {
    uVar2 = __alldiv(uVar5,iVar6,iVar1,iVar1 >> 0x1f);
    return uVar2;
  }
  return 0;
}
#endif
