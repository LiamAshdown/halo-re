// gt2.h -- GameTransport 2 (GameSpy SDK gt2*) as linked into halo.exe, 0x614540..0x614890, 0x617db0..0x619c89,
// 0x61c300..0x61db95, 0x620520..0x62089a. Halo's copy is a modified GT2: connections swap Diffie-Hellman style
// keys during the handshake (gt2_bignum_mod_exp, generator "3", modulus "10001", 16-byte numbers) and every
// data message carries a CRC32 and is TEA-encrypted with the shared key (tea_encrypt_buffer / tea_decrypt_buffer).
// Layouts below are recovered from the code (offsets in comments); the functions are cdecl unless their header
// gives a register convention, which only matters to the binary -- in C they are called with plain arguments.
#ifndef HALO_GT2_H
#define HALO_GT2_H
#include "gamespy.h"

typedef struct GTI2Buffer {
    unsigned char *buffer;            // 0x0
    int size;                         // 0x4
    int len;                          // 0x8
} GTI2Buffer;

typedef struct GTI2Socket GTI2Socket;
typedef struct GTI2Connection GTI2Connection;

typedef void (*gt2ConnectedCallback)(GTI2Connection *connection, int result, const unsigned char *message, int len);
typedef void (*gt2ReceivedCallback)(GTI2Connection *connection, const unsigned char *message, int len, int reliable);
typedef void (*gt2ClosedCallback)(GTI2Connection *connection, int reason);
typedef void (*gt2PingCallback)(GTI2Connection *connection, int latency);
typedef void (*gt2ConnectAttemptCallback)(GTI2Socket *socket, GTI2Connection *connection, unsigned int ip,
    unsigned short port, int latency, const unsigned char *message, int len);
typedef void (*gt2SocketErrorCallback)(GTI2Socket *socket);
typedef void (*gt2DumpCallback)(GTI2Socket *socket, GTI2Connection *connection, unsigned int ip, unsigned short port,
    int reset, const unsigned char *message, int len, int arg9, int arg10);
typedef int (*gt2UnrecognizedMessageCallback)(GTI2Socket *socket, unsigned int ip, unsigned short port,
    const unsigned char *message, int len);
typedef void (*gt2FilterCallback)(GTI2Connection *connection, int filterID, const unsigned char *message, int len,
    int reliable);

typedef struct GT2ConnectionCallbacks {
    gt2ConnectedCallback connected;   // 0x0
    gt2ReceivedCallback received;     // 0x4
    gt2ClosedCallback closed;         // 0x8
    gt2PingCallback ping;             // 0xc
} GT2ConnectionCallbacks;

struct GTI2Socket {
    SOCKET socket;                                    // 0x00
    unsigned int ip;                                  // 0x04
    unsigned short port;                              // 0x08
    HashTable connections;                            // 0x0c, of GTI2Connection *
    DArray closedConnections;                         // 0x10, of GTI2Connection *
    int close;                                        // 0x14, free once the callback level drops to 0
    int callbackLevel;                                // 0x18
    gt2ConnectAttemptCallback connectAttemptCallback; // 0x1c
    gt2SocketErrorCallback socketErrorCallback;       // 0x20
    gt2DumpCallback sendDumpCallback;                 // 0x24
    gt2DumpCallback receiveDumpCallback;              // 0x28
    gt2UnrecognizedMessageCallback unrecognizedMessageCallback; // 0x2c
    void *data;                                       // 0x30
    int outgoingBufferSize;                           // 0x34
    int incomingBufferSize;                           // 0x38
    int error;                                        // 0x3c
    int unknown40;                                    // 0x40
    int unknown44;                                    // 0x44
};                                                    // size 0x48

// connection states (+0x0c)
enum {
    GTI2AwaitingServerChallenge = 0,
    GTI2AwaitingAcceptance = 1,
    GTI2AwaitingClientChallenge = 2,
    GTI2AwaitingClientResponse = 3,
    GTI2AwaitingAcceptReject = 4,
    GTI2Connected = 5,
    GTI2Closing = 6,
    GTI2Closed = 7
};

