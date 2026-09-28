// cache_io_thread_proc_async  (Ghidra: cache_io_thread_proc_async, already named)
// address 0x443940, size 188 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: field offsets match cache_io_request in types/cache.h exactly (pending 0x1d,
// started 0x1e, priority 0x1c, offset 0x08, destination 0x18, data_file_index 0x20); the
// data_file_index 1/2 dispatch to bitmaps_data_file/sounds_data_file matches
// out/phase4/cache_types_notes.md's data_file section, including the "[0xf]" == +0x3c ==
// data_file::file read for the handle.
// register convention: CreateThread's LPTHREAD_START_ROUTINE parameter, unused.
//
// UNSURE: the call to FUN_00442c70 (outside this function's range, not rewritten here) passes
// only 3 visible arguments in the decompile; per the note in cache_file_slot_read_header.c, the
// completion record it actually needs travels through ESI. Here the natural candidate is the
// cache_io_request's own embedded `completion` field, so that is what is passed explicitly.

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


// blam-cc: request in ESI, size in EBX, offset in EDX, completion_routine in EDI; the three
// named arguments are the stack ones. Recovered by disassembly at this call site
// (0x004439e1-0x004439f6) and in the callee itself; see cache_file_slot_read_header.c.
extern void cache_io_read_file_ex_retry(void *read_file_ex, void *file, void *buffer,
    cache_io_request *request, uint32_t size, uint32_t offset, void *completion_routine); // 0x442c70

// The APC this worker installs (0x443ae0): sets *request->completion.flag and clears the
// request's pending/started bytes. Distinct from cache_io_completion_routine @0x443b00, which
// runs completion.procedure instead and leaves the queue bookkeeping alone. Ghidra gives
// 0x443ae0 no function boundary, so it is not listed in out/phase4/cache_functions.md and has
// no file of its own; declared here only to take its address.
extern void __stdcall cache_io_request_completion_routine(uint32_t error_code, uint32_t bytes_transferred,
    cache_io_request *overlapped); // 0x443ae0

// blam-cc: unused CreateThread parameter
// Background IO thread procedure that services queued cache-file read requests using overlapped
// ReadFileEx with a retry wrapper. Waits alertably on cache_io_event; each time it wakes, scans
// the whole request queue for pending-but-not-started requests and starts the one with the
// lowest (priority, offset) pair, repeating until none remain, then waits again.
uint32_t cache_io_thread_proc_async(void *parameter)
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
            wait_result = WaitForSingleObjectEx(cache_io_event, 0xffffffff, 1);
        } while (wait_result == 0xc0); // WAIT_IO_COMPLETION: an APC ran, keep waiting

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

            best->started = 1;
            cache_io_read_file_ex_retry(read_function, file_handle, best->destination, best,
                best->size, best->offset, (void *)cache_io_request_completion_routine);
        }
    }
}

#if 0
Original Ghidra decompilation (0x443940):

void cache_io_thread_proc_async(void)

{
  char cVar1;
  code *pcVar2;
  DWORD DVar3;
  char *pcVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 uVar7;
  char *pcVar8;

  pcVar2 = (void *)ReadFileEx;
  do {
    do {
      DVar3 = WaitForSingleObjectEx(DAT_006ac498,0xffffffff,1);
    } while (DVar3 == 0xc0);
    while( true ) {
      pcVar8 = (char *)0x0;
      pcVar4 = (char *)(DAT_006ac4a0 + 0x1e);
      iVar5 = 0x200;
      do {
        if (((pcVar4[-1] != '\0') && (*pcVar4 == '\0')) &&
           ((pcVar8 == (char *)0x0 ||
            (((byte)pcVar4[-2] < (byte)pcVar8[0x1c] &&
             (*(uint *)(pcVar4 + -0x16) < *(uint *)(pcVar8 + 8))))))) {
          pcVar8 = pcVar4 + -0x1e;
        }
        pcVar4 = pcVar4 + 0x30;
        iVar5 = iVar5 + -1;
      } while (iVar5 != 0);
      if (pcVar8 == (char *)0x0) break;
      uVar7 = *(undefined4 *)(&DAT_006a9428 + DAT_006ac494 * 0x80c);
      cVar1 = pcVar8[0x20];
      if (cVar1 != '\0') {
        puVar6 = (undefined4 *)0x0;
        if (cVar1 == '\x01') {
          puVar6 = &DAT_006ac4e8;
        }
        else if (cVar1 == '\x02') {
          puVar6 = &DAT_006ac4a8;
        }
        uVar7 = puVar6[0xf];
      }
      pcVar8[0x1e] = '\x01';
      FUN_00442c70(pcVar2,uVar7,*(undefined4 *)(pcVar8 + 0x18));
    }
  } while( true );
}
#endif
