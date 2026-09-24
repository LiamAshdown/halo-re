// effect_first_person_screen_timer_active  (Ghidra: FUN_00450680, still unnamed there; named here
// from its own summary in out/phase4/effects_functions.md: "Looks up a fixed object slot and, if
// it has a certain attachment flag, validates a linked node index, returning whether a related
// condition (likely local/first-person player context) holds")
// address 0x450680, size 65 bytes
// name confidence: 0.25 (low -- name is a guess)   rewrite confidence: 0.2 (LOW -- see UNSURE)
// evidence: src/objects/device_frontfacing.c and similar establish object_try_and_get(index,
// type_mask); the type mask literal `1` here is unexplained (types/objects.h's object type mask
// enum is not visible in this pack).
// register convention: ECX -> object_index, forwarded straight through to object_try_and_get
// (established convention: ECX -> object_index); type mask 1 is a hardcoded literal.
// FIXED (register inputs, objdump): ECX (read at 0x450685, the call to object_try_and_get
// itself, whose own recovered convention is ECX -> object_index) was hardcoded as a guessed
// literal 0 instead of being taken as a parameter and forwarded. Added object_index (ECX) and
// use it in place of the guess.
// UNSURE (structural, TYPES-GAP): object+0x106 and object+0x41c are read here but are not part
// of the common object header types/objects.h documents (0x1f4 bytes); they belong to whatever
// per-type extension starts at +0x1f4 for this object's type (most likely units, given the
// "first-person" framing), which this pass has no header for. Both are kept as raw offsets.
// UNSURE: the original packs its bool return into the low byte of a 32-bit value that otherwise
// carries leftover garbage in the upper 3 bytes (`CONCAT31((int3)(uVar1 >> 8), 1)`); simplified
// here to a plain uint8_t, since no caller in this batch is visible to confirm the upper bits
// are read.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0,
    // established; blam-cc: ECX -> object_index, stack -> type_mask
extern int32_t *game_time; // 0x006f1d6c; +0x0c is the current game tick

// UNSURE overall (see file header): whether object_index has flag bit 2 of the byte at +0x106
// set, and if so, whether the linked index at +0x41c (offset by 0x1e ticks) is still within
// k_game_tick globals's current tick.
// blam-cc: ECX -> object_index
uint8_t effect_first_person_screen_timer_active(datum_index object_index)
{
    object *self = object_try_and_get(object_index, 1);

    if (self == 0 || (*((uint8_t *)self + 0x106) & 4) == 0) {
        return 0;
    }

    {
        int32_t linked = *(int32_t *)((uint8_t *)self + 0x41c);

        if (linked != -1) {
            linked = linked + 0x1e;
            if (linked < game_time[3]) {
                return 1;
            }
        }
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x450680):

uint FUN_00450680(void)

{
  uint uVar1;

  uVar1 = object_try_and_get(1);
  if ((uVar1 == 0) || ((*(byte *)(uVar1 + 0x106) & 4) == 0)) {
    return uVar1 & 0xffffff00;
  }
  uVar1 = *(uint *)(uVar1 + 0x41c);
  if ((uVar1 != 0xffffffff) && (uVar1 = uVar1 + 0x1e, (int)uVar1 < *(int *)(DAT_006f1d6c + 0xc))) {
    return CONCAT31((int3)(uVar1 >> 8),1);
  }
  return uVar1 & 0xffffff00;
}
#endif