// message types (byte 2 after the 0xfe 0xfe magic)
enum {
    GTI2MsgAppReliable = 0,
    GTI2MsgClientChallenge = 1,
    GTI2MsgServerChallenge = 2,
    GTI2MsgClientResponse = 3,
    GTI2MsgAccept = 4,
    GTI2MsgReject = 5,
    GTI2MsgClose = 6,
    GTI2MsgKeepAlive = 7,
    GTI2MsgAck = 100,
    GTI2MsgNack = 101,
    GTI2MsgPing = 102,
    GTI2MsgPong = 103,
    GTI2MsgClosed = 104
};

struct GTI2Connection {
    unsigned int ip;                  // 0x000
    unsigned short port;              // 0x004
    GTI2Socket *socket;               // 0x008
    int state;                        // 0x00c
    int initiated;                    // 0x010
    int freeAtAcceptReject;           // 0x014
    int connectionResult;             // 0x018
    unsigned long startTime;          // 0x01c
    unsigned long timeout;            // 0x020
    int callbackLevel;                // 0x024
    GT2ConnectionCallbacks callbacks; // 0x028
    unsigned char *initialMessage;    // 0x038
    int initialMessageLen;            // 0x03c
    void *data;                       // 0x040
    GTI2Buffer incomingBuffer;        // 0x044
    GTI2Buffer outgoingBuffer;        // 0x050
    DArray incomingBufferMessages;    // 0x05c, of GTI2IncomingBufferMessage
    DArray outgoingBufferMessages;    // 0x060, of GTI2OutgoingBufferMessage
    unsigned short serialNumber;      // 0x064
    unsigned short expectedSerialNumber; // 0x066
    unsigned char response[0x20];     // 0x068
    unsigned long lastSend;           // 0x088
    unsigned long challengeTime;      // 0x08c
    unsigned long lastAck;            // 0x090
    int maxResends;                   // 0x094
    int pendingAck;                   // 0x098
    unsigned long pendingAckTime;     // 0x09c
    DArray sendFilters;               // 0x0a0, of gt2FilterCallback
    DArray receiveFilters;            // 0x0a4, of gt2FilterCallback
    unsigned char key[0x20];          // 0x0a8, the shared TEA key (16 bytes used); 3 8 3 3 until the exchange
    unsigned char publicKey[0x20];    // 0x0c8, 3 ^ private mod 0x10001 (16 bytes)
    char privateExponent[0x20];       // 0x0e8, 16 random hex digits
    unsigned char remotePublicKey[0x20]; // 0x108 (16 bytes)
    char generator[0x14];             // 0x128, "3"
    char modulus[0x14];               // 0x13c, "10001"
};                                    // size 0x150

typedef struct GTI2IncomingBufferMessage {
    int start;                        // 0x0
    int len;                          // 0x4
    int type;                         // 0x8
    unsigned short serialNumber;      // 0xc
} GTI2IncomingBufferMessage;          // size 0x10

typedef struct GTI2OutgoingBufferMessage {
    int start;                        // 0x00
    int len;                          // 0x04
    int resends;                      // 0x08
    unsigned short serialNumber;      // 0x0c
    unsigned long lastSend;           // 0x10
} GTI2OutgoingBufferMessage;          // size 0x14

static const unsigned char GTI2Magic[2] = {0xfe, 0xfe}; // the binary keeps these at 0x64e4d8
static const char GTI2PingTag[4] = {'t', 'i', 'm', 'e'};  // 0x64e4d0, first 4 bytes of a pong

// crypto (gt2_bignum_*.c, src/cseries/tea_*.c)
void gt2_bignum_mod_exp(char *base_hex, char *exponent_hex, char *modulus_hex, unsigned char *result);
void gt2_bignum_to_hex(const unsigned char *number, char *hex);
void tea_encrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key);
void tea_decrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key);
void datum_index_invalidate(void *index); // 0x4d02c0, *index = -1 (ICF: the CRC32 start value)
void crc32_update(uint32_t *crc, const void *data, int32_t length); // 0x4d02d0

