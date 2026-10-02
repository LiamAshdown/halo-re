// format_local_time_and_date  (Ghidra: format_local_time_and_date, already named)
// address 0x4e52c0, size 86 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md; disassembly (objdump -d -M intel) shows this
// function only saves/restores ESI (its own scratch), never touching EBX or EDI, which proves
// they are pass-through parameters received from its own caller and forwarded unchanged into
// format_time_and_date_strings (this batch).
// register convention: Ghidra recognized both stack parameters (time_value, time_dest); EBX and
// EDI are pass-through, not read or written by this function's own body.
//   // blam-cc: EBX -> date_dest, EDI -> max_len, stack -> time_value, stack -> time_dest
// UNSURE: this function reuses its own incoming time_value stack slot's address as the `time_t *`
// passed to localtime (a common MSVC space-saving idiom); this rewrite uses an ordinary local
// copy instead, which is observably identical.

#include "tags.h"
#include "memory.h"
#include <time.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void format_time_and_date_strings(char *date_dest, struct tm *time_value, int32_t max_len,
    char *time_dest); // this batch, 0x4e5320

// Converts time_value to local time and formats it into time_dest ("HH:MM:SS") and date_dest
// ("YYYY-MM-DD") via format_time_and_date_strings, falling back to an all-zero broken-down time
// if localtime() fails.
void format_local_time_and_date(char *date_dest, int32_t max_len, int32_t time_value, char *time_dest)
    // blam-cc: EBX -> date_dest, EDI -> max_len, stack -> time_value, stack -> time_dest
{
    time_t t = time_value;
    struct tm zero_tm;
    struct tm *tm_now;

    tm_now = localtime(&t);
    if (tm_now == 0) {
        zero_tm.tm_sec = 0;
        zero_tm.tm_min = 0;
        zero_tm.tm_hour = 0;
        zero_tm.tm_mday = 0;
        zero_tm.tm_mon = 0;
        zero_tm.tm_year = 0;
        zero_tm.tm_wday = 0;
        zero_tm.tm_yday = 0;
        zero_tm.tm_isdst = 0;
        tm_now = &zero_tm;
    }
    format_time_and_date_strings(date_dest, tm_now, max_len, time_dest);
}

#if 0
Original Ghidra decompilation (0x4e52c0), from tools/pack.py 0x4e52c0:

void format_local_time_and_date(undefined4 param_1,undefined4 param_2)

{
  _localtime((time_t *)&param_1);
  format_time_and_date_strings(param_2);
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) fills in the register setup Ghidra dropped:
  4e52c0: sub esp,0x24
  4e52c3..4e52e1: zero seven dwords (the fallback broken-down-time scratch)
  4e52dd: lea ecx,[esp+0x28]         ; &time_value (the caller's own pushed stack slot)
  4e52e5: push ecx
  4e52f2: call localtime
  4e52fa: test eax,eax
  4e52fc: jne 0x4e5301
  4e52fe: lea eax,[esp]             ; fallback: the zeroed scratch above
  4e5301: mov edx,[esp+0x2c]        ; edx = time_dest (2nd stack arg)
  4e5306: mov esi,eax               ; esi = tm pointer (localtime's result or the fallback)
  4e5309: call format_time_and_date_strings   ; EBX, EDI still whatever this function's own
                                               ; caller set them to
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
