// NegotiateThink  (GameSpy SDK in halo.exe; no C existed)
// address 0x615360, size 390 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x615360..0x6154e5: one negotiator: finished ones are removed; received packets
//   go through NNProcessData; in states 0 / 2 an expired retry resends (INIT / PING) or, past the retry limit,
//   completes with 2 and cancels; a connected one (3) completes -- without a game socket handing over its own socket
//   and the guessed address -- and cancels when its time is up; state 1 times out with 1.
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
