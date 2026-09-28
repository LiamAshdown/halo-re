// cache_file_download_finish  (Ghidra: FUN_00443540; named per types/cache.h's file_time comment
// and src/cache/cache_file_request_map.c's forward declaration, both of which already use this
// name)
// address 0x443540, size 159 bytes
// name confidence: 0.4 (out/phase4/cache_functions.md: "Stops the background map-download
// thread, stamps the newly-downloaded cache file's timestamp, re-reads its header, and clears
// the download-in-progress state")
// rewrite confidence: 0.70
// evidence: types/cache.h map_download_state (finished_event 0x958, stop_event 0x954),
// cache_file_slot (file 0x00, last_write_time 0x04) and the globals list
// (map_download_in_progress 0x006ac470, map_download_slot_index 0x006ac472); one of the four
// map_download_state glue functions out/phase4/cache_types_notes.md item 5 keeps in-module.
// register convention: none (void).
// note: system_time (Win32 SYSTEMTIME) now lives in types/cache.h next to file_time, for the same
// reason: no windows.h is available to the CParser. Used here only as an opaque stack buffer that
// GetSystemTime fills and SystemTimeToFileTime consumes.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "win32.h"
#include "tags.h"
#include "cache.h"


extern void cache_file_slot_read_header(int32_t slot_index); // blam-cc: EAX; this module, 0x4435e0

extern map_download_state *map_download; // 0x006869c0
extern cache_file_slot cache_file_slots[k_cache_file_slot_count]; // 0x006a9428
extern uint8_t map_download_in_progress; // 0x006ac470
extern int16_t map_download_slot_index;  // 0x006ac472

// Tears down a finished (or still-running, which it forces to stop and waits for) map download:
// if the thread has not already finished, requests a stop and blocks indefinitely for it. Then
// stamps the current system time into the target slot's last_write_time, applies that same
// timestamp as the slot's file's last-write time on disk, re-reads the slot's header, and clears
// the download-in-progress state.
void cache_file_download_finish(void)
{
    uint32_t finished_signaled;
    int32_t slot_index;
    system_time now;

    finished_signaled = WaitForSingleObject(map_download->finished_event, 0);
    if (finished_signaled != 0) {
        SetEvent(map_download->stop_event);
        WaitForSingleObject(map_download->finished_event, 0xffffffff);
    }

    slot_index = map_download_slot_index;
    GetSystemTime((LPSYSTEMTIME)&now);
    SystemTimeToFileTime((const SYSTEMTIME *)&now, (LPFILETIME)&cache_file_slots[slot_index].last_write_time);
    SetFileTime(cache_file_slots[slot_index].file, (const FILETIME *)&cache_file_slots[slot_index].last_write_time,
        (const FILETIME *)((file_time *)0), (const FILETIME *)((file_time *)0));
    cache_file_slot_read_header(slot_index);

    map_download_in_progress = 0;
    map_download_slot_index = -1;
}

#if 0
Original Ghidra decompilation (0x443540):

void FUN_00443540(void)

{
  DWORD DVar1;
  int iVar2;
  _SYSTEMTIME local_10;

  DVar1 = WaitForSingleObject(*(HANDLE *)(PTR_DAT_006869c0 + 0x958),0);
  if (DVar1 != 0) {
    SetEvent(*(HANDLE *)(PTR_DAT_006869c0 + 0x954));
    WaitForSingleObject(*(HANDLE *)(PTR_DAT_006869c0 + 0x958),0xffffffff);
  }
  iVar2 = (int)DAT_006ac472;
  GetSystemTime(&local_10);
  SystemTimeToFileTime(&local_10,(LPFILETIME)(&DAT_006a942c + iVar2 * 0x80c));
  SetFileTime(*(HANDLE *)(&DAT_006a9428 + iVar2 * 0x80c),(LPFILETIME)(&DAT_006a942c + iVar2 * 0x80c)
              ,(FILETIME *)0x0,(FILETIME *)0x0);
  FUN_004435e0();
  DAT_006ac470 = 0;
  DAT_006ac472 = 0xffff;
  return;
}
#endif
