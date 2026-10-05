/**
 * @file src/platform/thread_posix.cpp
 * halo::platform threads on POSIX (pthreads; Emscripten builds with -pthread) with Win32 semantics: suspended starts,
 * exit codes, recursive mutexes, manual / auto-reset events, waits with timeouts, and alertable waits that run the
 * thread's queued file_read_async completions first (src/platform/file_posix.cpp).
 */

#include "halo/platform/thread.hpp"
#include "halo/platform/file.hpp"
#include "posix_internal.hpp"

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>

namespace halo::platform {

namespace posix {

namespace {

constexpr uint32_t k_infinite = 0xffffffff;
constexpr uint32_t k_still_active = 0x103;
constexpr uint32_t k_max_completions = 64;

struct thread_object {
    handle_header header;
    pthread_mutex_t lock;
    pthread_cond_t changed;
    int references;  // the handle and the running thread
    bool suspended;
    bool finished;
    uint32_t exit_code;
    pthread_t thread;
    thread_procedure procedure;
    void (*detached_procedure)(void *);
    void *parameter;
    uint32_t id;
};

struct mutex_object {
    handle_header header;
    pthread_mutex_t mutex;
};

struct event_object {
    handle_header header;
    pthread_mutex_t lock;
    pthread_cond_t changed;
    bool manual_reset;
    bool set;
};

struct completion {
    completion_routine routine;
    uint32_t error;
    uint32_t bytes;
    void *request;
};

thread_local thread_object *t_self;
thread_local completion t_completions[k_max_completions];
thread_local uint32_t t_completion_count;
uint32_t g_next_thread_id = 1;
pthread_mutex_t g_id_lock = PTHREAD_MUTEX_INITIALIZER;

uint32_t new_thread_id()
{
    uint32_t id;

    pthread_mutex_lock(&g_id_lock);
    id = g_next_thread_id++;
    pthread_mutex_unlock(&g_id_lock);
    return id;
}

/** The absolute CLOCK_REALTIME deadline milliseconds from now. */
timespec deadline_after(uint32_t milliseconds)
{
    timespec deadline;

    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += static_cast<time_t>(milliseconds / 1000);
    deadline.tv_nsec += static_cast<long>(milliseconds % 1000) * 1000000;
    if (deadline.tv_nsec >= 1000000000) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000;
    }
    return deadline;
}

thread_object *new_thread_object()
{
    thread_object *thread = static_cast<thread_object *>(calloc(1, sizeof(thread_object)));

    thread->header.kind = kind_thread;
    pthread_mutex_init(&thread->lock, nullptr);
    pthread_cond_init(&thread->changed, nullptr);
    thread->references = 2;
    thread->exit_code = k_still_active;
    thread->id = new_thread_id();
    return thread;
}

void release_thread_object(thread_object *thread)
{
    bool last;

    pthread_mutex_lock(&thread->lock);
    last = --thread->references == 0;
    pthread_mutex_unlock(&thread->lock);
    if (last) {
        pthread_mutex_destroy(&thread->lock);
        pthread_cond_destroy(&thread->changed);
        thread->header.kind = static_cast<handle_kind>(0);
        free(thread);
    }
}

void finish_thread(thread_object *thread, uint32_t exit_code)
{
    pthread_mutex_lock(&thread->lock);
    thread->exit_code = exit_code;
    thread->finished = true;
    pthread_cond_broadcast(&thread->changed);
    pthread_mutex_unlock(&thread->lock);
    release_thread_object(thread);
}

void *thread_main(void *argument)
{
    thread_object *thread = static_cast<thread_object *>(argument);
    uint32_t exit_code = 0;

    t_self = thread;
    pthread_mutex_lock(&thread->lock);
    while (thread->suspended) {
        pthread_cond_wait(&thread->changed, &thread->lock);
    }
    pthread_mutex_unlock(&thread->lock);
    if (thread->procedure != nullptr) {
        exit_code = thread->procedure(thread->parameter);
    } else {
        thread->detached_procedure(thread->parameter);
    }
    finish_thread(thread, exit_code);
    return nullptr;
}

/** The calling thread's object (made on first use for threads this layer did not start, such as the main thread). */
thread_object *self()
{
    if (t_self == nullptr) {
        t_self = new_thread_object();
        t_self->references = 1;
        t_self->thread = pthread_self();
    }
    return t_self;
}

uint32_t wait_thread(thread_object *thread, uint32_t milliseconds)
{
    timespec deadline = deadline_after(milliseconds);
    uint32_t result = k_wait_signaled;

    pthread_mutex_lock(&thread->lock);
    while (!thread->finished) {
        if (milliseconds == k_infinite) {
            pthread_cond_wait(&thread->changed, &thread->lock);
        } else if (pthread_cond_timedwait(&thread->changed, &thread->lock, &deadline) == ETIMEDOUT) {
            result = thread->finished ? k_wait_signaled : k_wait_timeout;
            break;
        }
    }
    pthread_mutex_unlock(&thread->lock);
    return result;
}

uint32_t wait_event(event_object *event, uint32_t milliseconds)
{
    timespec deadline = deadline_after(milliseconds);
    uint32_t result = k_wait_signaled;

    pthread_mutex_lock(&event->lock);
    while (!event->set) {
        if (milliseconds == k_infinite) {
            pthread_cond_wait(&event->changed, &event->lock);
        } else if (milliseconds == 0 || pthread_cond_timedwait(&event->changed, &event->lock, &deadline) == ETIMEDOUT) {
            if (!event->set) {
                result = k_wait_timeout;
            }
            break;
        }
    }
    if (result == k_wait_signaled && !event->manual_reset) {
        event->set = false;
    }
    pthread_mutex_unlock(&event->lock);
    return result;
}

uint32_t wait_mutex(mutex_object *mutex, uint32_t milliseconds)
{
    if (milliseconds == k_infinite) {
        return pthread_mutex_lock(&mutex->mutex) == 0 ? k_wait_signaled : k_wait_failed;
    }
    if (milliseconds == 0) {
        return pthread_mutex_trylock(&mutex->mutex) == 0 ? k_wait_signaled : k_wait_timeout;
    }
    {
        timespec deadline = deadline_after(milliseconds);

        return pthread_mutex_timedlock(&mutex->mutex, &deadline) == 0 ? k_wait_signaled : k_wait_timeout;
    }
}

}  // namespace

void queue_completion(completion_routine routine, uint32_t error, uint32_t bytes, void *request)
{
    if (t_completion_count == k_max_completions) {
        run_completions();  // ponytail: a full queue runs early; the map streamer keeps far fewer reads in flight
    }
    t_completions[t_completion_count++] = {routine, error, bytes, request};
}

bool run_completions()
{
    uint32_t count = t_completion_count;

    if (count == 0) {
        return false;
    }
    // routines may queue more reads: run the batch that was queued, keep what they add
    for (uint32_t i = 0; i < count; i++) {
        completion c = t_completions[i];

        c.routine(c.error, c.bytes, c.request);
    }
    for (uint32_t i = count; i < t_completion_count; i++) {
        t_completions[i - count] = t_completions[i];
    }
    t_completion_count -= count;
    return true;
}

}  // namespace posix

