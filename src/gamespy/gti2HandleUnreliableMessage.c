// gti2HandleUnreliableMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x618500, size 161 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618500..0x6185a0: EBX connection, ESI message, stack len: only while connected
//   or closing; the message is TEA-decrypted in place with the connection key and its trailing CRC32 checked (a bad
//   one is dropped: 1); the rest goes to receive filter 0 when any are set, else the received callback, unreliable. 0
//   when that freed the socket.
// blam-cc: EBX -> connection, ESI -> message, stack -> len

#include "gamespy.h"

#include "gt2.h"

int gti2HandleUnreliableMessage(GTI2Connection *connection, unsigned char *message, int len)
{
    uint32_t crc;

    if (connection->state != GTI2Connected && connection->state != GTI2Closing) {
        return 1;
    }
    tea_decrypt_buffer(len, message, (const uint32_t *)connection->key);
    datum_index_invalidate(&crc);
    crc32_update(&crc, message, len - 4);
    if (*(uint32_t *)(message + len - 4) != crc) {
        return 1;
    }
    if (ArrayLength(connection->receiveFilters) != 0) {
        return gti2ReceiveFilterCallback(connection, 0, message, len - 4, 0) != 0;
    }
    return gti2ReceivedCallback(connection, message, len - 4, 0) != 0;
}