// gt2Buffer.c / gt2Auth.c
int gti2AllocateBuffer(GTI2Buffer *buffer, int size);
int gti2GetBufferFreeSpace(const GTI2Buffer *buffer);
void gti2BufferWriteByte(GTI2Buffer *buffer, unsigned char b);
void gti2BufferWriteUShort(GTI2Buffer *buffer, unsigned short s);
void gti2BufferWriteData(GTI2Buffer *buffer, const unsigned char *data, int length);
void gti2BufferShorten(GTI2Buffer *buffer, int start, int shortenBy);
unsigned char *gti2GetChallenge(unsigned char *challenge);
unsigned char *gti2GetResponse(unsigned char *response, const unsigned char *challenge);
int gti2CheckResponse(const unsigned char *response1, const unsigned char *response2);

// gt2Main.c / gt2Utility.c
int gt2CreateSocket(void **socket_out, const char *local_address, int outgoing_buffer_size, int incoming_buffer_size,
    void *socket_error_callback);
char *gt2AddressToString(unsigned int ip, unsigned short port, char *string);
int gt2StringToAddress(const char *string, unsigned int *ip, unsigned short *port);
void gti2MessageCheck(const char **message, int *length);
void gt2Think(GTI2Socket *socket);
void gt2CloseSocket(GTI2Socket *socket);
int CanSendOnSocket(SOCKET sock);

// gt2Socket.c
GTI2Connection *gti2SocketFindConnection(GTI2Socket *socket, unsigned int ip, unsigned short port);
void gti2GeneratePrivateExponent(char *hex);
int gti2NewSocketConnection(GTI2Socket *socket, GTI2Connection **connection, unsigned int ip, unsigned short port);
int gti2NewOutgoingConnection(GTI2Socket *socket, GTI2Connection **connection, unsigned int ip, unsigned short port);
int gti2NewIncomingConnection(GTI2Socket *socket, GTI2Connection **connection, unsigned int ip, unsigned short port);
void gti2FreeSocket(GTI2Socket *socket);
void gti2FreeClosedConnection(GTI2Connection *connection);
int gti2SocketConnectionsThink(GTI2Socket *socket);
void gti2FreeClosedConnections(GTI2Socket *socket);
void gti2SocketError(GTI2Socket *socket);
int gti2SocketSend(GTI2Socket *socket, unsigned int ip, unsigned short port, const unsigned char *message, int len,
    int arg6, int arg7);

// gt2Connection.c
int gti2StartConnectionAttempt(GTI2Connection *connection, const unsigned char *message, int len,
    const GT2ConnectionCallbacks *callbacks);
int gti2ConnectionSendData(GTI2Connection *connection, const unsigned char *message, int len, int arg4, int arg5);
int gti2ResendMessages(GTI2Connection *connection, unsigned long now);
void gti2ConnectionClosed(GTI2Connection *connection);
int gti2CheckTimeout(GTI2Connection *connection, unsigned long now);
int gti2ConnectionThink(GTI2Connection *connection, unsigned long now);
void gti2CloseConnection(GTI2Connection *connection, int hard);

// gt2Callback.c
int gti2SocketErrorCallback(GTI2Socket *socket);
int gti2ConnectAttemptCallback(GTI2Socket *socket, GTI2Connection *connection, unsigned int ip, unsigned short port,
    int latency, const unsigned char *message, int len);
int gti2ConnectedCallback(GTI2Connection *connection, int result, const unsigned char *message, int len);
int gti2ReceivedCallback(GTI2Connection *connection, const unsigned char *message, int len, int reliable);
int gti2ClosedCallback(GTI2Connection *connection, int reason);
int gti2PingCallback(GTI2Connection *connection, int latency);
int gti2SendFilterCallback(GTI2Connection *connection, int filterID, const unsigned char *message, int len,
    int reliable);
int gti2ReceiveFilterCallback(GTI2Connection *connection, int filterID, const unsigned char *message, int len,
    int reliable);
