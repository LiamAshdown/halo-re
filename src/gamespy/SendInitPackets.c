// SendInitPackets  (GameSpy SDK in halo.exe; no C existed)
// address 0x614c80, size 350 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614c80..0x614ddd: ESI negotiator: INIT packets (21 bytes) to natneg1:27901 as
//   port type 0 through the game socket (when used and unacknowledged) and type 1 through the negotiator socket
//   (unacknowledged), then -- the local port now filled from getsockname of the game (or negotiator) socket -- type 2
//   to natneg2; retry in 500 ms, up to 30 times. The local ip comes from NNGetLocalIP.
// blam-cc: ESI -> negotiator

#include "gamespy.h"

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