using namespace posix;

thread_handle thread_create(uint32_t stack_size, thread_procedure procedure, void *parameter, uint32_t flags, uint32_t *thread_id)
{
    thread_object *thread = new_thread_object();
    pthread_attr_t attributes;

    thread->procedure = procedure;
    thread->parameter = parameter;
    thread->suspended = (flags & k_thread_create_suspended) != 0;
    pthread_attr_init(&attributes);
    if (stack_size != 0) {
        pthread_attr_setstacksize(&attributes, stack_size < 0x10000 ? 0x10000 : stack_size);
    }
    if (pthread_create(&thread->thread, &attributes, thread_main, thread) != 0) {
        pthread_attr_destroy(&attributes);
        free(thread);
        set_last_error(k_error_not_enough_memory);
        return nullptr;
    }
    pthread_attr_destroy(&attributes);
    pthread_detach(thread->thread);
    if (thread_id != nullptr) {
        *thread_id = thread->id;
    }
    return thread;
}

void thread_start_detached(void (*procedure)(void *), uint32_t stack_size, void *parameter)
{
    thread_object *thread = new_thread_object();
    pthread_attr_t attributes;

    thread->detached_procedure = procedure;
    thread->parameter = parameter;
    thread->references = 1;  // nobody keeps a handle
    pthread_attr_init(&attributes);
    if (stack_size != 0) {
        pthread_attr_setstacksize(&attributes, stack_size < 0x10000 ? 0x10000 : stack_size);
    }
    if (pthread_create(&thread->thread, &attributes, thread_main, thread) == 0) {
        pthread_detach(thread->thread);
    } else {
        free(thread);
    }
    pthread_attr_destroy(&attributes);
}

uint32_t thread_resume(thread_handle handle)
{
    thread_object *thread = static_cast<thread_object *>(handle);
    uint32_t previous;

    pthread_mutex_lock(&thread->lock);
    previous = thread->suspended ? 1 : 0;
    thread->suspended = false;
    pthread_cond_broadcast(&thread->changed);
    pthread_mutex_unlock(&thread->lock);
    return previous;
}

bool thread_set_priority(thread_handle, int32_t)
{
    return true;  // the browser has no thread priorities; the scheduler decides
}

