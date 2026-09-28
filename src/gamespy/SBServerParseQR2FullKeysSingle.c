// SBServerParseQR2FullKeysSingle  (GameSpy SDK in halo.exe; no C existed)
// address 0x617840, size 511 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617840..0x617a3e: a QR2 full-keys reply: server key/value string pairs up to an
//   empty string; then two sections (players, teams), each a big-endian count, key names (each at most 100 bytes) up
//   to an empty string, and count rows of values entered as "<key><row>" ("%s%d", 0x0064e3dc). Any string without its
//   NUL inside the remaining length ends the parse.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

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
