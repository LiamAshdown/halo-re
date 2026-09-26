// message_delta_sample_ring_buffer_append  (Ghidra: FUN_004ed390; named per this rewrite)
// address 0x4ed390, size 169 bytes
// name confidence: 0.35   rewrite confidence: 0.95 (step 1: checked against objdump -d 0x4ed390..0x4ed438)
// evidence: out/phase2/networking/07.md decompilation: appends a 5-dword (20-byte) record,
// growing the buffer up to 30 (0x1e) entries and then wrapping a separate write cursor; after
// every append it recomputes and caches the average of entry field index 4 (offset 0x10, the
// last dword of the 5) over all `count` entries at ring+0x00.
// register convention: ring buffer in ESI (unaff_ESI), the new 20-byte record in EAX (recognized
// parameter).
// blam-cc: ESI -> ring, EAX -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"



// blam-cc: ESI -> ring, EAX -> entry
// Appends a new 20-byte sample entry: while the buffer has not yet reached 30 entries it grows
// in place, otherwise it overwrites the slot at the wrapping write_cursor. Either way, recomputes
// the buffer's cached running average (of entry field index 4) over every entry currently held.
void message_delta_sample_ring_buffer_append(message_delta_sample_ring_buffer *ring, const int32_t *entry)
{
    int32_t slot;
    int32_t count;
    int64_t sum;
    int32_t i;

    if (ring->count < 0x1e) {
        slot = ring->count;
        ring->entries[slot][0] = entry[0];
        ring->entries[slot][1] = entry[1];
        ring->entries[slot][2] = entry[2];
        ring->entries[slot][3] = entry[3];
        ring->entries[slot][4] = entry[4];
        ring->count = ring->count + 1;
        ring->write_cursor = 0; // FIXED: 0x4ed3be jumps to 0x4ed3f4, resetting the cursor while filling
    } else {
        slot = ring->write_cursor;
        ring->entries[slot][0] = entry[0];
        ring->entries[slot][1] = entry[1];
        ring->entries[slot][2] = entry[2];
        ring->entries[slot][3] = entry[3];
        ring->entries[slot][4] = entry[4];
        ring->write_cursor = ring->write_cursor + 1;
        if (ring->write_cursor == 0x1e) {
            ring->write_cursor = 0;
        }
    }

    count = ring->count;
    sum = 0;
    if (0 < count) {
        for (i = 0; i < count; i++) {
            sum = sum + (int64_t)ring->entries[i][4]; // FIXED: cdq / adc sign-extends each sample
        }
    }
    if (count != 0) {
        ring->cached_average = (int32_t)(sum / (int64_t)count); // __alldiv, signed
    } else {
        ring->cached_average = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4ed390):

void FUN_004ed390(void)

{
  undefined4 *puVar1;
  uint uVar2;
  undefined4 *in_EAX;
  undefined4 uVar3;
  uint *puVar4;
  int iVar5;
  int iVar6;
  undefined4 *unaff_ESI;
  uint uVar7;
  bool bVar8;

  if ((int)unaff_ESI[1] < 0x1e) {
    puVar1 = unaff_ESI + unaff_ESI[1] * 5 + 3;
    *puVar1 = *in_EAX;
    puVar1[1] = in_EAX[1];
    puVar1[2] = in_EAX[2];
    puVar1[3] = in_EAX[3];
    puVar1[4] = in_EAX[4];
    unaff_ESI[1] = unaff_ESI[1] + 1;
  }
  else {
    puVar1 = unaff_ESI + unaff_ESI[2] * 5 + 3;
    *puVar1 = *in_EAX;
    puVar1[1] = in_EAX[1];
    puVar1[2] = in_EAX[2];
    puVar1[3] = in_EAX[3];
    puVar1[4] = in_EAX[4];
    iVar6 = unaff_ESI[2];
    unaff_ESI[2] = iVar6 + 1;
    if (iVar6 + 1 != 0x1e) goto LAB_004ed3fb;
  }
  unaff_ESI[2] = 0;
LAB_004ed3fb:
  iVar6 = unaff_ESI[1];
  uVar7 = 0;
  iVar5 = 0;
  if (0 < iVar6) {
    puVar4 = unaff_ESI + 7;
    uVar7 = 0;
    do {
      uVar2 = *puVar4;
      bVar8 = CARRY4(uVar7,uVar2);
      uVar7 = uVar7 + uVar2;
      iVar5 = iVar5 + ((int)uVar2 >> 0x1f) + (uint)bVar8;
      puVar4 = puVar4 + 5;
      iVar6 = iVar6 + -1;
    } while (iVar6 != 0);
  }
  iVar6 = unaff_ESI[1];
  if (iVar6 != 0) {
    uVar3 = __alldiv(uVar7,iVar5,iVar6,iVar6 >> 0x1f);
    *unaff_ESI = uVar3;
    return;
  }
  *unaff_ESI = 0;
  return;
}
#endif
