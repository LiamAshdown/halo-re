// format_time_and_date_strings  (Ghidra: format_time_and_date_strings, already named)
// address 0x4e5320, size 98 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md; literal "%02d:%02d:%02d" / "%04d-%02d-%02d"
// formats matching a standard struct tm's hour/min/sec and year/mon/mday fields (in that
// canonical order: tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year).
// register convention: Ghidra recognized only the time-string stack parameter; the disassembly
// (objdump -d -M intel) of this function together with its one caller (format_local_time_and_date,
// this batch) shows EBX carries the date destination, ESI the source struct tm and EDI the
// shared maximum length, all three simply forwarded unchanged from format_local_time_and_date's
// own incoming registers.
//   // blam-cc: EBX -> date_dest, ESI -> time_value, EDI -> max_len, stack -> time_dest
// UNSURE: whether the source object is truly a full struct tm or just an anonymous 6-int record
// with the same layout; only the first six fields (sec..year) are ever read here.

#include "tags.h"
#include "memory.h"
#include <stdio.h>
#include <time.h>

// VERIFIED against disassembly 0x4e5320..0x4e5381 (2026-09-30); fixed: the original calls the CRT _snprintf (0x623a2d),
//   which writes count characters and does NOT NUL-terminate on truncation (the C99 snprintf the draft used writes a
//   NUL at count-1); the forced NUL at max_len-1 is the only terminator, as here.
// Formats time_value's hour:min:sec into time_dest and its year-mon-mday into date_dest, each
// truncated to at most max_len-1 characters with a forced NUL. Either destination may be NULL
// (skipped), and nothing is written if max_len is 0.
void format_time_and_date_strings(char *date_dest, struct tm *time_value, int32_t max_len,
    char *time_dest) // blam-cc: EBX -> date_dest, ESI -> time_value, EDI -> max_len, stack -> time_dest
{
    if (time_dest != 0 && max_len != 0) {
        _snprintf(time_dest, max_len - 1, "%02d:%02d:%02d", time_value->tm_hour, time_value->tm_min, time_value->tm_sec);
        time_dest[max_len - 1] = 0;
    }
    if (date_dest != 0 && max_len != 0) {
        _snprintf(date_dest, max_len - 1, "%04d-%02d-%02d", time_value->tm_year + 0x76c, time_value->tm_mon + 1, time_value->tm_mday);
        date_dest[max_len - 1] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4e5320), from tools/pack.py 0x4e5320:

void format_time_and_date_strings(char *param_1)

{
  char *unaff_EBX;
  undefined4 *unaff_ESI;
  int unaff_EDI;

  if ((param_1 != (char *)0x0) && (unaff_EDI != 0)) {
    __snprintf(param_1,unaff_EDI - 1,"%02d:%02d:%02d",unaff_ESI[2],unaff_ESI[1],*unaff_ESI);
    param_1[unaff_EDI + -1] = '\0';
  }
  if ((unaff_EBX != (char *)0x0) && (unaff_EDI != 0)) {
    __snprintf(unaff_EBX,unaff_EDI - 1,"%04d-%02d-%02d",unaff_ESI[5] + 0x76c,unaff_ESI[4] + 1,
               unaff_ESI[3]);
    unaff_EBX[unaff_EDI + -1] = '\0';
  }
  return;
}
#endif