bool thread_exit_code(thread_handle handle, uint32_t *exit_code)
{
    thread_object *thread = static_cast<thread_object *>(handle);

    pthread_mutex_lock(&thread->lock);
    *exit_code = thread->finished ? thread->exit_code : k_still_active;
    pthread_mutex_unlock(&thread->lock);
    return true;
}

bool thread_terminate(thread_handle, uint32_t)
{
    set_last_error(k_error_access_denied);
    return false;  // threads cannot be killed safely; the engine only gives up on slow hostname lookups, which then end on their own
}

void thread_exit(uint32_t exit_code)
{
    thread_object *thread = self();

    if (thread->references > 1 || thread->procedure != nullptr || thread->detached_procedure != nullptr) {
        finish_thread(thread, exit_code);
    }
    pthread_exit(nullptr);
}

thread_handle current_thread()
{
    return self();
}

uint32_t current_thread_id()
{
    return self()->id;
}

sync_handle mutex_create(bool initially_owned, const char *name)
{
    mutex_object *mutex = static_cast<mutex_object *>(calloc(1, sizeof(mutex_object)));
    pthread_mutexattr_t attributes;

    (void)name;  // ponytail: names are not shared between processes, so the single-instance check always passes
    mutex->header.kind = kind_mutex;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex->mutex, &attributes);
    pthread_mutexattr_destroy(&attributes);
    if (initially_owned) {
        pthread_mutex_lock(&mutex->mutex);
    }
    set_last_error(k_error_success);
    return mutex;
}

bool mutex_release(sync_handle handle)
{
    mutex_object *mutex = static_cast<mutex_object *>(handle);

    return mutex != nullptr && mutex->header.kind == kind_mutex && pthread_mutex_unlock(&mutex->mutex) == 0;
}

sync_handle event_create(bool manual_reset, bool initially_set, const char *name)
{
    event_object *event = static_cast<event_object *>(calloc(1, sizeof(event_object)));

    (void)name;
    event->header.kind = kind_event;
    pthread_mutex_init(&event->lock, nullptr);
    pthread_cond_init(&event->changed, nullptr);
    event->manual_reset = manual_reset;
    event->set = initially_set;
    return event;
}

bool event_set(sync_handle handle)
{
    event_object *event = static_cast<event_object *>(handle);

    if (event == nullptr || event->header.kind != kind_event) {
        return false;
    }
    pthread_mutex_lock(&event->lock);
    event->set = true;
    if (event->manual_reset) {
        pthread_cond_broadcast(&event->changed);
    } else {
        pthread_cond_signal(&event->changed);
    }
    pthread_mutex_unlock(&event->lock);
    return true;
}

uint32_t wait(void *handle, uint32_t milliseconds)
{
    handle_header *header = static_cast<handle_header *>(handle);

    if (header == nullptr) {
        set_last_error(k_error_invalid_handle);
        return k_wait_failed;
    }
    switch (header->kind) {
    case kind_thread: return wait_thread(reinterpret_cast<thread_object *>(header), milliseconds);
    case kind_event: return wait_event(reinterpret_cast<event_object *>(header), milliseconds);
    case kind_mutex: return wait_mutex(reinterpret_cast<mutex_object *>(header), milliseconds);
    default:
        set_last_error(k_error_invalid_handle);
        return k_wait_failed;
    }
}

uint32_t wait_alertable(void *handle, uint32_t milliseconds)
{
    // completions only come from this thread's own reads, so any are already queued when the wait starts
    if (run_completions()) {
        return k_wait_io_completion;
    }
    return wait(handle, milliseconds);
}

uint32_t sleep_alertable(uint32_t milliseconds)
{
    timespec duration = {static_cast<time_t>(milliseconds / 1000), static_cast<long>(milliseconds % 1000) * 1000000};

    if (run_completions()) {
        return k_wait_io_completion;
    }
    if (milliseconds == 0) {
        sched_yield();
    } else {
        nanosleep(&duration, nullptr);
    }
    return 0;
}

bool handle_close(void *handle)
{
    handle_header *header = static_cast<handle_header *>(handle);

    if (header == nullptr) {
        return false;
    }
    switch (header->kind) {
    case kind_thread:
        release_thread_object(reinterpret_cast<thread_object *>(header));
        return true;
    case kind_mutex: {
        mutex_object *mutex = reinterpret_cast<mutex_object *>(header);

        pthread_mutex_destroy(&mutex->mutex);
        mutex->header.kind = static_cast<handle_kind>(0);
        free(mutex);
        return true;
    }
    case kind_event: {
        event_object *event = reinterpret_cast<event_object *>(header);

        pthread_mutex_destroy(&event->lock);
        pthread_cond_destroy(&event->changed);
        event->header.kind = static_cast<handle_kind>(0);
        free(event);
        return true;
    }
    default:
        set_last_error(k_error_invalid_handle);
        return false;
    }
}

}  // namespace halo::platform
