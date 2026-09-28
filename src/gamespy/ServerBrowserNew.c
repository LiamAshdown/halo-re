// ServerBrowserNew  (GameSpy SDK in halo.exe; no C existed)
// address 0x616eb0, size 116 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616eb0..0x616f23: a 0x620-byte browser: the callback and instance, updates
//   allowed, the server list (with the list callback 0x616ce0) and the query engine (with the engine callback
//   0x616e40), both static here. The list callback: added -- reported, then queried unless it already has keys or
//   updates are held (a basic query when connected with engine keys, else full); updated -- reported as updated (1)
//   with keys, else failed (2); deleted -- dropped from the engine queues, reported (3); initial list complete --
//   disconnects when asked; query error -- reported (5); public ip -- given to the engine. The engine callback
//   reports success (1), failure (2) and idle (4). Either clears a pending trigger for that server.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

static void ListCallback(SBServerList *slist, int reason, SBServer *server, void *instance) // 0x616ce0
{
    ServerBrowser *sb = (ServerBrowser *)instance;

    switch (reason) {
    case 0:
        sb->BrowserCallback(sb, 0, server, sb->instance);
        if ((server->state & 3) == 0 && sb->dontUpdate == 0) {
            if (sb->list.state != 0 && sb->engine.numserverkeys != 0) {
                SBQueryEngineUpdateServer(&sb->engine, server, 0, 0);
            } else {
                SBQueryEngineUpdateServer(&sb->engine, server, 0, 1);
            }
        }
        break;
    case 1:
        if ((server->state & 3) == 0) {
            sb->BrowserCallback(sb, 2, server, sb->instance);
        } else {
            sb->BrowserCallback(sb, 1, server, sb->instance);
        }
        break;
    case 2:
        if (server->state & 0xc) {
            SBQueryEngineRemoveServerFromFIFOs(&sb->engine, server);
        }
        sb->BrowserCallback(sb, 3, server, sb->instance);
        break;
    case 3:
        if (sb->disconnectFlag != 0) {
            SBServerListDisconnect(slist);
        }
        break;
    case 5:
        sb->BrowserCallback(sb, 5, 0, sb->instance);
        break;
    case 6:
        gt2SetReceiveDump(&sb->engine, (void *)sb->list.mypublicip);
        break;
    }
    if (server != 0 && server->publicip == sb->triggerIP && server->publicport == sb->triggerPort) {
        sb->triggerIP = 0;
    }
}

static void EngineCallback(SBQueryEngine *engine, int reason, SBServer *server, void *instance) // 0x616e40
{
    ServerBrowser *sb = (ServerBrowser *)instance;

    (void)engine;
    if (reason == 0) {
        sb->BrowserCallback(sb, 1, server, sb->instance);
    } else if (reason == 1) {
        sb->BrowserCallback(sb, 2, server, sb->instance);
    } else if (reason == 2) {
        sb->BrowserCallback(sb, 4, server, sb->instance);
    }
    if (server != 0 && server->publicip == sb->triggerIP && server->publicport == sb->triggerPort) {
        sb->triggerIP = 0;
    }
}

ServerBrowser *ServerBrowserNew(const char *queryForGamename, const char *queryFromGamename,
    const char *queryFromKey, int queryFromVersion, int maxConcurrentUpdates, int queryVersion,
    ServerBrowserCallback callback, void *instance)
{
    ServerBrowser *sb = (ServerBrowser *)malloc(sizeof(ServerBrowser));

    if (sb == 0) {
        return 0;
    }
    sb->BrowserCallback = callback;
    sb->instance = instance;
    sb->dontUpdate = 0;
    SBServerListInit(&sb->list, queryForGamename, queryFromGamename, queryFromKey, queryFromVersion,
        (SBListCallBackFn)ListCallback, sb);
    SBQueryEngineInit(&sb->engine, maxConcurrentUpdates, queryVersion, EngineCallback, sb);
    return sb;
}
