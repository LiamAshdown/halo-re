// cache_io_thread_start  (Ghidra: cache_io_thread_start, already named)
// address 0x4438d0, size 107 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: types/cache.h globals list (cache_io_event 0x006ac498, cache_io_thread 0x006ac49c);
// creates the two worker procedures this batch also rewrites, cache_io_thread_proc_sync
// (0x443a10) and cache_io_thread_proc_async (0x443940).
// register convention: none, plain __cdecl with no parameters.

#include "win32.h"
#include "tags.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *cache_io_event;   // 0x006ac498, auto-reset event that wakes the worker
extern void *cache_io_thread;  // 0x006ac49c, 0x4000-byte stack
extern int32_t os_platform;    // 0x00721ef0

extern void os_platform_identify(void); // 0x5427e0

extern uint32_t cache_io_thread_proc_sync(void *unused);  // this module, cache_io_thread_proc_sync.c
extern uint32_t cache_io_thread_proc_async(void *unused); // this module, cache_io_thread_proc_async.c

// Creates the synchronization event and background worker thread that services the async
// cache-IO request queue, picking the synchronous or overlapped worker procedure depending on
// the OS platform (identified lazily on first use).
void cache_io_thread_start(void)
{
    uint32_t thread_id;

    cache_io_event = CreateEventA((LPSECURITY_ATTRIBUTES)((void *)0), 0, 0, (char *)0);

    if (os_platform == 0) {
        os_platform_identify();
    }

    if (os_platform < 3) {
        cache_io_thread = CreateThread((LPSECURITY_ATTRIBUTES)((void *)0), 0x4000, (LPTHREAD_START_ROUTINE)((void *)cache_io_thread_proc_sync), (void *)0, 0, (LPDWORD)(&thread_id));
        return;
    }

    cache_io_thread = CreateThread((LPSECURITY_ATTRIBUTES)((void *)0), 0x4000, (LPTHREAD_START_ROUTINE)((void *)cache_io_thread_proc_async), (void *)0, 0, (LPDWORD)((uint32_t *)0));
    return;
}

#if 0
Original Ghidra decompilation (0x4438d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl cache_io_thread_start(void)

{
  DWORD local_4;

  DAT_006ac498 = CreateEventA((LPSECURITY_ATTRIBUTES)0x0,0,0,(LPCSTR)0x0);
  if (DAT_00721ef0 == 0) {
    os_platform_identify();
  }
  if (DAT_00721ef0 < 3) {
    _DAT_006ac49c =
         CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x4000,cache_io_thread_proc_sync,(LPVOID)0x0,0,
                      &local_4);
    return;
  }
  _DAT_006ac49c =
       CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x4000,cache_io_thread_proc_async,(LPVOID)0x0,0,
                    (LPDWORD)0x0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
