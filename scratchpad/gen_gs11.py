exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
import textwrap

F = '#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))\n'

emit(0x61c300, 22, 'gti2ConnectionHash', 'the connection table hash: remote port * remote ip, unsigned modulo the bucket count.', F + '''
int gti2ConnectionHash(const void *elem, int num_buckets)
{
    void *connection = *(void *const *)elem;

    return (int)((FIELD(connection, 0x04, unsigned short) * FIELD(connection, 0x00, unsigned int)) % (unsigned int)num_buckets);
}
''')
emit(0x61c320, 38, 'gti2ConnectionCompare', 'the connection table comparator: by remote ip, then (16-bit) by remote port.', F + '''
int gti2ConnectionCompare(const void *elem1, const void *elem2)
{
    void *a = *(void *const *)elem1;
    void *b = *(void *const *)elem2;

    if (FIELD(a, 0x00, unsigned int) != FIELD(b, 0x00, unsigned int)) {
        return (int)(FIELD(a, 0x00, unsigned int) - FIELD(b, 0x00, unsigned int));
    }
    return (short)(FIELD(a, 0x04, unsigned short) - FIELD(b, 0x04, unsigned short));
}
''')
emit(0x61d040, 134, 'gti2FreeConnection', 'frees the connection  initial message (+0x38), buffers (+0x44, +0x50), its four arrays (+0x5c, +0x60, +0xa0, +0xa4) and itself.', F + '''
void gti2FreeConnection(void *connection)
{
    if (FIELD(connection, 0x38, void *) != 0) {
        free(FIELD(connection, 0x38, void *));
    }
    if (FIELD(connection, 0x44, void *) != 0) {
        free(FIELD(connection, 0x44, void *));
    }
    if (FIELD(connection, 0x50, void *) != 0) {
        free(FIELD(connection, 0x50, void *));
    }
    if (FIELD(connection, 0x5c, DArray) != 0) {
        ArrayFree(FIELD(connection, 0x5c, DArray));
    }
    if (FIELD(connection, 0x60, DArray) != 0) {
        ArrayFree(FIELD(connection, 0x60, DArray));
    }
    if (FIELD(connection, 0xa0, DArray) != 0) {
        ArrayFree(FIELD(connection, 0xa0, DArray));
    }
    if (FIELD(connection, 0xa4, DArray) != 0) {
        ArrayFree(FIELD(connection, 0xa4, DArray));
    }
    free(connection);
}
''')
emit(0x61c350, 14, 'gti2ClosedConnectionFree', 'the closed-connections array  element free: gti2FreeConnection of the element.', '''
extern void gti2FreeConnection(void *connection);

void gti2ClosedConnectionFree(void *elem)
{
    gti2FreeConnection(*(void **)elem);
}
''')
emit(0x61c620, 62, 'gti2FreeSocket', 'inside a callback only marks the socket to close (+0x14); otherwise closes it, frees its connection table, closed connections and itself, and WSACleanup.', F + '''
void gti2FreeSocket(void *socket)
{
    if (FIELD(socket, 0x18, int) != 0) {
        FIELD(socket, 0x14, int) = 1;
        return;
    }
    closesocket(FIELD(socket, 0x00, SOCKET));
    TableFree(FIELD(socket, 0x0c, HashTable));
    ArrayFree(FIELD(socket, 0x10, DArray));
    free(socket);
    WSACleanup();
}
''')
emit(0x61c3e0, 562, 'gt2CreateSocket', 'a 0x48 byte GT2 socket on the local address: buffer sizes default to 64 KB, a connection table (TableNew2(4, 32, 2, ...)) and closed-connection array; a broadcast-enabled UDP socket bound to the address, whose actual address is kept. 4 for a bad address, 1 out of memory, 3 on a socket error, else 0.', F + '''
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
''')

# ---------------- game side: 0x4410b0
lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x4410b0..0x4411fa: the game socket's GT2 unrecognized-message callback (socket, ip, port, message, length): like 0x441200 (copy of at most 0x1fff bytes to 0x006a4140; natneg packets to NNProcessData and handled) but a query ("\\\\" or ";" first, or 0xfe 0xfd) also goes to qr2_parse_queryA for the host record when there is one; queries count as handled, anything else 0.', 113)
wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
open('src/networking/network_channel_gap_4410b0.c', 'w', encoding='utf-8').write(
'''// network_channel_gap_4410b0  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x4410b0, size 331 bytes
// name confidence: 0.4   rewrite confidence: 0.85
''' + wr + '''// blam-cc: cdecl (a GT2 unrecognized message callback)

#include "tags.h"
#include <string.h>

extern uint8_t network_game_receive_buffer[0x2000]; // 0x006a4140
extern const uint8_t natneg_magic[6];               // 0x00657208
extern void *network_session_host_object;           // 0x00722a20
extern void FUN_00615240(char *data, int32_t len, void *fromaddr); // 0x615240 NNProcessData
extern void FUN_00616050(void *qrec, char *query, int32_t len, void *sender); // 0x616050 qr2_parse_queryA

int32_t network_channel_gap_4410b0(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length)
{
    uint8_t is_natneg = 0;
    uint8_t is_query;
    uint8_t address[16];

    (void)socket;
    if (length >= 0x1fff) {
        length = 0x1fff;
    }
    memcpy(network_game_receive_buffer, message, length);
    network_game_receive_buffer[length] = 0;
    if ((int32_t)length >= 6 && memcmp(network_game_receive_buffer, natneg_magic, 6) == 0) {
        is_natneg = 1;
    }
    is_query = ((int32_t)length >= 1 && network_game_receive_buffer[0] == 0x5c) || network_game_receive_buffer[0] == 0x3b ||
               ((int32_t)length >= 2 && network_game_receive_buffer[0] == 0xfe && network_game_receive_buffer[1] == 0xfd);
    memset(address, 0, sizeof(address));
    *(uint16_t *)(address + 0) = 2;
    *(uint16_t *)(address + 2) = (uint16_t)((port >> 8) | (port << 8));
    *(uint32_t *)(address + 4) = ip;
    if (is_natneg) {
        FUN_00615240((char *)network_game_receive_buffer, (int32_t)length, address);
        return 1;
    }
    if (!is_query) {
        return 0;
    }
    if (network_session_host_object != 0) {
        FUN_00616050(network_session_host_object, (char *)network_game_receive_buffer, (int32_t)length, address);
    }
    return 1;
}
''')
print('ok')
