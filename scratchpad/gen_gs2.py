exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())

# GT2 (GameSpy Transport 2) sockets and connections are opaque here; the fields are read at the offsets the
# binary uses (socket: +0x00 SOCKET, +0x04 local ip, +0x08 local port, +0x0c connections table, +0x28 send dump,
# +0x2c unrecognized message callback, +0x30 user data; connection: +0x00 remote ip, +0x04 remote port,
# +0x0c state, +0x40 user data).
F = '#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))\n'

emit(0x6147a0, 48, 'gt2GetConnectionState', 'the connection state (+0x0c) as GT2ConnectionState: below 5 connecting (0), 5 connected (1), 6 closing (2), above closed (3).', F + '''
int gt2GetConnectionState(void *connection)
{
    int state = FIELD(connection, 0x0c, int);

    if (state < 5) {
        return 0;
    }
    if (state == 5) {
        return 1;
    }
    return state != 6 ? 3 : 2;
}
''')
emit(0x6147d0, 9, 'gt2GetRemotePort', 'the connection\'s remote port (+0x04).', F + '''
unsigned short gt2GetRemotePort(void *connection)
{
    return FIELD(connection, 0x04, unsigned short);
}
''')
emit(0x6147e0, 8, 'gt2GetLocalIP', 'the socket\'s local ip (+0x04).', F + '''
unsigned int gt2GetLocalIP(void *socket)
{
    return FIELD(socket, 0x04, unsigned int);
}
''')
emit(0x6147f0, 9, 'gt2GetLocalPort', 'the socket\'s local port (+0x08).', F + '''
unsigned short gt2GetLocalPort(void *socket)
{
    return FIELD(socket, 0x08, unsigned short);
}
''')
emit(0x614800, 12, 'gt2SetUnrecognizedMessageCallback', 'stores the callback at socket +0x2c.', F + '''
void gt2SetUnrecognizedMessageCallback(void *socket, void *callback)
{
    FIELD(socket, 0x2c, void *) = callback;
}
''')
emit(0x614810, 12, 'gt2SetSocketData', 'stores the user data at socket +0x30.', F + '''
void gt2SetSocketData(void *socket, void *data)
{
    FIELD(socket, 0x30, void *) = data;
}
''')
emit(0x614820, 8, 'gt2GetSocketData', 'the user data at socket +0x30.', F + '''
void *gt2GetSocketData(void *socket)
{
    return FIELD(socket, 0x30, void *);
}
''')
emit(0x614830, 12, 'gt2SetConnectionData', 'stores the user data at connection +0x40.', F + '''
void gt2SetConnectionData(void *connection, void *data)
{
    FIELD(connection, 0x40, void *) = data;
}
''')
emit(0x614840, 8, 'gt2GetConnectionData', 'the user data at connection +0x40.', F + '''
void *gt2GetConnectionData(void *connection)
{
    return FIELD(connection, 0x40, void *);
}
''')
emit(0x614850, 12, 'gt2SetSendDump', 'stores the dump callback at socket +0x28.', F + '''
void gt2SetSendDump(void *socket, void *callback)
{
    FIELD(socket, 0x28, void *) = callback;
}
''')
emit(0x614890, 11, 'gt2NetworkToHostInt', 'ntohl (WSOCK32 #14 through its delay-import thunk).', '''
unsigned int gt2NetworkToHostInt(unsigned int value)
{
    return ntohl(value);
}
''')
emit(0x6148a0, 11, 'gt2NetworkToHostShort', 'ntohs (WSOCK32 #15 through its delay-import thunk).', '''
unsigned short gt2NetworkToHostShort(unsigned short value)
{
    return ntohs(value);
}
''')
emit(0x6148b0, 144, 'gt2AddressToString', 'without a buffer it alternates between two static 0x16 byte strings (0x006a2494, index 0x006a24c0, kept here as statics); "%s:%d" / "%s" with inet_ntoa (WS2_32 #12) for a non-zero ip, ":%d" for a port alone, else "".', '''
static char address_strings[2][0x16]; // 0x006a2494
static int address_string_index;      // 0x006a24c0

char *gt2AddressToString(unsigned int ip, unsigned short port, char *string)
{
    struct in_addr address;

    if (string == 0) {
        address_string_index ^= 1;
        string = address_strings[address_string_index];
    }
    address.s_addr = ip;
    if (ip != 0) {
        if (port != 0) {
            sprintf(string, "%s:%d", inet_ntoa(address), port);
        } else {
            sprintf(string, "%s", inet_ntoa(address));
        }
    } else if (port != 0) {
        sprintf(string, ":%d", port);
    } else {
        string[0] = 0;
    }
    return string;
}
''')
emit(0x614940, 294, 'gt2StringToAddress', '"host", "host:port" or ":port": the host part (copied to a stack buffer before the colon) goes through inet_addr (WS2_32 #11) and, when that fails, gethostbyname (#52, its first address); the port must be all digits and 0..0xffff. An empty or NULL string is address 0 port 0. Returns 0 on a bad port or an unknown host, else stores the results and returns 1.', '''
int gt2StringToAddress(const char *string, unsigned int *ip, unsigned short *port)
{
    char host_buffer[0x100];
    const char *host = string;
    const char *colon;
    unsigned int ip_value = 0;
    unsigned short port_value = 0;

    if (string != 0 && string[0] != 0) {
        colon = strchr(string, ':');
        if (colon != 0) {
            const char *digits;
            int value;

            if (colon == string) {
                host = 0;
            } else {
                memcpy(host_buffer, string, colon - string);
                host_buffer[colon - string] = 0;
                host = host_buffer;
            }
            for (digits = colon + 1; *digits != 0; digits++) {
                if (!isdigit(*digits)) {
                    return 0;
                }
            }
            value = atoi(colon + 1);
            if (value < 0 || value > 0xffff) {
                return 0;
            }
            port_value = (unsigned short)value;
        }
        if (host != 0) {
            ip_value = inet_addr(host);
            if (ip_value == INADDR_NONE) {
                struct hostent *entry = gethostbyname(host);

                if (entry == 0) {
                    return 0;
                }
                ip_value = *(unsigned int *)entry->h_addr_list[0];
            }
        }
    }
    if (ip != 0) {
        *ip = ip_value;
    }
    if (port != 0) {
        *port = port_value;
    }
    return 1;
}
''', extra='#include <ctype.h>\n')
emit(0x614a70, 54, 'gti2MessageCheck', 'normalizes a (message, length) pair in place: a NULL message becomes "" (0x0065512c) with length 0, and length -1 becomes strlen + 1.', '''
void gti2MessageCheck(const char **message, int *length)
{
    if (*message == 0) {
        *message = "";
        *length = 0;
        return;
    }
    if (*length == -1) {
        *length = (int)strlen(*message) + 1;
    }
}
''', name_confidence='0.6')
print('ok')
