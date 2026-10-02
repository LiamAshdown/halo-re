// server_list_compare_by_ping_then_hostname  (Ghidra: server_list_compare_by_ping_then_hostname,
// already named)
// address 0x4b6da0, size 205 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_types_notes.md "server browser" section documents
// SBServerGetIntValue as the GameSpy int accessor and SBServerGetStringValue as the string accessor;
// server_browser_sort_comparator_select.c already declares this function's qsort-compatible
// (const void *, const void *) prototype.
// register convention: __cdecl, both parameters real (Ghidra-recognized param_1/param_2) --
// pointers to array elements (each element itself a GameSpy entry pointer), i.e. genuine qsort
// comparator arguments.
// UNSURE: DAT_0066b090 (the "ping" key string, inferred from SBServerGetIntValue's role) is not in
// this function's own literal-strings list, so it is presumably a shared key string defined
// elsewhere; declared here as an opaque extern.
// UNSURE: DAT_006953f8 (sort direction) matches server_list_compare_by_mapname.c's
// `server_browser_sort_ascending` by address and by identical use (negate unless nonzero).
// UNSURE: on a full tie (equal ping and equal hostname), the original falls back to comparing
// the two *array element addresses* themselves (not the entries), which only gives a stable
// sort relative to qsort's own element movement, not to entry identity; preserved exactly.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t server_browser_sort_ascending; // 0x006953f8
extern char DAT_0066b090[]; // see UNSURE, presumably "ping"

extern int32_t SBServerGetIntValue(void *entry, const char *key, int32_t default_value); // foreign, GameSpy int accessor // foreign, GameSpy library, int accessor
extern char *SBServerGetStringValue(void *entry, const char *key, const char *default_value); // foreign, GameSpy library, string accessor

int32_t server_list_compare_by_ping_then_hostname(void **a, void **b)
{
    int32_t ping_a;
    int32_t ping_b;
    int32_t ping_diff;
    char *hostname_a;
    char *hostname_b;
    int32_t hostname_diff;

    ping_a = SBServerGetIntValue(*a, DAT_0066b090, 0);
    ping_b = SBServerGetIntValue(*b, DAT_0066b090, 0);
    if (ping_a == 0) {
        ping_a = 9999;
    }
    if (ping_b == 0) {
        ping_b = 9999;
    }
    ping_diff = ping_a - ping_b;
    if (ping_diff == 0) {
        hostname_b = SBServerGetStringValue(*b, "hostname", "");
        hostname_a = SBServerGetStringValue(*a, "hostname", "");
        hostname_diff = _stricmp(hostname_a, hostname_b);
        if (server_browser_sort_ascending == 0) {
            hostname_diff = -hostname_diff;
        }
        if (hostname_diff == 0) {
            if (server_browser_sort_ascending == 0) {
                if ((void **)b <= (void **)a) {
                    return -(int32_t)((void **)b < (void **)a);
                }
                return 1;
            }
            if ((void **)b <= (void **)a) {
                return (int32_t)((void **)b < (void **)a);
            }
            return -1;
        }
        return hostname_diff;
    }
    if (server_browser_sort_ascending == 0) {
        ping_diff = -ping_diff;
    }
    return ping_diff;
}

#if 0
Original Ghidra decompilation (0x4b6da0):

uint server_list_compare_by_ping_then_hostname(undefined4 *param_1,undefined4 *param_2)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  char *_Str2;
  char *_Str1;
  uint uVar4;

  uVar1 = *param_2;
  iVar2 = FUN_00617c10(*param_1,&DAT_0066b090,0);
  iVar3 = FUN_00617c10(uVar1,&DAT_0066b090,0);
  if (iVar2 == 0) {
    iVar2 = 9999;
  }
  if (iVar3 == 0) {
    iVar3 = 9999;
  }
  uVar4 = iVar2 - iVar3;
  if (uVar4 == 0) {
    uVar1 = *param_1;
    _Str2 = (char *)FUN_00617490(*param_2,"hostname",&DAT_0065512c);
    _Str1 = (char *)FUN_00617490(uVar1,"hostname",&DAT_0065512c);
    uVar4 = __stricmp(_Str1,_Str2);
    if (DAT_006953f8 == '\0') {
      uVar4 = -uVar4;
    }
    if (uVar4 == 0) {
      if (DAT_006953f8 == '\0') {
        if (param_2 <= param_1) {
          return -(uint)(param_2 < param_1);
        }
        return 1;
      }
      if (param_2 <= param_1) {
        return (uint)(param_2 < param_1);
      }
      return 0xffffffff;
    }
  }
  else if (DAT_006953f8 == '\0') {
    uVar4 = -uVar4;
  }
  return uVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
