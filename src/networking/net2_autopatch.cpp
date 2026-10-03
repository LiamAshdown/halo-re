/**
 * @file src/networking/net2_autopatch.cpp
 * Auto-patch version check, download pool and updater launch.
 */
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "networking.h"
#include "crt.h"
#include <wininet.h>
#include <ctype.h>
#include "interface.h"
#include "saved_games.h"
#include "main.h"
#include "halo/networking/net2_autopatch.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/main/api.hpp"

extern "C" {
extern int32_t autopatch_update_check_state;
extern uint8_t * autopatch_update_cfg_directory;
extern autopatch_download_slot autopatch_download_slots[2];
extern network_mutex_record network_mutex_table[k_network_mutex_table_count];
extern int32_t network_mutex_name_counter;
extern network_thread_record network_thread_table[k_network_thread_table_count];
extern network_mutex_record * autopatch_download_mutex;
extern network_thread_record * autopatch_download_thread;
extern uint8_t autopatch_download_pool_stop;
extern uint8_t autopatch_download_active_count;
extern void ghttpStartup(void);
extern int32_t _snprintf(char *buffer, uint32_t count, const char *format, ...);
extern int32_t ghttpCleanup(void);
extern void ghttpThink(void);
extern void ghttpCancelRequest(int32_t request_id);
extern int32_t ghttpGetEx(void *path, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6,
                             int32_t a7, void *progress_callback, void *complete_callback, int32_t a8);
extern int32_t ghttpSaveEx(void *url, void *filename, void *headers, void *post, int32_t throttle, int32_t blocking,
                             void *progress_callback, void *complete_callback, void *param);
extern char autopatch_proxy_server[0x100];
extern char autopatch_update_url[0x100];
extern char autopatch_update_version[0x100];
extern uint8_t autopatch_proxy_ready;
extern void ghttpSetProxy(void *proxy_settings);
extern uint8_t ai_update_stagger[11];
extern uint8_t autopatch_temp_name_flag;
extern int32_t autopatch_update_file_id;
extern char * registry_get_halo_version(void);
extern uint32_t registry_get_dist_id(void);
extern int32_t ptCheckForPatch(int32_t request_type, char *version, uint32_t dist_id,
                             void *callback, int32_t a5, int32_t a6);
int32_t autopatch_check_for_update_start(void);
void autopatch_current_version_string_get(char *out);
uint32_t autopatch_download_complete_callback(int32_t request_id, int32_t error, uint8_t *data, uint32_t size);
uint8_t autopatch_download_get_result(void **out_data, int32_t *out_size, int32_t slot_index);
uint8_t autopatch_download_pool_initialize(void);
uint32_t autopatch_download_pool_shutdown(void);
int32_t autopatch_download_pool_tick(void);
void autopatch_download_progress_callback(int32_t request, int32_t state, const char *buffer, int32_t buffer_length,
    int32_t bytes_received, int32_t total_size, void *param);
int32_t autopatch_download_start(void *path, int32_t local_file);
uint32_t autopatch_download_worker_thread(void);
char * autopatch_get_proxy_settings(void);
uint8_t autopatch_launch_updater(void);
uint32_t __stdcall autopatch_proxy_initialize(void *parameter);
char * autopatch_temp_name_generate(void);
void autopatch_version_check_completed(int32_t available, int32_t mandatory, const char *version_name, int32_t file_id,
    const char *download_url, void *param);
uint32_t autopatch_version_check_request(void);
uint32_t autopatch_version_string_is_outdated(char *version);
}

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

