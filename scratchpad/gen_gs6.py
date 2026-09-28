exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
import textwrap

F = '#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))\n'

emit(0x616640, 60, 'qr2_buffer_add_int', 'the value printed with "%d" (0x0065fb30), then qr2_buffer_add.', '''
extern void qr2_buffer_add(char *buffer, const char *value);

void qr2_buffer_add_int(char *buffer, int value)
{
    char text[0x14];

    sprintf(text, "%d", value);
    qr2_buffer_add(buffer, text);
}
''')
emit(0x61c660, 12, 'gt2Listen', 'stores the connect-attempt callback at socket +0x1c.', F + '''
void gt2Listen(void *socket, void *callback)
{
    FIELD(socket, 0x1c, void *) = callback;
}
''')
emit(0x61e550, 12, 'gt2SetReceiveDump', 'stores the receive dump callback at socket +0x24.', F + '''
void gt2SetReceiveDump(void *socket, void *callback)
{
    FIELD(socket, 0x24, void *) = callback;
}
''')
emit(0x61aa50, 91, 'gcd_getkeyhash', 'the CD key hash (+4) of the client with this local id in the game with this id (the 0x10 byte game table at 0x00723200, count 0x006a2e64, each with a client list at +8 of {client, next} nodes), or "".', '''
typedef struct gcd_client_node {
    int *client;                  // 0x00 -> { local id, hash[] }
    struct gcd_client_node *next; // 0x04
} gcd_client_node;

typedef struct gcd_game {
    int game_id;                  // 0x00
    int unknown_04;               // 0x04
    gcd_client_node *clients;     // 0x08
    int unknown_0c;               // 0x0c
} gcd_game;

extern gcd_game gcd_games[];      // 0x00723200
extern int gcd_game_count;        // 0x006a2e64

const char *gcd_getkeyhash(int game_id, int local_id)
{
    int i;
    gcd_client_node *node;

    for (i = 0; i < gcd_game_count; i++) {
        if (gcd_games[i].game_id == game_id) {
            break;
        }
    }
    if (i >= gcd_game_count) {
        return "";
    }
    for (node = gcd_games[i].clients; node != 0; node = node->next) {
        if (node->client[0] == local_id) {
            return (const char *)(node->client + 1);
        }
    }
    return "";
}
''')
emit(0x617c70, 312, 'gcd_compute_response', 'the CD key challenge response: "CD Key or challenge too long" when 2*len(key) + len(challenge) + 8 >= 0x200; else a random 32-bit value (srand(time(0) ^ 0x33333333), two rand()s), the key\'s MD5 hex at response+0, the value as "%.8x" at +0x20 and the MD5 hex of key . (value % 0xffff) . challenge ("%s%d%s") at +0x28.', '''
extern void md5_hex_digest(const unsigned char *data, int length, char *out); // 0x61a730
#include <time.h>

void gcd_compute_response(const char *cdkey, const char *challenge, char *response)
{
    char random_text[0x10];
    char text[0x200];
    unsigned int value;

    if (strlen(cdkey) * 2 + strlen(challenge) + 8 >= 0x200) {
        strcpy(response, "CD Key or challenge too long");
        return;
    }
    srand((unsigned int)time(0) ^ 0x33333333);
    value = (unsigned int)rand() << 16;
    value |= (unsigned int)rand();
    sprintf(random_text, "%.8x", value);
    sprintf(text, "%s%d%s", cdkey, value % 0xffff, challenge);
    md5_hex_digest((const unsigned char *)cdkey, (int)strlen(cdkey), response);
    strcpy(response + 0x20, random_text);
    md5_hex_digest((const unsigned char *)text, (int)strlen(text), response + 0x28);
}
''')
emit(0x616ff0, 44, 'ServerBrowserState', 'querying (2) while the engine has queries outstanding (+0x10); otherwise from the list state (+0x48): 3 (connected) for list state 1, 0 for 0 or 3, 1 otherwise.', F + '''
int ServerBrowserState(void *sb)
{
    int state;

    if (FIELD(sb, 0x10, int) > 0) {
        return 2;
    }
    state = FIELD(sb, 0x48, int);
    if (state == 3 || state == 0) {
        return 1;
    }
    return state == 1 ? 0 : 3;
}
''')
emit(0x617020, 16, 'ServerBrowserGetServer', 'SBServerListNth of the server list (+0x48).', '''
extern void *SBServerListNth(void *slist, int i);

void *ServerBrowserGetServer(void *sb, int index)
{
    return SBServerListNth((char *)sb + 0x48, index);
}
''')
emit(0x617030, 16, 'ServerBrowserCount', 'SBServerListCount of the server list (+0x48).', '''
extern int SBServerListCount(void *slist);

int ServerBrowserCount(void *sb)
{
    return SBServerListCount((char *)sb + 0x48);
}
''')
emit(0x617040, 17, 'ServerBrowserGetMyPublicIP', 'inet_ntoa of the public ip the master reported (+0x4d8).', F + '''
char *ServerBrowserGetMyPublicIP(void *sb)
{
    struct in_addr address;

    address.s_addr = FIELD(sb, 0x4d8, unsigned int);
    return inet_ntoa(address);
}
''')
emit(0x617060, 11, 'ServerBrowserGetMyPublicIPAddr', 'the public ip the master reported (+0x4d8).', F + '''
unsigned int ServerBrowserGetMyPublicIPAddr(void *sb)
{
    return FIELD(sb, 0x4d8, unsigned int);
}
''')
emit(0x622050, 149, 'ghttpSetProxy', 'drops the old proxy (ghiProxyAddress 0x007231e0, port 0x007231dc); a non-empty "host[:port]" is copied (goastrdup) with the port split off (atoi; a zero port rejects the proxy) or port 80 by default. Returns 0 when the copy fails or the port is 0.', '''
extern char *ghiProxyAddress;          // 0x007231e0
extern unsigned short ghiProxyPort;    // 0x007231dc

int ghttpSetProxy(const char *server)
{
    char *colon;

    if (ghiProxyAddress != 0) {
        free(ghiProxyAddress);
        ghiProxyAddress = 0;
    }
    ghiProxyPort = 0;
    if (server == 0 || server[0] == 0) {
        return 1;
    }
    ghiProxyAddress = goastrdup(server);
    if (ghiProxyAddress == 0) {
        return 0;
    }
    colon = strchr(ghiProxyAddress, ':');
    if (colon == 0) {
        ghiProxyPort = 80;
        return 1;
    }
    *colon = 0;
    ghiProxyPort = (unsigned short)atoi(colon + 1);
    if (ghiProxyPort != 0) {
        return 1;
    }
    free(ghiProxyAddress);
    ghiProxyAddress = 0;
    return 0;
}
''')


