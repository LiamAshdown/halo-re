#include "tags.h"

#include "halo/cache/cache.hpp"

#include "win32.h"
#include "memory.h"
#include "halo/cache/globals.hpp"
#include "halo/cache/api.hpp"
#include "halo/shell/api.hpp"

typedef int32_t (*read_file_ex_procedure)(void *file, void *buffer, uint32_t bytes_to_read, cache_io_request *overlapped, void *completion_routine);

namespace halo::cache {

/**
 * APC completion routine for overlapped reads. overlapped is the cache_io_request itself: runs the
 * request's optional completion procedure with its embedded completion record, then raises the
 * completion flag.
 *
 * @address 0x443b00
 */
void cache_io::completion_routine(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped)
{
    if (overlapped->completion.procedure != (void *)0) {
        overlapped->completion.procedure(&overlapped->completion);
        *overlapped->completion.flag = 1;
        return;
    }
    *overlapped->completion.flag = 1;
    return;
}

/**
 * The Win32 completion routine for the first header read of a cache file: Windows calls it with three stack
 * arguments and the routine pops them (the original at 0x443b00 ends ret 12), so this adapter is stdcall and
 * forwards to completion_routine.
 */
void __stdcall cache_io::completion_routine_stdcall(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped)
{
    completion_routine(error_code, bytes_transferred, overlapped);
}

/**
 * Issues one overlapped ReadFileEx for request and retries until it succeeds: clears the OVERLAPPED
 * prefix, stamps the offset, drains queued APCs with an alertable SleepEx(0) and clears the last error
 * before each attempt.
 */
void cache_io::read_file_ex_retry(void *read_file_ex, void *file, void *buffer, cache_io_request *request, uint32_t size, uint32_t offset, void *completion_routine)
{
    read_file_ex_procedure read_procedure;
    int32_t started;

    read_procedure = (read_file_ex_procedure)read_file_ex;

    request->internal = 0;
    request->internal_high = 0;
    request->offset = 0;
    request->offset_high = 0;
    request->event = (void *)0;

    request->offset = offset;
    request->offset_high = 0;
    request->event = (void *)0;

    SleepEx(0, 1);
    SetLastError(0);
    started = read_procedure(file, buffer, size, request, completion_routine);

    while (started == 0) {
        GetLastError();
        SleepEx(0, 1);
        SetLastError(0);
        started = read_procedure(file, buffer, size, request, completion_routine);
    }
}

/**
 * Marks an asynchronous cache read finished: raises the caller's completion flag and frees the request
 * slot for the worker thread.
 *
 * @address 0x443ae0
 */
void cache_io::request_completion_routine(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped)
{
    *overlapped->completion.flag = 1;
    overlapped->pending = 0;
    overlapped->started = 0;
}

/**
 * The Win32 completion routine passed to ReadFileEx for queued requests; stdcall adapter for
 * request_completion_routine.
 */
void __stdcall cache_io::request_completion_routine_stdcall(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped)
{
    request_completion_routine(error_code, bytes_transferred, overlapped);
}

/**
 * Returns the index of the first request queue slot whose pending flag is clear, re-scanning until one
 * frees up.
 *
 * @address 0x443270
 */
int16_t cache_io::request_find_free_slot()
{
    uint8_t retried;
    short slot_index;

    retried = 0;
    for (;;) {
        for (slot_index = 0; slot_index < (short)k_cache_io_request_count; slot_index++) {
            if (globals().cache_io_requests[slot_index].pending == 0) {
                return slot_index;
            }
        }
        if (!retried) {
            retried = 1;
        }
    }
}

/**
 * Claims a free slot of the 0x200 entry request queue, fills it as a pending read of size bytes at
 * offset into destination from the given data file, copies the completion record and wakes the worker
 * thread. Returns the slot index.
 *
 * @address 0x442b20
 */
int16_t cache_io::request_new(cache_io_completion *completion, int32_t offset, uint32_t size, void *destination, uint8_t priority, uint8_t data_file_index)
{
    short slot_index;
    cache_io_request *request;

    slot_index = halo::cache::cache_io::request_find_free_slot();
    request = &globals().cache_io_requests[slot_index];

    *completion->flag = 0;
    request->internal = 0;
    request->internal_high = 0;
    request->offset = 0;
    request->offset_high = 0;
    request->event = 0;
    request->size = size;
    request->offset = offset;
    request->event = 0;
    request->offset_high = 0;
    request->destination = destination;
    request->priority = priority;
    request->pending = 1;
    request->started = 0;
    request->data_file_index = data_file_index;
    request->completion = *completion;

    SetEvent(globals().cache_io_event);
    return slot_index;
}

/**
 * Completion procedure installed for every sound format except xbox adpcm: recovers the
 * SoundPermutation from the completion record's sound cache entry and decodes it.
 *
 * @address 0x443e00
 */
void cache_io::sound_decode_thunk(cache_io_completion *record)
{
    sound_cache_entry *entry;

    entry = (sound_cache_entry *)record->data;
    halo::cache::sound_cache_manager::decode_permutation(entry->permutation);
}

/**
 * Worker thread body for overlapped IO. Waits alertably on the IO event, then repeatedly starts the
 * pending request with the lowest (priority, offset) through read_file_ex_retry until none is left.
 *
 * @address 0x443940
 */
uint32_t cache_io::thread_proc_async(void *parameter)
{
    void *read_function;
    int32_t i;
    cache_io_request *best;
    cache_io_request *candidate;
    uint32_t wait_result;
    void *file_handle;
    data_file *source;

    read_function = (void *)ReadFileEx;
    for (;;) {
        do {
            wait_result = WaitForSingleObjectEx(globals().cache_io_event, 0xffffffff, 1);
        } while (wait_result == 0xc0);

        for (;;) {
            best = (cache_io_request *)0;
            for (i = 0; i < k_cache_io_request_count; i++) {
                candidate = &globals().cache_io_requests[i];
                if (candidate->pending != 0 && candidate->started == 0) {
                    if (best == (cache_io_request *)0 ||
                        (candidate->priority < best->priority && candidate->offset < best->offset)) {
                        best = candidate;
                    }
                }
            }

            if (best == (cache_io_request *)0) {
                break;
            }

            file_handle = globals().cache_file_slots[globals().cache_file_index].file;
            if (best->data_file_index != 0) {
                source = (data_file *)0;
                if (best->data_file_index == 1) {
                    source = &globals().bitmaps_data_file;
                } else if (best->data_file_index == 2) {
                    source = &globals().sounds_data_file;
                }
                file_handle = source->file;
            }

            best->started = 1;
            halo::cache::cache_io::read_file_ex_retry(read_function, file_handle, best->destination, best, best->size, best->offset, (void *)&cache_io::request_completion_routine_stdcall);
        }
    }
}

/**
 * Worker thread body for synchronous IO. Waits on the IO event, then services the pending request with
 * the lowest (priority, offset) with SetFilePointer and ReadFile, signals its completion flag and
 * clears pending and started.
 *
 * @address 0x443a10
 */
uint32_t cache_io::thread_proc_sync(void *parameter)
{
    int32_t i;
    cache_io_request *best;
    cache_io_request *candidate;
    void *file_handle;
    data_file *source;
    uint32_t bytes_read;

    for (;;) {
        WaitForSingleObject(globals().cache_io_event, 0xffffffff);

        for (;;) {
            best = (cache_io_request *)0;
            for (i = 0; i < k_cache_io_request_count; i++) {
                candidate = &globals().cache_io_requests[i];
                if (candidate->pending != 0 && candidate->started == 0) {
                    if (best == (cache_io_request *)0 ||
                        (candidate->priority < best->priority && candidate->offset < best->offset)) {
                        best = candidate;
                    }
                }
            }

            if (best == (cache_io_request *)0) {
                break;
            }

            file_handle = globals().cache_file_slots[globals().cache_file_index].file;
            if (best->data_file_index != 0) {
                source = (data_file *)0;
                if (best->data_file_index == 1) {
                    source = &globals().bitmaps_data_file;
                } else if (best->data_file_index == 2) {
                    source = &globals().sounds_data_file;
                }
                file_handle = source->file;
            }

            if (SetFilePointer(file_handle, (int32_t)best->offset, (PLONG)((void *)0), 0) != 0xffffffff) {
                ReadFile(file_handle, best->destination, best->size, (LPDWORD)(&bytes_read), (LPOVERLAPPED)((void *)0));
            }

            *best->completion.flag = 1;
            best->pending = 0;
            best->started = 0;
        }
    }
}

/**
 * Creates the IO event and the worker thread, choosing the synchronous or overlapped procedure from
 * the OS platform.
 *
 * @address 0x4438d0
 */
void cache_io::thread_start()
{
    uint32_t thread_id;

    globals().cache_io_event = CreateEventA((LPSECURITY_ATTRIBUTES)((void *)0), 0, 0, (char *)0);

    if (globals().os_platform == 0) {
        halo::shell::os_platform_identify();
    }

    if (globals().os_platform < 3) {
        globals().cache_io_thread = CreateThread((LPSECURITY_ATTRIBUTES)((void *)0), 0x4000, (LPTHREAD_START_ROUTINE)((void *)&cache_io::thread_proc_sync), (void *)0, 0, (LPDWORD)(&thread_id));
        return;
    }

    globals().cache_io_thread = CreateThread((LPSECURITY_ATTRIBUTES)((void *)0), 0x4000, (LPTHREAD_START_ROUTINE)((void *)&cache_io::thread_proc_async), (void *)0, 0, (LPDWORD)((uint32_t *)0));
    return;
}

/**
 * Waits, spinning on Sleep(0), until every request queue slot has cleared its pending flag.
 *
 * @address 0x4432b0
 */
void cache_io::wait_all_requests()
{
    int32_t slot_index;

    for (slot_index = 0; slot_index < (int32_t)k_cache_io_request_count; slot_index++) {
        while (globals().cache_io_requests[slot_index].pending != 0) {
            Sleep(0);
        }
    }
}

/**
 * Blocks in alertable five second waits until the completion APC sets *flag. Returns the flag's final
 * value.
 *
 * @address 0x442ce0
 */
uint8_t cache_io::wait_for_flag(uint8_t *flag)
{
    uint32_t wait_result;

    if (*flag != 0) {
        return *flag;
    }

    do {
        wait_result = SleepEx(5000, 1);
        if (wait_result != 0xc0) {
            break;
        }
    } while (*flag == 0);

    return *flag;
}

} // namespace halo::cache
