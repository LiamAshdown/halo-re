/**
 * @file src/networking/net2_master_server.cpp
 * Master server connection, list refresh and GameSpy glue.
 */
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "halo/networking/browser_state.hpp"
#include "halo/networking/net2_master_server.hpp"
#include "halo/sound/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/thread.hpp"

static auto &negotiatorList = halo::link::ref<void *>(halo::networking::vars().negotiatorList);
static auto &server_list_mutex = halo::link::ref<network_mutex_record *>(halo::networking::vars().server_list_mutex);
static auto &server_list_thread = halo::link::ref<network_thread_record *>(halo::networking::vars().server_list_thread);
static auto &master_server_request_flags = halo::link::ref<uint32_t>(halo::networking::vars().master_server_request_flags);
static auto &master_server_last_result = halo::link::ref<int32_t>(halo::networking::vars().master_server_last_result);
static auto &master_server_query_engine = halo::link::ref<void *>(halo::networking::vars().master_server_query_engine);
static auto &server_browser_query_elapsed_ms = halo::link::ref<int32_t>(halo::networking::vars().server_browser_query_elapsed_ms);
static auto &server_list = halo::link::ref<server_list_globals>(halo::networking::vars().server_list);
static auto &server_browser_require_valid_entry = halo::link::ref<uint8_t>(halo::networking::vars().server_browser_require_valid_entry);
static auto &network_session_start_game_type = halo::link::ref<uint32_t>(halo::networking::vars().network_session_start_game_type);
static auto &server_browser_selected_index = halo::link::ref<int32_t>(halo::networking::vars().server_browser_selected_index);
static auto &qr2_registered_key_list = halo::link::ref<const char * [255]>(halo::networking::vars().qr2_registered_key_list);
static auto &server_browser_join_requested = halo::link::ref<uint8_t>(halo::networking::vars().server_browser_join_requested);
static auto &network_session_start_host_name = halo::link::ref<char []>(halo::networking::vars().network_session_start_host_name);
static auto &network_session_start_map_name = halo::link::ref<char []>(halo::networking::vars().network_session_start_map_name);


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
    for (i = halo::networking::gamespy_array_length(negotiatorList) - 1; i >= 0; i--) {
        NegotiateThink(halo::networking::gamespy_array_nth(negotiatorList, i));
    }
}

int32_t MasterServerConnection::connection_start(void)
{
    int32_t mutex_ok;
    int32_t thread_ok;
    network_mutex_record *mutex_slot;

    master_server_request_flags = 0;
    master_server_last_result = 0;
    mutex_ok = halo::networking::mutex_create(&server_list_mutex);
    if (mutex_ok != 0) {
        thread_ok = halo::networking::network_thread_create(0, (void *)halo::networking::sig__setup_master_server_connection_sig, 0,
                                           (network_thread_record **)&server_list_thread);
        mutex_slot = server_list_mutex;
        if (thread_ok != 0) {
            return 1;
        }
        halo::platform::handle_close(mutex_slot->handle);
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
        exited = halo::platform::thread_exit_code(server_list_thread->handle, &exit_code);
        if (exited != 0 && exit_code != 0x103) {
            break;
        }
        halo::platform::read_performance_counter(&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
        if (0x84 < (uint32_t)(now_ms - halo::sound::globals().time)) {
            halo::sound::sound_idle_update();
        }
        master_server_request_flags = master_server_request_flags | 2;
        halo::platform::sleep_milliseconds(0x14);
    }
    halo::platform::handle_close(server_list_thread->handle);
    server_list_thread->handle = 0;
    server_list_thread->in_use = 0;
    halo::platform::handle_close(server_list_mutex->handle);
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
                halo::networking::master_server_list_refresh_request();
                return;
            }
            browser_state::refresh_in_flight = 1;
            master_server_request_flags = master_server_request_flags | 0x10;
            return;
        }
        master_server_request_flags = master_server_request_flags | 4;
        server_browser_query_elapsed_ms = 9999;
        browser_state::refresh_in_flight = 0;
    }
}

void MasterServerConnection::list_refresh_request(void)
{
    large_integer counter;
    int32_t now_ms;

    master_server_request_flags = master_server_request_flags | 8;
    halo::platform::read_performance_counter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    browser_state::next_auto_refresh_ms = now_ms + 10000;
    browser_state::refresh_in_flight = 1;
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
                (wait_result = halo::platform::wait(server_list_mutex->handle, 100),
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
                    halo::networking::server_list_reset(&server_list);
                }
                if (server_list_thread != 0) {
                    halo::platform::mutex_release(server_list_mutex->handle);
                }
            } else {
                master_server_request_flags = master_server_request_flags | 0x10;
            }
        }
        if ((flags & 8) != 0) {
            if (server_list_thread == 0 ||
                (wait_result = halo::platform::wait(server_list_mutex->handle, 100),
                 wait_result == 0) || wait_result == 0x80) {
                ServerBrowserClear(master_server_query_engine);
                halo::networking::server_list_reset(&server_list);
                if (server_list_thread != 0) {
                    halo::platform::mutex_release(server_list_mutex->handle);
                }
#if defined(__EMSCRIPTEN__)
                // The page's only network is the virtual LAN behind its server (src/platform/net_web.cpp), with no
                // GameSpy master to ask, so the Internet list searches that LAN the way the LAN list does.
                const bool lan_search = true;
#else
                const bool lan_search = server_browser_require_valid_entry == 0;
#endif
                if (lan_search) {
                    last_result = ServerBrowserLANUpdate(master_server_query_engine, 1, network_session_start_game_type,
                                                (uint16_t)network_session_start_game_type);
                } else {
                    halo::networking::qr2_register_key(0x33, "dedicated");
                    halo::networking::qr2_register_key(0x34, "player_flags");
                    halo::networking::qr2_register_key(0x35, "game_flags");
                    halo::networking::qr2_register_key(0x36, "game_classic");
                    last_result = ServerBrowserUpdate(master_server_query_engine, 1, 0, browser_state::master_query_key_ids, 10, 0);
                }
            } else {
                master_server_request_flags = master_server_request_flags | 0x10;
            }
        }
        index = server_browser_selected_index;
        if ((flags & 0x20) != 0 && server_browser_selected_index != -1) {
            server_list_globals *locked = halo::networking::server_list_mutex_try_lock(100);
            if (locked == 0) {
                master_server_request_flags = master_server_request_flags | 0x20;
            } else {
                last_result = ServerBrowserAuxUpdateServer(master_server_query_engine,
                                            locked->list[index], 1, 1);
                if (server_list_thread != 0) {
                    halo::platform::mutex_release(server_list_mutex->handle);
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
        network_session_start_map_name, 0, 10, 1, (void *)halo::networking::network_channel_gap_4ba660, 0);
    if ((master_server_request_flags & 2) == 0) {
        do {
            if (master_server_last_result == 0) {
                halo::networking::master_server_process_pending_requests();
            }
            halo::platform::sleep_milliseconds(10);
        } while ((master_server_request_flags & 2) == 0);
    }
    ServerBrowserFree(master_server_query_engine);
    master_server_query_engine = 0;
    return 0;
}

}  // namespace halo::networking

namespace halo::networking {
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
