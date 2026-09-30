// gti2ConnectionHash  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c300, size 22 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c300..0x61c315: the connection table hash: remote port * remote ip, unsigned
//   modulo the bucket count.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int gti2ConnectionHash(const void *elem, int num_buckets)
{
    void *connection = *(void *const *)elem;

    return (int)((FIELD(connection, 0x04, unsigned short) * FIELD(connection, 0x00, unsigned int)) % (unsigned int)num_buckets);
}
