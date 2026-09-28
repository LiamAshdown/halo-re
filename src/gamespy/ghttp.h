// ghttp.h -- the GameSpy HTTP SDK (ghttpMain.c, ghttpConnection.c, ghttpProcess.c, ghttpBuffer.c,
// ghttpCallbacks.c, ghttpCommon.c, ghttpPost.c) and the patch tracker (pt.c) as linked into halo.exe,
// 0x61bb60..0x61c2f6, 0x6208a0..0x622e95. Layouts recovered from the code (offsets in comments). Globals are
// declared in the .c files that use them (their address comments bind them).
#ifndef HALO_GHTTP_H
#define HALO_GHTTP_H
#include "gamespy.h"

typedef struct GHIConnection GHIConnection;

typedef struct GHIBuffer {
    GHIConnection *connection;        // 0x00
    char *data;                       // 0x04
    int size;                         // 0x08
    int len;                          // 0x0c
    int pos;                          // 0x10
    int sizeIncrement;                // 0x14
    int fixed;                        // 0x18
    int dontFree;                     // 0x1c
} GHIBuffer;                          // size 0x20

typedef void (*ghttpProgressCallback)(int request, int state, const char *buffer, int bufferLen, int bytesReceived,
    int totalSize, void *param);
typedef int (*ghttpCompletedCallback)(int request, int result, char *buffer, int bufferLen, void *param);
typedef void (*ghttpPostCallback)(int request, int bytesPosted, int totalBytes, int objectsPosted, int totalObjects,
    void *param);

typedef struct GHIPost {
    DArray data;                      // 0x00, of GHIPostData
    ghttpPostCallback callback;       // 0x04
    void *param;                      // 0x08
    int useMultipart;                 // 0x0c
    int autoFree;                     // 0x10
} GHIPost;

typedef struct GHIPostData {
    int type;                         // 0x00: 0 string, 1 file on disk, 2 file in memory
    char *name;                       // 0x04
    union {
        struct { char *string; int len; int invalidChars; int extendedChars; } string;          // 0x08
        struct { char *filename; char *reportFilename; char *contentType; } fileDisk;            // 0x08
        struct { const char *buffer; int len; char *reportFilename; char *contentType; } fileMemory; // 0x08
    } data;
} GHIPostData;                        // size 0x18

typedef struct GHIPostState {
    GHIPostData *data;                // 0x0
    int pos;                          // 0x4, -1 before the part header went out
    FILE *file;                       // 0x8
    int len;                          // 0xc
} GHIPostState;                       // size 0x10

struct GHIConnection {
    int inUse;                        // 0x000
    int request;                      // 0x004, the connection index
    int uniqueID;                     // 0x008
    int type;                         // 0x00c: 0 get, 1 save, 2 stream, 3 head
    int state;                        // 0x010: 0 host lookup .. 7 receiving file
    char *URL;                        // 0x014
    char *serverAddress;              // 0x018
    unsigned int serverIP;            // 0x01c
    unsigned short serverPort;        // 0x020
    char *requestPath;                // 0x024
    char *sendHeaders;                // 0x028
    FILE *saveFile;                   // 0x02c
    int blocking;                     // 0x030
    int result;                       // 0x034
    ghttpProgressCallback progressCallback;    // 0x038
    ghttpCompletedCallback completedCallback;  // 0x03c
    void *callbackParam;              // 0x040
    SOCKET socket;                    // 0x044
    int socketError;                  // 0x048
    GHIBuffer sendBuffer;             // 0x04c
    GHIBuffer recvBuffer;             // 0x06c
    GHIBuffer getFileBuffer;          // 0x08c
    int userBufferSupplied;           // 0x0ac
    int statusMajorVersion;           // 0x0b0
    int statusMinorVersion;           // 0x0b4
    int statusCode;                   // 0x0b8
    int statusStringIndex;            // 0x0bc
    int completed;                    // 0x0c0
    int fileBytesReceived;            // 0x0c4
    int totalSize;                    // 0x0c8
    char *redirectURL;                // 0x0cc
    int redirectCount;                // 0x0d0
    int chunked;                      // 0x0d4
    char chunkHeader[0xc];            // 0x0d8
    int chunkHeaderLen;               // 0x0e4
    int chunkBytesLeft;               // 0x0e8
    int chunkReadingState;            // 0x0ec: 0 header, 1 data, 2 crlf, 3 done
    int processing;                   // 0x0f0
    int connectionClosed;             // 0x0f4
    int throttle;                     // 0x0f8
    unsigned long lastThrottleRecv;   // 0x0fc
    GHIPost *post;                    // 0x100
    DArray postingStates;             // 0x104, of GHIPostState
    int objectsPosted;                // 0x108
    int bytesPosted;                  // 0x10c
    int totalBytes;                   // 0x110
    ghttpPostCallback postCallback;   // 0x114
    void *postCallbackParam;          // 0x118
};                                    // size 0x11c

