exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
H = '#include "sb.h"\n\n'


def e(addr, size, name, note, code, cc='cdecl'):
    emit(addr, size, name, note, H + code.lstrip('\n'), cc=cc)


TRIG = '''    if (server != 0 && server->publicip == sb->triggerIP && server->publicport == sb->triggerPort) {
        sb->triggerIP = 0;
    }
'''
e(0x616eb0, 116, 'ServerBrowserNew', 'a 0x620-byte browser: the callback and instance, updates allowed, the server list (with the list callback 0x616ce0) and the query engine (with the engine callback 0x616e40), both static here. The list callback: added -- reported, then queried unless it already has keys or updates are held (a basic query when connected with engine keys, else full); updated -- reported as updated (1) with keys, else failed (2); deleted -- dropped from the engine queues, reported (3); initial list complete -- disconnects when asked; query error -- reported (5); public ip -- given to the engine. The engine callback reports success (1), failure (2) and idle (4). Either clears a pending trigger for that server.', '''
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
''' + TRIG + '''}

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
''' + TRIG + '''}

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
''')
e(0x616f30, 31, 'ServerBrowserFree', 'list cleanup, engine cleanup, free.', '''
void ServerBrowserFree(ServerBrowser *sb)
{
    SBServerListCleanup(&sb->list);
    SBEngineCleanup(&sb->engine);
    free(sb);
}
''')
e(0x616f50, 44, 'ServerBrowserSendNatNegotiateCookieToServer', '(browser, dotted ip, port, cookie) -> SBSendNatNegotiateCookieToServer with inet_addr / htons.', '''
int ServerBrowserSendNatNegotiateCookieToServer(ServerBrowser *sb, const char *ip, unsigned short port, int cookie)
{
    unsigned short network_port = htons(port);

    return SBSendNatNegotiateCookieToServer(&sb->list, inet_addr(ip), network_port, cookie);
}
''')
e(0x616f80, 25, 'ServerBrowserThink', 'engine think, then list think (its result).', '''
int ServerBrowserThink(ServerBrowser *sb)
{
    SBQueryEngineThink(&sb->engine);
    return SBListThink(&sb->list);
}
''')
e(0x616fa0, 25, 'ServerBrowserHalt', 'disconnects the list and halts the engine.', '''
void ServerBrowserHalt(ServerBrowser *sb)
{
    SBServerListDisconnect(&sb->list);
    SBEngineHaltUpdates(&sb->engine);
}
''')
e(0x616fc0, 33, 'ServerBrowserClear', 'disconnects, halts and clears the server list.', '''
void ServerBrowserClear(ServerBrowser *sb)
{
    SBServerListDisconnect(&sb->list);
    SBEngineHaltUpdates(&sb->engine);
    SBServerListClear(&sb->list);
}
''')
e(0x617070, 302, 'ServerBrowserBeginUpdate2', '(browser, async, disconnect on complete, basic fields, count, filter, options, max servers): remembers the disconnect flag, rebuilds the engine key list and a "\\\\name" field string (up to 255 chars) from the qr2 key names, and asks the master. Synchronous: thinks every 10 ms while the main list comes in or queries are out and nothing failed. The last result.', '''
extern char *qr2_registered_key_list[0x100]; // 0x00683990

int ServerBrowserBeginUpdate2(ServerBrowser *sb, int async, int disconnectOnComplete, const unsigned char *basicFields,
    int numBasicFields, const char *serverFilter, int updateOptions, int maxServers)
{
    char keylist[0x100];
    int len = 0;
    int i;
    int error;

    memset(keylist, 0, sizeof(keylist));
    sb->disconnectFlag = disconnectOnComplete;
    sb->engine.numserverkeys = 0;
    for (i = 0; i < numBasicFields; i++) {
        const char *name = qr2_registered_key_list[basicFields[i]];

        if ((int)strlen(name) + len + 1 >= 0x100) {
            break;
        }
        len += sprintf(keylist + len, "\\\\%s", name);
        SBQueryEngineAddQueryKey(&sb->engine, basicFields[i]);
    }
    error = SBServerListConnectAndQuery(&sb->list, keylist, serverFilter, updateOptions, maxServers);
    if (error == 0 && async == 0) {
        while (sb->list.state == 3 || (sb->engine.querylist.count > 0 && error == 0)) {
            msleep(10);
            SBQueryEngineThink(&sb->engine);
            error = SBListThink(&sb->list);
        }
    }
    return error;
}
''')
e(0x6171a0, 43, 'ServerBrowserUpdate', 'ServerBrowserBeginUpdate2 with no options and no server limit.', '''
extern int ServerBrowserBeginUpdate2(ServerBrowser *sb, int async, int disconnectOnComplete,
    const unsigned char *basicFields, int numBasicFields, const char *serverFilter, int updateOptions, int maxServers);

int ServerBrowserUpdate(ServerBrowser *sb, int async, int disconnectOnComplete, const unsigned char *basicFields,
    int numBasicFields, const char *serverFilter)
{
    return ServerBrowserBeginUpdate2(sb, async, disconnectOnComplete, basicFields, numBasicFields, serverFilter, 0, 0);
}
''')
e(0x6171d0, 102, 'ServerBrowserLANUpdate', '(browser, async, start port, end port): halts everything and broadcasts for LAN servers with the engine  query version; synchronous: thinks every 10 ms while LAN browsing or queries are out and nothing failed.', '''
int ServerBrowserLANUpdate(ServerBrowser *sb, int async, unsigned short startSearchPort, unsigned short endSearchPort)
{
    int error = 0;

    SBServerListDisconnect(&sb->list);
    SBEngineHaltUpdates(&sb->engine);
    SBServerListGetLANList(&sb->list, startSearchPort, endSearchPort, sb->engine.queryversion);
    if (async == 0) {
        while (sb->list.state == 0 || (sb->engine.querylist.count > 0 && error == 0)) {
            msleep(10);
            SBQueryEngineThink(&sb->engine);
            error = SBListThink(&sb->list);
        }
    }
    return error;
}
''')
e(0x617240, 66, 'WaitForTriggerUpdate', 'EDI browser, EBX via master: while the trigger server is pending and nothing failed: sleep 10 ms, think; via the master it also stops once the list is back to LAN (0) state.', '''
int WaitForTriggerUpdate(ServerBrowser *sb, int viaMaster)
{
    int error = 0;

    while (sb->triggerIP != 0 && error == 0) {
        msleep(10);
        SBQueryEngineThink(&sb->engine);
        error = SBListThink(&sb->list);
        if (viaMaster != 0 && sb->list.state == 0) {
            break;
        }
    }
    return error;
}
''', cc='EDI -> sb, EBX -> viaMaster')
e(0x617290, 162, 'ServerBrowserAuxUpdateServer', '(browser, server, async, full update): updates held meanwhile. A LAN server (flag 1) is queried directly (out of the queues, to the front, full when asked); others are requested through the master. Synchronous and without error: waits for that server as the trigger. The result.', '''
int ServerBrowserAuxUpdateServer(ServerBrowser *sb, SBServer *server, int async, int fullUpdate)
{
    int error = 0;
    int viaMaster;

    sb->dontUpdate = 1;
    if (server->flags & 1) {
        SBQueryEngineRemoveServerFromFIFOs(&sb->engine, server);
        SBQueryEngineUpdateServer(&sb->engine, server, 1, fullUpdate != 0);
        viaMaster = 0;
    } else {
        error = SBRequestServerUpdate(&sb->list, server->publicip, server->publicport);
        viaMaster = 1;
    }
    if (async == 0 && error == 0) {
        sb->triggerIP = server->publicip;
        sb->triggerPort = server->publicport;
        error = WaitForTriggerUpdate(sb, viaMaster);
    }
    sb->dontUpdate = 0;
    return error;
}
''')

# the null server is a pointer variable: compare with its value
p = 'src/gamespy/SBIsNullServer.c'
s = open(p, encoding='utf-8').read()
s = s.replace('extern unsigned char SBNullServer[]; // 0x006a27f8', 'extern void *SBNullServer; // 0x006a27f8')
s = s.replace('    return server == (void *)SBNullServer;', '    return server == SBNullServer;')
s = s.replace('whether the server is the shared null server (0x006a27f8).', 'whether the server is the shared null server (the pointer held at 0x006a27f8). FIXED 2026-09-28: compared with the value there, not its address.')
open(p, 'w', encoding='utf-8').write(s)
print('ok')
