// gti2HandlePong  (GameSpy SDK in halo.exe; no C existed)
// address 0x6187e0, size 67 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6187e0..0x618822: EDI connection, EAX data, stack len: with a ping callback, an
//   8-byte pong starting "time" reports the latency now - its time stamp.
// blam-cc: EDI -> connection, EAX -> data, stack -> len

#include "gamespy.h"

#include "gt2.h"

int gti2HandlePong(GTI2Connection *connection, const unsigned char *data, int len)
{
    if (connection->callbacks.ping == 0 || len != 8) {
        return 1;
    }
    if (*(const uint32_t *)data != *(const uint32_t *)GTI2PingTag) {
        return 1;
    }
    return gti2PingCallback(connection, (int)(current_time() - ((const uint32_t *)data)[1])) != 0;
}
