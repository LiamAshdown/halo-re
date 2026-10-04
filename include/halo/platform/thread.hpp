/**
 * @file include/halo/platform/thread.hpp
 * Threads, mutexes, events and waits, independent of the operating system. The calls mirror the Win32 functions the
 * engine used (same results, wait return codes and priorities), with the Windows-only arguments dropped;
 * src/platform/thread_win32.cpp implements them with exactly those functions. The crash reporter's processes and
 * inheritable handles (src/shell/crash_reporter.cpp) and the account token checks (src/shell/system.cpp) stay
 * Windows-only and are not here.
 */
#pragma once

#include <cstdint>

namespace halo::platform {

/** A thread, mutex or event; on Windows the HANDLE itself, so engine records that store one keep their size. */
using thread_handle = void *;
using sync_handle = void *;

/** The start routine of a thread created with thread_create. */
using thread_procedure = uint32_t(__stdcall *)(void *parameter);

/** thread_create flag: the thread starts suspended until thread_resume (CREATE_SUSPENDED). */
inline constexpr uint32_t k_thread_create_suspended = 4;

/** Wait results (WAIT_OBJECT_0, WAIT_ABANDONED, WAIT_IO_COMPLETION, WAIT_TIMEOUT, WAIT_FAILED). */
inline constexpr uint32_t k_wait_signaled = 0;
inline constexpr uint32_t k_wait_abandoned = 0x80;
inline constexpr uint32_t k_wait_io_completion = 0xc0;
inline constexpr uint32_t k_wait_timeout = 0x102;
inline constexpr uint32_t k_wait_failed = 0xffffffff;

/** Starts a thread (CreateThread); stores its id in *thread_id when that is not null. Null on failure. */
thread_handle thread_create(uint32_t stack_size, thread_procedure procedure, void *parameter, uint32_t flags, uint32_t *thread_id);
/** Starts a thread whose handle nobody keeps (_beginthread). */
void thread_start_detached(void (*procedure)(void *), uint32_t stack_size, void *parameter);
uint32_t thread_resume(thread_handle thread);
bool thread_set_priority(thread_handle thread, int32_t priority);
/** The thread's exit code, 0x103 (STILL_ACTIVE) while it runs (GetExitCodeThread). */
bool thread_exit_code(thread_handle thread, uint32_t *exit_code);
bool thread_terminate(thread_handle thread, uint32_t exit_code);
[[noreturn]] void thread_exit(uint32_t exit_code);
thread_handle current_thread();
uint32_t current_thread_id();

sync_handle mutex_create(bool initially_owned, const char *name);
bool mutex_release(sync_handle mutex);
sync_handle event_create(bool manual_reset, bool initially_set, const char *name);
bool event_set(sync_handle event);

/** Waits for a thread, mutex or event (WaitForSingleObject); returns one of the k_wait_ results. */
uint32_t wait(void *handle, uint32_t milliseconds);
/** As wait, also returning k_wait_io_completion after running this thread's queued file_read_async completions. */
uint32_t wait_alertable(void *handle, uint32_t milliseconds);
/** Sleeps, returning early (k_wait_io_completion) to run this thread's queued file_read_async completions (SleepEx). */
uint32_t sleep_alertable(uint32_t milliseconds);

/** Closes a thread, mutex or event handle (files use file_close). */
bool handle_close(void *handle);

}  // namespace halo::platform
