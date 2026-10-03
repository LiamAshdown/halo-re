#include "tags.h"

#include "halo/cache/cache.hpp"

#include "win32.h"
#include "crt.h"
#include "memory.h"
#include <string.h>
#include "halo/cache/globals.hpp"
#include "halo/cache/api.hpp"
#include "halo/memory/api.hpp"

extern "C" {
extern char map_path_prefix[];
extern void os_platform_identify(void);
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal);
extern void interface_handle_quit_request(void);
extern int32_t os_platform;
extern char *rasterizer_shader_file_name;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern char profile_directory[0x105];
extern int32_t sound_cache_size_megabytes;
typedef uint32_t (*get_mapped_file_name_a_t)(void *process, void *address, char *filename, uint32_t size);
}

namespace halo::cache {

/**
 * Finishes the multiplayer map download: asks a still-running download thread to stop and waits for
 * it, stamps the current system time as the slot's last write time (in memory and on disk), re-reads
 * the slot header and clears the download state.
 *
 * @address 0x443540
 */
void cache_files::download_finish()
{
    uint32_t finished_signaled;
    int32_t slot_index;
    system_time now;

    finished_signaled = WaitForSingleObject(globals().map_download->finished_event, 0);
    if (finished_signaled != 0) {
        SetEvent(globals().map_download->stop_event);
        WaitForSingleObject(globals().map_download->finished_event, 0xffffffff);
    }

    slot_index = globals().map_download_slot_index;
    GetSystemTime((LPSYSTEMTIME)&now);
    SystemTimeToFileTime((const SYSTEMTIME *)&now, (LPFILETIME)&globals().cache_file_slots[slot_index].last_write_time);
    SetFileTime(globals().cache_file_slots[slot_index].file, (const FILETIME *)&globals().cache_file_slots[slot_index].last_write_time,
        (const FILETIME *)((file_time *)0), (const FILETIME *)((file_time *)0));
    halo::cache::cache_files::slot_read_header(slot_index);

    globals().map_download_in_progress = 0;
    globals().map_download_slot_index = -1;
}

/**
 * Returns 1 when the base name of name equals the map currently being downloaded, 0 when nothing is
 * downloading or the names differ.
 *
 * @address 0x4432f0
 */
uint8_t cache_files::download_matches(char *name)
{
    char *slash;
    char *basename;
    char *a;
    char *b;
    uint8_t ca;
    uint8_t cb;

    if (globals().map_download_slot_index == -1) {
        return 0;
    }

    slash = strrchr(name, '\\');
    basename = (slash != 0) ? slash + 1 : name;

    a = globals().map_download_name;
    b = basename;
    for (;;) {
        ca = (uint8_t)*a;
        cb = (uint8_t)*b;
        if (ca != cb) {
            return 0;
        }
        if (ca == 0) {
            return 1;
        }
        a = a + 1;
        b = b + 1;
    }
}

/**
 * Polls the map downloader and returns its raw status code (0 failed, 1 cancelled, 2 reset, 3 idle, 4
 * running), writing the progress fraction, clamped to [0, 1], through progress_out when it is
 * available.
 *
 * @address 0x442720
 */
int16_t cache_files::download_poll(float *progress_out)
{
    uint32_t status_flags;
    uint32_t finished_signaled;
    uint32_t progress_ready;
    int16_t code;
    uint32_t raw;
    float progress;

    status_flags = globals().map_download->status_flags;
    if (globals().map_download->thread_busy != 0) {
        Sleep(0x10);
    }

    if (status_flags != 0 || globals().map_download->thread == 0) {
        if ((status_flags & 2) != 0) {
            *progress_out = 0.0f;
            return 1;
        }
        *progress_out = 0.0f;
        raw = (uint8_t)(~(uint8_t)status_flags);
        raw = raw >> 1;
        return (int16_t)(raw & 2);
    }

    if (globals().map_download->queued_file_count < 1) {
        *progress_out = 0.0f;
        return 3;
    }

    finished_signaled = WaitForSingleObject(globals().map_download->finished_event, 0);
    code = (int16_t)(4 - (finished_signaled != 0));

    progress_ready = WaitForSingleObject(globals().map_download->progress_event, 0);
    if (progress_ready == 0) {
        progress = globals().map_download->progress;
        if (progress < 0.0f) {
            *progress_out = 0.0f;
        } else if (1.0f < progress) {
            *progress_out = 1.0f;
        } else {
            *progress_out = progress;
        }
    }
    return code;
}

/**
 * Maps the raw download status to the three results request_map acts on: 2 when the download is
 * resolved (a reset also invalidates the slot's file handle), 0 when idle and 1 while it is still
 * running.
 *
 * @address 0x4434a0
 */
int16_t cache_files::download_status_get(float *progress_out, int32_t unaff_ecx)
{
    int16_t raw;

    raw = halo::cache::cache_files::download_poll(progress_out);
    switch (raw) {
    case 0:
    case 1:
        return 2;
    case 2:
        globals().cache_file_slots[globals().map_download_slot_index].file = (void *)0xffffffff;
        return 2;
    case 3:
        return 0;
    case 4:
        return 1;
    default:
        return (int16_t)unaff_ecx;
    }
}

/**
 * Asks the download thread to stop unless it has already finished. Does not wait for the thread.
 *
 * @address 0x443510
 */
void cache_files::download_stop()
{
    uint32_t finished_signaled;

    finished_signaled = WaitForSingleObject(globals().map_download->finished_event, 0);
    if (finished_signaled != 0) {
        SetEvent(globals().map_download->stop_event);
    }
}

/**
 * Opens maps\<name>.map under the map path prefix, reads its 0x800 byte header into header_out and
 * validates it (head and foot signatures, size range, name length, version 7). Returns 1 when the file
 * exists and the header is valid.
 *
 * @address 0x442bb0
 */
uint8_t cache_files::exists(char *name, cache_file_header *header_out)
{
    char path[256];
    void *file;
    uint32_t bytes_read;
    uint8_t valid;
    char *name_scan;

    valid = 0;
    sprintf(path, "%s%s%s.map", map_path_prefix, "maps\\", name);
    file = CreateFileA(path, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)((void *)0), 3, 0, (void *)0);
    if (file != (void *)0xffffffff) {
        if (ReadFile(file, header_out, k_cache_file_header_size, (LPDWORD)(&bytes_read), (LPOVERLAPPED)((void *)0)) != 0 &&
            bytes_read == k_cache_file_header_size &&
            header_out->head == k_cache_file_head_signature &&
            header_out->foot == k_cache_file_foot_signature &&
            header_out->file_size >= 0 && header_out->file_size < k_cache_file_maximum_size + 1) {
            name_scan = header_out->name;
            while (*name_scan != '\0') {
                name_scan++;
            }
            if ((uint32_t)(name_scan - header_out->name) < k_cache_file_name_length &&
                header_out->version == k_cache_file_version) {
                valid = 1;
            }
        }
        CloseHandle(file);
    }
    return valid;
}

