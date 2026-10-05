/**
 * @file src/networking/net2_autopatch.cpp
 * Auto-patch version check, download pool and updater launch.
 */
#include "win32.h"
#include "halo/core/datum.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "networking.h"
#include "crt.h"
#if defined(_WIN32)
#include <wininet.h>
#endif
#include <ctype.h>
#include "interface.h"
#include "saved_games.h"
#include "main.h"
#include "halo/networking/net2_autopatch.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"
#include "halo/units/vars.hpp"
#include <stdio.h>
#include "../gamespy/gamespy_calls.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "halo/units/api.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/thread.hpp"
#include "halo/platform/memory.hpp"
#include "halo/platform/system.hpp"

static auto &autopatch_download_slots = halo::link::ref<autopatch_download_slot [2]>(halo::networking::vars().autopatch_download_slots);
static auto &network_mutex_table = halo::link::ref<network_mutex_record [k_network_mutex_table_count]>(halo::networking::vars().network_mutex_table);
static auto &network_mutex_name_counter = halo::link::ref<int32_t>(halo::networking::vars().network_mutex_name_counter);
static auto &network_thread_table = halo::link::ref<network_thread_record [k_network_thread_table_count]>(halo::networking::vars().network_thread_table);
static auto &autopatch_download_mutex = halo::link::ref<network_mutex_record *>(halo::networking::vars().autopatch_download_mutex);
static auto &autopatch_download_thread = halo::link::ref<network_thread_record *>(halo::networking::vars().autopatch_download_thread);
static auto &autopatch_download_pool_stop = halo::link::ref<uint8_t>(halo::networking::vars().autopatch_download_pool_stop);
static auto &autopatch_download_active_count = halo::link::ref<uint8_t>(halo::networking::vars().autopatch_download_active_count);
static auto &autopatch_proxy_server = halo::link::ref<char [0x100]>(halo::networking::vars().autopatch_proxy_server);
static auto &autopatch_proxy_ready = halo::link::ref<uint8_t>(halo::networking::vars().autopatch_proxy_ready);
static auto &ai_update_stagger = halo::link::ref<uint8_t [11]>(halo::units::vars().ai_update_stagger);
static auto &autopatch_temp_name_flag = halo::link::ref<uint8_t>(halo::networking::vars().autopatch_temp_name_flag);

typedef struct internet_proxy_info {       
    uint32_t access_type;
    const char *proxy;
    const char *proxy_bypass;
} internet_proxy_info;
typedef struct winhttp_autoproxy_options { 
    uint32_t flags;
    uint32_t auto_detect_flags;
    const uint16_t *auto_config_url;
    void *reserved_pointer;
    uint32_t reserved;
    int32_t auto_logon_if_challenged;
} winhttp_autoproxy_options;
typedef struct winhttp_proxy_info {        
    uint32_t access_type;
    uint16_t *proxy;
    uint16_t *proxy_bypass;
} winhttp_proxy_info;
typedef void *(__stdcall *winhttp_open_proc)(const uint16_t *agent, uint32_t access_type,
    const uint16_t *proxy, const uint16_t *bypass, uint32_t flags);
typedef int32_t (__stdcall *winhttp_get_proxy_for_url_proc)(void *session, const uint16_t *url,
    winhttp_autoproxy_options *options, winhttp_proxy_info *proxy_info);
typedef int32_t (__stdcall *winhttp_close_handle_proc)(void *handle);
static const uint16_t k_agent_halopc[] = { 'H', 'a', 'l', 'o', 'P', 'C', 0 };
static const uint16_t k_bungie_url[] = { 'h', 't', 't', 'p', ':', '/', '/', 'w', 'w', 'w', '.',
    'b', 'u', 'n', 'g', 'i', 'e', '.', 'n', 'e', 't', 0 };
typedef struct win32_process_information { 
    void *process;
    void *thread;
    uint32_t process_id;
    uint32_t thread_id;
} win32_process_information;
typedef struct win32_startupinfo {         
    uint32_t cb;
    uint8_t unknown_04[0x40];
} win32_startupinfo;
static uint32_t autopatch_string_length(const char *string)
{
    const char *end = string;
    while (*end != 0) {
        end++;
    }
    return (uint32_t)(end - string);
}

