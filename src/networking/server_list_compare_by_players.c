// server_list_compare_by_players  (not a Ghidra function; a server browser qsort comparator returned by
//   server_browser_sort_comparator_select; no C existed, so it trapped as server_list_compare_by_players)
// address 0x4b6e70, size 169 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b6e70..0x4b6f18: numplayers (0x0066b0a4), then maxplayers (0x0066b098) -- the
//   smaller count first -- then hostname through server_list_compare_by_string_key (EAX a, ECX b, ESI key; it already
//   applies the direction, and the address tie-break inside is ascending-shaped), with the whole result negated when
//   descending.
// blam-cc: cdecl (qsort comparator over server pointers)

#include "tags.h"

extern uint8_t server_browser_sort_ascending; // 0x006953f8
extern const char *SBServerGetStringValue(void *server, const char *key, const char *default_value); // 0x617490 SBServerGetStringValue
extern int32_t SBServerGetIntValue(void *server, const char *key, int32_t default_value); // 0x617c10 SBServerGetIntValue
extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b

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
    int32_t result = __stricmp(string_a, string_b);

    return server_browser_sort_ascending != 0 ? result : -result;
}

extern int32_t server_list_compare_by_string_key(void **a, void **b, const char *key); // 0x4b6be0

int32_t server_list_compare_by_players(const void *a, const void *b)
{
    void *server_a = *(void **)a;
    void *server_b = *(void **)b;
    int32_t players_a = SBServerGetIntValue(server_a, "numplayers", 0);
    int32_t players_b = SBServerGetIntValue(server_b, "numplayers", 0);
    int32_t maximum_a = SBServerGetIntValue(server_a, "maxplayers", 0);
    int32_t maximum_b = SBServerGetIntValue(server_b, "maxplayers", 0);
    int32_t result;

    if (players_a != players_b) {
        result = players_a < players_b ? 1 : -1;
    } else if (maximum_a != maximum_b) {
        result = maximum_a < maximum_b ? 1 : -1;
    } else {
        result = server_list_compare_by_string_key((void **)a, (void **)b, "hostname");
        if (result == 0) {
            result = (uint32_t)b > (uint32_t)a ? -1 : ((uint32_t)b < (uint32_t)a ? 1 : 0);
        }
    }
    return server_browser_sort_ascending != 0 ? result : -result;
}
