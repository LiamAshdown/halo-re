// server_list_compare_by_string_key  (Ghidra: FUN_004b6be0, still unnamed -> renamed)
// address 0x4b6be0, size 59 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("sort comparator that compares a
// string field (key supplied by the caller) between two server-list entries, honoring the
// current sort direction"); mirrors server_list_compare_by_mapname.c's entry-pointer-pointer
// (EAX/ECX) convention for the same array-of-entry-pointers shape.
// register convention: entry-pointer-pointer `a` in EAX (in_EAX), `b` in ECX (in_ECX), key
// string in EDX -- Ghidra shows SBServerGetStringValue called with only one visible argument at each call
// site (the entry), confirming (per the module summary) that the key itself is a parameter of
// this function, not a literal, forwarded unchanged into both accessor calls.
// UNSURE: SBServerGetStringValue's default-value argument is not visible at either call site; assumed to
// be an empty string, matching this module's other SBServerGetStringValue call sites.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t server_browser_sort_ascending; // 0x006953f8

extern char *SBServerGetStringValue(void *entry, const char *key, const char *default_value); // foreign, GameSpy library
extern int32_t __stricmp(const char *a, const char *b);

// blam-cc: entry-pointer-pointer `a` in EAX (in_EAX), `b` in ECX (in_ECX), key string in EDX
int32_t server_list_compare_by_string_key(void **a, void **b, const char *key)
{
    char *string_b;
    char *string_a;
    int32_t result;

    string_b = SBServerGetStringValue(*b, key, "");
    string_a = SBServerGetStringValue(*a, key, "");
    result = __stricmp(string_a, string_b);
    if (server_browser_sort_ascending == 0) {
        result = -result;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4b6be0):

int FUN_004b6be0(void)

{
  undefined4 uVar1;
  undefined4 *in_EAX;
  char *_Str2;
  char *_Str1;
  int iVar2;
  undefined4 *in_ECX;

  uVar1 = *in_EAX;
  _Str2 = (char *)FUN_00617490(*in_ECX);
  _Str1 = (char *)FUN_00617490(uVar1);
  iVar2 = __stricmp(_Str1,_Str2);
  if (DAT_006953f8 == '\0') {
    iVar2 = -iVar2;
  }
  return iVar2;
}
#endif
