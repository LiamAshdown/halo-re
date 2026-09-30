// ptaGetKeyValue  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c060, size 105 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c060..0x61c0c8: EAX buffer, ECX key: the value after the key up to the next
//   backslash (at most 255 chars) copied into a static buffer (0x6a2e70); NULL when the key is missing.
// blam-cc: EAX -> buffer, ECX -> key

#include "gamespy.h"

#include "ghttp.h"
#include "fn_gamespy.h"

extern char ptaKeyValue[0x100];           // 0x006a2e70

char *ptaGetKeyValue(const char *buffer, const char *key)
{
    const char *value = strstr(buffer, key);
    size_t len;

    if (value == 0) {
        return 0;
    }
    value += strlen(key);
    len = strcspn(value, "\\");
    if (len >= 0xff) {
        len = 0xff;
    }
    memcpy(ptaKeyValue, value, len);
    ptaKeyValue[len] = 0;
    return ptaKeyValue;
}