namespace halo::networking {

void AutopatchUpdater::current_version_string_get(char *out)
{
    int32_t i;
    static const char version[] = "01.00.10.0621";
    for (i = 0; i <= 13; i++) {
        out[i] = version[i];
    }
}

uint32_t AutopatchUpdater::download_complete_callback(int32_t request_id, int32_t error, uint8_t *data, uint32_t size)
{
    int32_t i;

    for (i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id == request_id) {
            if (error != 0) {
                autopatch_download_slots[i].state = k_autopatch_download_error;
                return 1;
            }
            if (autopatch_download_slots[i].cancelled == 0) {
                uint8_t *buffer = (uint8_t *)halo::platform::heap_allocate(0, size + 1);
                uint32_t j;

                autopatch_download_slots[i].data = buffer;
                for (j = 0; j < size; j++) {
                    buffer[j] = data[j];
                }
                buffer[size] = 0;
                autopatch_download_slots[i].size = size + 1;
            }
            autopatch_download_slots[i].state = k_autopatch_download_ready;
            return 1;
        }
    }
    return 1;
}

uint8_t AutopatchUpdater::download_get_result(void **out_data, int32_t *out_size, int32_t slot_index)
{
    if (slot_index >= 0 && slot_index < 2 &&
        autopatch_download_slots[slot_index].state == k_autopatch_download_ready &&
        autopatch_download_slots[slot_index].cancelled == 0) {
        *out_data = autopatch_download_slots[slot_index].data;
        *out_size = autopatch_download_slots[slot_index].size;
        return 1;
    }
    return 0;
}

uint8_t AutopatchUpdater::download_pool_initialize(void)
{
    int32_t i;
    network_mutex_record *mutex_slot;
    network_thread_record *thread_slot;
    uint32_t thread_id;
    uint8_t started;

    for (i = 0; i < 2; i++) {
        autopatch_download_slots[i].request_id = 0;
        autopatch_download_slots[i].state = 0;
        autopatch_download_slots[i].data = 0;
        autopatch_download_slots[i].size = 0;
        autopatch_download_slots[i].local_file = 0;
        autopatch_download_slots[i].request_id = -1;
    }

    ghttpStartup();

    started = 0;
    mutex_slot = 0;
    for (i = 0; i < k_network_mutex_table_count; i++) {
        if (network_mutex_table[i].in_use == 0) {
            mutex_slot = &network_mutex_table[i];
            mutex_slot->name[0] = 0;
            mutex_slot->handle = 0;
            network_mutex_table[i].in_use = 1;
            break;
        }
    }
    if (mutex_slot != 0) {
        int32_t name_index = network_mutex_name_counter;
        network_mutex_name_counter = network_mutex_name_counter + 1;
        _snprintf(mutex_slot->name, 0x20, "mutex_%ld", name_index);
        mutex_slot->handle = halo::platform::mutex_create(false, nullptr);
        if (mutex_slot->handle == 0) {
            mutex_slot = 0;
        } else {
            started = 1;
        }
    }
    autopatch_download_mutex = mutex_slot;

    if (started) {
        thread_slot = 0;
        for (i = 0; i < 0x20; i++) {
            if (network_thread_table[i].in_use == 0) {
                thread_slot = &network_thread_table[i];
                thread_slot->handle = 0;
                network_thread_table[i].in_use = 1;
                thread_slot->handle = halo::platform::thread_create(0x4000, (halo::platform::thread_procedure)halo::networking::autopatch_download_worker_thread, 0, 4, &thread_id);
                autopatch_download_thread = thread_slot;
                if (thread_slot->handle != 0) {
                    if (halo::platform::thread_set_priority(thread_slot->handle, 0) != 0 &&
                        halo::platform::thread_resume(thread_slot->handle) != halo::k_dword_none) {
                        autopatch_download_pool_stop = 1;
                        autopatch_download_active_count = 0;
                        return 1;
                    }
                    halo::platform::handle_close(thread_slot->handle);
                }
                break;
            }
        }
        halo::platform::handle_close(autopatch_download_mutex->handle);
        autopatch_download_mutex->in_use = 0;
        autopatch_download_mutex->handle = 0;
        autopatch_download_mutex = 0;
        autopatch_download_thread = 0;
    }
    autopatch_download_pool_stop = 1;
    autopatch_download_active_count = 0;
    return 0;
}

