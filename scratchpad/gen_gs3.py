exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
import os

emit(0x6175f0, 7, 'ArrayLength', 'the element count.', '''
int ArrayLength(DArray array)
{
    return array->count;
}
''')
emit(0x61e140, 48, 'TableCount', 'the element count over every bucket.', '''
int TableCount(HashTable table)
{
    int count = 0;
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        count += ArrayLength(table->buckets[i]);
    }
    return count;
}
''')

# ---------------- sb_server.c
SB = '#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))\n'
emit(0x617340, 26, 'KeyValCompareKeyA', 'the key/value comparator: _stricmp of the two keys (0x628d8b).', '''
int KeyValCompareKeyA(const void *elem1, const void *elem2)
{
    return _stricmp(((const SBKeyValuePair *)elem1)->key, ((const SBKeyValuePair *)elem2)->key);
}
''')
emit(0x617360, 14, 'SBRefStrFree', 'the ref-string table\'s element free: free(the string).', '''
void SBRefStrFree(void *elem)
{
    free((void *)((SBKeyValuePair *)elem)->key);
}
''', name_confidence='0.6')
HASH = '''
// 0x617a40 (EAX string, stack buckets): the case-insensitive string hash (hash * -1664117991 + tolower(c)),
//   unsigned modulo the bucket count. Only KeyValHashKeyA calls it; a static here.
static int string_hash(const char *s, int num_buckets)
{
    unsigned int hash = 0;

    for (; *s != 0; s++) {
        hash = (unsigned int)tolower(*s) - hash * 0x63306ce7;
    }
    return (int)(hash % (unsigned int)num_buckets);
}
'''
emit(0x617ba0, 20, 'KeyValHashKeyA', 'the key/value hash: the string hash (0x617a40, EAX string) of the key.', HASH + '''
int KeyValHashKeyA(const void *elem, int num_buckets)
{
    return string_hash(((const SBKeyValuePair *)elem)->key, num_buckets);
}
''', extra='#include <ctype.h>\n')
emit(0x617bc0, 72, 'SBRefStrHash', 'the per-thread ref-string table (a __declspec(thread) variable, TLS +4), created on first use as TableNew2(8, 500, 4, KeyValHashKeyA, KeyValCompareKeyA, SBRefStrFree).', '''
__declspec(thread) HashTable g_SBRefStrList;

HashTable SBRefStrHash(void)
{
    if (g_SBRefStrList == 0) {
        g_SBRefStrList = TableNew2(sizeof(SBKeyValuePair), 500, 4, KeyValHashKeyA, KeyValCompareKeyA, SBRefStrFree);
    }
    return g_SBRefStrList;
}
''')
emit(0x617370, 66, 'SBRefStrHashCleanup', 'frees this thread\'s ref-string table once it is empty.', '''
extern __declspec(thread) HashTable g_SBRefStrList;

void SBRefStrHashCleanup(void)
{
    if (g_SBRefStrList != 0 && TableCount(g_SBRefStrList) == 0) {
        TableFree(g_SBRefStrList);
        g_SBRefStrList = 0;
    }
}
''', name_confidence='0.6')
emit(0x617490, 49, 'SBServerGetStringValue', 'the value of key in the server\'s key table (+0x18), or the default.', SB + '''
const char *SBServerGetStringValue(void *server, const char *key, const char *default_value)
{
    SBKeyValuePair pair;
    SBKeyValuePair *found;

    pair.key = key;
    found = (SBKeyValuePair *)TableLookup(FIELD(server, 0x18, HashTable), &pair);
    return found != 0 ? found->value : default_value;
}
''')
emit(0x6174d0, 86, 'SBServerGetBoolValue', 'the key as a bool: the default when missing or NULL, 0 for a value starting with 0 F f N n, else 1.', SB + '''
int SBServerGetBoolValue(void *server, const char *key, int default_value)
{
    SBKeyValuePair pair;
    SBKeyValuePair *found;
    char first;

    pair.key = key;
    found = (SBKeyValuePair *)TableLookup(FIELD(server, 0x18, HashTable), &pair);
    if (found == 0 || found->value == 0) {
        return default_value;
    }
    first = found->value[0];
    if (first == '0' || first == 'F' || first == 'f' || first == 'N' || first == 'n') {
        return 0;
    }
    return 1;
}
''')
emit(0x617530, 133, 'SBServerGetPlayerStringValue', 'the value of "<key>_<index>" (0x0064e488 "%s_%d"), or the default.', SB + '''
const char *SBServerGetPlayerStringValue(void *server, int index, const char *key, const char *default_value)
{
    char name[0x80];
    SBKeyValuePair pair;
    SBKeyValuePair *found;

    sprintf(name, "%s_%d", key, index);
    pair.key = name;
    found = (SBKeyValuePair *)TableLookup(FIELD(server, 0x18, HashTable), &pair);
    return found != 0 ? found->value : default_value;
}
''')
emit(0x617c10, 95, 'SBServerGetIntValue', '"ping" (0x0066b090) is the server\'s ping (+0x1c); otherwise atoi of the value, or the default when missing or NULL.', SB + '''
int SBServerGetIntValue(void *server, const char *key, int default_value)
{
    SBKeyValuePair pair;
    SBKeyValuePair *found;

    if (strcmp(key, "ping") == 0) {
        return FIELD(server, 0x1c, int);
    }
    pair.key = key;
    found = (SBKeyValuePair *)TableLookup(FIELD(server, 0x18, HashTable), &pair);
    if (found == 0 || found->value == 0) {
        return default_value;
    }
    return atoi(found->value);
}
''')
simple = [
    (0x6175c0, 12, 'SBServerHasBasicKeys', 'state (+0x14) bit 1.', 'int %s(void *server)', 'return FIELD(server, 0x14, unsigned char) & 1;'),
    (0x6175d0, 12, 'SBServerHasFullKeys', 'state (+0x14) bit 2.', 'int %s(void *server)', 'return FIELD(server, 0x14, unsigned char) & 2;'),
    (0x617620, 12, 'SBServerHasPrivateAddress', 'flags (+0x15) bit 2.', 'int %s(void *server)', 'return FIELD(server, 0x15, unsigned char) & 2;'),
    (0x617630, 12, 'SBServerDirectConnect', 'flags (+0x15) bit 1.', 'int %s(void *server)', 'return FIELD(server, 0x15, unsigned char) & 1;'),
    (0x617aa0, 8, 'SBServerGetPing', 'the ping (+0x1c).', 'int %s(void *server)', 'return FIELD(server, 0x1c, int);'),
    (0x617670, 12, 'SBServerSetNext', 'the list link (+0x20).', 'void %s(void *server, void *next)', 'FIELD(server, 0x20, void *) = next;'),
    (0x617680, 8, 'SBServerGetNext', 'the list link (+0x20).', 'void *%s(void *server)', 'return FIELD(server, 0x20, void *);'),
    (0x617b20, 12, 'SBServerSetFlags', 'flags byte (+0x15).', 'void %s(void *server, unsigned char flags)', 'FIELD(server, 0x15, unsigned char) = flags;'),
    (0x617b50, 12, 'SBServerSetICMPIP', 'the icmp ip (+0x10).', 'void %s(void *server, unsigned int ip)', 'FIELD(server, 0x10, unsigned int) = ip;'),
    (0x617b60, 12, 'SBServerSetState', 'state byte (+0x14).', 'void %s(void *server, unsigned char state)', 'FIELD(server, 0x14, unsigned char) = state;'),
    (0x617b70, 8, 'SBServerGetState', 'state byte (+0x14).', 'unsigned char %s(void *server)', 'return FIELD(server, 0x14, unsigned char);'),
]
for addr, size, name, note, sig, body in simple:
    emit(addr, size, name, note, SB + '\n' + sig % name + '\n{\n    ' + body + '\n}\n')