/**
 * Chooses the least recently written cache file slot of the given category (single player 0-1,
 * multiplayer 3-5, ui 2) whose size limit can hold required_size, skipping the active slot. Returns -1
 * when no slot qualifies.
 *
 * An unknown category leaves the scan range uninitialised, as in the original.
 *
 * @address 0x4437b0
 */
int16_t cache_files::find_oldest_slot(cache_file_slot_category slot_category, int32_t required_size)
{
    int16_t start;
    int16_t end;
    int16_t i;
    int16_t best_slot;
    cache_file_slot *current;
    cache_file_slot *best;

    best_slot = -1;

    if (slot_category == _cache_file_slot_category_single_player) {
        start = 0;
        end = 1;
    } else if (slot_category == _cache_file_slot_category_multiplayer) {
        start = 3;
        end = 5;
    } else if (slot_category == _cache_file_slot_category_ui) {
        start = 2;
        end = 2;
    }

    if (start <= end) {
        i = start;
        current = &globals().cache_file_slots[start];
        best = current;
        do {
            if (globals().cache_file_index != i) {
                int32_t limit = halo::cache::cache_files::slot_size_limit(i);
                if (required_size < limit) {
                    if (best_slot != -1) {
                        int32_t best_limit = halo::cache::cache_files::slot_size_limit(best_slot);
                        if (best_limit <= limit &&
                            CompareFileTime((const FILETIME *)&best->last_write_time, (const FILETIME *)&current->last_write_time) < 1) {
                            goto next;
                        }
                    }
                    best_slot = i;
                    best = current;
                }
            }
next:
            i = i + 1;
            current = current + 1;
        } while (i <= end);
    }

    return best_slot;
}

/**
 * Returns the index of the open cache file slot whose header name equals filename (case-insensitive),
 * or -1.
 *
 * @address 0x443770
 */
