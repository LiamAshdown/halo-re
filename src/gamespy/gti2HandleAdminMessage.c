// gti2HandleAdminMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x618e10, size 169 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618e10..0x618eb8: EAX len, EBX message, EDX connection, stack type: ack (a
//   2-byte serial, else a negotiation error), nack, ping (answered in place as a pong), pong, closed; anything else
//   is ignored.
// blam-cc: EDX -> connection, EBX -> message, EAX -> len, stack -> type

#include "gamespy.h"

#include "gt2.h"

int gti2HandleAdminMessage(GTI2Connection *connection, unsigned char *message, int len, int type)
{
    unsigned char *data = message + 3;
    int data_len = len - 3;

    if (type == GTI2MsgAck) {
        if (data_len != 2) {
            return gti2ConnectionError(connection, 7, 2) != 0;
        }
        return gti2HandleAck(connection, (unsigned short)((data[0] << 8) | data[1])) != 0;
    }
    if (type == GTI2MsgNack) {
        return gti2HandleNack(connection, data, data_len) != 0;
    }
    if (type == GTI2MsgPing) {
        message[2] = GTI2MsgPong;
        return gti2ConnectionSendData(connection, message, len, 0, 0) != 0;
    }
    if (type == GTI2MsgPong) {
        return gti2HandlePong(connection, data, data_len) != 0;
    }
    if (type == GTI2MsgClosed) {
        return gti2HandleClosed(connection) != 0;
    }
    return 1;
}
