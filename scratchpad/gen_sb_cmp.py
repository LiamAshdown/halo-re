import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')

HEAD = '''#include "tags.h"

extern uint8_t server_browser_sort_ascending; // 0x006953f8
extern const char *FUN_00617490(void *server, const char *key, const char *default_value); // 0x617490 SBServerGetStringValue
extern int32_t FUN_00617c10(void *server, const char *key, int32_t default_value); // 0x617c10 SBServerGetIntValue
extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b
'''
TIE = '''
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
    const char *string_b = FUN_00617490(*(void **)b, key, "");
    const char *string_a = FUN_00617490(*(void **)a, key, "");
    int32_t result = __stricmp(string_a, string_b);

    return server_browser_sort_ascending != 0 ? result : -result;
}
'''


def emit(addr, size, name, note, body):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    src = ('// %s  (not a Ghidra function; a server browser qsort comparator returned by\n'
           '//   server_browser_sort_comparator_select; no C existed, so it trapped as FUN_%08x)\n'
           '// address 0x%x, size %d bytes\n// name confidence: 0.7   rewrite confidence: 0.85\n%s'
           '// blam-cc: cdecl (qsort comparator over server pointers)\n\n%s%s\n%s'
           % (name, addr, addr, size, wr, HEAD, TIE, body.lstrip('\n')))
    open('src/networking/%s.c' % name, 'w', encoding='utf-8').write(src)


emit(0x4b6cd0, 193, 'server_list_compare_by_hostname',
     'hostname (0x0066b0d4), then gametype (0x0066b0c0), then the element addresses, all in the sort direction (0x006953f8).', '''
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
''')
emit(0x4b6fb0, 193, 'server_list_compare_by_gametype',
     'gametype, then hostname, then the element addresses, all in the sort direction.', '''
int32_t server_list_compare_by_gametype(const void *a, const void *b)
{
    int32_t result = key_order(a, b, "gametype");

    if (result != 0) {
        return result;
    }
    result = key_order(a, b, "hostname");
    if (result != 0) {
        return result;
    }
    return address_order(a, b);
}
''')
emit(0x4b6e70, 169, 'server_list_compare_by_players',
     'numplayers (0x0066b0a4), then maxplayers (0x0066b098) -- the smaller count first -- then hostname through server_list_compare_by_string_key (EAX a, ECX b, ESI key; it already applies the direction, and the address tie-break inside is ascending-shaped), with the whole result negated when descending.', '''
extern int32_t server_list_compare_by_string_key(void **a, void **b, const char *key); // 0x4b6be0

int32_t server_list_compare_by_players(const void *a, const void *b)
{
    void *server_a = *(void **)a;
    void *server_b = *(void **)b;
    int32_t players_a = FUN_00617c10(server_a, "numplayers", 0);
    int32_t players_b = FUN_00617c10(server_b, "numplayers", 0);
    int32_t maximum_a = FUN_00617c10(server_a, "maxplayers", 0);
    int32_t maximum_b = FUN_00617c10(server_b, "maxplayers", 0);
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
''')
print('ok')
