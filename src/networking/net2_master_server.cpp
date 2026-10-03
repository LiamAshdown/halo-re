/**
 * @file src/networking/net2_master_server.cpp
 * Master server connection, list refresh and GameSpy glue.
 */
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "win32.h"
#include "game.h"
#include "networking.h"
#include "halo/networking/net2_master_server.hpp"

extern "C" {
extern void * negotiatorList;
extern void NegotiateThink(void *element);
extern network_mutex_record * server_list_mutex;
extern network_thread_record * server_list_thread;
extern uint32_t master_server_request_flags;
extern int32_t master_server_last_result;
extern int32_t mutex_create(network_mutex_record **out_handle);
extern int32_t network_thread_create(uint8_t flags, void *start_address, void *parameter,
                                       network_thread_record **out_handle);
extern int32_t sound_time;
extern int64_t performance_frequency;
extern void sound_idle_update(void);
extern void * master_server_query_engine;
extern uint8_t DAT_00719488;
extern int32_t server_browser_query_elapsed_ms;
extern int32_t ServerBrowserState(void *engine);
extern int32_t ServerBrowserCount(void *engine);
extern int32_t DAT_006953fc;
extern server_list_globals server_list;
extern uint8_t server_browser_require_valid_entry;
extern uint8_t DAT_00695424[10];
extern uint32_t network_session_start_game_type;
extern int32_t server_browser_selected_index;
extern server_list_globals * server_list_mutex_try_lock(uint32_t timeout_ms);
extern void server_list_reset(void);
extern void ServerBrowserHalt(void *engine);
extern int32_t ServerBrowserThink(void *engine);
extern void ServerBrowserClear(void *engine);
extern int32_t ServerBrowserLANUpdate(void *engine, int32_t flag, uint32_t address, uint16_t port);
extern int32_t ServerBrowserUpdate(void *engine, int32_t flag, int32_t unused_a, void *buffer,
                              int32_t buffer_length, int32_t unused_b);
extern int32_t ServerBrowserAuxUpdateServer(void *engine, void *server_record, int32_t flag_a, int32_t flag_b);
extern const char * qr2_registered_key_list[255];
extern uint8_t server_browser_join_requested;
extern char network_session_start_host_name[];
extern char network_session_start_map_name[];
extern void network_channel_gap_4ba660(void *sb, uint32_t reason, void *server, void *instance);
extern void * ServerBrowserNew(const char *queryForGamename, const char *queryFromGamename, const char *queryFromKey,
    int32_t queryFromVersion, int32_t maxConcurrentUpdates, int32_t queryVersion, void *callback, void *instance);
extern void ServerBrowserFree(void *sb);
int32_t gamespy_array_length(void *array);
void * gamespy_array_nth(void *array, int32_t index);
void gamespy_think_all(void);
int32_t master_server_connection_start(void);
void master_server_connection_wait_thread(void);
void master_server_ensure_list_connection(void);
void master_server_list_refresh_request(void);
void master_server_process_pending_requests(void);
void qr2_register_key(int32_t keyid, const char *key);
uint32_t __stdcall sig__setup_master_server_connection_sig(void *parameter);
}


