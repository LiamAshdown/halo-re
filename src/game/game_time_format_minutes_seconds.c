// game_time_format_minutes_seconds  (Ghidra: game_time_format_minutes_seconds, already named)
// address 0x466530, size 198 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: the three "DAT_"/"PTR_s_parameter_handles_..." operands Ghidra shows are all
//   compile-time wide-string format templates, read straight out of bin/halo.exe (.rdata):
//   0x006607a8 = L" " (a single space), 0x006607a0 = L"%d", 0x00660798 = L"0%d", and the
//   strings-referenced "%s:%s" is the final combine. They are reproduced here as ordinary wide
//   string literals rather than as opaque externs.
// register convention: tick count in ECX (in_ECX); Ghidra's own two stack parameters are kept
//   as-is -- the first (`unused`) is never read anywhere in the body, and the second is the
//   destination buffer, used only in the final combine call.
//   // blam-cc: ECX -> ticks, stack -> (unused, dest)

#include "tags.h"
#include <wchar.h>

// FIXED 2026-09-28 (retail-independence loop), from objdump 0x466530..0x4665f5: every string_format_wide_va_bounded
//   call passes its character count in EDX (0x40 for the two parts, the caller's second argument -- 0x100 at every
//   call site -- for the result), which the earlier version dropped; the tick count is signed (idiv by 30 and 60);
//   the formats are L" " (0x006607a8), L"%d" (0x006607a0), L"0%d" (0x00660798) and L"%s:%s" (0x0066078c).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, blam-cc: EDX count

void game_time_format_minutes_seconds(uint32_t ticks, uint32_t count, wchar_t *dest)
{
    int32_t total_seconds = (int32_t)ticks / 30;
    int32_t minutes = total_seconds / 60;
    int32_t seconds = total_seconds - minutes * 60;
    uint16_t minutes_text[0x40];
    uint16_t seconds_text[0x40];

    if (minutes == 0) {
        string_format_wide_va_bounded(0x40, minutes_text, (const uint16_t *)L" ");
    } else {
        string_format_wide_va_bounded(0x40, minutes_text, (const uint16_t *)L"%d", minutes);
    }
    string_format_wide_va_bounded(0x40, seconds_text, (const uint16_t *)(seconds <= 9 ? L"0%d" : L"%d"), seconds);
    string_format_wide_va_bounded(count, (uint16_t *)dest, (const uint16_t *)L"%s:%s", minutes_text, seconds_text);
}

#if 0
Original Ghidra decompilation (0x466530), from tools/pack.py 0x466530:

void game_time_format_minutes_seconds(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int in_ECX;
  int iVar2;
  undefined **ppuVar3;
  undefined1 local_100 [128];
  undefined1 local_80 [128];

  iVar1 = (in_ECX / 0x1e) / 0x3c;
  iVar2 = (in_ECX / 0x1e) % 0x3c;
  if (iVar1 == 0) {
    string_format_wide_va_bounded(local_80,&DAT_006607a8);
  }
  else {
    string_format_wide_va_bounded(local_80,&PTR_s_parameter_handles_0063fff0_0x35_006607a0,iVar1);
  }
  if (iVar2 < 10) {
    ppuVar3 = (undefined **)&DAT_00660798;
  }
  else {
    ppuVar3 = &PTR_s_parameter_handles_0063fff0_0x35_006607a0;
  }
  string_format_wide_va_bounded(local_100,ppuVar3,iVar2);
  string_format_wide_va_bounded(param_2,L"%s:%s",local_80,local_100);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
