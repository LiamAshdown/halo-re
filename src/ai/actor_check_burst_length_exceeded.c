// actor_check_burst_length_exceeded  (Ghidra: actor_check_burst_length_exceeded, renamed)
// address 0x4281b0, size 56 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4281b0..0x4281e7.)
// evidence: types/ai.h actor.unknown_6e/mode(0x6c); offset 0xa8 falls inside actor.mode_data.raw
//   (the per-mode union at 0x9c..0x11f), used here only in death mode. Phase-4 summary: "Boolean
// check combining the actor's burst-length field (0x6e), a mode value (0x6c), and a
// secondary counter (0xa8); precise behavioral meaning is not confirmed from the code
// alone."
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index
// Returns true when unknown_6e exceeds 6, unless the actor is in death mode (4) with a
// positive unknown_a8, in which case it returns false instead.
uint8_t actor_check_burst_length_exceeded(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t result = self->unknown_6e > 6;

    if (result && self->mode == _actor_mode_death && *(int16_t *)&self->mode_data.raw[0xc] > 0) {
        result = 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4281b0):

int FUN_004281b0(void)

{
  bool bVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  uint3 uVar4;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  bVar1 = 6 < *(short *)(iVar2 + 0x6e);
  uVar4 = (uint3)((uint)iVar2 >> 8);
  iVar3 = CONCAT31(uVar4,bVar1);
  if (((bVar1) && (*(short *)(iVar2 + 0x6c) == 4)) && (0 < *(short *)(iVar2 + 0xa8))) {
    iVar3 = (uint)uVar4 << 8;
  }
  return iVar3;
}
#endif
