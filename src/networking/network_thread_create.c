// network_thread_create  (Ghidra: network_thread_create, already named)
// address 0x440460, size 163 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/networking_types_notes.md "network_mutex_record (0x28) /
// network_thread_record (0x08)": the loop strides `network_thread_table` by
// sizeof(network_thread_record), and the header already documents the flag byte mapping
// ("bit1 -> -1, bit2 -> +1, else 0").
// register convention: __cdecl, all four arguments recognized by Ghidra (flags, entry point,
// thread argument, out-handle pointer).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_thread_record network_thread_table[k_network_thread_table_count]; // 0x006f0cb0


// Finds a free slot in the static worker-thread table, creates a suspended thread with a
// 0x4000-byte stack, applies a priority derived from the low bits of flags (bit1 set ->
// below normal, else bit2 set -> above normal, else normal), resumes it, and reports the
// handle through out_handle. Returns 1 on success, 0 if the table is full or any Win32 call
// fails (the handle is closed and the table slot left marked in use on failure).
int32_t network_thread_create(uint8_t flags, void *start_address, void *parameter,
                               network_thread_record **out_handle)
{
    uint32_t i;
    network_thread_record *slot;
    uint32_t thread_id;
    int32_t priority;

    for (i = 0; i < k_network_thread_table_count; i++) {
        if (network_thread_table[i].in_use == 0) {
            slot = &network_thread_table[i];
            slot->handle = 0;
            slot->in_use = 1;
            slot->handle = CreateThread(0, 0x4000, (LPTHREAD_START_ROUTINE)start_address, parameter, 4, (LPDWORD)&thread_id);
            *out_handle = slot;
            if (slot->handle != 0) {
                if ((flags & 2) == 0) {
                    priority = (flags & 4) != 0 ? 1 : 0;
                } else {
                    priority = -1;
                }
                if (SetThreadPriority(slot->handle, priority) != 0) {
                    if (ResumeThread(slot->handle) != 0xffffffff) {
                        return 1;
                    }
                }
                CloseHandle(slot->handle);
            }
            return 0;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x440460):

undefined4
network_thread_create
          (byte param_1,LPTHREAD_START_ROUTINE param_2,LPVOID param_3,undefined4 *param_4)

{
  undefined4 *puVar1;
  int iVar2;
  HANDLE pvVar3;
  BOOL BVar4;
  DWORD DVar5;
  DWORD local_4;

  iVar2 = 0;
  do {
    if ((&DAT_006f0cb4)[iVar2 * 8] == '\0') {
      puVar1 = (undefined4 *)(iVar2 * 8 + 0x6f0cb0);
      *puVar1 = 0;
      (&DAT_006f0cb4)[iVar2 * 8] = 1;
      pvVar3 = CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x4000,param_2,param_3,4,&local_4);
      *puVar1 = pvVar3;
      *param_4 = puVar1;
      if ((HANDLE)*puVar1 != (HANDLE)0x0) {
        iVar2 = 0;
        if ((param_1 & 2) == 0) {
          if ((param_1 & 4) != 0) {
            iVar2 = 1;
          }
        }
        else {
          iVar2 = -1;
        }
        BVar4 = SetThreadPriority((HANDLE)*puVar1,iVar2);
        if (BVar4 != 0) {
          DVar5 = ResumeThread((HANDLE)*puVar1);
          if (DVar5 != 0xffffffff) {
            return 1;
          }
        }
        CloseHandle((HANDLE)*puVar1);
      }
      return 0;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x20);
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
