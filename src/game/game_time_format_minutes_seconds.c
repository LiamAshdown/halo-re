// game_time_format_minutes_seconds  (Ghidra: game_time_format_minutes_seconds, already named)
// address 0x466530, size 198 bytes
// name confidence: 0.55   rewrite confidence: 0.7
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

extern wchar_t *string_format_wide_va_bounded(wchar_t *dest, const wchar_t *format, ...); // 0x557910

// blam-cc: ECX -> ticks, stack -> (unused, dest)
// Formats a tick count (30 ticks/second) as a "minutes:seconds" wide string into `dest`, e.g.
// "5:07". Minutes are left blank (a single space) when zero; seconds are zero-padded below 10.
// `unused` is Ghidra's own param_1, which no instruction in this function ever reads. Its one
// caller in this module (game_engine_build_kill_feed_message_text, objdump 0x45f0fe) passes
// its own `buffer_size` there, so it is declared as an integer.
void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest)
{
    (void)unused;
    int32_t total_seconds;
    int32_t minutes;
    int32_t seconds;
    wchar_t minutes_text[64];
    wchar_t seconds_text[64];

    total_seconds = ticks / 30;
    minutes = total_seconds / 60;
    seconds = total_seconds % 60;

    if (minutes == 0) {
        string_format_wide_va_bounded(minutes_text, L" ");
    } else {
        string_format_wide_va_bounded(minutes_text, L"%d", minutes);
    }

    if (seconds < 10) {
        string_format_wide_va_bounded(seconds_text, L"0%d", seconds);
    } else {
        string_format_wide_va_bounded(seconds_text, L"%d", seconds);
    }

    string_format_wide_va_bounded(dest, L"%s:%s", minutes_text, seconds_text);
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
