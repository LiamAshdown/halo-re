// cache_io_thread_proc_sync  (Ghidra: cache_io_thread_proc_sync, already named)
// address 0x443a10, size 194 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: same request-queue scan as cache_io_thread_proc_async (0x443940), field offsets
// confirmed there; here the completion is signalled directly through
// cache_io_request::completion.flag (offset 0x24), matching types/cache.h cache_io_completion.
// register convention: CreateThread's LPTHREAD_START_ROUTINE parameter, unused.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "win32.h"
#include "tags.h"
#include "cache.h"

extern void *cache_io_event;                    // 0x006ac498
extern cache_io_request *cache_io_requests;      // 0x006ac4a0, 0x200 entries
extern int16_t cache_file_index;                 // 0x006ac494, active slot, -1 when none
extern cache_file_slot cache_file_slots[k_cache_file_slot_count]; // 0x006a9428
extern data_file sounds_data_file;               // 0x006ac4a8
extern data_file bitmaps_data_file;              // 0x006ac4e8


// blam-cc: unused CreateThread parameter
// Background IO thread procedure that services queued cache-file read requests using blocking
// SetFilePointer/ReadFile. Waits on cache_io_event; each time it wakes, scans the whole request
// queue for pending-but-not-started requests and services the one with the lowest
// (priority, offset) pair, repeating until none remain, then waits again. Signals completion by
// writing 1 through the request's own completion flag pointer, then clears pending and started.
uint32_t cache_io_thread_proc_sync(void *parameter)
{
    int32_t i;
    cache_io_request *best;
    cache_io_request *candidate;
    void *file_handle;
    data_file *source;
    uint32_t bytes_read;

    for (;;) {
        WaitForSingleObject(cache_io_event, 0xffffffff);

        for (;;) {
            best = (cache_io_request *)0;
            for (i = 0; i < k_cache_io_request_count; i++) {
                candidate = &cache_io_requests[i];
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

            file_handle = cache_file_slots[cache_file_index].file;
            if (best->data_file_index != 0) {
                source = (data_file *)0;
                if (best->data_file_index == 1) {
                    source = &bitmaps_data_file;
                } else if (best->data_file_index == 2) {
                    source = &sounds_data_file;
                }
                file_handle = source->file;
            }

            if (SetFilePointer(file_handle, (int32_t)best->offset, (void *)0, 0) != 0xffffffff) {
                ReadFile(file_handle, best->destination, best->size, &bytes_read, (void *)0);
            }

            *best->completion.flag = 1;
            best->pending = 0;
            best->started = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x443a10):

void cache_io_thread_proc_sync(void)

{
  char cVar1;
  char *pcVar2;
  DWORD DVar3;
  int iVar4;
  undefined4 *puVar5;
  char *pcVar6;
  HANDLE hFile;
  DWORD local_4;

  do {
    WaitForSingleObject(DAT_006ac498,0xffffffff);
    while( true ) {
      pcVar6 = (char *)0x0;
      pcVar2 = (char *)(DAT_006ac4a0 + 0x1e);
      iVar4 = 0x200;
      do {
        if (((pcVar2[-1] != '\0') && (*pcVar2 == '\0')) &&
           ((pcVar6 == (char *)0x0 ||
            (((byte)pcVar2[-2] < (byte)pcVar6[0x1c] &&
             (*(uint *)(pcVar2 + -0x16) < *(uint *)(pcVar6 + 8))))))) {
          pcVar6 = pcVar2 + -0x1e;
        }
        pcVar2 = pcVar2 + 0x30;
        iVar4 = iVar4 + -1;
      } while (iVar4 != 0);
      if (pcVar6 == (char *)0x0) break;
      hFile = *(HANDLE *)(&DAT_006a9428 + DAT_006ac494 * 0x80c);
      cVar1 = pcVar6[0x20];
      if (cVar1 != '\0') {
        puVar5 = (undefined4 *)0x0;
        if (cVar1 == '\x01') {
          puVar5 = &DAT_006ac4e8;
        }
        else if (cVar1 == '\x02') {
          puVar5 = &DAT_006ac4a8;
        }
        hFile = (HANDLE)puVar5[0xf];
      }
      DVar3 = SetFilePointer(hFile,*(LONG *)(pcVar6 + 8),(PLONG)0x0,0);
      if (DVar3 != 0xffffffff) {
        ReadFile(hFile,*(LPVOID *)(pcVar6 + 0x18),*(DWORD *)(pcVar6 + 0x14),&local_4,
                 (LPOVERLAPPED)0x0);
      }
      **(undefined1 **)(pcVar6 + 0x24) = 1;
      pcVar6[0x1d] = '\0';
      pcVar6[0x1e] = '\0';
    }
  } while( true );
}
#endif
