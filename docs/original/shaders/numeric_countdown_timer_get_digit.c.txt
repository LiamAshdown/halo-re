// numeric_countdown_timer_get_digit  (Ghidra: game_timer_get_digit_pair)
// address 0x5400c0, size 329 bytes
// name confidence: 0.75   rewrite confidence: 0.9 (phase-4 review, objdump checked)
// evidence: out/phase4/shaders_types_notes.md. Ghidra's "digit_pair"/CONCAT22 name is
// misleading: every case ends in mov ax,dx after a 32-bit idiv, so the upper half of EAX is
// just the leftover quotient (or, for index -1, whatever the load left there). Only AX is the
// result: the hs caller 0x47ae9d stores AX, and the chicago draws 0x5320ee / 0x532c91 store EAX
// but only ever pass it on as the int16 frame argument of 0x518960. Three callers: the hs
// evaluator of the script call numeric_countdown_timer_get (0x47ae60, the source of the name)
// and the numeric transparent_chicago / chicago_extended draws (0x5320e9, 0x532c8c).
// register convention: EAX = digit_index (in_AX, sign-extended to EAX on entry then only the
// low word used again) -> AX = digit. EDX is always clobbered (xor edx,edx on entry).
// objdump (0x5400c0..0x540208) confirms Ghidra's switch table exactly: `movsx eax,ax; xor
// edx,edx; inc eax; cmp eax,0x9; ja <default>; jmp [eax*4+0x54020c]`, ten jump-table cases
// (index 0 = digit_index -1, ..1 = digit_index 0, ... 9 = digit_index 8) each computing the
// truncating-division formula in the enum comments below via a magic-multiply /10 or /6, and a
// default that falls straight into the case-0 tail with EDX still zero from entry -- i.e.
// returns 0 for any digit_index outside -1..8. No case does any clamping beyond the modulo
// itself; if numeric_countdown_timer_remaining_ms is negative (should not happen -- 0x540240
// clamps it at 0) the C '%' below matches idiv's truncating semantics exactly.

#include "tags.h"
#include "shaders.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t numeric_countdown_timer_remaining_ms; // 0x00721e50

int16_t numeric_countdown_timer_get_digit(int16_t digit_index) // blam-cc: AX -> digit_index
{
    switch (digit_index) {
    case _numeric_countdown_timer_raw:
        return (int16_t)numeric_countdown_timer_remaining_ms;
    case _numeric_countdown_timer_millisecond_ones:
        return (int16_t)(numeric_countdown_timer_remaining_ms % 10);
    case _numeric_countdown_timer_millisecond_tens:
        return (int16_t)(numeric_countdown_timer_remaining_ms / 10 % 10);
    case _numeric_countdown_timer_millisecond_hundreds:
        return (int16_t)(numeric_countdown_timer_remaining_ms / 100 % 10);
    case _numeric_countdown_timer_second_ones:
        return (int16_t)(numeric_countdown_timer_remaining_ms / 1000 % 10);
    case _numeric_countdown_timer_second_tens:
        return (int16_t)(numeric_countdown_timer_remaining_ms / 10000 % 6);
    case _numeric_countdown_timer_minute_ones:
        return (int16_t)(numeric_countdown_timer_remaining_ms / 60000 % 10);
    case _numeric_countdown_timer_minute_tens:
        return (int16_t)(numeric_countdown_timer_remaining_ms / 600000 % 6);
    case _numeric_countdown_timer_hour_ones:
        return (int16_t)(numeric_countdown_timer_remaining_ms / 3600000 % 10);
    case _numeric_countdown_timer_hour_tens:
        return (int16_t)(numeric_countdown_timer_remaining_ms / 36000000 % 10);
    default:
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x5400c0):

undefined4 game_timer_get_digit_pair(void)

{
  short in_AX;
  int iVar1;
  undefined2 uVar2;

  uVar2 = 0;
  iVar1 = in_AX + 1;
  switch(iVar1) {
  case 0:
    return CONCAT22((short)((uint)iVar1 >> 0x10),(short)DAT_00721e50);
  case 1:
    return CONCAT22((short)((uint)(DAT_00721e50 / 10) >> 0x10),(short)(DAT_00721e50 % 10));
  case 2:
    return CONCAT22((short)((uint)((DAT_00721e50 / 10) / 10) >> 0x10),
                    (short)((DAT_00721e50 / 10) % 10));
  case 3:
    return CONCAT22((short)((uint)((DAT_00721e50 / 100) / 10) >> 0x10),
                    (short)((DAT_00721e50 / 100) % 10));
  case 4:
    return CONCAT22((short)((uint)((DAT_00721e50 / 1000) / 10) >> 0x10),
                    (short)((DAT_00721e50 / 1000) % 10));
  case 5:
    return CONCAT22((short)((uint)((DAT_00721e50 / 10000) / 6) >> 0x10),
                    (short)((DAT_00721e50 / 10000) % 6));
  case 6:
    return CONCAT22((short)((uint)((DAT_00721e50 / 60000) / 10) >> 0x10),
                    (short)((DAT_00721e50 / 60000) % 10));
  case 7:
    return CONCAT22((short)((uint)((DAT_00721e50 / 600000) / 6) >> 0x10),
                    (short)((DAT_00721e50 / 600000) % 6));
  case 8:
    return CONCAT22((short)((uint)((DAT_00721e50 / 3600000) / 10) >> 0x10),
                    (short)((DAT_00721e50 / 3600000) % 10));
  case 9:
    iVar1 = (DAT_00721e50 / 36000000) / 10;
    uVar2 = (undefined2)((DAT_00721e50 / 36000000) % 10);
  }
  return CONCAT22((short)((uint)iVar1 >> 0x10),uVar2);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