int16_t cache_files::find_slot_by_name(char *filename)
{
    int16_t slot_index;

    slot_index = 0;
    do {
        if (_stricmp(filename, globals().cache_file_slots[slot_index].header.name) == 0) {
            return slot_index;
        }
        slot_index = slot_index + 1;
    } while (slot_index < k_cache_file_slot_count);
    return -1;
}

/**
 * Loads a map: flushes the texture and sound caches, ensures the 1 MB sound decode buffer, finds the
 * open slot for the map name, copies its header into the current header and validates it, reads the
 * tag data block to tag_data_base and publishes tag_header and tag_instances.
 *
 * Returns the scenario tag id, or k_datum_index_none when the slot header is invalid.
 *
 * @address 0x442290
 */
datum_index cache_files::load(char *path)
{
    char *slash;
    char *basename;
    cache_io_completion completion;
    uint8_t completion_flag;
    cache_file_header *slot_header;
    uint32_t *destination;
    uint32_t *source;
    int32_t i;

    slash = strrchr(path, '\\');
    basename = (slash != 0) ? slash + 1 : path;

    globals().texture_cache_entries->valid = 1;
    halo::memory::view(globals().texture_cache_entries)->delete_all();
    globals().sound_cache_entries->valid = 1;
    halo::memory::view(globals().sound_cache_entries)->delete_all();

    if (globals().sound_decode_buffer_size < 0x100000) {
        if (globals().sound_decode_buffer != 0) {
            GlobalFree(globals().sound_decode_buffer);
        }
        globals().sound_decode_buffer_size = 0x100000;
        globals().sound_decode_buffer = GlobalAlloc(0, globals().sound_decode_buffer_size);
    }

    globals().cache_file_index = halo::cache::cache_files::find_slot_by_name(basename);

    destination = (uint32_t *)globals().cache_io_requests;
    for (i = 0x1800; i != 0; i--) {
        *destination++ = 0;
    }

    slot_header = &globals().cache_file_slots[globals().cache_file_index].header;
    destination = (uint32_t *)&globals().cache_file_current_header;
    source = (uint32_t *)slot_header;
    for (i = 0x200; i != 0; i--) {
        *destination++ = *source++;
    }

    if (globals().cache_file_current_header.head != k_cache_file_head_signature ||
        globals().cache_file_current_header.foot != k_cache_file_foot_signature ||
        globals().cache_file_current_header.file_size < 0 ||
        globals().cache_file_current_header.file_size > k_cache_file_maximum_size ||
        strlen(globals().cache_file_current_header.name) >= k_cache_file_name_length ||
        globals().cache_file_current_header.version != k_cache_file_version) {
        return (datum_index)0xffffffff;
    }

    completion_flag = 0;
    completion.flag = &completion_flag;
    completion.procedure = 0;
    completion.data = 0;
    halo::cache::cache_io::request_new(&completion, globals().cache_file_current_header.tag_data_offset, globals().cache_file_current_header.tag_data_size, globals().tag_data_base, 1, 0);
    while (completion_flag == 0) {
        Sleep(0);
    }

    globals().tag_header = (cache_file_tag_header *)globals().tag_data_base;
    globals().tag_instances = globals().tag_header->tags;
    globals().cache_file_loaded = 1;
    halo::cache::model_vertex_buffers::load(globals().tag_header);
    return globals().tag_header->scenario_tag;
}

/**
 * Makes sure a cache file slot for name is open. Returns 1 when one already is; otherwise validates
 * the file header, picks a slot with find_oldest_slot, opens the file there and reads its header. On
 * failure it optionally reports a fatal error and requests a quit, then returns 0.
 *
 * @address 0x443360
 */
uint8_t cache_files::open_by_name(char *name, uint8_t report_fatal_error)
{
    char *slash;
    char *basename;
    int16_t slot_index;
    cache_file_header header;
    char path[264];
    void *file;
    uint32_t flags_and_attributes;
    uint32_t *destination;
    int32_t i;

    slash = strrchr(name, '\\');
    basename = (slash != 0) ? slash + 1 : name;

    slot_index = halo::cache::cache_files::find_slot_by_name(basename);
    if (slot_index != -1) {
        return 1;
    }

    if (halo::cache::cache_files::exists(basename, &header) == 0) {
        if (report_fatal_error != 0) {
            rasterizer_shader_file_name = name;
            shell_display_fatal_error_dialog(0x89, 0x7e, 1);
            interface_handle_quit_request();
        }
        return 0;
    }

    slot_index = halo::cache::cache_files::find_oldest_slot((cache_file_slot_category)header.map_type, header.file_size);

    destination = (uint32_t *)&globals().cache_file_slots[slot_index].header;
    for (i = 0x200; i != 0; i--) {
        *destination++ = 0;
    }

    sprintf(path, "%s%s%s.map", map_path_prefix, "maps\\", basename);

    flags_and_attributes = 0x48000080;
    if (os_platform == 0) {
        os_platform_identify();
    }
    if (os_platform < 3) {
        flags_and_attributes = 0x8000080;
    }

    file = CreateFileA(path, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)((void *)0), 4, flags_and_attributes, (void *)0);
    globals().cache_file_slots[slot_index].file = file;
    halo::cache::cache_files::slot_read_header(slot_index);
    return 1;
}