def emit_net(addr, size, name, note, cc, body):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    src = ('// %s  (not a Ghidra function; no C existed)\n// address 0x%x, size %d bytes\n'
           '// name confidence: 0.6   rewrite confidence: 0.85\n%s// blam-cc: %s\n\n#include "tags.h"\n\n%s'
           % (name, addr, size, wr, cc, body.lstrip('\n')))
    open('src/networking/%s.c' % name, 'w', encoding='utf-8').write(src)


emit_net(0x441f30, 35, 'network_channel_gap_441f30', 'the GT2 connection callback set by network_listen_accept_pending_connection (config.error_callback): the connection\'s receive queue (gt2GetConnectionData) gets byte +5 = 1, network_receive_queue_close_socket (ESI queue) and flag 0x40 in +0x0c. (The name keeps the one its registrant uses.)', 'cdecl (a GT2 connection callback)', '''
typedef struct network_receive_queue network_receive_queue;
extern void *FUN_00614840(void *connection); // 0x614840 gt2GetConnectionData
extern void network_receive_queue_close_socket(network_receive_queue *queue); // 0x442040, blam-cc: ESI queue

void network_channel_gap_441f30(void *connection)
{
    uint8_t *queue = (uint8_t *)FUN_00614840(connection);

    if (queue != 0) {
        queue[0x05] = 1;
        network_receive_queue_close_socket((network_receive_queue *)queue);
        queue[0x0c] |= 0x40;
    }
}
''')
emit_net(0x5777d0, 118, 'autopatch_version_check_completed', 'the ptCheckForPatch callback (available, mandatory, version name, file id, download url, param): with a patch and a url, keeps the url (0xff chars, 0x007228d8) and the version name (0x3f chars, 0x007229d8), the file id (0x007228d4) and state 3; otherwise clears them and sets state 2.', 'cdecl (the autopatch check callback)', '''
extern char autopatch_update_url[0x100];      // 0x007228d8
extern char autopatch_update_version[0x100];  // 0x007229d8
extern int32_t autopatch_update_file_id;      // 0x007228d4
extern int32_t autopatch_update_check_state;  // 0x0069fe04
extern char *strncpy(char *dest, const char *source, unsigned int count);

void autopatch_version_check_completed(int32_t available, int32_t mandatory, const char *version_name, int32_t file_id,
    const char *download_url, void *param)
{
    (void)mandatory;
    (void)param;
    if (available == 0 || download_url[0] == 0) {
        autopatch_update_file_id = 0;
        autopatch_update_url[0] = 0;
        autopatch_update_version[0] = 0;
        autopatch_update_check_state = 2;
        return;
    }
    strncpy(autopatch_update_url, download_url, 0xff);
    autopatch_update_url[0xff] = 0;
    autopatch_update_check_state = 3;
    autopatch_update_file_id = file_id;
    strncpy(autopatch_update_version, version_name, 0x3f);
    autopatch_update_version[0x3f] = 0;
}
''')
emit_net(0x4ba660, 225, 'network_channel_gap_4ba660', 'the ServerBrowser list callback (sb, reason, server, instance) registered by server_browser_open, active once the browser is initialized: server added (0) with basic or full keys, and server updated (1), add the server to the locked list (0x4ba760 / 0x4ba8a0); deleted (2, 3) removes it (0x4ba870 / 0x4ba940), dropping the selection and refreshing the UI when it was selected; query complete (4) resets the elapsed time to 9999 when a query was pending (0x00719488). (Named as its registrant names it.)', 'cdecl (a serverbrowsing callback)', '''
typedef struct server_list_globals server_list_globals;
extern uint8_t server_browser_initialized;       // 0x00719470
extern uint8_t server_browser_query_pending;     // 0x00719488, UNSURE name
extern int32_t server_browser_query_elapsed_ms;  // 0x007196c8
extern int32_t server_browser_selected_index;    // 0x006953f4
extern int32_t server_browser_selection_valid;   // 0x0071947c, UNSURE name
extern int32_t FUN_006175c0(void *server); // 0x6175c0 SBServerHasBasicKeys
extern int32_t FUN_006175d0(void *server); // 0x6175d0 SBServerHasFullKeys
extern server_list_globals *server_list_mutex_try_lock(uint32_t timeout_ms); // 0x4ba760
extern int32_t dynamic_pointer_array_add_unique(void *value, server_list_globals *array); // 0x4ba8a0
extern void server_list_mutex_unlock(server_list_globals **list_slot); // 0x4ba7a0
extern int32_t dynamic_pointer_array_find_index(server_list_globals *array, void *value); // 0x4ba870
extern void dynamic_pointer_array_remove_at(int32_t index, server_list_globals *array); // 0x4ba940
extern void server_browser_ui_refresh(void); // 0x4b73a0

void network_channel_gap_4ba660(void *sb, uint32_t reason, void *server, void *instance)
{
    server_list_globals *list;

    (void)sb;
    (void)instance;
    if (server_browser_initialized == 0 || reason > 4) {
        return;
    }
    switch (reason) {
    case 0:
        if (FUN_006175c0(server) == 0 && FUN_006175d0(server) == 0) {
            return;
        }
        // fall through
    case 1:
        list = server_list_mutex_try_lock(100);
        if (list != 0) {
            dynamic_pointer_array_add_unique(server, list);
            server_list_mutex_unlock(&list);
        }
        return;
    case 2:
    case 3:
        list = server_list_mutex_try_lock(100);
        if (list != 0) {
            int32_t index = dynamic_pointer_array_find_index(list, server);

            if (index != -1) {
                dynamic_pointer_array_remove_at(index, list);
                if (index == server_browser_selected_index) {
                    server_browser_selected_index = -1;
                    server_browser_selection_valid = 0;
                    server_browser_ui_refresh();
                }
            }
            server_list_mutex_unlock(&list);
        }
        return;
    default:
        if (server_browser_query_pending != 0) {
            server_browser_query_elapsed_ms = 9999;
        }
        server_browser_query_pending = 0;
        return;
    }
}
''')
print('ok')
