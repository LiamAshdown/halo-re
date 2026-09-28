// gti2HandleReliableData  (GameSpy SDK in halo.exe; no C existed)
// address 0x6185b0, size 182 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6185b0..0x618665: EAX connection, EDI message, stack len: a reliable
//   application message outside connected/closing is a negotiation error (7, communication error 2); otherwise
//   decrypted and CRC-checked like gti2HandleUnreliableMessage (bad: dropped, 1) and delivered reliable.
// blam-cc: EAX -> connection, EDI -> message, stack -> len

#include "gamespy.h"

#include "gt2.h"

int gti2HandleReliableData(GTI2Connection *connection, unsigned char *message, int len)
{
    uint32_t crc;

    if (connection->state != GTI2Connected && connection->state != GTI2Closing) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    tea_decrypt_buffer(len, message, (const uint32_t *)connection->key);
    datum_index_invalidate(&crc);
    crc32_update(&crc, message, len - 4);
    if (*(uint32_t *)(message + len - 4) != crc) {
        return 1;
    }
    if (ArrayLength(connection->receiveFilters) != 0) {
        return gti2ReceiveFilterCallback(connection, 0, message, len - 4, 1) != 0;
    }
    if (!gti2ReceivedCallback(connection, message, len - 4, 1)) {
        return 0;
    }
    return 1;
}
