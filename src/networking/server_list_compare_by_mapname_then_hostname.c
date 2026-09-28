// server_list_compare_by_mapname_then_hostname  (Ghidra: server_list_compare_by_mapname_then_hostname,
// already named)
// address 0x4b6f20, size 135 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary; server_browser_sort_comparator_select.c
// already declares this function's qsort-compatible (const void *, const void *) prototype;
// server_list_compare_by_mapname.c is this module's own rewrite of the primary-key comparator
// this one falls back to.
// register convention: __cdecl, both parameters real (Ghidra-recognized param_1/param_2) --
// genuine qsort comparator arguments. The bare `server_list_compare_by_mapname()` call here is
// that helper's documented EAX/ECX convention, reconstructed explicitly as (a, b).
// UNSURE: on a full tie (equal map name and equal hostname), falls back to comparing the two
// *array element addresses* themselves, exactly like server_list_compare_by_ping_then_hostname;
// preserved exactly despite the asymmetric ascending/descending branch shapes (this function's
// ascending branch returns 1/-1 while its descending branch returns 0/-1, unlike the ping
// comparator's mirrored shape -- kept as decompiled, not harmonized).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t server_browser_sort_ascending; // 0x006953f8

extern int32_t server_list_compare_by_mapname(void **a, void **b); // 0x4b6c20, this module
extern char *SBServerGetStringValue(void *entry, const char *key, const char *default_value); // foreign, GameSpy library
extern int32_t __stricmp(const char *a, const char *b);

int32_t server_list_compare_by_mapname_then_hostname(void **a, void **b)
{
    int32_t mapname_diff;
    char *hostname_a;
    char *hostname_b;
    int32_t hostname_diff;

    mapname_diff = server_list_compare_by_mapname(a, b);
    if (mapname_diff == 0) {
        hostname_b = SBServerGetStringValue(*b, "hostname", "");
        hostname_a = SBServerGetStringValue(*a, "hostname", "");
        hostname_diff = __stricmp(hostname_a, hostname_b);
        if (server_browser_sort_ascending == 0) {
            hostname_diff = -hostname_diff;
        }
        if (hostname_diff == 0) {
            if (server_browser_sort_ascending != 0) {
                if ((void **)a < (void **)b) {
                    return -1;
                }
                return (int32_t)((void **)b < (void **)a);
            }
            if ((void **)a < (void **)b) {
                return 1;
            }
            return -(int32_t)((void **)b < (void **)a);
        }
        return hostname_diff;
    }
    return mapname_diff;
}

#if 0
Original Ghidra decompilation (0x4b6f20):

uint server_list_compare_by_mapname_then_hostname(undefined4 *param_1,undefined4 *param_2)

{
  undefined4 uVar1;
  uint uVar2;
  char *_Str2;
  char *_Str1;

  uVar2 = server_list_compare_by_mapname();
  if (uVar2 == 0) {
    uVar1 = *param_1;
    _Str2 = (char *)FUN_00617490(*param_2,"hostname",&DAT_0065512c);
    _Str1 = (char *)FUN_00617490(uVar1,"hostname",&DAT_0065512c);
    uVar2 = __stricmp(_Str1,_Str2);
    if (DAT_006953f8 == '\0') {
      uVar2 = -uVar2;
    }
    if (uVar2 == 0) {
      if (DAT_006953f8 != '\0') {
        if (param_1 < param_2) {
          return 0xffffffff;
        }
        return (uint)(param_2 < param_1);
      }
      if (param_1 < param_2) {
        return 1;
      }
      uVar2 = -(uint)(param_2 < param_1);
    }
  }
  return uVar2;
}
#endif
