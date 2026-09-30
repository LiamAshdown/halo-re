// SBServerParseKeyVals  (GameSpy SDK in halo.exe; no C existed)
// address 0x617760, size 222 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617760..0x61783d: walks "\\key\\value..." (from the character after the first):
//   each key whose value token is missing gets "" and every key except "queryid" and "final" (SBIsReservedKey) is
//   entered with ref strings. The tokenizer 0x617710 (ECX string, DL delimiter) is a static here.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

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


void SBServerParseKeyVals(void *server, char *keyvals)
{
    char *key = next_token(keyvals + 1, '\\');

    while (key != 0) {
        char *value = next_token(0, '\\');
        SBKeyValuePair pair;

        if (value == 0) {
            value = "";
        }
        if (SBIsReservedKey(key)) {
            pair.key = SBRefStr(0, key);
            pair.value = SBRefStr(0, value);
            TableEnter(FIELD(server, 0x18, HashTable), &pair);
        }
        key = next_token(0, '\\');
    }
}
