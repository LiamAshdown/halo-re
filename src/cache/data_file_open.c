// data_file_open
// address 0x442840, size 528 bytes
// name confidence: 0.8 (already named by Ghidra; out/phase4/cache_functions.md: "Opens and
// validates the external bitmaps.map and sounds.map data files, reading their headers and
// raw/reference tables, and allocates the shared 0x6000-byte scratch buffer used by the IO
// worker thread")
// rewrite confidence: 0.85
// evidence: types/cache.h data_file layout and the bitmaps_data_file/sounds_data_file globals
// (every field offset below -- name 0x38, file 0x3c, data 0x20, references 0x10, data_offset
// 0x04, unknown_24 -- matches exactly, field-for-field, in both the bitmaps and sounds blocks).
// register convention: no parameters.
// UNSURE: the CreateFileA flag constants (0x48000080 / 0x8000080) are reproduced as literals;
// they decode to FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL and
// FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL respectively, chosen by os_platform, but
// named Win32 constants aren't available to this header (no include is allowed here).
// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "cache.h"

extern void os_platform_identify(void); // 0x5427e0

extern int32_t data_file_read_header(data_file *file, int32_t expected_file_id); // blam-cc:
    // file in ESI; this module, 0x443b30
extern uint8_t data_file_read_data_block(data_file *file); // blam-cc: ESI; this module, 0x443ba0
extern uint8_t data_file_read_offset_table(data_file *file); // blam-cc: EDI; this module, 0x443c20
extern void cache_io_thread_start(void); // this module, 0x4438d0

extern data_file bitmaps_data_file; // 0x006ac4e8
extern data_file sounds_data_file;  // 0x006ac4a8
extern int16_t cache_file_index;    // 0x006ac494
extern int32_t os_platform;         // 0x00721ef0
extern cache_io_request *cache_io_requests; // 0x006ac4a0

static void zero_data_file(data_file *file)
{
    uint32_t *word;
    int32_t i;

    word = (uint32_t *)file;
    for (i = 0x10; i != 0; i--) {
        *word++ = 0;
    }
}

// Opens both external data files (bitmaps.map, sounds.map) into bitmaps_data_file and
// sounds_data_file, reading and validating each one's header, raw data block and reference
// table. On any failure for a file, frees whatever it already allocated for that file and prints
// a diagnostic, but still proceeds to the other file. Finally allocates the shared 0x6000-byte
// IO request queue and starts the worker thread, regardless of whether either file opened.
void data_file_open(void)
{
    char path[260];
    uint32_t flags;

    zero_data_file(&bitmaps_data_file);
    cache_file_index = (int16_t)0xffff;
    bitmaps_data_file.name = "bitmaps";
    bitmaps_data_file.unknown_24 = 0;
    sprintf(path, "maps\\%s.map", "bitmaps");

    flags = 0x48000080;
    if (os_platform == 0) {
        os_platform_identify();
    }
    if (os_platform < 3) {
        flags = 0x8000080;
    }
    bitmaps_data_file.file = CreateFileA(path, 0x80000000, 1, (void *)0, 4, flags, (void *)0);
    if (bitmaps_data_file.file == (void *)0xffffffff) {
        printf("### FAILED TO OPEN DATA-CACHE FILE.\n\n");
    } else {
        // `mov esi,0x6ac4e8` at 0x004428d0 and `mov edi,esi` at 0x004428ea: all three readers
        // are handed &bitmaps_data_file, in ESI for the first two and EDI for the third.
        if (data_file_read_header(&bitmaps_data_file, 1) == 0 ||
            data_file_read_data_block(&bitmaps_data_file) == 0 ||
            data_file_read_offset_table(&bitmaps_data_file) == 0) {
            if (bitmaps_data_file.data != 0) {
                GlobalFree(bitmaps_data_file.data);
                bitmaps_data_file.data = 0;
            }
            if (bitmaps_data_file.references != 0) {
                GlobalFree(bitmaps_data_file.references);
                bitmaps_data_file.references = 0;
            }
            printf("### FAILED TO OPEN DATA-CACHE FILE.\n\n");
        } else {
            SetFilePointer(bitmaps_data_file.file, bitmaps_data_file.data_offset, (void *)0, 0);
        }
    }

    zero_data_file(&sounds_data_file);
    sounds_data_file.name = "sounds";
    sounds_data_file.unknown_24 = 0;
    sprintf(path, "maps\\%s.map", "sounds");

    flags = 0x48000080;
    if (os_platform == 0) {
        os_platform_identify();
    }
    if (os_platform < 3) {
        flags = 0x8000080;
    }
    sounds_data_file.file = CreateFileA(path, 0x80000000, 1, (void *)0, 4, flags, (void *)0);
    if (sounds_data_file.file != (void *)0xffffffff) {
        if (data_file_read_header(&sounds_data_file, 2) != 0 &&
            data_file_read_data_block(&sounds_data_file) != 0 &&
            data_file_read_offset_table(&sounds_data_file) != 0) {
            SetFilePointer(sounds_data_file.file, sounds_data_file.data_offset, (void *)0, 0);
            goto allocate_io_queue;
        }
        if (sounds_data_file.data != 0) {
            GlobalFree(sounds_data_file.data);
            sounds_data_file.data = 0;
        }
        if (sounds_data_file.references != 0) {
            GlobalFree(sounds_data_file.references);
            sounds_data_file.references = 0;
        }
    }
    printf("### FAILED TO OPEN DATA-CACHE FILE.\n\n");

allocate_io_queue:
    cache_io_requests = (cache_io_request *)GlobalAlloc(0, 0x6000);
    cache_io_thread_start();
}

