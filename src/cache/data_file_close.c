// data_file_close
// address 0x442a50, size 198 bytes
// name confidence: 0.7 (already named by Ghidra; out/phase4/cache_functions.md: "Closes the
// bitmaps.map and sounds.map data files opened by data_file_open and frees all of their
// associated buffers")
// rewrite confidence: 0.75
// evidence: types/cache.h data_file layout (file_id 0x00, data_offset 0x04, table_offset 0x08,
// entry_count 0x0c, references 0x10, data 0x20, file 0x3c) and cache_file_slot (0x80c stride).
// register convention: no parameters.
// UNSURE: this function also tears down the active cache_file_slot (identical to the start of
// cache_file_unload @0x442430), and closes each data file's HANDLE without clearing the `file`
// field itself afterward (only file_id/data_offset/table_offset/entry_count are zeroed);
// reproduced exactly as decompiled, not "fixed".

#include "win32.h"
#include "tags.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern void cache_io_wait_all_requests(void); // this module, 0x4432b0

extern int16_t cache_file_index;            // 0x006ac494
extern cache_file_slot cache_file_slots[k_cache_file_slot_count]; // 0x006a9428
extern data_file bitmaps_data_file;         // 0x006ac4e8
extern data_file sounds_data_file;          // 0x006ac4a8
extern cache_io_request *cache_io_requests; // 0x006ac4a0

// Closes the bitmaps.map and sounds.map data files opened by data_file_open, freeing their data
// and reference-table buffers and the shared IO request queue. Also closes and zeroes the active
// cache-file slot, if any, the same as the start of cache_file_unload.
void data_file_close(void)
{
    uint32_t *destination;
    int32_t i;

    if (cache_file_index != -1) {
        cache_io_wait_all_requests();
        CloseHandle(cache_file_slots[cache_file_index].file);
        destination = (uint32_t *)&cache_file_slots[cache_file_index];
        for (i = 0x203; i != 0; i--) {
            *destination++ = 0;
        }
        cache_file_index = -1;
    }

    CloseHandle(bitmaps_data_file.file);
    if (bitmaps_data_file.data != 0) {
        GlobalFree(bitmaps_data_file.data);
    }
    if (bitmaps_data_file.references != 0) {
        GlobalFree(bitmaps_data_file.references);
    }
    bitmaps_data_file.file_id = 0;
    bitmaps_data_file.data_offset = 0;
    bitmaps_data_file.table_offset = 0;
    bitmaps_data_file.entry_count = 0;

    CloseHandle(sounds_data_file.file);
    if (sounds_data_file.data != 0) {
        GlobalFree(sounds_data_file.data);
    }
    if (sounds_data_file.references != 0) {
        GlobalFree(sounds_data_file.references);
    }
    sounds_data_file.file_id = 0;
    sounds_data_file.data_offset = 0;
    sounds_data_file.table_offset = 0;
    sounds_data_file.entry_count = 0;

    GlobalFree(cache_io_requests);
}

#if 0
Original Ghidra decompilation (0x442a50):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl data_file_close(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;

  if (DAT_006ac494 != -1) {
    iVar2 = (int)DAT_006ac494;
    cache_io_wait_all_requests();
    CloseHandle(*(HANDLE *)(&DAT_006a9428 + iVar2 * 0x80c));
    puVar3 = (undefined4 *)(&DAT_006a9428 + iVar2 * 0x80c);
    for (iVar1 = 0x203; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    DAT_006ac494 = -1;
  }
  CloseHandle(DAT_006ac524);
  if (DAT_006ac508 != (HGLOBAL)0x0) {
    GlobalFree(DAT_006ac508);
  }
  if (DAT_006ac4f8 != (HGLOBAL)0x0) {
    GlobalFree(DAT_006ac4f8);
  }
  DAT_006ac4e8 = 0;
  DAT_006ac4ec = 0;
  _DAT_006ac4f0 = 0;
  _DAT_006ac4f4 = 0;
  CloseHandle(DAT_006ac4e4);
  if (DAT_006ac4c8 != (HGLOBAL)0x0) {
    GlobalFree(DAT_006ac4c8);
  }
  if (DAT_006ac4b8 != (HGLOBAL)0x0) {
    GlobalFree(DAT_006ac4b8);
  }
  DAT_006ac4a8 = 0;
  DAT_006ac4ac = 0;
  _DAT_006ac4b0 = 0;
  _DAT_006ac4b4 = 0;
  GlobalFree(DAT_006ac4a0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