uint32_t AutopatchUpdater::download_pool_shutdown(void)
{
    int32_t i;
    uint32_t exit_code;

    for (i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id != -1) {
            autopatch_download_slots[i].cancelled = 1;
        }
    }

    autopatch_download_active_count = 1;
    do {
        while (halo::platform::thread_exit_code(autopatch_download_thread->handle, &exit_code) == 0) {
        }
    } while (exit_code == 0x103);

    halo::platform::handle_close(autopatch_download_thread->handle);
    autopatch_download_thread->handle = 0;
    autopatch_download_thread->in_use = 0;

    halo::platform::handle_close(autopatch_download_mutex->handle);
    autopatch_download_mutex->in_use = 0;
    autopatch_download_mutex->handle = 0;
    autopatch_download_mutex->name[0] = 0;

    autopatch_download_thread = 0;
    autopatch_download_mutex = 0;

    ghttpCleanup();
    return 0;  // the original returned ghttpCleanup's leftover EAX with AL cleared: callers read AL, which is 0
}

int32_t AutopatchUpdater::download_pool_tick(void)
{
    int32_t active_count;
    int32_t i;

    active_count = 0;
    for (i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id != -1) {
            active_count = active_count + 1;
        }
        if (autopatch_download_slots[i].cancelled != 0) {
            if (autopatch_download_slots[i].data != 0 && autopatch_download_slots[i].local_file == 0) {
                halo::platform::heap_free(autopatch_download_slots[i].data);
            }
            ghttpCancelRequest(autopatch_download_slots[i].request_id);
            autopatch_download_slots[i].request_id = 0;
            autopatch_download_slots[i].state = 0;
            autopatch_download_slots[i].data = 0;
            autopatch_download_slots[i].size = 0;
            autopatch_download_slots[i].local_file = 0;
            autopatch_download_slots[i].request_id = -1;
        }
    }
    ghttpThink();
    return active_count;
}

void AutopatchUpdater::download_progress_callback(int32_t request, int32_t state, const char *buffer, int32_t buffer_length,
    int32_t bytes_received, int32_t total_size, void *param)
{
    void (*const ghttpCancelRequest)(int32_t request) = reinterpret_cast<void (*)(int32_t request)>(&::ghttpCancelRequest);
    int32_t i;

    (void)buffer;
    (void)buffer_length;
    (void)bytes_received;
    (void)total_size;
    (void)param;
    for (i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id == request) {
            break;
        }
    }
    if (i == 2) {
        ghttpCancelRequest(request);
        return;
    }
    if (state >= 5 && state <= 7) {
        autopatch_download_slots[i].state = 3;
    } else {
        autopatch_download_slots[i].state = 2;
    }
}

int32_t AutopatchUpdater::download_start(void *path, int32_t local_file)
{
    int32_t slot_index;
    int32_t request_id;

    slot_index = -1;
    for (int32_t i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id == -1) {
            slot_index = i;
            break;
        }
    }
    if (slot_index == -1) {
        return -1;
    }

    autopatch_download_slots[slot_index].state = k_autopatch_download_active;
    if (local_file == 0) {
        request_id = ghttpGetEx(path, 0, 0, 0, 0, 0, 0, (void *)halo::networking::autopatch_download_progress_callback,
                                   (void *)halo::networking::autopatch_download_complete_callback, 0);
        autopatch_download_slots[slot_index].local_file = 0;
    } else {

        request_id = ghttpSaveEx(path, (void *)local_file, 0, 0, 0, 0, (void *)halo::networking::autopatch_download_progress_callback,
                                  (void *)halo::networking::autopatch_download_complete_callback, 0);
        autopatch_download_slots[slot_index].local_file = 1;
    }
    autopatch_download_slots[slot_index].request_id = request_id;
    if (request_id == -1) {
        autopatch_download_slots[slot_index].request_id = 0;
        autopatch_download_slots[slot_index].state = 0;
        autopatch_download_slots[slot_index].data = 0;
        autopatch_download_slots[slot_index].size = 0;
        autopatch_download_slots[slot_index].local_file = 0;
        autopatch_download_slots[slot_index].request_id = -1;
        return -1;
    }
    return slot_index;
}

