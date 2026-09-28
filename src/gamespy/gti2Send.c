// gti2Send  (GameSpy SDK in halo.exe; no C existed)
// address 0x619510, size 102 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619510..0x619575: appends the CRC32 of the message (4 bytes PAST the given
//   length -- the caller  buffer must have room), TEA-encrypts message + CRC in place with the connection key, and
//   sends it reliable or unreliable.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2Send(GTI2Connection *connection, unsigned char *message, int len, int reliable)
{
    uint32_t crc;

    datum_index_invalidate(&crc);
    crc32_update(&crc, message, len);
    *(uint32_t *)(message + len) = crc;
    len += 4;
    tea_encrypt_buffer(len, message, (const uint32_t *)connection->key);
    if (reliable != 0) {
        return gti2SendDataReliable(connection, message, len);
    }
    return gti2SendUnreliable(connection, message, len);
}