namespace halo::networking {

int32_t MasterServerConnection::array_length(void *array)
{
    return *(int32_t *)array;
}

void * MasterServerConnection::array_nth(void *array, int32_t index)
{
    return *(uint8_t **)((uint8_t *)array + 0x14) + index * *(int32_t *)((uint8_t *)array + 0x08);
}

void MasterServerConnection::think_all(void)
{
    int32_t i;

    if (negotiatorList == 0) {
        return;
    }
    for (i = gamespy_array_length(negotiatorList) - 1; i >= 0; i--) {
        NegotiateThink(gamespy_array_nth(negotiatorList, i));
    }
}

int32_t MasterServerConnection::connection_start(void)
{
    int32_t mutex_ok;
    int32_t thread_ok;
    network_mutex_record *mutex_slot;

    master_server_request_flags = 0;
    master_server_last_result = 0;
    mutex_ok = mutex_create(&server_list_mutex);
    if (mutex_ok != 0) {
        thread_ok = network_thread_create(0, (void *)sig__setup_master_server_connection_sig, 0,
                                           (network_thread_record **)&server_list_thread);
        mutex_slot = server_list_mutex;
        if (thread_ok != 0) {
            return 1;
        }
        CloseHandle(mutex_slot->handle);
        mutex_slot->name[0] = 0;
        mutex_slot->handle = 0;
        mutex_slot->in_use = 0;
        server_list_mutex = 0;
        server_list_thread = 0;
    }
    return 0;
}

void MasterServerConnection::connection_wait_thread(void)
{
    int32_t exited;
    uint32_t exit_code;
    large_integer counter;
    int32_t now_ms;

    master_server_request_flags = master_server_request_flags | 2;
    while (1) {
        exited = GetExitCodeThread(server_list_thread->handle, (LPDWORD)&exit_code);
        if (exited != 0 && exit_code != 0x103) {
            break;
        }
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        if (0x84 < (uint32_t)(now_ms - sound_time)) {
            sound_idle_update();
        }
        master_server_request_flags = master_server_request_flags | 2;
        Sleep(0x14);
    }
    CloseHandle(server_list_thread->handle);
    server_list_thread->handle = 0;
    server_list_thread->in_use = 0;
    CloseHandle(server_list_mutex->handle);
    server_list_mutex->name[0] = 0;
    server_list_mutex->handle = 0;
    server_list_mutex->in_use = 0;
    server_list_thread = 0;
    server_list_mutex = 0;
}

void MasterServerConnection::ensure_list_connection(void)
{
    int32_t state;
    int32_t connect_result;

    if (master_server_query_engine != 0) {
        state = ServerBrowserState(master_server_query_engine);
        if (state != 2 && state != 1) {
            connect_result = ServerBrowserCount(master_server_query_engine);
            if (connect_result < 1) {
                master_server_list_refresh_request();
                return;
            }
            DAT_00719488 = 1;
            master_server_request_flags = master_server_request_flags | 0x10;
            return;
        }
        master_server_request_flags = master_server_request_flags | 4;
        server_browser_query_elapsed_ms = 9999;
        DAT_00719488 = 0;
    }
}

void MasterServerConnection::list_refresh_request(void)
{
    large_integer counter;
    int32_t now_ms;

    master_server_request_flags = master_server_request_flags | 8;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    DAT_006953fc = now_ms + 10000;
    DAT_00719488 = 1;
}

void MasterServerConnection::process_pending_requests(void)
{
    uint32_t flags;
    int32_t last_result;
    uint32_t wait_result;
    int32_t i;
    int32_t index;

    flags = master_server_request_flags;
    master_server_request_flags = 0;
    if (master_server_query_engine != 0) {
        last_result = 0;
        if ((flags & 4) != 0) {
            ServerBrowserHalt(master_server_query_engine);
        }
        master_server_last_result = ServerBrowserThink(master_server_query_engine);
        if ((flags & 0x10) != 0) {
            if (server_list_thread == 0 ||
                (wait_result = WaitForSingleObject(server_list_mutex->handle, 100),
                 wait_result == 0) || wait_result == 0x80) {
                if (server_list.result_count < 1) {
                    flags = flags | 8;
                } else {
                    i = 0;
                    if (0 < server_list.result_count) {
                        do {
                            last_result = ServerBrowserAuxUpdateServer(master_server_query_engine,
                                                        server_list.list[i], 1, 1);
                            if (last_result != 0) {
                                break;
                            }
                            i = i + 1;
                        } while (i < server_list.result_count);
                    }
                    server_list_reset();
                }
                if (server_list_thread != 0) {
                    ReleaseMutex(server_list_mutex->handle);
                }
            } else {
                master_server_request_flags = master_server_request_flags | 0x10;
            }
        }
        if ((flags & 8) != 0) {
            if (server_list_thread == 0 ||
                (wait_result = WaitForSingleObject(server_list_mutex->handle, 100),
                 wait_result == 0) || wait_result == 0x80) {
                ServerBrowserClear(master_server_query_engine);
                server_list_reset();
                if (server_list_thread != 0) {
                    ReleaseMutex(server_list_mutex->handle);
                }
                if (server_browser_require_valid_entry == 0) {
                    last_result = ServerBrowserLANUpdate(master_server_query_engine, 1, network_session_start_game_type,
                                                (uint16_t)network_session_start_game_type);
                } else {
                    last_result = ServerBrowserUpdate(master_server_query_engine, 1, 0, DAT_00695424, 10, 0);
                }
            } else {
                master_server_request_flags = master_server_request_flags | 0x10;
            }
        }
        index = server_browser_selected_index;
        if ((flags & 0x20) != 0 && server_browser_selected_index != -1) {
            server_list_globals *locked = server_list_mutex_try_lock(100);
            if (locked == 0) {
                master_server_request_flags = master_server_request_flags | 0x20;
            } else {
                last_result = ServerBrowserAuxUpdateServer(master_server_query_engine,
                                            locked->list[index], 1, 1);
                if (server_list_thread != 0) {
                    ReleaseMutex(server_list_mutex->handle);
                }
            }
        }
        if (master_server_last_result == 0 && last_result != 0) {
            master_server_last_result = last_result;
        }
    }
}

void MasterServerConnection::register_key(int32_t keyid, const char *key)
{
    if (keyid >= 50 && keyid <= 254) {
        qr2_registered_key_list[keyid] = key;
    }
}

uint32_t __stdcall MasterServerConnection::setup_master_server_connection_sig(void *parameter)
{
    (void)parameter;
    server_browser_join_requested = 1;
    master_server_query_engine = ServerBrowserNew(network_session_start_host_name, network_session_start_host_name,
        network_session_start_map_name, 0, 10, 1, (void *)network_channel_gap_4ba660, 0);
    if ((master_server_request_flags & 2) == 0) {
        do {
            if (master_server_last_result == 0) {
                master_server_process_pending_requests();
            }
            Sleep(10);
        } while ((master_server_request_flags & 2) == 0);
    }
    ServerBrowserFree(master_server_query_engine);
    master_server_query_engine = 0;
    return 0;
}

}  // namespace halo::networking

extern "C" {
int32_t gamespy_array_length(void *array)
{
    return halo::networking::MasterServerConnection::array_length(array);
}

void * gamespy_array_nth(void *array, int32_t index)
{
    return halo::networking::MasterServerConnection::array_nth(array, index);
}

void gamespy_think_all(void)
{
    halo::networking::MasterServerConnection::think_all();
}

int32_t master_server_connection_start(void)
{
    return halo::networking::MasterServerConnection::connection_start();
}

void master_server_connection_wait_thread(void)
{
    halo::networking::MasterServerConnection::connection_wait_thread();
}

void master_server_ensure_list_connection(void)
{
    halo::networking::MasterServerConnection::ensure_list_connection();
}

void master_server_list_refresh_request(void)
{
    halo::networking::MasterServerConnection::list_refresh_request();
}

void master_server_process_pending_requests(void)
{
    halo::networking::MasterServerConnection::process_pending_requests();
}

void qr2_register_key(int32_t keyid, const char *key)
{
    halo::networking::MasterServerConnection::register_key(keyid, key);
}

uint32_t __stdcall sig__setup_master_server_connection_sig(void *parameter)
{
    return halo::networking::MasterServerConnection::setup_master_server_connection_sig(parameter);
}

}
