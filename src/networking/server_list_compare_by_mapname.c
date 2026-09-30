// server_list_compare_by_mapname  (Ghidra: server_list_compare_by_mapname, already named)
// address 0x4b6c20, size 162 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("sort comparator that compares the
// friendly map name between two server-list entries, honoring the current sort direction");
// out/phase2/networking's decompile of server_list_compare_by_mapname_then_hostname shows the
// only call site, `server_list_compare_by_mapname();` with zero visible arguments -- since that
// caller's own two parameters are real (const void *) arguments, EAX/ECX here must be an
// internal register-based shorthand the caller sets up rather than this function's own
// (non-existent) parameters.
// register convention: entry-pointer-pointer `a` in EAX (in_EAX), entry-pointer-pointer `b` in
// ECX (in_ECX) -- this is a private helper, not itself registered as a qsort comparator, so it
// does not need qsort's (const void *, const void *) stack convention.
// UNSURE: SBServerGetStringValue's return value (the raw "mapname" string) and map_list_get_friendly_
// level_name's map-name input are both shown with no visible argument at this call site;
// reconstructed as an EAX pass-through between the two calls, matching this module's
// established pattern for chained foreign calls.
// UNSURE: DAT_006953f8 (the sort-direction flag) has no documented name; declared here as
// `server_browser_sort_ascending`, distinct from server_browser_sort_column (0x00719489) which
// server_browser_sort_comparator_select.c already documents.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <wchar.h>
#include <string.h>

extern uint8_t server_browser_sort_ascending; // 0x006953f8, see UNSURE

extern char *SBServerGetStringValue(void *entry, const char *key, const char *default_value); // foreign, GameSpy library
extern void map_list_get_friendly_level_name(const char *map_name, wchar_t out_buffer[0x20]); // foreign, outside this session's range

// blam-cc: entry-pointer-pointer `a` in EAX (in_EAX), `b` in ECX (in_ECX)
int32_t server_list_compare_by_mapname(void **a, void **b)
{
    void *entry_a;
    void *entry_b;
    char *map_name_a;
    char *map_name_b;
    wchar_t friendly_a[0x20];
    wchar_t friendly_b[0x20];
    int32_t result;

    entry_a = *a;
    entry_b = *b;
    memset(friendly_b, 0, sizeof(friendly_b));
    memset(friendly_a, 0, sizeof(friendly_a));
    map_name_a = SBServerGetStringValue(entry_a, "mapname", "");
    map_name_b = SBServerGetStringValue(entry_b, "mapname", "");
    map_list_get_friendly_level_name(map_name_a, friendly_a);
    map_list_get_friendly_level_name(map_name_b, friendly_b);
    result = wcscmp(friendly_a, friendly_b);
    if (server_browser_sort_ascending == 0) {
        result = -result;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4b6c20):

int server_list_compare_by_mapname(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 *in_EAX;
  undefined4 *in_ECX;
  int iVar3;
  undefined4 *puVar4;
  wchar_t local_80;
  undefined4 local_7e [15];
  wchar_t local_40;
  undefined4 local_3e [15];

  uVar1 = *in_EAX;
  uVar2 = *in_ECX;
  local_40 = L'\0';
  puVar4 = local_3e;
  for (iVar3 = 0xf; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = 0;
  local_80 = L'\0';
  puVar4 = local_7e;
  for (iVar3 = 0xf; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined2 *)puVar4 = 0;
  FUN_00617490(uVar1,"mapname",&DAT_0065512c);
  FUN_00617490(uVar2,"mapname",&DAT_0065512c);
  map_list_get_friendly_level_name(&local_40);
  map_list_get_friendly_level_name(&local_80);
  iVar3 = _wcscmp(&local_40,&local_80);
  if (DAT_006953f8 == '\0') {
    iVar3 = -iVar3;
  }
  return iVar3;
}
#endif
