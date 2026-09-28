// gti2NewSocketConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c730, size 811 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c730..0x61ca5a: 5 when the (ip, port) already has a connection; otherwise a
//   zeroed 0x150-byte connection (ip, port, socket, start and last-send time now, default key bytes 3 8 3 3). More
//   than 16 connections from the same ip (counted with TableMap and the static 0x61c710, which bumps the int the new
//   connection  data temporarily points at) fail. The key exchange: generator "3", modulus "10001" (0x64e644), a
//   random private exponent, publicKey = 3 ^ private mod 0x10001. Then the incoming/outgoing buffers (socket sizes),
//   the incoming (0x10-byte, grow 0x40) and outgoing (0x14, 0x40) message arrays and the send/receive filter arrays
//   (4, 2); it is entered in the table and handed out (0). Any allocation failure frees everything and gives 1.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

static void gti2CountConnectionsFromIP(void *elem, void *client_data) // 0x61c710
{
    GTI2Connection *connection = (GTI2Connection *)client_data;

    if ((*(GTI2Connection **)elem)->ip == connection->ip) {
        (*(int *)connection->data)++;
    }
}

int gti2NewSocketConnection(GTI2Socket *socket, GTI2Connection **connection_out, unsigned int ip, unsigned short port)
{
    GTI2Connection *connection = 0;
    int count = 0;
    void *data;
    char address[0x18];

    gt2AddressToString(ip, port, address);
    if (gti2SocketFindConnection(socket, ip, port) != 0) {
        return 5;
    }
    connection = (GTI2Connection *)malloc(0x150);
    if (connection == 0) {
        return 1;
    }
    memset(connection, 0, 0x150);
    connection->ip = ip;
    connection->port = port;
    connection->socket = socket;
    connection->startTime = current_time();
    connection->lastSend = connection->startTime;
    connection->lastAck = 0;
    connection->maxResends = 0;
    connection->serialNumber = 0;
    connection->expectedSerialNumber = 0;
    connection->key[0] = 3;
    connection->key[1] = 8;
    connection->key[2] = 3;
    connection->key[3] = 3;
    data = connection->data;
    connection->data = &count;
    TableMap(socket->connections, gti2CountConnectionsFromIP, connection);
    if (count <= 0x10) {
        connection->data = data;
        *(unsigned short *)connection->generator = 0x33;
        memcpy(connection->modulus, "10001", 6);
        gti2GeneratePrivateExponent(connection->privateExponent);
        gt2_bignum_mod_exp(connection->generator, connection->privateExponent, connection->modulus,
            connection->publicKey);
        if (gti2AllocateBuffer(&connection->incomingBuffer, socket->incomingBufferSize) &&
            gti2AllocateBuffer(&connection->outgoingBuffer, socket->outgoingBufferSize) &&
            (connection->incomingBufferMessages = ArrayNew(0x10, 0x40, 0)) != 0 &&
            (connection->outgoingBufferMessages = ArrayNew(0x14, 0x40, 0)) != 0 &&
            (connection->sendFilters = ArrayNew(4, 2, 0)) != 0 &&
            (connection->receiveFilters = ArrayNew(4, 2, 0)) != 0) {
            TableEnter(socket->connections, &connection);
            *connection_out = gti2SocketFindConnection(socket, ip, port);
            if (*connection_out != 0) {
                return 0;
            }
        }
    }
    free(connection->incomingBuffer.buffer);
    free(connection->outgoingBuffer.buffer);
    if (connection->incomingBufferMessages != 0) {
        ArrayFree(connection->incomingBufferMessages);
    }
    if (connection->outgoingBufferMessages != 0) {
        ArrayFree(connection->outgoingBufferMessages);
    }
    if (connection->sendFilters != 0) {
        ArrayFree(connection->sendFilters);
    }
    if (connection->receiveFilters != 0) {
        ArrayFree(connection->receiveFilters);
    }
    free(connection);
    return 1;
}