int gti2DumpCallback(GTI2Socket *socket, GTI2Connection *connection, unsigned int ip, unsigned short port, int reset,
    const unsigned char *message, int len, int send, int arg9, int arg10);
int gti2UnrecognizedMessageCallback(GTI2Socket *socket, unsigned int ip, unsigned short port,
    const unsigned char *message, int len, int *handled);

// gt2Message.c
int gti2ConnectionError(GTI2Connection *connection, int result, int reason);
int gti2HandleAck(GTI2Connection *connection, unsigned short serialNumber);
int gti2HandleUnreliableMessage(GTI2Connection *connection, unsigned char *message, int len);
int gti2HandleReliableData(GTI2Connection *connection, unsigned char *message, int len);
int gti2HandleAccept(GTI2Connection *connection, const unsigned char *data);
void gti2RemoveIncomingBufferMessage(GTI2Connection *connection, int index, const GTI2IncomingBufferMessage *message);
void gti2SetPendingAck(GTI2Connection *connection);
int gti2HandlePong(GTI2Connection *connection, const unsigned char *data, int len);
int gti2HandleClosed(GTI2Connection *connection);
int gti2HandleConnectionReset(GTI2Socket *socket, unsigned int ip, unsigned short port);
int gti2AddOutgoingBufferMessage(GTI2Connection *connection, int len, unsigned short serialNumber);
int gti2SendLastOutgoingMessage(GTI2Connection *connection);
int gti2SendUnreliable(GTI2Connection *connection, const unsigned char *message, int len);
int gti2SendAck(GTI2Connection *connection);
int gti2SendNack(GTI2Connection *connection, unsigned short from, unsigned short to);
int gti2SendClosed(GTI2Socket *socket, unsigned int ip, unsigned short port);
int gti2ResendMessage(GTI2Connection *connection, GTI2OutgoingBufferMessage *message);
int gti2BufferIncomingMessage(GTI2Connection *connection, int len, int type, unsigned short serialNumber,
    const unsigned char *message, int *overflow);
int gti2HandleNack(GTI2Connection *connection, const unsigned char *data, int len);
int gti2HandleAdminMessage(GTI2Connection *connection, unsigned char *message, int len, int type);
int gti2ConnectionSendClosed(GTI2Connection *connection);
int gti2HandleOutOfMemory(GTI2Connection *connection);
int gti2HandleClientResponse(GTI2Connection *connection, const unsigned char *data, int len);
int gti2HandleReject(GTI2Connection *connection, const unsigned char *message, int len);
int gti2HandleClose(GTI2Connection *connection);
int gti2BeginReliableMessage(GTI2Connection *connection, int len, int type, int *overflow);
int gti2SendDataReliable(GTI2Connection *connection, const unsigned char *message, int len);
int gti2SendClientChallenge(GTI2Connection *connection, const unsigned char *challenge);
int gti2SendServerChallenge(GTI2Connection *connection, const unsigned char *response, const unsigned char *challenge);
int gti2SendClientResponse(GTI2Connection *connection, const unsigned char *response, const unsigned char *message,
    int len);
int gti2SendAccept(GTI2Connection *connection);
int gti2SendReject(GTI2Connection *connection, const unsigned char *message, int len);
int gti2SendClose(GTI2Connection *connection);
int gti2SendKeepAlive(GTI2Connection *connection);
int gti2Send(GTI2Connection *connection, unsigned char *message, int len, int reliable);
int gti2HandleClientChallenge(GTI2Connection *connection, const unsigned char *challenge, int len);
int gti2HandleServerChallenge(GTI2Connection *connection, const unsigned char *data, int len);
int gti2DeliverReliableMessage(GTI2Connection *connection, int type, unsigned char *data, int len);
int gti2DeliverQueuedMessages(GTI2Connection *connection);
int gti2HandleReliableMessage(GTI2Connection *connection, unsigned char *message, int len, int type);
int gti2HandleMessage(GTI2Socket *socket, unsigned char *message, int len, unsigned int ip, unsigned short port);
int gti2ReceiveMessages(GTI2Socket *socket);

#endif
