// sb.h -- the GameSpy ServerBrowser (serverbrowsing: sb_serverbrowsing.c, sb_queryengine.c, sb_serverlist.c,
// sb_crypt.c) as linked into halo.exe, 0x616ce0..0x617331, 0x61e340..0x62051f, 0x622ea0..0x623141. Layouts are
// recovered from the code (offsets in comments). Register-convention helpers are plain C here; their file headers
// give the binary's convention. Globals are declared in the .c files that use them (address comments bind them).
#ifndef HALO_SB_H
#define HALO_SB_H
#include "gamespy.h"

typedef struct SBServer {
    unsigned int publicip;            // 0x00
    unsigned short publicport;        // 0x04, network order
    unsigned int privateip;           // 0x08
    unsigned short privateport;       // 0x0c
    unsigned int icmpip;              // 0x10
    unsigned char state;              // 0x14: 1 basic keys, 2 full keys, 4 basic query pending, 8 full pending
    unsigned char flags;              // 0x15: 1 unsolicited (LAN), 2 private address, 0x10 failed
    HashTable keyvals;                // 0x18
    unsigned long updatetime;         // 0x1c, send time, then the ping
    struct SBServer *next;            // 0x20
} SBServer;                           // size 0x24

typedef struct SBServerFIFO {
    SBServer *first;                  // 0x0
    SBServer *last;                   // 0x4
    int count;                        // 0x8
} SBServerFIFO;

typedef struct SBQueryEngine SBQueryEngine;
typedef void (*SBEngineCallbackFn)(SBQueryEngine *engine, int reason, SBServer *server, void *instance);

struct SBQueryEngine {
    int queryversion;                 // 0x00
    int maxupdates;                   // 0x04
    SBServerFIFO querylist;           // 0x08
    SBServerFIFO pendinglist;         // 0x14
    SOCKET querysock;                 // 0x20
    unsigned int mypublicip;          // 0x24
    unsigned char serverkeys[0x14];   // 0x28
    int numserverkeys;                // 0x3c
    SBEngineCallbackFn ListCallback;  // 0x40
    void *instance;                   // 0x44
};                                    // size 0x48

typedef struct GOACryptState {
    unsigned char cards[0x100];       // 0x000
    unsigned char rotor;              // 0x100
    unsigned char ratchet;            // 0x101
    unsigned char avalanche;          // 0x102
    unsigned char last_plain;         // 0x103
    unsigned char last_cipher;        // 0x104
    unsigned char pad[3];
} GOACryptState;                      // 0x108 in the list

typedef struct SBKeyInfo {
    const char *name;                 // 0x0, a ref string
    int type;                         // 0x4: 0 string, 1 byte, 2 short
} SBKeyInfo;

typedef struct SBRefString {
    const char *str;                  // 0x0
    int refcount;                     // 0x4
} SBRefString;

typedef struct SBServerList SBServerList;
typedef void (*SBListCallbackFn)(SBServerList *slist, int reason, SBServer *server, void *instance);

struct SBServerList {
    int state;                        // 0x000: 0 LAN browse, 1 disconnected, 2 connected, 3 main list
    DArray servers;                   // 0x004, of SBServer *
    DArray keylist;                   // 0x008, of SBKeyInfo
    char queryforgamename[0x20];      // 0x00c
    char queryfromgamename[0x20];     // 0x02c
    char queryfromkey[0x20];          // 0x04c
    unsigned char mychallenge[8];     // 0x06c
    unsigned char *inbuffer;          // 0x074, 0x1000 bytes
    int inbufferlen;                  // 0x078
    const char *popularvalues[0xff];  // 0x07c
    int numpopularvalues;             // 0x478
    int expectedelements;             // 0x47c
    SBListCallbackFn ListCallback;    // 0x480
    void *instance;                   // 0x484
    const char *sortkey;              // 0x488
    int sortascending;                // 0x48c
    unsigned int mypublicip;          // 0x490
    unsigned int srcip;               // 0x494
    unsigned short defaultport;       // 0x498
    const char *lasterror;            // 0x49c
    SOCKET slsocket;                  // 0x4a0
    unsigned long lanstarttime;       // 0x4a4
    int fromgamever;                  // 0x4a8
    GOACryptState cryptkey;           // 0x4ac
    int queryoptions;                 // 0x5b4
    int pstate;                       // 0x5b8
    SBServer *deadlist;               // 0x5bc
};                                    // size 0x5c0

typedef struct ServerBrowser ServerBrowser;
typedef void (*ServerBrowserCallback)(ServerBrowser *sb, int reason, SBServer *server, void *instance);

struct ServerBrowser {
    SBQueryEngine engine;             // 0x000
    SBServerList list;                // 0x048
    int disconnectFlag;               // 0x608
    int dontUpdate;                   // 0x60c
    unsigned int triggerIP;           // 0x610
    unsigned short triggerPort;       // 0x614
    ServerBrowserCallback BrowserCallback; // 0x618
    void *instance;                   // 0x61c
};                                    // size 0x620