int32_t AutopatchUpdater::check_for_update_start(void)
{
    if (autopatch_update_check_state == -1) {
        file_reference reference;
        uint8_t *flags = (uint8_t *)&reference + 8;
        void *thread;
        uint32_t thread_id;

        if (halo::shell::security_check_write_access() != 0) {
            uint32_t *raw = (uint32_t *)&reference;
            int32_t i;
            for (i = 0; i < 0x43; i++) {
                raw[i] = 0;
            }
            *(int32_t *)((uint8_t *)&reference + 4) = 0x66696c6f;
            *(uint16_t *)((uint8_t *)&reference + 6) = 0xffff;
            if ((*flags & 1) != 0) {
                halo::saved_games::path_remove_last_component((char *)((uint8_t *)&reference + 8));
            }
            halo::saved_games::path_append_component((char *)&reference + 8, "currentupdate.cfg");
            *flags = *flags | 1;
            if (halo::saved_games::file_reference_exists((file_reference_record *)&reference) != 0) {
                halo::saved_games::file_reference_delete((file_reference_record *)&reference);
            }
        }

        thread = CreateThread(0, 0x10400, (LPTHREAD_START_ROUTINE)autopatch_version_check_request, 0, 0, (LPDWORD)&thread_id);
        if (thread != (void *)0xffffffff) {
            autopatch_update_check_state = 1;
            CloseHandle(thread);
            return autopatch_update_check_state;
        }
        autopatch_update_check_state = 0;
    }
    return autopatch_update_check_state;
}

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
            if (((uint8_t *)&autopatch_download_slots[i].local_file)[1] == 0) {
                uint8_t *buffer = (uint8_t *)GlobalAlloc(0, size + 1);
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
        ((uint8_t *)&autopatch_download_slots[slot_index].local_file)[1] == 0) {
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
        mutex_slot->handle = CreateMutexA(0, 0, 0);
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
                thread_slot->handle = CreateThread(0, 0x4000, (LPTHREAD_START_ROUTINE)autopatch_download_worker_thread,
                                                    0, 4, (LPDWORD)&thread_id);
                autopatch_download_thread = thread_slot;
                if (thread_slot->handle != 0) {
                    if (SetThreadPriority(thread_slot->handle, 0) != 0 &&
                        ResumeThread(thread_slot->handle) != 0xffffffff) {
                        autopatch_download_pool_stop = 1;
                        autopatch_download_active_count = 0;
                        return 1;
                    }
                    CloseHandle(thread_slot->handle);
                }
                break;
            }
        }
        CloseHandle(autopatch_download_mutex->handle);
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
            ((uint8_t *)&autopatch_download_slots[i].local_file)[1] = 1;
        }
    }

    autopatch_download_active_count = 1;
    do {
        while (GetExitCodeThread(autopatch_download_thread->handle, (LPDWORD)&exit_code) == 0) {
        }
    } while (exit_code == 0x103);

    CloseHandle(autopatch_download_thread->handle);
    autopatch_download_thread->handle = 0;
    autopatch_download_thread->in_use = 0;

    CloseHandle(autopatch_download_mutex->handle);
    autopatch_download_mutex->in_use = 0;
    autopatch_download_mutex->handle = 0;
    autopatch_download_mutex->name[0] = 0;

    autopatch_download_thread = 0;
    autopatch_download_mutex = 0;

    return (uint32_t)ghttpCleanup() & 0xffffff00;
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
        if (((uint8_t *)&autopatch_download_slots[i].local_file)[1] != 0) {
            if (autopatch_download_slots[i].data != 0 && autopatch_download_slots[i].local_file == 0) {
                GlobalFree(autopatch_download_slots[i].data);
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
        request_id = ghttpGetEx(path, 0, 0, 0, 0, 0, 0, (void *)autopatch_download_progress_callback,
                                   (void *)autopatch_download_complete_callback, 0);
        autopatch_download_slots[slot_index].local_file = 0;
    } else {

        request_id = ghttpSaveEx(path, (void *)local_file, 0, 0, 0, 0, (void *)autopatch_download_progress_callback,
                                  (void *)autopatch_download_complete_callback, 0);
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
        Sleep(active_count < 1 ? 1000 : 0x14);
        active_count = autopatch_download_pool_tick();
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
    if (InternetQueryOptionA(0, 0x26, query_buffer, (LPDWORD)&query_length) && query_length > 1 &&
        ((internet_proxy_info *)query_buffer)->proxy != 0) {
        strncpy(proxy_list, ((internet_proxy_info *)query_buffer)->proxy, 0x400);
        proxy_list[0x3ff] = 0;
    }

    if (proxy_list[0] == 0) {
        winhttp = LoadLibraryA("winhttp.dll");
        if (winhttp != 0) {
            get_proxy_for_url = (winhttp_get_proxy_for_url_proc)GetProcAddress((HMODULE)winhttp, "WinHttpGetProxyForUrl");
            open = (winhttp_open_proc)GetProcAddress((HMODULE)winhttp, "WinHttpOpen");
            close_handle = (winhttp_close_handle_proc)GetProcAddress((HMODULE)winhttp, "WinHttpCloseHandle");
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
                                WideCharToMultiByte(0, 0, (LPCWCH)proxy_info.proxy, -1, proxy_list, 0x400, 0, 0);
                                proxy_list[0x3ff] = 0;
                            }
                            if (proxy_info.proxy != 0) {
                                GlobalFree(proxy_info.proxy);
                            }
                        }
                        if (proxy_info.proxy_bypass != 0) {
                            GlobalFree(proxy_info.proxy_bypass);
                        }
                    }
                    close_handle(session);
                }
            }
            FreeLibrary((HMODULE)winhttp);
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
    do {
        token_count++;
        if (strstr(token, "http=") == token) {
            proxy = token + 5;
            if (proxy == 0) {
                return autopatch_proxy_server;
            }
            goto copy_proxy;
        }
        token = strtok(0, " ;");
    } while (token != 0);
    if (token_count <= 0) {
        return autopatch_proxy_server;
    }
    proxy = proxy_list;

