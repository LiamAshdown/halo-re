import os
os.chdir(r'C:\Users\Liam-\halo-re')
p = 'src/game/game_time_format_minutes_seconds.c'
s = open(p, encoding='utf-8').read()
start = s.index('#include "tags.h"')
end = s.index('#if 0') if '#if 0' in s else len(s)
new = r'''#include "tags.h"
#include <wchar.h>

// FIXED 2026-09-28 (retail-independence loop), from objdump 0x466530..0x4665f5: every string_format_wide_va_bounded
//   call passes its character count in EDX (0x40 for the two parts, the caller's second argument -- 0x100 at every
//   call site -- for the result), which the earlier version dropped; the tick count is signed (idiv by 30 and 60);
//   the formats are L" " (0x006607a8), L"%d" (0x006607a0), L"0%d" (0x00660798) and L"%s:%s" (0x0066078c).
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

'''
s = s[:start] + new + s[end:]
s = s.replace('rewrite confidence: 0.7', 'rewrite confidence: 0.85', 1)
open(p, 'w', encoding='utf-8').write(s)

open('src/game/game_time_format_minutes_seconds_ascii.c', 'w', encoding='utf-8').write(r'''// game_time_format_minutes_seconds_ascii  (not a Ghidra function; the ASCII twin of game_time_format_minutes_seconds,
//   used by the King / Oddball GameSpy player query hooks)
// address 0x466600, size 188 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x466600..0x4666bb: ticks / 30 as seconds; minutes (" " when 0, else "%d") and
//   seconds ("0%d" below 10, else "%d") into 0x40 byte parts with _snprintf, then "%s:%s" into the destination
//   bounded by count.
// blam-cc: ECX ticks, stack -> count, dest

#include "tags.h"
#include <stdio.h>

void game_time_format_minutes_seconds_ascii(uint32_t ticks, uint32_t count, char *dest)
{
    int32_t total_seconds = (int32_t)ticks / 30;
    int32_t minutes = total_seconds / 60;
    int32_t seconds = total_seconds - minutes * 60;
    char minutes_text[0x40];
    char seconds_text[0x40];

    if (minutes == 0) {
        _snprintf(minutes_text, 0x40, " "); // 0x00660788
    } else {
        _snprintf(minutes_text, 0x40, "%d", minutes); // 0x0065fb30
    }
    _snprintf(seconds_text, 0x40, seconds <= 9 ? "0%d" : "%d", seconds); // 0x00660784
    _snprintf(dest, count, "%s:%s", minutes_text, seconds_text); // 0x0066077c
}
''')
print('ok')
