// input_get_key_state  (Ghidra: FUN_00490b50)
// address 0x490b50, size 138 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/input_types_notes.md: "input_get_key_state, with virtual either-side
// modifiers 0x6e..0x71." types/input.h documents each case exactly: any_shift =
// max(left_shift, right_shift), any_control = max(right_control, left_control), any_windows and
// any_alt use a <= comparison (tie goes to the second key). The default path blocks a key while
// it has a live key_block_timer entry (returns 0), else returns its raw hold-count byte from
// key_frames. A key_index of -1 falls through the "not a virtual modifier" switch default and
// indexes key_frames[-1] out of bounds, exactly as decompiled; not fixed.
// register convention: key_index in CX (in_CX)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t input_suppressed;              // 0x006b15f9
extern uint8_t key_frames[0x6d];              // 0x006b1620
extern key_block_timer key_block_timers[k_input_key_block_timer_count]; // 0x006b1600

// blam-cc: key_index in ECX
// Returns the current hold-frame count for key_index, or 0 while input is suppressed. The four
// virtual either-side modifier keys resolve to the larger (or, for windows/alt, the
// tie-broken-toward-the-second) of their two physical keys. Every other key is blocked (reads as
// 0) while a key_block_timer names it, otherwise returns its raw key_frames byte.
uint8_t input_get_key_state(int16_t key_index)
{
    int32_t i;

    if (input_suppressed != 0) {
        return 0;
    }

    switch (key_index - _input_key_any_shift) {
    case 0: // _input_key_any_shift
        if (key_frames[_input_key_right_shift] < key_frames[_input_key_left_shift]) {
            return key_frames[_input_key_left_shift];
        }
        return key_frames[_input_key_right_shift];

    case 1: // _input_key_any_control
        if (key_frames[_input_key_right_control] < key_frames[_input_key_left_control]) {
            return key_frames[_input_key_left_control];
        }
        return key_frames[_input_key_right_control];

    case 2: // _input_key_any_windows
        if (key_frames[_input_key_left_windows] <= key_frames[_input_key_right_windows]) {
            return key_frames[_input_key_right_windows];
        }
        return key_frames[_input_key_left_windows];

    case 3: // _input_key_any_alt
        if (key_frames[_input_key_left_alt] <= key_frames[_input_key_right_alt]) {
            return key_frames[_input_key_right_alt];
        }
        return key_frames[_input_key_left_alt];

    default:
        if (key_index != -1) {
            for (i = 0; i < k_input_key_block_timer_count; i++) {
                if (key_block_timers[i].key == key_index) {
                    return 0;
                }
            }
        }
        return key_frames[key_index];
    }
}

#if 0
Original Ghidra decompilation (0x490b50):

uint FUN_00490b50(void)

{
  uint in_EAX;
  uint uVar1;
  undefined3 uVar3;
  short *psVar2;
  short in_CX;

  uVar1 = in_EAX & 0xffffff00;
  if (DAT_006b15f9 == '\0') {
    psVar2 = (short *)(in_CX + -0x6e);
    uVar3 = (undefined3)((uint)psVar2 >> 8);
    switch(psVar2) {
    case (short *)0x0:
      uVar1 = CONCAT31(uVar3,DAT_006b1664);
      if (DAT_006b1664 < DAT_006b1659) {
        return CONCAT31(uVar3,DAT_006b1659);
      }
      break;
    case (short *)0x1:
      uVar1 = CONCAT31(uVar3,DAT_006b166c);
      if (DAT_006b166c < DAT_006b1665) {
        return CONCAT31(uVar3,DAT_006b1665);
      }
      break;
    case (short *)0x2:
      uVar1 = CONCAT31(uVar3,DAT_006b1666);
      if (DAT_006b1666 <= DAT_006b166a) {
        return CONCAT31(uVar3,DAT_006b166a);
      }
      break;
    case (short *)0x3:
      uVar1 = CONCAT31(uVar3,DAT_006b1667);
      if (DAT_006b1667 <= DAT_006b1669) {
        return CONCAT31(uVar3,DAT_006b1669);
      }
      break;
    default:
      if (in_CX != -1) {
        psVar2 = &DAT_006b1604;
        do {
          if (in_CX == *psVar2) {
            return (uint)psVar2 & 0xffffff00;
          }
          psVar2 = psVar2 + 4;
        } while ((int)psVar2 < 0x6b1624);
      }
      return CONCAT31((int3)((uint)psVar2 >> 8),(&DAT_006b1620)[in_CX]);
    }
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
