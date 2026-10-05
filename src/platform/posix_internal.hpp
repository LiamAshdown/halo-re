#pragma once

/**
 * @file src/platform/posix_internal.hpp
 * Shared pieces of the POSIX halo::platform implementation (src/platform/*_posix.cpp; Emscripten and Linux): handle
 * objects, the per-thread queue of file_read_async completions (Win32 APCs) and Win32 error codes.
 */

#include <stdint.h>

namespace halo::platform::posix {

/** First member of every object behind a platform handle. */
enum handle_kind : uint32_t {
    kind_file = 0x46494c45,    // "FILE"
    kind_find = 0x46494e44,    // "FIND"
    kind_thread = 0x54485244,  // "THRD"
    kind_mutex = 0x4d555458,   // "MUTX"
    kind_event = 0x45564e54,   // "EVNT"
};

struct handle_header {
    handle_kind kind;
};

/** A completion routine (LPOVERLAPPED_COMPLETION_ROUTINE). */
using completion_routine = void(__stdcall *)(uint32_t error, uint32_t bytes, void *request);

/** Queues a completion to run on the calling thread at its next alertable wait. */
void queue_completion(completion_routine routine, uint32_t error, uint32_t bytes, void *request);

/** Runs the calling thread's queued completions; returns whether there were any. */
bool run_completions();

/** Sets the calling thread's last error (Win32 code) from errno. */
void set_last_error_from_errno();

/** Win32 error codes the engine looks at. */
constexpr uint32_t k_error_success = 0;
constexpr uint32_t k_error_file_not_found = 2;
constexpr uint32_t k_error_path_not_found = 3;
constexpr uint32_t k_error_access_denied = 5;
constexpr uint32_t k_error_invalid_handle = 6;
constexpr uint32_t k_error_not_enough_memory = 8;
constexpr uint32_t k_error_no_more_files = 18;
constexpr uint32_t k_error_handle_eof = 38;
constexpr uint32_t k_error_file_exists = 80;
constexpr uint32_t k_error_invalid_parameter = 87;
constexpr uint32_t k_error_disk_full = 112;
constexpr uint32_t k_error_dir_not_empty = 145;
constexpr uint32_t k_error_already_exists = 183;

/**
 * An engine path as a native one: backslashes become slashes, and when the path does not exist as written each
 * component is matched case-insensitively (the engine says "maps\\" where the install has "MAPS/"). The final
 * component is kept as written when no match exists, so the result also names files about to be created.
 */
void native_path(const char *path, char *out, uint32_t size);

}  // namespace halo::platform::posix
