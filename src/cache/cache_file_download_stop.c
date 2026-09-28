// cache_file_download_stop  (Ghidra: FUN_00443510; named per types/cache.h's map_download_state
// comment, which cites this function directly)
// address 0x443510, size 44 bytes
// name confidence: 0.35 (out/phase4/cache_functions.md: "Signals the background map-download
// thread to stop if it has not already finished")
// rewrite confidence: 0.75
// evidence: types/cache.h map_download_state fields (finished_event 0x958, stop_event 0x954);
// one of the four map_download_state glue functions out/phase4/cache_types_notes.md item 5 keeps
// in-module. Raw disassembly (objdump -d -M intel --start-address=0x443510
// --stop-address=0x44353c bin/halo.exe) confirms there are no register or stack arguments.
// register convention: none (void).

#include "tags.h"
#include "cache.h"

extern uint32_t __stdcall WaitForSingleObject(void *handle, uint32_t timeout_ms); // 0x0063a310 IAT
extern void __stdcall SetEvent(void *event); // 0x0063a294 IAT

extern map_download_state *map_download; // 0x006869c0

// Asks the background map-download thread to stop, unless it has already finished. A
// non-blocking check of finished_event (timeout 0) that comes back signaled means the thread is
// already done and nothing further is needed; otherwise stop_event is signaled to request a
// shutdown, without waiting for it to take effect.
void cache_file_download_stop(void)
{
    uint32_t finished_signaled;

    finished_signaled = WaitForSingleObject(map_download->finished_event, 0);
    if (finished_signaled != 0) {
        SetEvent(map_download->stop_event);
    }
}

#if 0
Original Ghidra decompilation (0x443510):

void FUN_00443510(void)

{
  DWORD DVar1;

  DVar1 = WaitForSingleObject(*(HANDLE *)(PTR_DAT_006869c0 + 0x958),0);
  if (DVar1 != 0) {
    SetEvent(*(HANDLE *)(PTR_DAT_006869c0 + 0x954));
  }
  return;
}

Raw disassembly (0x443510-0x44353b), objdump -d -M intel --start-address=0x443510
--stop-address=0x44353c bin/halo.exe:

00443510: mov eax,ds:0x6869c0
00443515: mov ecx,[eax+0x958]           ; finished_event
0044351b: push 0x0
0044351d: push ecx
0044351e: call dword ptr ds:0x63a310    ; WaitForSingleObject(finished_event, 0)
00443524: test eax,eax
00443526: je 0x44353b                   ; signaled (already finished): skip
00443528: mov edx,ds:0x6869c0
0044352e: mov eax,[edx+0x954]           ; stop_event
00443534: push eax
00443535: call dword ptr ds:0x63a294    ; SetEvent(stop_event)
0044353b: ret
#endif
