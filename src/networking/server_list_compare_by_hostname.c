// server_list_compare_by_hostname  (not a Ghidra function; a server browser qsort comparator returned by
//   server_browser_sort_comparator_select; no C existed, so it trapped as server_list_compare_by_hostname)
// address 0x4b6cd0, size 193 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b6cd0..0x4b6d90: hostname (0x0066b0d4), then gametype (0x0066b0c0), then the
//   element addresses, all in the sort direction (0x006953f8).
// blam-cc: cdecl (qsort comparator over server pointers)

#include "crt.h"
#include "tags.h"
#include "fn_networking.h"

extern uint8_t server_browser_sort_ascending; // 0x006953f8
extern const char *SBServerGetStringValue(void *server, const char *key, const char *default_value); // 0x617490 SBServerGetStringValue
extern int32_t SBServerGetIntValue(void *server, const char *key, int32_t default_value); // 0x617c10 SBServerGetIntValue

// the final tie-break on the element addresses, in the sort direction
static int32_t address_order(const void *a, const void *b)
{
    if (server_browser_sort_ascending != 0) {
        return (uint32_t)b > (uint32_t)a ? -1 : ((uint32_t)b < (uint32_t)a ? 1 : 0);
    }
    return (uint32_t)b > (uint32_t)a ? 1 : ((uint32_t)b < (uint32_t)a ? -1 : 0);
}

// _stricmp of one string key of the two servers, in the sort direction
static int32_t key_order(const void *a, const void *b, const char *key)
{
    const char *string_b = SBServerGetStringValue(*(void **)b, key, "");
    const char *string_a = SBServerGetStringValue(*(void **)a, key, "");
    int32_t result = _stricmp(string_a, string_b);

    return server_browser_sort_ascending != 0 ? result : -result;
}

int32_t server_list_compare_by_hostname(const void *a, const void *b)
{
    int32_t result = key_order(a, b, "hostname");

    if (result != 0) {
        return result;
    }
    result = key_order(a, b, "gametype");
    if (result != 0) {
        return result;
    }
    return address_order(a, b);
}