emit(0x617b30, 21, 'SBServerSetPrivateAddr', 'the private ip (+0x08) and port (+0x0c).', SB + '''
void SBServerSetPrivateAddr(void *server, unsigned int ip, unsigned short port)
{
    FIELD(server, 0x08, unsigned int) = ip;
    FIELD(server, 0x0c, unsigned short) = port;
}
''')
emit(0x6175e0, 13, 'SBServerGetPublicAddress', 'inet_ntoa (WS2_32 #12) of the public ip (+0x00).', SB + '''
char *SBServerGetPublicAddress(void *server)
{
    struct in_addr address;

    address.s_addr = FIELD(server, 0x00, unsigned int);
    return inet_ntoa(address);
}
''')
emit(0x617640, 14, 'SBServerGetPrivateAddress', 'inet_ntoa (WS2_32 #12) of the private ip (+0x08).', SB + '''
char *SBServerGetPrivateAddress(void *server)
{
    struct in_addr address;

    address.s_addr = FIELD(server, 0x08, unsigned int);
    return inet_ntoa(address);
}
''')
emit(0x617600, 17, 'SBServerGetPublicQueryPort', 'ntohs of the public port (+0x04).', SB + '''
unsigned short SBServerGetPublicQueryPort(void *server)
{
    return ntohs(FIELD(server, 0x04, unsigned short));
}
''')
emit(0x617650, 17, 'SBServerGetPrivateQueryPort', 'ntohs of the private port (+0x0c).', SB + '''
unsigned short SBServerGetPrivateQueryPort(void *server)
{
    return ntohs(FIELD(server, 0x0c, unsigned short));
}
''')
emit(0x617b80, 18, 'SBIsNullServer', 'whether the server is the shared null server (0x006a27f8).', '''
extern unsigned char SBNullServer[]; // 0x006a27f8

int SBIsNullServer(void *server)
{
    return server == (void *)SBNullServer;
}
''')
emit(0x617690, 116, 'SBIsReservedKey', 'returns 0 for the keys "queryid" and "final" (0x0064e498, 0x0064e490), else 1 (a key worth storing).', '''
int SBIsReservedKey(const char *key)
{
    static const char *const reserved[2] = { "queryid", "final" };
    int i;

    for (i = 0; i < 2; i++) {
        if (strcmp(key, reserved[i]) == 0) {
            return 0;
        }
    }
    return 1;
}
''', name_confidence='0.5')

