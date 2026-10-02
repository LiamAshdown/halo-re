// autopatch_download_pool_initialize  (Ghidra: autopatch_download_pool_initialize, already named)
// address 0x576c30, size 380 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/networking_types_notes.md "network_mutex_record (0x28) /
// network_thread_record (0x08)": "autopatch_download_pool_initialize (0x576c30) inlines both
// allocators and confirms the same offsets independently." String "mutex_%ld" confirms the
// mutex-name format matches mutex_create's own.
// register convention: no register-passed arguments.
// UNSURE: ghttpStartup (a foreign initialization call before the mutex/thread setup).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern autopatch_download_slot autopatch_download_slots[2]; // 0x006ef93c
extern network_mutex_record network_mutex_table[k_network_mutex_table_count];   // 0x006f0db0
extern int32_t network_mutex_name_counter;                                      // 0x006f0cac
extern network_thread_record network_thread_table[k_network_thread_table_count]; // 0x006f0cb0
extern network_mutex_record *autopatch_download_mutex;   // 0x007227c0
extern network_thread_record *autopatch_download_thread; // 0x007227c4
extern uint8_t autopatch_download_pool_stop;              // 0x007227bc, set to 1 once init has run
extern uint8_t autopatch_download_active_count;           // 0x007227c8, UNSURE: used as a stop signal, see autopatch_download_worker_thread.c

extern void ghttpStartup(void); // foreign, UNSURE

extern int32_t _snprintf(char *buffer, uint32_t count, const char *format, ...);
extern uint32_t autopatch_download_worker_thread(void); // 0x576b80, this module

// VERIFIED against disassembly 0x576c30..0x576db0 (2026-09-30): slot init (5 dwords, request id -1), the 32 entry 0x28 byte
//   mutex scan (in_use at +0x24, name at +4), the 32 entry 8 byte thread scan, the CreateThread/SetThreadPriority/ResumeThread
//   sequence and the cleanup match. Fixed: 0x7227c8 (autopatch_download_active_count) is a BYTE everywhere in the original (the
//   4-byte C store clobbered 0x7227c9..0x7227cb), and the mutex name uses the CRT _snprintf.
// Initializes the two-slot asynchronous download table, then inline-allocates a named mutex
// (mirroring mutex_create) and a suspended worker thread (mirroring network_thread_create),
// resuming it on success. Returns 1 once the pool is fully up, 0 if the mutex or thread could
// not be created (the pool is left initialized but idle either way).
uint8_t autopatch_download_pool_initialize(void)
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
        _snprintf(mutex_slot->name, 0x20, "mutex_%ld", name_index); // 0x576cbb: CRT _snprintf (0x623a2d)
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

#if 0
Original Ghidra decompilation (0x576c30):

undefined1 autopatch_download_pool_initialize(void)

{
  bool bVar1;
  undefined4 *puVar2;
  char *pcVar3;
  HANDLE pvVar4;
  BOOL BVar5;
  DWORD DVar6;
  int iVar7;
  DWORD local_4;

  puVar2 = &DAT_006ef93c;
  do {
    *puVar2 = 0;
    puVar2[1] = 0;
    puVar2[2] = 0;
    puVar2[3] = 0;
    puVar2[4] = 0;
    *puVar2 = 0xffffffff;
    puVar2 = puVar2 + 5;
  } while ((int)puVar2 < 0x6ef964);
  FUN_0061bd00();
  bVar1 = false;
  puVar2 = (undefined4 *)0x0;
  iVar7 = 0;
  pcVar3 = &DAT_006f0dd4;
  do {
    if (*pcVar3 == '\0') {
      iVar7 = iVar7 * 0x28;
      puVar2 = (undefined4 *)(iVar7 + 0x6f0db0);
      *(undefined1 *)(iVar7 + 0x6f0db4) = 0;
      *puVar2 = 0;
      (&DAT_006f0dd4)[iVar7] = 1;
      break;
    }
    pcVar3 = pcVar3 + 0x28;
    iVar7 = iVar7 + 1;
  } while ((int)pcVar3 < 0x6f12d4);
  iVar7 = DAT_006f0cac;
  if (puVar2 != (undefined4 *)0x0) {
    DAT_006f0cac = DAT_006f0cac + 1;
    __snprintf((char *)(puVar2 + 1),0x20,"mutex_%ld",iVar7);
    pvVar4 = CreateMutexA((LPSECURITY_ATTRIBUTES)0x0,0,(LPCSTR)0x0);
    *puVar2 = pvVar4;
    if (pvVar4 == (HANDLE)0x0) {
      puVar2 = (undefined4 *)0x0;
    }
    else {
      bVar1 = true;
    }
  }
  DAT_007227c0 = puVar2;
  if (bVar1) {
    iVar7 = 0;
    do {
      if ((&DAT_006f0cb4)[iVar7 * 8] == '\0') {
        puVar2 = (undefined4 *)(iVar7 * 8 + 0x6f0cb0);
        *puVar2 = 0;
        (&DAT_006f0cb4)[iVar7 * 8] = 1;
        pvVar4 = CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x4000,autopatch_download_worker_thread,
                              (LPVOID)0x0,4,&local_4);
        *puVar2 = pvVar4;
        DAT_007227c4 = puVar2;
        if (pvVar4 != (HANDLE)0x0) {
          BVar5 = SetThreadPriority(pvVar4,0);
          if ((BVar5 != 0) && (DVar6 = ResumeThread((HANDLE)*puVar2), DVar6 != 0xffffffff)) {
            DAT_007227bc = 1;
            DAT_007227c8 = 0;
            return 1;
          }
          CloseHandle((HANDLE)*puVar2);
        }
        break;
      }
      iVar7 = iVar7 + 1;
    } while (iVar7 < 0x20);
    puVar2 = DAT_007227c0;
    CloseHandle((HANDLE)*DAT_007227c0);
    *(undefined1 *)(puVar2 + 1) = 0;
    *puVar2 = 0;
    *(undefined1 *)(puVar2 + 9) = 0;
    DAT_007227c0 = (undefined4 *)0x0;
    DAT_007227c4 = (undefined4 *)0x0;
  }
  DAT_007227bc = 1;
  DAT_007227c8 = 0;
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
