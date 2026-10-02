// cache_reserve_map_memory  (Ghidra: cache_reserve_map_memory, already named)
// address 0x4448d0, size 274 bytes
// name confidence: 0.8 (out/phase4/cache_functions.md: "Reserves the fixed virtual-memory
// region at 0x40000000 that the map/tag heap requires, fatally exiting with a diagnostic
// message identifying the conflicting module if the reservation fails")
// rewrite confidence: 0.8
// evidence: types/cache.h k_map_memory_base/k_map_memory_size constants and the global list
// (map_memory 0x006ac548, tag_data_base 0x006ac54c, texture_cache_memory 0x006ac550,
// sound_cache_memory 0x006ac554, sound_cache_size_megabytes 0x006869c4) all match this
// function's own writes/reads byte for byte; strings "Psapi.dll" / "GetMappedFileNameA" /
// "Error" / the "Cannot allocate required memory..." message match verbatim.
// register convention: cdecl (Ghidra: __cdecl), no parameters, no return value.
//
// UNSURE: the 0x104-byte stack buffer Ghidra renders as a lone CHAR plus a run of zeroing
// stores is reconstructed here as a plain char[0x104], memset the same way the original
// zeroes it (1 byte, then 0x40 dwords, then a word and a byte -- 0x104 bytes total). The
// GetMappedFileNameA call is resolved dynamically (LoadLibraryA/GetProcAddress) rather than
// linked directly, so its prototype is declared locally as a function-pointer type instead of
// an extern.

#include "win32.h"
#include "tags.h"
#include "cache.h"
#include "memory.h"
#include <string.h>

extern void *map_memory;             // 0x006ac548, VirtualAlloc at 0x40000000, 0x1b40000 bytes
extern void *tag_data_base;          // 0x006ac54c, constant 0x40440000
extern void *texture_cache_memory;   // 0x006ac550, VirtualAlloc 0x4000 bytes
extern void *sound_cache_memory;     // 0x006ac554, VirtualAlloc sound_cache_size_megabytes << 20
extern int32_t sound_cache_size_megabytes; // 0x006869c4, read but not owned by this module


typedef uint32_t (*get_mapped_file_name_a_t)(void *process, void *address, char *filename, uint32_t size);

// Reserves the fixed 0x1b40000-byte virtual-memory region at k_map_memory_base (0x40000000)
// the map/tag heap requires, plus the fixed tag-data pointer, the texture-cache page region
// and the sound-cache page region (sized from sound_cache_size_megabytes). If the map-memory
// reservation fails (something else is already mapped there), tries to identify the culprit
// module via Psapi's GetMappedFileNameA for the message box caption, then fatally exits.
void cache_reserve_map_memory(void)
{
    char path_buffer[0x104];
    void *psapi_module;
    get_mapped_file_name_a_t get_mapped_file_name_a;
    const char *caption;

    map_memory = (void *)0;
    tag_data_base = (void *)0;
    texture_cache_memory = (void *)0;
    sound_cache_memory = (void *)0;

    map_memory = VirtualAlloc((void *)k_map_memory_base, k_map_memory_size, 0x3000, 4);
    tag_data_base = (void *)k_tag_data_base;
    texture_cache_memory = VirtualAlloc((void *)0, 0x4000, 0x3000, 4);
    sound_cache_memory = VirtualAlloc((void *)0, (uint32_t)sound_cache_size_megabytes << 0x14, 0x3000, 4);

    if (map_memory == (void *)0) {
        memset(path_buffer, 0, sizeof(path_buffer));

        psapi_module = LoadLibraryA("Psapi.dll");
        if (psapi_module != (void *)0) {
            get_mapped_file_name_a = (get_mapped_file_name_a_t)GetProcAddress((HMODULE)psapi_module, "GetMappedFileNameA");
            if (get_mapped_file_name_a != (get_mapped_file_name_a_t)0) {
                get_mapped_file_name_a(GetCurrentProcess(), (void *)k_map_memory_base, path_buffer, 0x104);
            }
            FreeLibrary((HMODULE)psapi_module);
        }

        caption = path_buffer;
        if (path_buffer[0] == '\0') {
            caption = "Error";
        }
        MessageBoxA((HWND)((void *)0),
            "Cannot allocate required memory. Some other application has loaded where Halo needs to be located.",
            caption, 0);
        ExitProcess(1); // does not return
    }
    return;
}

#if 0
Original Ghidra decompilation (0x4448d0):

void __cdecl cache_reserve_map_memory(void)

{
  HMODULE hModule;
  FARPROC pFVar1;
  HANDLE pvVar2;
  char *lpCaption;
  int iVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  CHAR *pCVar6;
  undefined4 uVar7;
  CHAR local_110;
  undefined4 local_10f;

  DAT_006ac548 = (LPVOID)0x0;
  DAT_006ac54c = 0;
  DAT_006ac550 = (LPVOID)0x0;
  DAT_006ac554 = (LPVOID)0x0;
  DAT_006ac548 = VirtualAlloc(&DAT_40000000,0x1b40000,0x3000,4);
  DAT_006ac54c = 0x40440000;
  DAT_006ac550 = VirtualAlloc((LPVOID)0x0,0x4000,0x3000,4);
  DAT_006ac554 = VirtualAlloc((LPVOID)0x0,(int)DAT_006869c4 << 0x14,0x3000,4);
  if (DAT_006ac548 == (LPVOID)0x0) {
    local_110 = '\0';
    puVar4 = &local_10f;
    for (iVar3 = 0x40; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *(undefined2 *)puVar4 = 0;
    *(undefined1 *)((int)puVar4 + 2) = 0;
    hModule = LoadLibraryA("Psapi.dll");
    if (hModule != (HMODULE)0x0) {
      pFVar1 = GetProcAddress(hModule,"GetMappedFileNameA");
      if (pFVar1 != (FARPROC)0x0) {
        uVar7 = 0x104;
        pCVar6 = &local_110;
        uVar5 = 0x40000000;
        pvVar2 = GetCurrentProcess();
        (*pFVar1)(pvVar2,uVar5,pCVar6,uVar7);
      }
      FreeLibrary(hModule);
    }
    lpCaption = &local_110;
    if (local_110 == '\0') {
      lpCaption = "Error";
    }
    MessageBoxA((HWND)0x0,
                "Cannot allocate required memory. Some other application has loaded where Halo needs to be located."
                ,lpCaption,0);
                    /* WARNING: Subroutine does not return */
    ExitProcess(1);
  }
  return;
}
#endif
