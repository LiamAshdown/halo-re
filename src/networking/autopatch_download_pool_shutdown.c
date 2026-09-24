// autopatch_download_pool_shutdown  (Ghidra: autopatch_download_pool_shutdown, already named)
// address 0x576db0, size 164 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary; sets every in-use slot's close-requested
// byte (local_file+1, offset +0x11, the same byte autopatch_download_pool_tick tests), signals
// the worker thread to stop via autopatch_download_active_count, waits for it to exit, then
// closes both the thread and mutex handles autopatch_download_pool_initialize created.
// register convention: no register-passed arguments.
// UNSURE: FUN_0061bd40 (a foreign teardown call at the end).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern autopatch_download_slot autopatch_download_slots[2]; // 0x006ef93c
extern int32_t autopatch_download_active_count;              // 0x007227c8, UNSURE: stop signal, see autopatch_download_worker_thread.c
extern network_thread_record *autopatch_download_thread;     // 0x007227c4
extern network_mutex_record *autopatch_download_mutex;       // 0x007227c0

extern int32_t GetExitCodeThread(void *thread, uint32_t *exit_code);
extern int32_t CloseHandle(void *object);
extern int32_t FUN_0061bd40(void); // foreign, UNSURE

// Signals the download pool's worker thread to stop, waits for it to exit (STILL_ACTIVE ==
// 0x103), then closes its thread and mutex handles and resets the pool table slots.
uint32_t autopatch_download_pool_shutdown(void)
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
        while (GetExitCodeThread(autopatch_download_thread->handle, &exit_code) == 0) {
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

    return (uint32_t)FUN_0061bd40() & 0xffffff00;
}

#if 0
Original Ghidra decompilation (0x576db0):

uint autopatch_download_pool_shutdown(void)

{
  undefined4 *puVar1;
  undefined1 *puVar2;
  BOOL BVar3;
  uint uVar4;
  DWORD local_4;

  puVar2 = (undefined1 *)((int)&DAT_006ef94c + 1);
  do {
    if (((0x6ef94c < (int)puVar2) && ((int)puVar2 < 0x6ef975)) && (*(int *)(puVar2 + -0x11) != -1))
    {
      *puVar2 = 1;
    }
    puVar2 = puVar2 + 0x14;
  } while ((int)puVar2 < 0x6ef975);
  DAT_007227c8 = 1;
  do {
    do {
      BVar3 = GetExitCodeThread((HANDLE)*DAT_007227c4,&local_4);
      puVar1 = DAT_007227c4;
    } while (BVar3 == 0);
  } while (local_4 == 0x103);
  CloseHandle((HANDLE)*DAT_007227c4);
  *puVar1 = 0;
  *(undefined1 *)(puVar1 + 1) = 0;
  puVar1 = DAT_007227c0;
  CloseHandle((HANDLE)*DAT_007227c0);
  *(undefined1 *)(puVar1 + 1) = 0;
  *puVar1 = 0;
  *(undefined1 *)(puVar1 + 9) = 0;
  DAT_007227c4 = (undefined4 *)0x0;
  DAT_007227c0 = (undefined4 *)0x0;
  uVar4 = FUN_0061bd40();
  return uVar4 & 0xffffff00;
}
#endif
