exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())

# natneg.c: negotiators are 0x40 byte records in the DArray at 0x006a26c8.
N = '''
typedef void (*NegotiateProgressFunc)(int state, void *userdata);
typedef void (*NegotiateCompletedFunc)(int result, SOCKET gamesocket, struct sockaddr_in *remoteaddr, void *userdata);

typedef struct NATNegotiator {
    SOCKET negotiatorSocket;          // 0x00
    SOCKET gameSocket;                // 0x04
    int cookie;                       // 0x08
    int clientIndex;                  // 0x0c
    int state;                        // 0x10 0 init sent, 1 init complete, 2 connecting, 3 connected, 4 finished
    int initAckReceived[3];           // 0x14
    int retryCount;                   // 0x20
    int maxRetryCount;                // 0x24
    unsigned long retryTime;          // 0x28
    unsigned int guessedIP;           // 0x2c
    unsigned short guessedPort;       // 0x30
    unsigned char gotRemoteData;      // 0x32
    unsigned char pad_33;             // 0x33
    NegotiateProgressFunc progressCallback;   // 0x34
    NegotiateCompletedFunc completedCallback; // 0x38
    void *userdata;                   // 0x3c
} NATNegotiator;                      // size 0x40

extern DArray negotiatorList;          // 0x006a26c8
extern unsigned int matchup1ip;        // 0x006a26cc
extern unsigned int matchup2ip;        // 0x006a26d0
extern const char *Matchup1Hostname;   // 0x0068382c "natneg1.hosthpc.com"
extern const char *Matchup2Hostname;   // 0x00683830 "natneg2.hosthpc.com"
extern unsigned char NNMagicData[6];   // 0x00683824
extern char natneg_receive_buffer[0x200]; // 0x006a24c8

#define MATCHUP_PORT 0x6cfd
NATNegotiator *FindNegotiatorForCookie(int cookie);
NATNegotiator *AddNegotiator(void);
void RemoveNegotiator(NATNegotiator *negotiator);
void NNSendPacket(SOCKET sock, unsigned int ip, unsigned short port, void *data, int len);
unsigned int NNGetLocalIP(void);
void SendInitPackets(NATNegotiator *negotiator);
void SendPingPacket(NATNegotiator *negotiator);
int ResolveServers(void);
void NNCancel(int cookie);
void NNProcessData(char *data, int len, struct sockaddr_in *fromaddr);
'''

def e(addr, size, name, note, code, extra='', cc='cdecl'):
    emit(addr, size, name, note, N + extra + '\n' + code, cc=cc)