uint32_t AutopatchUpdater::download_worker_thread(void)
{
    int32_t active_count = 1;

    do {
        halo::platform::sleep_milliseconds(active_count < 1 ? 1000 : 0x14);
        active_count = halo::networking::autopatch_download_pool_tick();
    } while (autopatch_download_active_count == 0);
    autopatch_download_active_count = 0;
    return 0;
}

char * AutopatchUpdater::get_proxy_settings(void)
{
    uint32_t query_length;
    winhttp_autoproxy_options options;
    winhttp_proxy_info proxy_info;
    char proxy_list[0x400];
    uint8_t query_buffer[0x400];
    void *winhttp;
    winhttp_get_proxy_for_url_proc get_proxy_for_url;
    winhttp_open_proc open;
    winhttp_close_handle_proc close_handle;
    void *session;
    char *token;
    char *proxy;
    char *cursor;
    int32_t token_count;
    int32_t i;

    for (i = 0; i < 0x400; i++) {
        query_buffer[i] = 0;
        proxy_list[i] = 0;
    }
    for (i = 0; i < 0x100; i++) {
        autopatch_proxy_server[i] = 0;
    }

    query_length = 0x3ff;
#if defined(_WIN32)
    if (InternetQueryOptionA(0, 0x26, query_buffer, (LPDWORD)&query_length) && query_length > 1 &&
        ((internet_proxy_info *)query_buffer)->proxy != 0) {
        strncpy(proxy_list, ((internet_proxy_info *)query_buffer)->proxy, 0x400);
        proxy_list[0x3ff] = 0;
    }
#endif

    if (proxy_list[0] == 0) {
        winhttp = halo::platform::library_open("winhttp.dll");
        if (winhttp != 0) {
            get_proxy_for_url = (winhttp_get_proxy_for_url_proc)halo::platform::library_symbol(winhttp, "WinHttpGetProxyForUrl");
            open = (winhttp_open_proc)halo::platform::library_symbol(winhttp, "WinHttpOpen");
            close_handle = (winhttp_close_handle_proc)halo::platform::library_symbol(winhttp, "WinHttpCloseHandle");
            if (get_proxy_for_url != 0 && open != 0 && close_handle != 0) {
                session = open(k_agent_halopc, 0, 0, 0, 0);
                if (session != 0) {
                    options.flags = 1;
                    options.auto_detect_flags = 3;
                    options.auto_config_url = 0;
                    options.reserved_pointer = 0;
                    options.reserved = 0;
                    options.auto_logon_if_challenged = 1;
                    if (get_proxy_for_url(session, k_bungie_url, &options, &proxy_info)) {
                        if (proxy_info.proxy != 0) {
                            if (proxy_info.proxy[0] != 0) {
                                halo::platform::wide_to_ansi(proxy_info.proxy, -1, proxy_list, 0x400);
                                proxy_list[0x3ff] = 0;
                            }
                            if (proxy_info.proxy != 0) {
                                halo::platform::heap_free(proxy_info.proxy);
                            }
                        }
                        if (proxy_info.proxy_bypass != 0) {
                            halo::platform::heap_free(proxy_info.proxy_bypass);
                        }
                    }
                    close_handle(session);
                }
            }
            halo::platform::library_close(winhttp);
        }
        if (proxy_list[0] == 0) {
            return autopatch_proxy_server;
        }
    }

    for (cursor = proxy_list; *cursor != 0; cursor++) {
        *cursor = (char)tolower((uint8_t)*cursor);
    }

    token_count = 0;
    token = strtok(proxy_list, " ;");
    if (token == 0) {
        return autopatch_proxy_server;
    }
    proxy = 0;
    do {
        token_count++;
        if (strstr(token, "http=") == token) {
            proxy = token + 5;
            if (proxy == 0) {
                return autopatch_proxy_server;
            }
            break;
        }
        token = strtok(0, " ;");
    } while (token != 0);
    if (proxy == 0) {
        if (token_count <= 0) {
            return autopatch_proxy_server;
        }
        proxy = proxy_list;
    }
    while (strstr(proxy, "http://") == proxy) {
        proxy += 7;
    }
    strncpy(autopatch_proxy_server, proxy, 0x100);
    autopatch_proxy_server[0xff] = 0;
    return autopatch_proxy_server;
}