// sb_server.c (written earlier)
SBServer *SBAllocServer(void *slist, unsigned int public_ip, unsigned short public_port);
int SBIsNullServer(void *server);
void SBServerSetFlags(void *server, unsigned char flags);
void SBServerSetPrivateAddr(void *server, unsigned int ip, unsigned short port);
void SBServerSetICMPIP(void *server, unsigned int ip);
void SBServerSetState(void *server, unsigned char state);
unsigned char SBServerGetState(void *server);
void SBServerAddKeyValue(void *server, const char *key, const char *value);
void SBServerAddIntKeyValue(void *server, const char *key, int value);
void SBServerParseKeyVals(void *server, char *keyvals);
void SBServerParseQR2FullKeysSingle(void *server, char *data, int len);
void SBServerListInit(void *slist, const char *query_for_gamename, const char *query_from_gamename,
    const char *query_from_key, int query_from_version, SBListCallBackFn callback, void *instance);
void SBServerListAppendServer(void *slist, void *server);
int SBServerListFindServer(void *slist, unsigned int ip, unsigned short port);
void SBServerListRemoveAt(void *slist, int index);
void SBServerListClear(void *slist);
void SBFreeDeadList(void *slist);
void SBRefStrHashCleanup(void);
void gt2SetReceiveDump(void *object, void *value); // 0x61e550: *(object + 0x24) = value; ICF-shared, here
                                                   // SBEngineSetPublicIP(engine, ip)

// sb_crypt.c
void GOAHashInit(GOACryptState *state);
void GOACryptInit(GOACryptState *state, const unsigned char *key, unsigned char keysize);
unsigned char GOADecryptByte(GOACryptState *state, unsigned char b);
void GOADecrypt(GOACryptState *state, unsigned char *data, int len);

// sb_queryengine.c
int FIFORemove(SBServer *server, SBServerFIFO *fifo);
void QEStartQuery(SBQueryEngine *engine, SBServer *server);
void SBQueryEngineInit(SBQueryEngine *engine, int maxupdates, int queryversion, SBEngineCallbackFn callback,
    void *instance);
void SBEngineHaltUpdates(SBQueryEngine *engine);
void SBEngineCleanup(SBQueryEngine *engine);
void SBQueryEngineUpdateServer(SBQueryEngine *engine, SBServer *server, int addfront, int querytype);
void ParseSingleQR2Reply(SBQueryEngine *engine, SBServer *server, char *data, int len);
void ParseSingleGOAReply(SBQueryEngine *engine, SBServer *server, char *data);
void ProcessIncomingReplies(SBQueryEngine *engine);
void TimeoutOldQueries(SBQueryEngine *engine);
void QueueNextQueries(SBQueryEngine *engine);
void SBQueryEngineThink(SBQueryEngine *engine);
void SBQueryEngineAddQueryKey(SBQueryEngine *engine, unsigned char keyid);
int SBQueryEngineRemoveServerFromFIFOs(SBQueryEngine *engine, SBServer *server);

// sb_serverlist.c
int ServerListConnect(SBServerList *slist);
void BufferAddNTS(char **buffer, int *len, const char *str);
int SetupListChallenge(SBServerList *slist);
void InitCryptKey(SBServerList *slist, const unsigned char *key, int keylen);
int FullRulesPresent(const char *data, int len);
int AllKeysPresent(SBServerList *slist, const unsigned char *data, int len);
void ParseServerIPPort(SBServerList *slist, const unsigned char *data, int len, unsigned int *ip,
    unsigned short *port);
int ParseServer(SBServerList *slist, SBServer *server, unsigned char *data, int len, int usepopularlist);
void FreePopularValues(SBServerList *slist);
void FreeKeyList(SBServerList *slist);
void SBServerListDisconnect(SBServerList *slist);
void SBServerListCleanup(SBServerList *slist);
int ProcessServerRecord(SBServerList *slist, unsigned char *data, int len);
int ProcessMainListData(SBServerList *slist);
int ProcessPushKeyList(SBServerList *slist, unsigned char *data, int len);
int ProcessPushServer(SBServerList *slist, unsigned char *data, int len);
int ProcessLanData(SBServerList *slist);
int SBServerListConnectAndQuery(SBServerList *slist, const char *fieldList, const char *serverFilter,
    int updateOptions, int maxServers);
int SBServerListGetLANList(SBServerList *slist, unsigned short startSearchPort, unsigned short endSearchPort,
    int queryversion);
int ProcessAdHocData(SBServerList *slist);
int SBListThinkConnected(SBServerList *slist);
int SBListThink(SBServerList *slist);
int SendWithRetry(SBServerList *slist, const char *data, int len);
int SBRequestServerUpdate(SBServerList *slist, unsigned int ip, unsigned short port);
int SBSendMessageToServer(SBServerList *slist, unsigned int ip, unsigned short port, const char *data, int len);
int SBSendNatNegotiateCookieToServer(SBServerList *slist, unsigned int ip, unsigned short port, int cookie);

// sb_serverbrowsing.c
ServerBrowser *ServerBrowserNew(const char *queryForGamename, const char *queryFromGamename,
    const char *queryFromKey, int queryFromVersion, int maxConcurrentUpdates, int queryVersion,
    ServerBrowserCallback callback, void *instance);
void ServerBrowserFree(ServerBrowser *sb);
int ServerBrowserThink(ServerBrowser *sb);
int WaitForTriggerUpdate(ServerBrowser *sb, int viaMaster);

#endif
