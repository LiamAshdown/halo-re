// gti2HandleMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x6199b0, size 476 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6199b0..0x619b8b: EDX message, ECX port, stack socket, len, ip: the receive
//   dump callback (when set) sees it first. From an unknown address: the unrecognized-message callback may take it;
//   otherwise a GT2 client challenge starts an incoming connection when anyone listens (an existing address, 5, is
//   ignored), a closed notice is ignored, and anything else is answered with closed. For a closed connection:
//   answered with closed (unless it was a closed notice). Otherwise non-GT2 data (no fe fe magic) and escaped data
//   (fe fe fe fe) are unreliable messages; types below 8 are reliable, the rest admin messages.
// blam-cc: EDX -> message, ECX -> port, stack -> socket, len, ip

#include "gamespy.h"

#include "gt2.h"

int gti2HandleMessage(GTI2Socket *socket, unsigned char *message, int len, unsigned int ip, unsigned short port)
{
    GTI2Connection *connection = gti2SocketFindConnection(socket, ip, port);
    int is_gt2;
    int type;

    if (socket->receiveDumpCallback != 0 &&
        !gti2DumpCallback(socket, connection, ip, port, 0, message, len, 0, 0, 0)) {
        return 0;
    }
    is_gt2 = len > 2 && *(const unsigned short *)message == *(const unsigned short *)GTI2Magic;
    if (connection == 0) {
        char address[0x18];
        int handled;

        gt2AddressToString(ip, port, address);
        if (!gti2UnrecognizedMessageCallback(socket, ip, port, message, len, &handled)) {
            return 0;
        }
        if (handled != 0) {
            return 1;
        }
        if (is_gt2) {
            if (message[2] == GTI2MsgClientChallenge) {
                int result;

                if (socket->connectAttemptCallback == 0) {
                    return 1;
                }
                result = gti2NewIncomingConnection(socket, &connection, ip, port);
                if (result == 5) {
                    return 1;
                }
                if (result != 0) {
                    return gti2SendClosed(socket, ip, port) != 0;
                }
            } else if (message[2] == GTI2MsgClosed) {
                return 1;
            } else {
                return gti2SendClosed(socket, ip, port) != 0;
            }
        } else {
            return gti2SendClosed(socket, ip, port) != 0;
        }
    }
    if (connection->state == GTI2Closed) {
        if (is_gt2 && message[2] == GTI2MsgClosed) {
            return 1;
        }
        return gti2SendClosed(connection->socket, connection->ip, connection->port) != 0;
    }
    if (!is_gt2) {
        return gti2HandleUnreliableMessage(connection, message, len) != 0;
    }
    if (len >= 4 && *(const unsigned short *)(message + 2) == *(const unsigned short *)GTI2Magic) {
        return gti2HandleUnreliableMessage(connection, message + 2, len - 2) != 0;
    }
    type = message[2];
    if (type < 8) {
        return gti2HandleReliableMessage(connection, message, len, type) != 0;
    }
    return gti2HandleAdminMessage(connection, message, len, type) != 0;
}
