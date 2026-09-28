// ProcessConnectPacket  (GameSpy SDK in halo.exe; no C existed)
// address 0x615090, size 176 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x615090..0x61513f: EAX packet, EDX negotiator, ECX from: acknowledged unless it
//   carries an error; before connecting, an error (1 deadbeat partner, 2 init timeout, else 3) completes with it and
//   cancels, otherwise the guessed address is taken, the state becomes 2 (progress) and pinging starts.
// blam-cc: EAX -> packet, EDX -> negotiator, ECX -> fromaddr

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