/**
 * Makes sure a map file is available: returns 1 when its slot is already open, otherwise resolves any
 * download in flight and starts a new open or download attempt. quit_on_fail decides whether a failure
 * requests a quit.
 *
 * @address 0x442640
 */
uint8_t cache_files::request_map(char *name, uint8_t quit_on_fail)
{
    char *slash;
    char *basename;
    short slot_index;
    uint8_t opened;
    int16_t status;
    float progress;

    slash = strrchr(name, '\\');
    basename = (slash != 0) ? slash + 1 : name;

    slot_index = halo::cache::cache_files::find_slot_by_name(basename);
    if (slot_index != -1) {
        return 1;
    }

    if (globals().map_download_in_progress != 0) {
        if (halo::cache::cache_files::download_matches(name) == 0) {
            halo::cache::cache_files::download_finish();
        }
        if (globals().map_download_in_progress != 0) {
            status = halo::cache::cache_files::download_status_get(&progress, 0);

            if (status == 2) {
                goto resolved;
            }
            if (status != 1) {
                return 0;
            }
            halo::cache::cache_files::download_finish();
            return 0;
        }
    }

    globals().map_download->thread_busy = 0;
    SetThreadPriority(globals().map_download->thread, 0);
    opened = halo::cache::cache_files::open_by_name(name, 0);
    if (opened != 0) {
        return 0;
    }

resolved:
    if (quit_on_fail == 0) {
        if (quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x23;
            quit_confirm_error_unknown_ae = 0;
            quit_confirm_error_modal = 0;
            quit_confirm_error_is_error = 0;
        }
        return 0;
    }
    interface_handle_quit_request();
    return 0;
}

/**
 * Reads and validates one slot's 0x800 byte header, synchronously below os_platform 3 and through the
 * overlapped read pair otherwise. A header that fails validation is cleared quietly; an unreadable
 * file reports a fatal error and marks the slot handle invalid.
 *
 * @address 0x4435e0
 */
void cache_files::slot_read_header(int32_t slot_index)
{
    cache_file_slot *slot;
    char path[256];
    uint8_t header_read_ok;
    uint32_t bytes_read;
    cache_io_request request;

    char *name_scan;

    slot = &globals().cache_file_slots[slot_index];
    sprintf(path, "%s\\cache%03d.map", profile_directory, slot_index);

    GetFileTime(slot->file, (LPFILETIME)&slot->last_write_time, (LPFILETIME)((void *)0), (LPFILETIME)((void *)0));

    header_read_ok = 0;
    request.completion.flag = &header_read_ok;
    request.completion.procedure = (void (*)(cache_io_completion *))0;
    request.completion.data = (void *)0;

    if (os_platform == 0) {
        os_platform_identify();
    }

    if (os_platform < 3) {
        if (SetFilePointer(slot->file, 0, (PLONG)((void *)0), 0) != 0xffffffff) {
            if (ReadFile(slot->file, &slot->header, k_cache_file_header_size, (LPDWORD)(&bytes_read), (LPOVERLAPPED)((void *)0)) != 0 &&
                bytes_read == k_cache_file_header_size) {
                goto validate_header;
            }
        }
    } else {
        halo::cache::cache_io::read_file_ex_retry((void *)ReadFileEx, slot->file, &slot->header, &request, k_cache_file_header_size, 0, (void *)&halo::cache::cache_io::completion_routine_stdcall);

        halo::cache::cache_io::wait_for_flag(&header_read_ok);
        if (header_read_ok != 0) {
validate_header:
            if (slot->header.head == k_cache_file_head_signature &&
                slot->header.foot == k_cache_file_foot_signature &&
                -1 < slot->header.file_size && slot->header.file_size < k_cache_file_maximum_size + 1) {
                name_scan = slot->header.name;
                while (*name_scan != '\0') {
                    name_scan++;
                }
                if ((uint32_t)(name_scan - slot->header.name) < k_cache_file_name_length &&
                    slot->header.version == k_cache_file_version) {
                    return;
                }
            }

            {
                uint8_t *zero;
                int32_t i;
                zero = (uint8_t *)&slot->header;
                for (i = 0x200; i != 0; i--) {
                    *(uint32_t *)zero = 0;
                    zero += 4;
                }
            }
            slot->last_write_time.low_date_time = 0;
            slot->last_write_time.high_date_time = 0;
            return;
        }
    }

    rasterizer_shader_file_name = path;
    shell_display_fatal_error_dialog(0x89, 0x7e, 1);
    slot->file = (void *)0xffffffff;
    return;
}

