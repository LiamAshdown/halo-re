// game_time_format_minutes_seconds_ascii  (not a Ghidra function; the ASCII twin of game_time_format_minutes_seconds,
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
