exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
import os

F = '#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))\n'

emit(0x61e9f0, 33, 'NTSLengthSB', 'the length of the NUL-terminated string at buf including its NUL, or -1 when no NUL lies within len bytes.', '''
int NTSLengthSB(const char *buf, int len)
{
    int i = 0;

    if (len > 0) {
        do {
            if (buf[i++] == 0) {
                return i;
            }
        } while (i < len);
    }
    return -1;
}
''')
emit(0x61f220, 113, 'SBRefStr', 'a reference-counted copy of str from the thread\'s ref-string table (entries {string, count}): an existing entry gains a reference, otherwise goastrdup starts one at 1. The server list argument is only passed on to SBRefStrHash.', '''
const char *SBRefStr(void *slist, const char *str)
{
    SBKeyValuePair ref;
    SBKeyValuePair *found;

    ref.key = str;
    found = (SBKeyValuePair *)TableLookup(SBRefStrHash(slist), &ref);
    if (found != 0) {
        found->value = (const char *)((int)found->value + 1);
        return found->key;
    }
    ref.key = goastrdup(str);
    ref.value = (const char *)1;
    TableEnter(SBRefStrHash(slist), &ref);
    return ref.key;
}
''')
emit(0x61f2a0, 76, 'SBReleaseStr', 'drops a reference; the last one removes the entry (the table frees the string).', '''
void SBReleaseStr(void *slist, const char *str)
{
    SBKeyValuePair ref;
    SBKeyValuePair *found;

    ref.key = str;
    found = (SBKeyValuePair *)TableLookup(SBRefStrHash(slist), &ref);
    if (found != 0) {
        found->value = (const char *)((int)found->value - 1);
        if (found->value == 0) {
            TableRemove(SBRefStrHash(slist), &ref);
        }
    }
}
''')
emit(0x617a80, 31, 'SBServerKeyValFree', 'a server key table element free: releases the key and the value ref strings.', '''
void SBServerKeyValFree(void *elem)
{
    SBKeyValuePair *pair = (SBKeyValuePair *)elem;

    SBReleaseStr(0, pair->key);
    SBReleaseStr(0, pair->value);
}
''', name_confidence='0.6')
emit(0x6173c0, 34, 'SBServerFree', 'takes a pointer to the server: frees its key table (clearing the field) and the server.', F + '''
void SBServerFree(void *elem)
{
    void *server = *(void **)elem;

    TableFree(FIELD(server, 0x18, HashTable));
    FIELD(server, 0x18, HashTable) = 0;
    free(server);
}
''')
emit(0x617ab0, 112, 'SBAllocServer', 'a 0x24 byte server with a key table TableNew2(8, 8, 4, KeyValHashKeyA, KeyValCompareKeyA, SBServerKeyValFree), the public ip and port, everything else 0; NULL when either allocation fails. The server list argument is not used.', F + '''
void *SBAllocServer(void *slist, unsigned int public_ip, unsigned short public_port)
{
    void *server = malloc(0x24);

    (void)slist;
    if (server == 0) {
        return 0;
    }
    FIELD(server, 0x18, HashTable) = TableNew2(sizeof(SBKeyValuePair), 8, 4, KeyValHashKeyA, KeyValCompareKeyA, SBServerKeyValFree);
    if (FIELD(server, 0x18, HashTable) == 0) {
        free(server);
        return 0;
    }
    FIELD(server, 0x00, unsigned int) = public_ip;
    FIELD(server, 0x14, unsigned char) = 0;
    FIELD(server, 0x15, unsigned char) = 0;
    FIELD(server, 0x20, void *) = 0;
    FIELD(server, 0x1c, int) = 0;
    FIELD(server, 0x10, unsigned int) = 0;
    FIELD(server, 0x08, unsigned int) = 0;
    FIELD(server, 0x0c, unsigned short) = 0;
    FIELD(server, 0x04, unsigned short) = public_port;
    return server;
}
''')
emit(0x6173f0, 57, 'SBServerAddKeyValue', 'enters (ref key, ref value) into the server\'s key table.', F + '''
void SBServerAddKeyValue(void *server, const char *key, const char *value)
{
    SBKeyValuePair pair;

    pair.key = SBRefStr(0, key);
    pair.value = SBRefStr(0, value);
    TableEnter(FIELD(server, 0x18, HashTable), &pair);
}
''')
emit(0x617430, 95, 'SBServerAddIntKeyValue', 'the value printed with "%d" (0x0065fb30), then as SBServerAddKeyValue.', F + '''
void SBServerAddIntKeyValue(void *server, const char *key, int value)
{
    char text[0x14];
    SBKeyValuePair pair;

    sprintf(text, "%d", value);
    pair.key = SBRefStr(0, key);
    pair.value = SBRefStr(0, text);
    TableEnter(FIELD(server, 0x18, HashTable), &pair);
}
''')
TOK = '''
// 0x617710 (ECX string or NULL to continue, DL delimiter): a strtok with one static position (0x006a27f4, a static
//   here) that returns NULL for an empty token; only this function calls it.
static char *tokenizer_position;

static char *next_token(char *string, char delimiter)
{
    char *start;
    char *p;

    if (string != 0) {
        tokenizer_position = string;
    }
    p = tokenizer_position;
    start = p;
    if (*p == 0) {
        start = 0;
    } else {
        while (*p != delimiter) {
            p++;
            if (*p == 0) {
                break;
            }
        }
        tokenizer_position = p;
        if (p == start) {
            start = 0;
        }
    }
    if (*p != 0) {
        *p = 0;
        tokenizer_position = p + 1;
    }
    return start;
}
'''
emit(0x617760, 222, 'SBServerParseKeyVals', 'walks "\\\\key\\\\value..." (from the character after the first): each key whose value token is missing gets "" and every key except "queryid" and "final" (SBIsReservedKey) is entered with ref strings. The tokenizer 0x617710 (ECX string, DL delimiter) is a static here.', F + TOK + '''
extern int SBIsReservedKey(const char *key);

void SBServerParseKeyVals(void *server, char *keyvals)
{
    char *key = next_token(keyvals + 1, '\\\\');

    while (key != 0) {
        char *value = next_token(0, '\\\\');
        SBKeyValuePair pair;

        if (value == 0) {
            value = "";
        }
        if (SBIsReservedKey(key)) {
            pair.key = SBRefStr(0, key);
            pair.value = SBRefStr(0, value);
            TableEnter(FIELD(server, 0x18, HashTable), &pair);
        }
        key = next_token(0, '\\\\');
    }
}
''')
emit(0x617840, 511, 'SBServerParseQR2FullKeysSingle', 'a QR2 full-keys reply: server key/value string pairs up to an empty string; then two sections (players, teams), each a big-endian count, key names (each at most 100 bytes) up to an empty string, and count rows of values entered as "<key><row>" ("%s%d", 0x0064e3dc). Any string without its NUL inside the remaining length ends the parse.', F + '''
void SBServerParseQR2FullKeysSingle(void *server, char *data, int len)
{
    int section;

    while (*data != 0) {
        int key_length = NTSLengthSB(data, len);
        int value_length;
        char *key;
        SBKeyValuePair pair;

        if (key_length < 0) {
            return;
        }
        len -= key_length;
        key = data;
        data += key_length;
        value_length = NTSLengthSB(data, len);
        if (value_length < 0) {
            return;
        }
        len -= value_length;
        pair.key = SBRefStr(0, key);
        pair.value = SBRefStr(0, data);
        TableEnter(FIELD(server, 0x18, HashTable), &pair);
        data += value_length;
    }
    data++;
    len--;
    for (section = 0; section < 2; section++) {
        int rows;
        int key_count = 0;
        char *keys;
        int row;

        if (len < 2) {
            return;
        }
        rows = ntohs(*(unsigned short *)data);
        data += 2;
        len -= 2;
        keys = data;
        while (*data != 0) {
            int key_length = NTSLengthSB(data, len);

            if (key_length < 0 || key_length > 100) {
                return;
            }
            data += key_length;
            len -= key_length;
            key_count++;
        }
        data++;
        len--;
        for (row = 0; row < rows; row++) {
            char *key = keys;
            int k;

            for (k = 0; k < key_count; k++) {
                char name[0x80];
                int value_length = NTSLengthSB(data, len);
                SBKeyValuePair pair;

                if (value_length < 0) {
                    return;
                }
                sprintf(name, "%s%d", key, row);
                pair.key = SBRefStr(0, name);
                pair.value = SBRefStr(0, data);
                TableEnter(FIELD(server, 0x18, HashTable), &pair);
                data += value_length;
                len -= value_length;
                key += strlen(key) + 1;
            }
        }
    }
}
''')

