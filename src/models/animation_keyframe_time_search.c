// animation_keyframe_time_search  (Ghidra: FUN_004d6b10, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d6b10, size 66 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/models_types_notes.md "Keyframe times are uint16 frame numbers
//   searched by 0x4d6b10 (EAX = count, BX = frame, stack = times)." Ghidra's decompilation
//   shows every `return;` with no value and no explicit function return type, but the
//   computed index (iVar2/mid below) is the only value left live at each return point and is
//   exactly what the three curve evaluators that call this (animation_node_get_rotation/
//   _translation/_scale) need back, so this is written returning it as int16_t.
//   The `goto LAB_004d6b20` in the Ghidra output targets the very top of the enclosing
//   do-while, i.e. it is a plain loop restart (`continue`); this rewrite keeps the same single
//   loop and the same lo/hi/mid bookkeeping (including restoring lo from before the last mid
//   computation when the loop does NOT restart) rather than a textbook binary search, since
//   the two are not obviously identical for every possible input and exact behaviour must be
//   preserved.
// return register CONFIRMED (review pass): all three callers (0x4d6c46, 0x4d6ddd, 0x4d6f50)
//   do movsx eax,ax right after the call, so the result is the int16 mid left in AX; the
//   time compares are signed (cmp word [times+mid*2(+2)], bx then jg / jle).
// register convention: keyframe count in EAX (in_EAX), target frame in BX (unaff_BX); the
//   keyframe time array as the recognized stack parameter (param_1).
//   // blam-cc: EAX -> count, BX -> frame, stack -> times

#include "tags.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Finds the index of the last keyframe time at or before `frame`, using a binary search that
// narrows [lo, mid] or [mid, hi] one step at a time (see the file header for why this mirrors
// the original control flow exactly instead of a textbook binary search).
int16_t animation_keyframe_time_search(uint16_t *times, int16_t count, int16_t frame)
{
    int16_t lo, hi, mid, saved_lo;

    lo = 0;
    hi = (int16_t)(count - 1);
    for (;;) {
        saved_lo = lo;
        mid = (int16_t)((hi + saved_lo) >> 1);
        if ((mid + 1 < count) && ((int16_t)times[mid + 1] <= frame)) {
            lo = mid;
            continue;
        }
        hi = mid;
        lo = saved_lo;
        if ((int16_t)times[mid] <= frame) {
            return mid;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d6b10):

void FUN_004d6b10(int param_1)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  int iVar3;
  short unaff_BX;
  int iVar4;

  iVar3 = in_EAX + -1;
  iVar1 = 0;
LAB_004d6b20:
  do {
    iVar4 = iVar1;
    iVar1 = (int)(short)iVar3 + (int)(short)iVar4 >> 1;
    iVar2 = (int)(short)iVar1;
    if (iVar2 + 1 < (int)(short)in_EAX) {
      if (*(short *)(param_1 + 2 + iVar2 * 2) <= unaff_BX) goto LAB_004d6b20;
    }
    iVar3 = iVar1;
    iVar1 = iVar4;
    if (*(short *)(param_1 + iVar2 * 2) <= unaff_BX) {
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