#if 0
Original Ghidra decompilation (0x442840):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl data_file_open(void)

{
  char cVar1;
  int iVar2;
  DWORD DVar3;
  undefined4 *puVar4;
  char local_104 [260];

  puVar4 = &DAT_006ac4e8;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  DAT_006ac494 = 0xffff;
  _DAT_006ac520 = "bitmaps";
  DAT_006ac50c = 0;
  _sprintf(local_104,"maps\\%s.map","bitmaps");
  DVar3 = 0x48000080;
  if (DAT_00721ef0 == 0) {
    os_platform_identify();
  }
  if (DAT_00721ef0 < 3) {
    DVar3 = 0x8000080;
  }
  DAT_006ac524 = CreateFileA(local_104,0x80000000,1,(LPSECURITY_ATTRIBUTES)0x0,4,DVar3,(HANDLE)0x0);
  if (DAT_006ac524 == (HANDLE)0xffffffff) {
LAB_00442938:
    _printf("### FAILED TO OPEN DATA-CACHE FILE.\n\n");
  }
  else {
    cVar1 = data_file_read_header(1);
    if (cVar1 == '\0') {
LAB_0044290c:
      if (DAT_006ac508 != (HGLOBAL)0x0) {
        GlobalFree(DAT_006ac508);
        DAT_006ac508 = (HGLOBAL)0x0;
      }
      if (DAT_006ac4f8 != (HGLOBAL)0x0) {
        GlobalFree(DAT_006ac4f8);
        DAT_006ac4f8 = (HGLOBAL)0x0;
      }
      goto LAB_00442938;
    }
    cVar1 = data_file_read_data_block();
    if (cVar1 == '\0') goto LAB_0044290c;
    cVar1 = data_file_read_offset_table();
    if (cVar1 == '\0') goto LAB_0044290c;
    SetFilePointer(DAT_006ac524,DAT_006ac4ec,(PLONG)0x0,0);
  }
  puVar4 = &DAT_006ac4a8;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  _DAT_006ac4e0 = "sounds";
  DAT_006ac4cc = 0;
  _sprintf(local_104,"maps\\%s.map","sounds");
  DVar3 = 0x48000080;
  if (DAT_00721ef0 == 0) {
    os_platform_identify();
  }
  if (DAT_00721ef0 < 3) {
    DVar3 = 0x8000080;
  }
  DAT_006ac4e4 = CreateFileA(local_104,0x80000000,1,(LPSECURITY_ATTRIBUTES)0x0,4,DVar3,(HANDLE)0x0);
  if (DAT_006ac4e4 != (HANDLE)0xffffffff) {
    cVar1 = data_file_read_header(2);
    if (cVar1 != '\0') {
      cVar1 = data_file_read_data_block();
      if (cVar1 != '\0') {
        cVar1 = data_file_read_offset_table();
        if (cVar1 != '\0') {
          SetFilePointer(DAT_006ac4e4,DAT_006ac4ac,(PLONG)0x0,0);
          goto LAB_00442a30;
        }
      }
    }
    if (DAT_006ac4c8 != (HGLOBAL)0x0) {
      GlobalFree(DAT_006ac4c8);
      DAT_006ac4c8 = (HGLOBAL)0x0;
    }
    if (DAT_006ac4b8 != (HGLOBAL)0x0) {
      GlobalFree(DAT_006ac4b8);
      DAT_006ac4b8 = (HGLOBAL)0x0;
    }
  }
  _printf("### FAILED TO OPEN DATA-CACHE FILE.\n\n");
LAB_00442a30:
  DAT_006ac4a0 = GlobalAlloc(0,0x6000);
  cache_io_thread_start();
  return;
}
#endif