# ---------------- sb_serverlist.c
emit(0x61f000, 45, 'SBServerListAppendServer', 'appends the server and reports it to the list callback (+0x480) as event 0 (server added) with the instance (+0x484).', F + '''
void SBServerListAppendServer(void *slist, void *server)
{
    ArrayAppend(FIELD(slist, 0x04, DArray), &server);
    FIELD(slist, 0x480, SBListCallBackFn)(slist, 0, server, FIELD(slist, 0x484, void *));
}
''')
emit(0x61f030, 99, 'SBServerListFindServer', 'the index of the server with this public ip and port, or -1. (The binary reads the ip through 0x6175f0 and the port through 0x6147d0 -- the linker folded SBServerGetPublicInetAddress / the raw port getter into ArrayLength / gt2GetRemotePort; plain field reads here.)', F + '''
int SBServerListFindServer(void *slist, unsigned int ip, unsigned short port)
{
    int count = ArrayLength(FIELD(slist, 0x04, DArray));
    int i;

    for (i = 0; i < count; i++) {
        void *server = *(void **)ArrayNth(FIELD(slist, 0x04, DArray), i);

        if (FIELD(server, 0x00, unsigned int) == ip && FIELD(server, 0x04, unsigned short) == port) {
            return i;
        }
    }
    return -1;
}
''')
emit(0x61f0a0, 103, 'SBServerListRemoveAt', 'reports the server to the list callback as event 2 (server deleted), deletes it from the array and pushes it onto the dead list (+0x5bc).', F + '''
extern void SBServerSetNext(void *server, void *next);

void SBServerListRemoveAt(void *slist, int index)
{
    void *server = *(void **)ArrayNth(FIELD(slist, 0x04, DArray), index);

    FIELD(slist, 0x480, SBListCallBackFn)(slist, 2, server, FIELD(slist, 0x484, void *));
    ArrayDeleteAt(FIELD(slist, 0x04, DArray), index);
    SBServerSetNext(server, FIELD(slist, 0x5bc, void *));
    FIELD(slist, 0x5bc, void *) = server;
}
''')
emit(0x61f110, 16, 'SBServerListCount', 'the number of servers.', F + '''
int SBServerListCount(void *slist)
{
    return ArrayLength(FIELD(slist, 0x04, DArray));
}
''')
emit(0x61f120, 24, 'SBServerListNth', 'the i-th server.', F + '''
void *SBServerListNth(void *slist, int i)
{
    return *(void **)ArrayNth(FIELD(slist, 0x04, DArray), i);
}
''')
emit(0x61f140, 60, 'SBFreeDeadList', 'frees every server on the dead list (+0x5bc) and empties it.', F + '''
extern void *SBServerGetNext(void *server);
extern void SBServerFree(void *elem);

void SBFreeDeadList(void *slist)
{
    void *server = FIELD(slist, 0x5bc, void *);

    if (server == 0) {
        return;
    }
    while (server != 0) {
        void *next = SBServerGetNext(server);

        SBServerFree(&server);
        server = next;
    }
    FIELD(slist, 0x5bc, void *) = server;
}
''', name_confidence='0.6')
emit(0x61f180, 153, 'SBServerListClear', 'moves every server onto the dead list, clears the array, then frees the dead list.', F + '''
extern void SBServerSetNext(void *server, void *next);
extern void *SBServerGetNext(void *server);
extern void SBServerFree(void *elem);

void SBServerListClear(void *slist)
{
    int count = ArrayLength(FIELD(slist, 0x04, DArray));
    int i;
    void *server;

    for (i = 0; i < count; i++) {
        server = *(void **)ArrayNth(FIELD(slist, 0x04, DArray), i);
        SBServerSetNext(server, FIELD(slist, 0x5bc, void *));
        FIELD(slist, 0x5bc, void *) = server;
    }
    ArrayClear(FIELD(slist, 0x04, DArray));
    server = FIELD(slist, 0x5bc, void *);
    if (server != 0) {
        while (server != 0) {
            void *next = SBServerGetNext(server);

            SBServerFree(&server);
            server = next;
        }
        FIELD(slist, 0x5bc, void *) = server;
    }
}
''')
emit(0x61f2f0, 216, 'SBServerListInit', 'state 1, a server array (ArrayNew(4, 100)), the ref-string table touched, the three query names copied to +0x0c / +0x2c / +0x4c, the callback and instance (+0x480 / +0x484), the version (+0x4a8), "" at +0x488 and +0x49c, -1 at +0x4a0 and +0x47c, zeros elsewhere; then srand(current_time()) and SocketStartUp.', F + '''
void SBServerListInit(void *slist, const char *query_for_gamename, const char *query_from_gamename,
    const char *query_from_key, int query_from_version, SBListCallBackFn callback, void *instance)
{
    FIELD(slist, 0x00, int) = 1;
    FIELD(slist, 0x04, DArray) = ArrayNew(sizeof(void *), 100, 0);
    FIELD(slist, 0x5bc, void *) = 0;
    SBRefStrHash(slist);
    strcpy((char *)slist + 0x0c, query_for_gamename);
    strcpy((char *)slist + 0x2c, query_from_gamename);
    strcpy((char *)slist + 0x4c, query_from_key);
    FIELD(slist, 0x480, SBListCallBackFn) = callback;
    FIELD(slist, 0x484, void *) = instance;
    FIELD(slist, 0x488, const char *) = "";
    FIELD(slist, 0x490, int) = 0;
    FIELD(slist, 0x4a0, int) = -1;
    FIELD(slist, 0x74, int) = 0;
    FIELD(slist, 0x78, int) = 0;
    FIELD(slist, 0x08, int) = 0;
    FIELD(slist, 0x47c, int) = -1;
    FIELD(slist, 0x478, int) = 0;
    FIELD(slist, 0x49c, const char *) = "";
    FIELD(slist, 0x494, int) = 0;
    FIELD(slist, 0x4a8, int) = query_from_version;
    srand(current_time());
    SocketStartUp();
}
''')

p = 'src/gamespy/gamespy.h'
s = open(p, encoding='utf-8').read()
if 'SBRefStr(' not in s:
    s = s.replace('HashTable SBRefStrHash(void);\n', '''HashTable SBRefStrHash(void *slist);
const char *SBRefStr(void *slist, const char *str);
void SBReleaseStr(void *slist, const char *str);
int NTSLengthSB(const char *buf, int len);
typedef void (*SBListCallBackFn)(void *slist, int reason, void *server, void *instance);
''')
    open(p, 'w', encoding='utf-8').write(s)
# SBRefStrHash takes the (unused) server list argument (0x61f232 pushes it)
p = 'src/gamespy/SBRefStrHash.c'
s = open(p, encoding='utf-8').read()
s = s.replace('HashTable SBRefStrHash(void)\n{\n', 'HashTable SBRefStrHash(void *slist)\n{\n    (void)slist;\n')
open(p, 'w', encoding='utf-8').write(s)
print('ok')