/**
 * Unloads the current map: disposes the sound cache, flushes the texture cache, closes and clears the
 * active slot, releases the structure bsp and model rendering resources and clears the loaded-map
 * globals.
 *
 * @address 0x442430
 */
void cache_files::unload()
{
    uint32_t *destination;
    int32_t i;

    halo::cache::sound_cache_manager::dispose();
    halo::memory::view(globals().texture_cache)->flush();
    globals().texture_cache_entries->valid = 0;

    if (globals().cache_file_index != -1) {
        halo::cache::cache_io::wait_all_requests();
        CloseHandle(globals().cache_file_slots[globals().cache_file_index].file);
        destination = (uint32_t *)&globals().cache_file_slots[globals().cache_file_index];
        for (i = 0x203; i != 0; i--) {
            *destination++ = 0;
        }
        globals().cache_file_index = -1;
    }

    halo::cache::structure_bsp_loader::dispose_material_vertex_buffers((ScenarioStructureBSPCompiledHeader *)globals().structure_bsp_data);
    halo::cache::model_vertex_buffers::dispose();
    globals().cache_file_loaded = 0;
    globals().tag_instances = 0;
}

/**
 * Reserves the fixed map memory range and the sound cache page region. When the map range is already
 * mapped it tries to name the culprit module and exits with a fatal error.
 *
 * @address 0x4448d0
 */
void cache_files::reserve_map_memory()
{
    char path_buffer[0x104];
    void *psapi_module;
    get_mapped_file_name_a_t get_mapped_file_name_a;
    const char *caption;

    globals().map_memory = (void *)0;
    globals().tag_data_base = (void *)0;
    globals().texture_cache_memory = (void *)0;
    globals().sound_cache_memory = (void *)0;

    globals().map_memory = VirtualAlloc((void *)k_map_memory_base, k_map_memory_size, 0x3000, 4);
    globals().tag_data_base = (void *)k_tag_data_base;
    globals().texture_cache_memory = VirtualAlloc((void *)0, 0x4000, 0x3000, 4);
    globals().sound_cache_memory = VirtualAlloc((void *)0, (uint32_t)sound_cache_size_megabytes << 0x14, 0x3000, 4);

    if (globals().map_memory == (void *)0) {
        memset(path_buffer, 0, sizeof(path_buffer));

        psapi_module = LoadLibraryA("Psapi.dll");
        if (psapi_module != (void *)0) {
            get_mapped_file_name_a = (get_mapped_file_name_a_t)GetProcAddress((HMODULE)psapi_module, "GetMappedFileNameA");
            if (get_mapped_file_name_a != (get_mapped_file_name_a_t)0) {
                get_mapped_file_name_a(GetCurrentProcess(), (void *)k_map_memory_base, path_buffer, 0x104);
            }
            FreeLibrary((HMODULE)psapi_module);
        }

        caption = path_buffer;
        if (path_buffer[0] == '\0') {
            caption = "Error";
        }
        MessageBoxA((HWND)((void *)0),
            "Cannot allocate required memory. Some other application has loaded where Halo needs to be located.",
            caption, 0);
        ExitProcess(1);
    }
    return;
}

/**
 * Returns the byte size limit of a cache file slot: 0x18000000 for slots 0 and 1, 0x02300000 for slot
 * 2 and 0x08000000 for slots 3 to 5.
 */
int32_t cache_files::slot_size_limit(int16_t slot_index)
{
    if (slot_index < 2) {
        return 0x18000000;
    }
    return (((2 < slot_index) - 1) & 0xfa300000) + 0x8000000;
}

} // namespace halo::cache
