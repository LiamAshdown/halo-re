// gt2CreateSocket  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c3e0, size 562 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c3e0..0x61c611: a 0x48 byte GT2 socket on the local address: buffer sizes
//   default to 64 KB, a connection table (TableNew2(4, 32, 2, ...)) and closed-connection array; a broadcast-enabled
//   UDP socket bound to the address, whose actual address is kept. 4 for a bad address, 1 out of memory, 3 on a
//   socket error, else 0.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

extern int gti2ConnectionHash(const void *elem, int num_buckets);
extern int gti2ConnectionCompare(const void *elem1, const void *elem2);
extern void gti2ClosedConnectionFree(void *elem);
extern int gt2StringToAddress(const char *string, unsigned int *ip, unsigned short *port);

int gt2CreateSocket(void **socket_out, const char *local_address, int outgoing_buffer_size, int incoming_buffer_size,
    void *socket_error_callback)
{
    unsigned int ip;
    unsigned short port;
    void *sock;
    struct sockaddr_in address;
    int length;
    BOOL broadcast;

    SocketStartUp();
    if (incoming_buffer_size == 0) {
        incoming_buffer_size = 0x10000;
    }
    if (outgoing_buffer_size == 0) {
        outgoing_buffer_size = 0x10000;
    }
    if (!gt2StringToAddress(local_address, &ip, &port)) {
        return 4;
    }
    sock = malloc(0x48);
    if (sock == 0) {
        return 1;
    }
    memset(sock, 0, 0x48);
    FIELD(sock, 0x00, SOCKET) = INVALID_SOCKET;
    FIELD(sock, 0x38, int) = incoming_buffer_size;
    FIELD(sock, 0x34, int) = outgoing_buffer_size;
    FIELD(sock, 0x20, void *) = socket_error_callback;
    FIELD(sock, 0x40, int) = 0;
    FIELD(sock, 0x44, int) = 1;
    FIELD(sock, 0x0c, HashTable) = TableNew2(4, 0x20, 2, gti2ConnectionHash, gti2ConnectionCompare, 0);
    if (FIELD(sock, 0x0c, HashTable) == 0) {
        free(sock);
        return 1;
    }
    FIELD(sock, 0x10, DArray) = ArrayNew(4, 4, gti2ClosedConnectionFree);
    if (FIELD(sock, 0x10, DArray) == 0) {
        TableFree(FIELD(sock, 0x0c, HashTable));
        free(sock);
        return 1;
    }
    FIELD(sock, 0x00, SOCKET) = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (FIELD(sock, 0x00, SOCKET) == INVALID_SOCKET) {
        WSAGetLastError();
    } else {
        broadcast = 1;
        if (setsockopt(FIELD(sock, 0x00, SOCKET), SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, 4) == SOCKET_ERROR) {
            WSAGetLastError();
        }
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = ip;
        address.sin_port = htons(port);
        if (bind(FIELD(sock, 0x00, SOCKET), (const struct sockaddr *)&address, 0x10) != SOCKET_ERROR) {
            length = 0x10;
            getsockname(FIELD(sock, 0x00, SOCKET), (struct sockaddr *)&address, &length);
            FIELD(sock, 0x04, unsigned int) = address.sin_addr.s_addr;
            FIELD(sock, 0x08, unsigned short) = ntohs(address.sin_port);
            *socket_out = sock;
            return 0;
        }
        WSAGetLastError();
        closesocket(FIELD(sock, 0x00, SOCKET));
    }
    TableFree(FIELD(sock, 0x0c, HashTable));
    ArrayFree(FIELD(sock, 0x10, DArray));
    free(sock);
    return 3;
}