# header additions
p = 'src/gamespy/gamespy.h'
s = open(p, encoding='utf-8').read()
if 'SBKeyValuePair' not in s:
    s = s.replace('#endif\n', '''int ArrayLength(DArray array);
int TableCount(HashTable table);

// ---- sb_server.c (serverbrowsing)
typedef struct SBKeyValuePair {
    const char *key;                // 0x00
    const char *value;              // 0x04
} SBKeyValuePair;

int KeyValCompareKeyA(const void *elem1, const void *elem2);
int KeyValHashKeyA(const void *elem, int num_buckets);
void SBRefStrFree(void *elem);
HashTable SBRefStrHash(void);

#endif
''')
    open(p, 'w', encoding='utf-8').write(s)

# the player list caller: 0x4b7450..0x4b7492 pushes (server, index, key, default)
p = 'src/networking/server_browser_player_list_populate.c'
s = open(p, encoding='utf-8').read()
h, sep, t = s.partition('#if 0')
pairs = [
    ('extern char *FUN_00617530(void *entry, const char *key, int32_t index); // foreign, GameSpy library, see UNSURE',
     'extern char *FUN_00617530(void *entry, int32_t index, const char *key, const char *default_value); // 0x617530 SBServerGetPlayerStringValue'),
    ('name = FUN_00617530(entry, "player", i);',
     'name = FUN_00617530(entry, i, "player", 0); // FIXED 2026-09-28: 0x4b7450 pushes (server, index, key, NULL)'),
    ('score = FUN_00617530(entry, "score", i);',
     'score = FUN_00617530(entry, i, "score", "--"); // FIXED 2026-09-28: default "--" (0x0066b038) at 0x4b7486'),
]
for o, n in pairs:
    assert h.count(o) == 1, o
    h = h.replace(o, n)
open(p, 'w', encoding='utf-8').write(h + sep + t)
print('ok')