copy_proxy:
    while (strstr(proxy, "http://") == proxy) {
        proxy += 7;
    }
    strncpy(autopatch_proxy_server, proxy, 0x100);
    autopatch_proxy_server[0xff] = 0;
    return autopatch_proxy_server;
}

uint8_t AutopatchUpdater::launch_updater(void)
{
    file_reference_record config;
    file_reference_record module_ref;
    char module_path[0x105];
    char line[0x401];
    char full_path[0x100];
    char *extension;
    char *file_name;
    char *path_start;
    char *directory;
    win32_process_information process;
    win32_startupinfo startup;
    uint32_t i;

    for (i = 0; i < sizeof(config); i++) {
        ((uint8_t *)&config)[i] = 0;
    }
    config.signature = k_file_reference_signature;
    config.location = -1;
    if (config.flags & _file_reference_is_file_bit) {
        halo::saved_games::path_remove_last_component(config.path);
    }
    halo::saved_games::path_append_component(config.path, "currentupdate.cfg");
    config.flags |= _file_reference_is_file_bit;

    if (!halo::saved_games::file_reference_create(&config) || !halo::saved_games::file_reference_open(&config, _file_open_write) ||
        !halo::saved_games::file_reference_seek(0, &config)) {
        return 0;
    }

    for (i = 0; i < sizeof(module_path); i++) {
        module_path[i] = 0;
    }
    if ((int32_t)GetModuleFileNameA(0, module_path, 0x104) <= 0) {
        return 0;
    }

    for (i = 0; i < sizeof(module_ref); i++) {
        ((uint8_t *)&module_ref)[i] = 0;
    }
    module_ref.signature = k_file_reference_signature;
    module_ref.location = -1;
    if (module_ref.flags & _file_reference_is_file_bit) {
        halo::saved_games::path_remove_last_component(module_ref.path);
    }
    halo::saved_games::path_append_component(module_ref.path, module_path);
    module_ref.flags |= _file_reference_is_file_bit;

    for (i = 0; i < sizeof(full_path); i++) {
        full_path[i] = 0;
    }
    halo::saved_games::path_build_full(module_ref.path, full_path, module_ref.location);
    halo::saved_games::path_split_components(&directory, full_path, &file_name, &path_start, &extension,
        (uint8_t)(module_ref.flags & _file_reference_is_file_bit));
    module_path[0] = 0;
    halo::saved_games::path_append_component(module_path, file_name);
    halo::saved_games::path_append_extension(module_path, extension);

    _snprintf(line, 0x400, "gamemode 1\n");
    line[0x400] = 0;
    if (!halo::saved_games::file_reference_write(&config, line, autopatch_string_length(line))) {
        return 0;
    }
    _snprintf(line, 0x400, "url \"%s\"\n", autopatch_update_url);
    line[0x400] = 0;
    if (!halo::saved_games::file_reference_write(&config, line, autopatch_string_length(line))) {
        return 0;
    }
    _snprintf(line, 0x400, "updateversion \"%s\"\n", autopatch_update_version);
    line[0x400] = 0;
    if (!halo::saved_games::file_reference_write(&config, line, autopatch_string_length(line))) {
        return 0;
    }
    _snprintf(line, 0x400, "gamecommand \"%s %s\"\n", module_path, halo::shell::globals().command_line);
    line[0x400] = 0;
    if (!halo::saved_games::file_reference_write(&config, line, autopatch_string_length(line))) {
        return 0;
    }
    if (!halo::saved_games::file_reference_close(&config)) {
        return 0;
    }

    process.process = 0;
    process.thread = 0;
    process.process_id = 0;
    process.thread_id = 0;
    for (i = 0; i < sizeof(startup); i++) {
        ((uint8_t *)&startup)[i] = 0;
    }
    startup.cb = 0x44;
    sprintf(line, "%s waitprocessid=%d", "haloupdate.exe", GetCurrentProcessId());

    if (CreateProcessA(0, line, 0, 0, 0, 0x4000020, 0, 0, (LPSTARTUPINFOA)&startup, (LPPROCESS_INFORMATION)&process)) {
        halo::main::globals().main_globals.return_to_main_menu = 0;
        halo::main::globals().main_globals.quit = 1;
        halo::main::globals().movie_playback_abort = 1;
        return 1;
    }

    halo::saved_games::file_reference_delete(&config);
    autopatch_update_check_state = 4;
    return 0;
}

