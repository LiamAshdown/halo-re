/**
 * @file src/platform/thread_win32.cpp
 * halo::platform threads on Windows: the same Win32 calls the engine made directly.
 */

#include "halo/platform/thread.hpp"

#include "win32.h"
#include <process.h>

namespace halo::platform {

thread_handle thread_create(uint32_t stack_size, thread_procedure procedure, void *parameter, uint32_t flags, uint32_t *thread_id)
{
    return CreateThread(nullptr, stack_size, reinterpret_cast<LPTHREAD_START_ROUTINE>(procedure), parameter, flags, reinterpret_cast<DWORD *>(thread_id));
}

void thread_start_detached(void (*procedure)(void *), uint32_t stack_size, void *parameter)
{
    _beginthread(procedure, stack_size, parameter);
}

uint32_t thread_resume(thread_handle thread)
{
    return ResumeThread(thread);
}

bool thread_set_priority(thread_handle thread, int32_t priority)
{
    return SetThreadPriority(thread, priority) != 0;
}

bool thread_exit_code(thread_handle thread, uint32_t *exit_code)
{
    return GetExitCodeThread(thread, reinterpret_cast<DWORD *>(exit_code)) != 0;
}

bool thread_terminate(thread_handle thread, uint32_t exit_code)
{
    return TerminateThread(thread, exit_code) != 0;
}

void thread_exit(uint32_t exit_code)
{
    ExitThread(exit_code);
}

thread_handle current_thread()
{
    return GetCurrentThread();
}

uint32_t current_thread_id()
{
    return GetCurrentThreadId();
}

sync_handle mutex_create(bool initially_owned, const char *name)
{
    return CreateMutexA(nullptr, initially_owned ? TRUE : FALSE, name);
}

bool mutex_release(sync_handle mutex)
{
    return ReleaseMutex(mutex) != 0;
}

sync_handle event_create(bool manual_reset, bool initially_set, const char *name)
{
    return CreateEventA(nullptr, manual_reset ? TRUE : FALSE, initially_set ? TRUE : FALSE, name);
}

bool event_set(sync_handle event)
{
    return SetEvent(event) != 0;
}

uint32_t wait(void *handle, uint32_t milliseconds)
{
    return WaitForSingleObject(handle, milliseconds);
}

uint32_t wait_alertable(void *handle, uint32_t milliseconds)
{
    return WaitForSingleObjectEx(handle, milliseconds, TRUE);
}

uint32_t sleep_alertable(uint32_t milliseconds)
{
    return SleepEx(milliseconds, TRUE);
}

bool handle_close(void *handle)
{
    return CloseHandle(handle) != 0;
}

}  // namespace halo::platform
