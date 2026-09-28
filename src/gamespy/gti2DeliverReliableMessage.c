// gti2DeliverReliableMessage  (GameSpy SDK in halo.exe; no C existed)
// address 0x6196f0, size 325 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6196f0..0x619834: ESI connection, ECX data (past the 7-byte header), EBX
//   length, stack type: counts the serial as received (expected + 1; the address is formatted, unused) and hands the
//   message to its handler: 0 application data, 1 client challenge, 2 server challenge, 3 client response, 4 accept,
//   5 reject, 6 close; 7 (keep-alive) and others need nothing. 0 when the handler failed.
// blam-cc: ESI -> connection, ECX -> data, EBX -> len, stack -> type

#include "gamespy.h"

#include "gt2.h"

int gti2DeliverReliableMessage(GTI2Connection *connection, int type, unsigned char *data, int len)
{
    char address[0x18];
    int result = 1;

    connection->expectedSerialNumber++;
    gt2AddressToString(connection->ip, connection->port, address);
    switch (type) {
    case GTI2MsgAppReliable:
        result = gti2HandleReliableData(connection, data, len);
        break;
    case GTI2MsgClientChallenge:
        result = gti2HandleClientChallenge(connection, data, len);
        break;
    case GTI2MsgServerChallenge:
        result = gti2HandleServerChallenge(connection, data, len);
        break;
    case GTI2MsgClientResponse:
        result = gti2HandleClientResponse(connection, data, len);
        break;
    case GTI2MsgAccept:
        result = gti2HandleAccept(connection, data);
        break;
    case GTI2MsgReject:
        result = gti2HandleReject(connection, data, len);
        break;
    case GTI2MsgClose:
        result = gti2HandleClose(connection);
        break;
    }
    return result != 0;
}