uint32_t __stdcall AutopatchUpdater::proxy_initialize(void *parameter)
{
    void * (*const autopatch_get_proxy_settings)(void) = reinterpret_cast<void * (*)(void)>(&::autopatch_get_proxy_settings);
    (void)parameter;
    void *settings = autopatch_get_proxy_settings();
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

void AutopatchUpdater::version_check_completed(int32_t available, int32_t mandatory, const char *version_name, int32_t file_id,
    const char *download_url, void *param)
{
    char * (*const strncpy)(char *dest, const char *source, unsigned int count) = reinterpret_cast<char * (*)(char *dest, const char *source, unsigned int count)>(&::strncpy);
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

uint32_t AutopatchUpdater::version_check_request(void)
{
    void (*const autopatch_version_check_completed)(void) = reinterpret_cast<void (*)(void)>(&::autopatch_version_check_completed);
    char *version = registry_get_halo_version();
    uint32_t dist_id = registry_get_dist_id();

    while (autopatch_proxy_ready != 1) {
        Sleep(0);
    }

    if (version[0] == 0 ||
        ptCheckForPatch(0x281b, version, dist_id, (void *)autopatch_version_check_completed, 1, 0) == 0) {
        autopatch_update_check_state = 0;
    }
    return 0;
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

extern "C" {
int32_t autopatch_check_for_update_start(void)
{
    return halo::networking::AutopatchUpdater::check_for_update_start();
}

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

uint8_t autopatch_launch_updater(void)
{
    return halo::networking::AutopatchUpdater::launch_updater();
}

uint32_t __stdcall autopatch_proxy_initialize(void *parameter)
{
    return halo::networking::AutopatchUpdater::proxy_initialize(parameter);
}

char * autopatch_temp_name_generate(void)
{
    return halo::networking::AutopatchUpdater::temp_name_generate();
}

void autopatch_version_check_completed(int32_t available, int32_t mandatory, const char *version_name, int32_t file_id,
    const char *download_url, void *param)
{
    halo::networking::AutopatchUpdater::version_check_completed(available, mandatory, version_name, file_id, download_url, param);
}

uint32_t autopatch_version_check_request(void)
{
    return halo::networking::AutopatchUpdater::version_check_request();
}

uint32_t autopatch_version_string_is_outdated(char *version)
{
    return halo::networking::AutopatchUpdater::version_string_is_outdated(version);
}

}
