// NNBeginNegotiationWithSocket  (GameSpy SDK in halo.exe; no C existed)
// address 0x614f30, size 142 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614f30..0x614fbd: a new negotiator (added before the servers are resolved): 3
//   when the servers do not resolve, 1 without a negotiator, 2 (and removed) when its own UDP socket fails; otherwise
//   the INIT packets go out and 0.
// blam-cc: cdecl

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
