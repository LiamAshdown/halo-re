// parse_time_duration_string  (Ghidra: parse_time_duration_string, already named)
// address 0x4e51c0, size 213 bytes
// name confidence: 0.7   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md ("Parses a numeric duration string with an
// optional d/h/m/s unit suffix and returns the equivalent number of seconds, or -1 on invalid
// input"); the case 'd'/'h'/'m'/'s' multipliers (86400/3600/60/1) confirm it.
// register convention: Ghidra recognized both stack parameters (default_unit, unit_table); the
// string itself arrives in EAX, confirmed by every caller in this batch (sv_ban.c, sv_ban_penalty.c,
// sv_tk_grace.c, sv_tk_cooldown.c) loading a console-argument pointer into EAX immediately before
// the call.
//   // blam-cc: EAX -> string, stack -> default_unit, stack -> unit_table
// UNSURE: strchr's exact identity (foreign, < this module's start); its shape (a table
// pointer plus a character, returning nonzero on membership) is consistent with a "does this
// char appear in this string" test such as strchr, so this rewrite treats it as one.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include <ctype.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t default_time_unit_table[]; // 0x00699568, used when unit_table is NULL

// Parses a leading unsigned integer from `string`, then an optional one-character unit suffix
// (falling back to default_unit if the suffix is not one strchr recognizes in unit_table,
// or NULL selects the built-in table). Returns the value in seconds for d/h/m/s, or -1 if the
// string has no leading digits or its resolved unit is not one of those four.
int32_t parse_time_duration_string(char *string, char default_unit, uint8_t *unit_table)
    // blam-cc: EAX -> string, stack -> default_unit, stack -> unit_table
{
    char resolved_unit;
    char unit_from_table;
    int32_t value;
    char *p;

    if (unit_table == 0) {
        unit_table = default_time_unit_table;
    }
    resolved_unit = (char)tolower((uint8_t)default_unit);
    value = atol(string);
    if (value == 0) {
        if (!isdigit((uint8_t)*string)) {
            return -1;
        }
    } else {
        unit_from_table = resolved_unit;
        p = string;
        if (*p != 0) {
            while (isdigit((uint8_t)*p)) {
                p = p + 1;
                if (*p == 0) {
                    goto have_unit;
                }
            }
        }
        if (*p != 0) {
            int32_t lowered = tolower((uint8_t)*p);
            unit_from_table = (char)lowered;
            if (strchr((char *)unit_table, lowered) == 0) {
                unit_from_table = resolved_unit;
            }
        }
    have_unit:
        resolved_unit = unit_from_table;
        if (value == -1) {
            return -1;
        }
    }
    switch (resolved_unit) {
    case 'd': return value * 0x15180;
    case 'h': return value * 0xe10;
    case 'm': return value * 0x3c;
    case 's': return value;
    default: return -1;
    }
}

#if 0
Original Ghidra decompilation (0x4e51c0), from tools/pack.py 0x4e51c0:

long parse_time_duration_string(char param_1,undefined *param_2)

{
  char *pcVar1;
  char cVar2;
  char cVar3;
  char *in_EAX;
  int iVar4;
  long lVar5;
  int iVar6;

  if (param_2 == (undefined *)0x0) {
    param_2 = PTR_DAT_00699568;
  }
  iVar4 = _tolower((int)param_1);
  cVar3 = (char)iVar4;
  lVar5 = _atol(in_EAX);
  if (lVar5 == 0) {
    iVar4 = _isdigit((int)*in_EAX);
    if (iVar4 == 0) {
      return -1;
    }
  }
  else {
    cVar2 = cVar3;
    if (*in_EAX == '\0') {
LAB_004e522d:
      if (*in_EAX != '\0') {
        iVar4 = _tolower((int)*in_EAX);
        iVar6 = FUN_006257e0(param_2,(int)(char)iVar4);
        cVar2 = (char)iVar4;
        if (iVar6 == 0) {
          cVar2 = cVar3;
        }
      }
    }
    else {
      do {
        iVar4 = _isdigit((int)*in_EAX);
        if (iVar4 == 0) goto LAB_004e522d;
        pcVar1 = in_EAX + 1;
        in_EAX = in_EAX + 1;
      } while (*pcVar1 != '\0');
    }
    cVar3 = cVar2;
    if (lVar5 == -1) {
      return -1;
    }
  }
  switch(cVar3) {
  case 'd':
    return lVar5 * 0x15180;
  default:
    return -1;
  case 'h':
    return lVar5 * 0xe10;
  case 'm':
    return lVar5 * 0x3c;
  case 's':
    return lVar5;
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