// connection table / locks
int ghiFindFreeSlot(void);
int ghiFreeConnection(GHIConnection *connection);
GHIConnection *ghiRequestToConnection(int request);
void ghiEnumConnections(int (*callback)(GHIConnection *connection));
void ghiRedirectConnection(GHIConnection *connection);
void ghiCleanupConnections(void);
GHIConnection *ghiNewConnection(void);
void ghiCreateLock(void);
void ghiFreeLock(void);
void ghiLock(void);
void ghiUnlock(void);

// callbacks / common
void ghiCallCompletedCallback(GHIConnection *connection);
void ghiCallProgressCallback(GHIConnection *connection, const char *buffer, int bufferLen);
void ghiCallPostCallback(GHIConnection *connection);
int ghiParseURL(GHIConnection *connection);
int ghiSocketSelect(SOCKET socket, int *readFlag, int *writeFlag, int *exceptFlag);
int ghiDoReceive(GHIConnection *connection, char *buffer, int *bufferLen);
int ghiDoSend(GHIConnection *connection, const char *buffer, int len);
int ghiTrySendThenBuffer(GHIConnection *connection, const char *buffer, int len);

// process
void ghiHandleStatus(GHIConnection *connection);
int ghiProcessConnection(GHIConnection *connection);
void ghiDoHostLookup(GHIConnection *connection);
void ghiDoConnecting(GHIConnection *connection);
void ghiDoSendingRequest(GHIConnection *connection);
void ghiDoPosting(GHIConnection *connection);
void ghiDoWaiting(GHIConnection *connection);
int ghiParseStatus(GHIConnection *connection);
void ghiDoReceivingStatus(GHIConnection *connection);
int ghiProcessIncomingFileData(GHIConnection *connection, char *buffer, int len);
void ghiChunkHeaderAppend(GHIConnection *connection, const char *buffer, int len);
int ghiProcessIncomingChunkedData(GHIConnection *connection, char *buffer, int len);
void ghiDoReceivingHeaders(GHIConnection *connection);
void ghiDoReceivingFile(GHIConnection *connection);

// post
int ghiIsPostAutoFree(GHIPost *post);
void ghttpFreePost(GHIPost *post);
const char *ghiPostGetContentType(GHIConnection *connection);
int ghiPostGetNoFilesContentLength(GHIConnection *connection);
int ghiPostGetHasFilesContentLength(GHIConnection *connection);
int ghiPostStateInit(GHIPostState *state);
int ghiPostInitState(GHIConnection *connection);
void ghiPostCleanupState(GHIConnection *connection);
int ghiPostStringStateDoPosting(GHIPostState *state, GHIConnection *connection);
int ghiPostFileDiskStateDoPosting(GHIPostState *state, GHIConnection *connection);
int ghiPostFileMemoryStateDoPosting(GHIPostState *state, GHIConnection *connection);
int ghiPostStateDoPosting(GHIPostState *state, GHIConnection *connection, int first);
int ghiPostDoPosting(GHIConnection *connection);

// buffers
int ghiResizeBuffer(GHIBuffer *buffer, int sizeIncrement);
int ghiInitBuffer(GHIConnection *connection, GHIBuffer *buffer, int initialSize, int sizeIncrement);
int ghiInitFixedBuffer(GHIConnection *connection, GHIBuffer *buffer, char *userBuffer, int size);
void ghiFreeBuffer(GHIBuffer *buffer);
int ghiAppendDataToBuffer(GHIBuffer *buffer, const char *data, int dataLen);
int ghiAppendHeaderToBuffer(GHIBuffer *buffer, const char *name, const char *value);
int ghiAppendCharToBuffer(GHIBuffer *buffer, int c);
int ghiAppendIntToBuffer(GHIBuffer *buffer, int i);
void ghiResetBuffer(GHIBuffer *buffer);
int ghiSendBufferedData(GHIBuffer *buffer, GHIConnection *connection);

// ghttpMain.c
void ghttpStartup(void);
void ghttpCleanup(void);
int ghttpGetEx(const char *URL, const char *headers, char *buffer, int bufferSize, GHIPost *post, int throttle,
    int blocking, ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void *param);

// nonport (written earlier)
char *goastrdup(const char *src);

#endif