e(0x614ab0, 78, 'FindNegotiatorForCookie', 'EDI cookie: the negotiator with that cookie, or NULL (also without a list).', '''
NATNegotiator *FindNegotiatorForCookie(int cookie)
{
    int i;

    if (negotiatorList == 0) {
        return 0;
    }
    for (i = 0; i < ArrayLength(negotiatorList); i++) {
        NATNegotiator *negotiator = (NATNegotiator *)ArrayNth(negotiatorList, i);

        if (negotiator->cookie == cookie) {
            return negotiator;
        }
    }
    return 0;
}
''', cc='EDI -> cookie')
e(0x614b00, 84, 'AddNegotiator', 'appends a zeroed negotiator (creating the list, 0x40 byte elements grown by 4, on first use) and returns it.', '''
NATNegotiator *AddNegotiator(void)
{
    NATNegotiator blank;

    memset(&blank, 0, sizeof(blank));
    if (negotiatorList == 0) {
        negotiatorList = ArrayNew(sizeof(NATNegotiator), 4, 0);
    }
    ArrayAppend(negotiatorList, &blank);
    return (NATNegotiator *)ArrayNth(negotiatorList, ArrayLength(negotiatorList) - 1);
}
''')
e(0x614b60, 80, 'RemoveNegotiator', 'EDI negotiator: removes it from the list (no free function).', '''
void RemoveNegotiator(NATNegotiator *negotiator)
{
    int i;

    for (i = 0; i < ArrayLength(negotiatorList); i++) {
        if ((NATNegotiator *)ArrayNth(negotiatorList, i) == negotiator) {
            ArrayRemoveAt(negotiatorList, i);
            return;
        }
    }
}
''', cc='EDI -> negotiator')
e(0x614bb0, 84, 'NNSendPacket', 'sendto the ip and (host order) port.', '''
void NNSendPacket(SOCKET sock, unsigned int ip, unsigned short port, void *data, int len)
{
    struct sockaddr_in address;

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = ip;
    sendto(sock, (const char *)data, len, 0, (const struct sockaddr *)&address, 0x10);
}
''')
e(0x61d3d0, 88, 'getlocalhost', 'gethostbyname(gethostname()) (a zeroed 0x100 byte name).', '''
struct hostent *getlocalhost(void)
{
    char hostname[0x100];

    memset(hostname, 0, sizeof(hostname));
    gethostname(hostname, 0x100);
    return gethostbyname(hostname);
}
''')
e(0x61d430, 76, 'IsPrivateIP', 'whether the address is in 10/8, 172.16/12 or 192.168/16.', '''
int IsPrivateIP(struct in_addr *addr)
{
    unsigned int ip = ntohl(addr->s_addr);
    unsigned int b1 = ip >> 24;
    unsigned int b2 = (ip >> 16) & 0xff;

    if (b1 == 10) {
        return 1;
    }
    if (b1 == 172) {
        return b2 >= 16 && b2 <= 31;
    }
    return b1 == 192 && b2 == 168;
}
''')
e(0x614c10, 103, 'NNGetLocalIP', 'the first private non-loopback address of the local host, else the last non-loopback one, else 0.', '''
extern struct hostent *getlocalhost(void);
extern int IsPrivateIP(struct in_addr *addr);

unsigned int NNGetLocalIP(void)
{
    struct hostent *host = getlocalhost();
    unsigned int result = 0;
    int i;

    if (host == 0) {
        return 0;
    }
    for (i = 0; host->h_addr_list[i] != 0; i++) {
        struct in_addr *address = (struct in_addr *)host->h_addr_list[i];

        if (address->s_addr != htonl(0x7f000001)) {
            result = address->s_addr;
            if (IsPrivateIP(address)) {
                return result;
            }
        }
    }
    return result;
}
''')
PKT = '''
#pragma pack(push, 1)
typedef struct NatNegPacket {
    unsigned char magic[6];           // 0x00
    unsigned char version;            // 0x06 2
    unsigned char packettype;         // 0x07
    int cookie;                       // 0x08 network order
    union {
        struct {
            unsigned char porttype;       // 0x0c
            unsigned char clientindex;    // 0x0d
            unsigned char usegameport;    // 0x0e
            unsigned char localip[4];     // 0x0f
            unsigned char localport[2];   // 0x13
        } init;                           // 0x15 bytes in all
        struct {
            unsigned int remoteIP;        // 0x0c
            unsigned short remotePort;    // 0x10
            unsigned char gotyourdata;    // 0x12
            unsigned char finished;       // 0x13
        } connect;                        // 0x14 bytes in all
    } Packet;
    unsigned char pad[4];
} NatNegPacket;
#pragma pack(pop)
'''
e(0x614c80, 350, 'SendInitPackets', 'ESI negotiator: INIT packets (21 bytes) to natneg1:27901 as port type 0 through the game socket (when used and unacknowledged) and type 1 through the negotiator socket (unacknowledged), then -- the local port now filled from getsockname of the game (or negotiator) socket -- type 2 to natneg2; retry in 500 ms, up to 30 times. The local ip comes from NNGetLocalIP.', PKT + '''
void SendInitPackets(NATNegotiator *negotiator)
{
    NatNegPacket packet;
    struct sockaddr_in address;
    int length;
    unsigned int local_ip;
    unsigned short port;
    SOCKET sock;

    memcpy(packet.magic, NNMagicData, 6);
    packet.version = 2;
    packet.packettype = 0;
    packet.cookie = htonl(negotiator->cookie);
    packet.Packet.init.clientindex = (unsigned char)negotiator->clientIndex;
    packet.Packet.init.usegameport = negotiator->gameSocket != INVALID_SOCKET;
    local_ip = ntohl(NNGetLocalIP());
    packet.Packet.init.localip[0] = (unsigned char)(local_ip >> 24);
    packet.Packet.init.localip[1] = (unsigned char)(local_ip >> 16);
    packet.Packet.init.localip[2] = (unsigned char)(local_ip >> 8);
    packet.Packet.init.localip[3] = (unsigned char)local_ip;
    packet.Packet.init.localport[0] = 0;
    packet.Packet.init.localport[1] = 0;
    if (packet.Packet.init.usegameport && negotiator->initAckReceived[0] == 0) {
        packet.Packet.init.porttype = 0;
        NNSendPacket(negotiator->gameSocket, matchup1ip, MATCHUP_PORT, &packet, 0x15);
    }
    if (negotiator->initAckReceived[1] == 0) {
        packet.Packet.init.porttype = 1;
        NNSendPacket(negotiator->negotiatorSocket, matchup1ip, MATCHUP_PORT, &packet, 0x15);
    }
    sock = packet.Packet.init.usegameport ? negotiator->gameSocket : negotiator->negotiatorSocket;
    length = 0x10;
    port = ntohs(getsockname(sock, (struct sockaddr *)&address, &length) != SOCKET_ERROR ? address.sin_port : 0);
    packet.Packet.init.localport[0] = (unsigned char)(port >> 8);
    packet.Packet.init.localport[1] = (unsigned char)port;
    if (negotiator->initAckReceived[2] == 0) {
        packet.Packet.init.porttype = 2;
        NNSendPacket(negotiator->negotiatorSocket, matchup2ip, MATCHUP_PORT, &packet, 0x15);
    }
    negotiator->retryTime = GetTickCount() + 500;
    negotiator->maxRetryCount = 30;
}
''', cc='ESI -> negotiator')
e(0x614de0, 200, 'SendPingPacket', 'ESI negotiator: a PING (type 7, 20 bytes: the guessed address, got-your-data, finished = state is not 2) to the guessed address through the game socket (or the negotiator socket without one); retry in 700 ms, up to 20 times.', PKT + '''
void SendPingPacket(NATNegotiator *negotiator)
{
    NatNegPacket packet;
    struct sockaddr_in address;
    SOCKET sock;

    memcpy(packet.magic, NNMagicData, 6);
    packet.version = 2;
    packet.packettype = 7;
    packet.cookie = htonl(negotiator->cookie);
    packet.Packet.connect.remoteIP = negotiator->guessedIP;
    packet.Packet.connect.remotePort = htons(negotiator->guessedPort);
    packet.Packet.connect.gotyourdata = negotiator->gotRemoteData;
    packet.Packet.connect.finished = negotiator->state != 2;
    sock = negotiator->gameSocket;
    if (sock == INVALID_SOCKET) {
        sock = negotiator->negotiatorSocket;
    }
    address.sin_family = AF_INET;
    address.sin_port = htons(negotiator->guessedPort);
    address.sin_addr.s_addr = negotiator->guessedIP;
    sendto(sock, (const char *)&packet, 0x14, 0, (const struct sockaddr *)&address, 0x10);
    negotiator->retryTime = GetTickCount() + 700;
    negotiator->maxRetryCount = 20;
}
''', cc='ESI -> negotiator')
e(0x614eb0, 123, 'ResolveServers', 'resolves natneg1 / natneg2 (inet_addr, else gethostbyname) when not yet known; whether both are.', '''
static unsigned int resolve(const char *hostname)
{
    unsigned int ip = inet_addr(hostname);

    if (ip == INADDR_NONE) {
        struct hostent *host = gethostbyname(hostname);

        ip = host != 0 ? *(unsigned int *)host->h_addr_list[0] : 0;
    }
    return ip;
}

int ResolveServers(void)
{
    if (matchup1ip == 0) {
        matchup1ip = resolve(Matchup1Hostname);
    }
    if (matchup2ip == 0) {
        matchup2ip = resolve(Matchup2Hostname);
    }
    return matchup1ip != 0 && matchup2ip != 0;
}
''')
e(0x614f30, 142, 'NNBeginNegotiationWithSocket', 'a new negotiator (added before the servers are resolved): 3 when the servers do not resolve, 1 without a negotiator, 2 (and removed) when its own UDP socket fails; otherwise the INIT packets go out and 0.', '''
int NNBeginNegotiationWithSocket(SOCKET gameSocket, int cookie, int clientindex, NegotiateProgressFunc progresscallback,
    NegotiateCompletedFunc completedcallback, void *userdata)
{
    NATNegotiator *negotiator = AddNegotiator();

    if (!ResolveServers()) {
        return 3;
    }
    if (negotiator == 0) {
        return 1;
    }
    negotiator->gameSocket = gameSocket;
    negotiator->clientIndex = clientindex;
    negotiator->cookie = cookie;
    negotiator->progressCallback = progresscallback;
    negotiator->completedCallback = completedcallback;
    negotiator->userdata = userdata;
    negotiator->negotiatorSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    negotiator->retryCount = 0;
    negotiator->gotRemoteData = 0;
    negotiator->guessedIP = 0;
    negotiator->guessedPort = 0;
    negotiator->maxRetryCount = 0;
    if (negotiator->negotiatorSocket == INVALID_SOCKET) {
        RemoveNegotiator(negotiator);
        return 2;
    }
    SendInitPackets(negotiator);
    return 0;
}
''')
e(0x614fc0, 46, 'NNCancel', 'closes the negotiator socket of the cookie\'s negotiator and marks it finished (4).', '''
void NNCancel(int cookie)
{
    NATNegotiator *negotiator = FindNegotiatorForCookie(cookie);

    if (negotiator == 0) {
        return;
    }
    if (negotiator->negotiatorSocket != INVALID_SOCKET) {
        closesocket(negotiator->negotiatorSocket);
    }
    negotiator->negotiatorSocket = INVALID_SOCKET;
    negotiator->state = 4;
}
''')
e(0x614ff0, 145, 'SendConnectAck', 'ECX from, EDX negotiator: a CONNECT_ACK (type 6) back to the sender through the negotiator socket. The binary sends 21 bytes of which the last 7 are uninitialized stack; they are zero here.', PKT + '''
void SendConnectAck(NATNegotiator *negotiator, struct sockaddr_in *fromaddr)
{
    NatNegPacket packet;
    struct sockaddr_in address;

    memset(&packet, 0, sizeof(packet));
    memcpy(packet.magic, NNMagicData, 6);
    packet.version = 2;
    packet.packettype = 6;
    packet.cookie = htonl(negotiator->cookie);
    packet.Packet.init.clientindex = (unsigned char)negotiator->clientIndex;
    address.sin_family = AF_INET;
    address.sin_port = htons(ntohs(fromaddr->sin_port));
    address.sin_addr.s_addr = fromaddr->sin_addr.s_addr;
    sendto(negotiator->negotiatorSocket, (const char *)&packet, 0x15, 0, (const struct sockaddr *)&address, 0x10);
}
''', cc='ECX -> fromaddr, EDX -> negotiator')
e(0x615090, 176, 'ProcessConnectPacket', 'EAX packet, EDX negotiator, ECX from: acknowledged unless it carries an error; before connecting, an error (1 deadbeat partner, 2 init timeout, else 3) completes with it and cancels, otherwise the guessed address is taken, the state becomes 2 (progress) and pinging starts.', PKT + '''
extern void SendConnectAck(NATNegotiator *negotiator, struct sockaddr_in *fromaddr);

void ProcessConnectPacket(NatNegPacket *packet, NATNegotiator *negotiator, struct sockaddr_in *fromaddr)
{
    if (packet->Packet.connect.finished == 0) {
        SendConnectAck(negotiator, fromaddr);
    }
    if (negotiator->state >= 2) {
        return;
    }
    if (packet->Packet.connect.finished != 0) {
        int result = packet->Packet.connect.finished == 1 ? 1 : (packet->Packet.connect.finished == 2 ? 2 : 3);

        negotiator->completedCallback(result, INVALID_SOCKET, 0, negotiator->userdata);
        NNCancel(negotiator->cookie);
        return;
    }
    negotiator->guessedIP = packet->Packet.connect.remoteIP;
    negotiator->guessedPort = ntohs(packet->Packet.connect.remotePort);
    negotiator->retryCount = 0;
    negotiator->state = 2;
    negotiator->progressCallback(2, negotiator->userdata);
    SendPingPacket(negotiator);
}
''', cc='EAX -> packet, EDX -> negotiator, ECX -> fromaddr')
e(0x615140, 106, 'ProcessPingPacket', 'EAX negotiator, EDI from, EBX packet: while connecting, the sender becomes the guessed address and data counts as received; a ping that got ours connects (state 3, finishing in 5 s; with a game socket the completion reports success with it and the sender) unless already done; otherwise we ping back.', PKT + '''
void ProcessPingPacket(NATNegotiator *negotiator, struct sockaddr_in *fromaddr, NatNegPacket *packet)
{
    if (negotiator->state < 2) {
        return;
    }
    negotiator->guessedIP = fromaddr->sin_addr.s_addr;
    negotiator->guessedPort = ntohs(fromaddr->sin_port);
    negotiator->gotRemoteData = 1;
    if (packet->Packet.connect.gotyourdata != 0) {
        if (negotiator->state == 2) {
            negotiator->state = 3;
            negotiator->retryTime = GetTickCount() + 5000;
            if (negotiator->gameSocket != INVALID_SOCKET) {
                negotiator->completedCallback(0, negotiator->gameSocket, fromaddr, negotiator->userdata);
            }
            return;
        }
        if (packet->Packet.connect.finished != 0) {
            return;
        }
    }
    SendPingPacket(negotiator);
}
''', cc='EAX -> negotiator, EDI -> fromaddr, EBX -> packet')
e(0x6151b0, 130, 'ProcessInitPacket', 'EAX packet, ESI negotiator, EDI from: an INIT_ACK (1) marks its port type; with ports 1 and 2 (and 0 when using the game socket) acknowledged the state becomes 1 (progress; finishing in 60 s). An ERT_TEST (2) is answered as ERT_ACK (3) to the sender.', PKT + '''
void ProcessInitPacket(NatNegPacket *packet, NATNegotiator *negotiator, struct sockaddr_in *fromaddr)
{
    if (packet->packettype == 1) {
        if (packet->Packet.init.porttype > 2) {
            return;
        }
        negotiator->initAckReceived[packet->Packet.init.porttype] = 1;
        if (negotiator->state == 0 && negotiator->initAckReceived[1] != 0 && negotiator->initAckReceived[2] != 0 &&
            (negotiator->gameSocket == INVALID_SOCKET || negotiator->initAckReceived[0] != 0)) {
            negotiator->state = 1;
            negotiator->retryTime = GetTickCount() + 60000;
            negotiator->progressCallback(negotiator->state, negotiator->userdata);
        }
    } else if (packet->packettype == 2) {
        packet->packettype = 3;
        NNSendPacket(negotiator->negotiatorSocket, fromaddr->sin_addr.s_addr, ntohs(fromaddr->sin_port), packet, 0x15);
    }
}
''', cc='EAX -> packet, ESI -> negotiator, EDI -> fromaddr')
e(0x615240, 284, 'NNProcessData', 'a packet with the natneg magic: CONNECT (5) and PING (7) of at least 20 bytes, other types of at least 21, go (as a stack copy) to the cookie\'s negotiator\'s handler.', PKT + '''
extern void ProcessConnectPacket(NatNegPacket *packet, NATNegotiator *negotiator, struct sockaddr_in *fromaddr);
extern void ProcessPingPacket(NATNegotiator *negotiator, struct sockaddr_in *fromaddr, NatNegPacket *packet);
extern void ProcessInitPacket(NatNegPacket *packet, NATNegotiator *negotiator, struct sockaddr_in *fromaddr);

void NNProcessData(char *data, int len, struct sockaddr_in *fromaddr)
{
    NatNegPacket packet;
    NATNegotiator *negotiator;
    unsigned char type;

    if (memcmp(data, NNMagicData, 6) != 0) {
        return;
    }
    type = (unsigned char)data[7];
    if (type == 5 || type == 7) {
        if (len < 0x14) {
            return;
        }
        memcpy(&packet, data, 0x14);
        negotiator = FindNegotiatorForCookie((int)ntohl(*(unsigned int *)(data + 8)));
        if (negotiator == 0) {
            return;
        }
        if (type == 5) {
            ProcessConnectPacket(&packet, negotiator, fromaddr);
        } else {
            ProcessPingPacket(negotiator, fromaddr, &packet);
        }
        return;
    }
    if (len < 0x15) {
        return;
    }
    memcpy(&packet, data, 0x15);
    negotiator = FindNegotiatorForCookie((int)ntohl(*(unsigned int *)(data + 8)));
    if (negotiator != 0) {
        ProcessInitPacket(&packet, negotiator, fromaddr);
    }
}
''')
e(0x615360, 390, 'NegotiateThink', 'one negotiator: finished ones are removed; received packets go through NNProcessData; in states 0 / 2 an expired retry resends (INIT / PING) or, past the retry limit, completes with 2 and cancels; a connected one (3) completes -- without a game socket handing over its own socket and the guessed address -- and cancels when its time is up; state 1 times out with 1.', '''
void NegotiateThink(NATNegotiator *negotiator)
{
    struct sockaddr_in from;
    int from_length = 0x10;

    if (negotiator->state == 4) {
        RemoveNegotiator(negotiator);
        return;
    }
    if (negotiator->negotiatorSocket != INVALID_SOCKET && CanReceiveOnSocket(negotiator->negotiatorSocket)) {
        for (;;) {
            int length = recvfrom(negotiator->negotiatorSocket, natneg_receive_buffer, 0x200, 0, (struct sockaddr *)&from,
                &from_length);

            if (length == SOCKET_ERROR) {
                break;
            }
            NNProcessData(natneg_receive_buffer, length, &from);
            if (negotiator->state == 4 || negotiator->negotiatorSocket == INVALID_SOCKET ||
                !CanReceiveOnSocket(negotiator->negotiatorSocket)) {
                break;
            }
        }
    }
    if ((negotiator->state == 0 || negotiator->state == 2) && GetTickCount() > negotiator->retryTime) {
        if (negotiator->maxRetryCount < negotiator->retryCount) {
            negotiator->completedCallback(2, INVALID_SOCKET, 0, negotiator->userdata);
            NNCancel(negotiator->cookie);
        } else {
            negotiator->retryCount++;
            if (negotiator->state == 0) {
                SendInitPackets(negotiator);
            } else {
                SendPingPacket(negotiator);
            }
        }
    }
    if (negotiator->state == 3 && GetTickCount() > negotiator->retryTime) {
        if (negotiator->gameSocket == INVALID_SOCKET) {
            struct sockaddr_in address;

            address.sin_family = AF_INET;
            address.sin_port = htons(negotiator->guessedPort);
            address.sin_addr.s_addr = negotiator->guessedIP;
            negotiator->completedCallback(0, negotiator->negotiatorSocket, &address, negotiator->userdata);
            negotiator->negotiatorSocket = INVALID_SOCKET;
        }
        NNCancel(negotiator->cookie);
    }
    if (negotiator->state == 1 && GetTickCount() > negotiator->retryTime) {
        negotiator->completedCallback(1, INVALID_SOCKET, 0, negotiator->userdata);
        NNCancel(negotiator->cookie);
    }
}
''')
print('ok')
