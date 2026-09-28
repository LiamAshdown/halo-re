// gcd_compute_response  (GameSpy SDK in halo.exe; no C existed)
// address 0x617c70, size 312 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617c70..0x617da7: the CD key challenge response: "CD Key or challenge too long"
//   when 2*len(key) + len(challenge) + 8 >= 0x200; else a random 32-bit value (srand(time(0) ^ 0x33333333), two
//   rand()s), the key's MD5 hex at response+0, the value as "%.8x" at +0x20 and the MD5 hex of key . (value % 0xffff)
//   . challenge ("%s%d%s") at +0x28.
// blam-cc: cdecl

#include "gamespy.h"

extern void md5_hex_digest(const unsigned char *data, int length, char *out); // 0x61a730
#include <time.h>

void gcd_compute_response(const char *cdkey, const char *challenge, char *response)
{
    char random_text[0x10];
    char text[0x200];
    unsigned int value;

    if (strlen(cdkey) * 2 + strlen(challenge) + 8 >= 0x200) {
        strcpy(response, "CD Key or challenge too long");
        return;
    }
    srand((unsigned int)time(0) ^ 0x33333333);
    value = (unsigned int)rand() << 16;
    value |= (unsigned int)rand();
    sprintf(random_text, "%.8x", value);
    sprintf(text, "%s%d%s", cdkey, value % 0xffff, challenge);
    md5_hex_digest((const unsigned char *)cdkey, (int)strlen(cdkey), response);
    strcpy(response + 0x20, random_text);
    md5_hex_digest((const unsigned char *)text, (int)strlen(text), response + 0x28);
}