uint32_t __stdcall AutopatchUpdater::proxy_initialize(void *parameter)
{
    (void)parameter;
    void *settings = halo::networking::autopatch_get_proxy_settings();
    ghttpSetProxy(settings);
    autopatch_proxy_ready = 1;
    return 0;
}

char * AutopatchUpdater::temp_name_generate(void)
{
    uint32_t now;
    int32_t i;

    now = (uint32_t)_time32(0);
    srand(now ^ 0x33333333);
    autopatch_temp_name_flag = 0;
    for (i = 10; i >= 4; i--) {
        ai_update_stagger[i] = (uint8_t)(rand() % 0x1a) + 'a';
    }
    return (char *)&ai_update_stagger[4];
}

uint32_t AutopatchUpdater::version_string_is_outdated(char *version)
{
    int32_t (*const strcmp)(const char *a, const char *b) = reinterpret_cast<int32_t (*)(const char *a, const char *b)>(&::strcmp);
    static const char minimum_baseline[] = "01.00.08.0616";
    static const char current_build[] = "01.00.10.0621";

    if (strcmp(version, current_build) != 0) {
        if (strcmp(version, minimum_baseline) != 0) {
            return 0;
        }
    }
    return 1;
}

}  // namespace halo::networking

namespace halo::networking {
void autopatch_current_version_string_get(char *out)
{
    halo::networking::AutopatchUpdater::current_version_string_get(out);
}

uint32_t autopatch_download_complete_callback(int32_t request_id, int32_t error, uint8_t *data, uint32_t size)
{
    return halo::networking::AutopatchUpdater::download_complete_callback(request_id, error, data, size);
}

uint8_t autopatch_download_get_result(void **out_data, int32_t *out_size, int32_t slot_index)
{
    return halo::networking::AutopatchUpdater::download_get_result(out_data, out_size, slot_index);
}

uint8_t autopatch_download_pool_initialize(void)
{
    return halo::networking::AutopatchUpdater::download_pool_initialize();
}

uint32_t autopatch_download_pool_shutdown(void)
{
    return halo::networking::AutopatchUpdater::download_pool_shutdown();
}

int32_t autopatch_download_pool_tick(void)
{
    return halo::networking::AutopatchUpdater::download_pool_tick();
}

void autopatch_download_progress_callback(int32_t request, int32_t state, const char *buffer, int32_t buffer_length,
    int32_t bytes_received, int32_t total_size, void *param)
{
    halo::networking::AutopatchUpdater::download_progress_callback(request, state, buffer, buffer_length, bytes_received, total_size, param);
}

int32_t autopatch_download_start(void *path, int32_t local_file)
{
    return halo::networking::AutopatchUpdater::download_start(path, local_file);
}

uint32_t autopatch_download_worker_thread(void)
{
    return halo::networking::AutopatchUpdater::download_worker_thread();
}

char * autopatch_get_proxy_settings(void)
{
    return halo::networking::AutopatchUpdater::get_proxy_settings();
}

uint32_t __stdcall autopatch_proxy_initialize(void *parameter)
{
    return halo::networking::AutopatchUpdater::proxy_initialize(parameter);
}

char * autopatch_temp_name_generate(void)
{
    return halo::networking::AutopatchUpdater::temp_name_generate();
}

uint32_t autopatch_version_string_is_outdated(char *version)
{
    return halo::networking::AutopatchUpdater::version_string_is_outdated(version);
}

}
