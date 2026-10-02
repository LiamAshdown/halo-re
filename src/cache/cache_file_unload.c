// cache_file_unload
// address 0x442430, size 121 bytes
// name confidence: 0.8 (already named by Ghidra; matches the cache-slot teardown it performs)
// rewrite confidence: 0.8
// evidence: types/cache.h cache_file_slot / data_array layouts; out/phase4/cache_types_notes.md
// globals list; raw disassembly at 0x442430-0x4424a8 (objdump -d -M intel
// --start-address=0x442430 --stop-address=0x4424b0 bin/halo.exe) used to recover the register
// arguments Ghidra elided for cache_flush (ESI = texture_cache) and
// structure_bsp_dispose_material_vertex_buffers (EAX = structure_bsp_data).
// register convention: no explicit parameters. cache_flush's cache* argument is texture_cache in
// ESI; structure_bsp_dispose_material_vertex_buffers's argument is structure_bsp_data in EAX; both are
// loaded from globals right before their calls, not passed in by this function's own caller.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void sound_cache_dispose(void); // this module, 0x443f30
extern void cache_flush(cache *self); // blam-cc: self in ESI; 0x4d17f0 memory module
extern void cache_io_wait_all_requests(void); // this module, 0x4432b0
extern void structure_bsp_dispose_material_vertex_buffers(
    ScenarioStructureBSPCompiledHeader *compiled_header); // blam-cc: EAX // blam-cc: EAX;
    // this module, 0x4431a0 (Ghidra: FUN_004431a0)
extern void model_dispose_vertex_buffers(void); // Ghidra: FUN_00442f00, misattributed name
    // (out/phase4/cache_types_notes.md item 2); this module, 0x442f00

extern uint8_t cache_file_loaded;                  // 0x006a8150
extern int16_t cache_file_index;                    // 0x006ac494
extern cache_file_slot cache_file_slots[k_cache_file_slot_count];         // 0x006a9428
extern struct cache *texture_cache;                        // 0x006ac540
extern data_array *texture_cache_entries;           // 0x006ac538
extern void *structure_bsp_data;                    // 0x006a8958
extern tag_instance *tag_instances;                 // 0x0087bc14

// Tears down the currently loaded map: disposes the sound cache, flushes the texture cache,
// closes and zeroes the active cache-file slot (if any), releases the resident structure_bsp's
// and every model's rendering resources, and clears the loaded-map globals.
void cache_file_unload(void)
{
    uint32_t *destination;
    int32_t i;

    sound_cache_dispose();
    cache_flush(texture_cache);
    texture_cache_entries->valid = 0;

    if (cache_file_index != -1) {
        cache_io_wait_all_requests();
        CloseHandle(cache_file_slots[cache_file_index].file);
        destination = (uint32_t *)&cache_file_slots[cache_file_index];
        for (i = 0x203; i != 0; i--) {
            *destination++ = 0;
        }
        cache_file_index = -1;
    }

    structure_bsp_dispose_material_vertex_buffers(
        (ScenarioStructureBSPCompiledHeader *)structure_bsp_data);
    model_dispose_vertex_buffers();
    cache_file_loaded = 0;
    tag_instances = 0;
}

#if 0
Original Ghidra decompilation (0x442430):

void __cdecl cache_file_unload(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;

  sound_cache_dispose();
  cache_flush();
  *(undefined1 *)(DAT_006ac538 + 0x24) = 0;
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
  FUN_004431a0();
  FUN_00442f00();
  DAT_006a8150 = 0;
  DAT_0087bc14 = 0;
  return;
}

Raw disassembly (0x442430-0x4424a8), objdump -d -M intel --start-address=0x442430
--stop-address=0x4424b0 bin/halo.exe:

00442430: push esi
00442431: call 0x443f30            ; sound_cache_dispose
00442436: mov esi,ds:0x6ac540      ; esi = texture_cache
0044243c: call 0x4d17f0            ; cache_flush(esi)
00442441: mov eax,ds:0x6ac538
00442446: mov BYTE PTR [eax+0x24],0x0   ; texture_cache_entries->valid = 0
0044244a: mov ax,ds:0x6ac494
00442450: cmp ax,0xffff
00442454: pop esi
00442455: je 0x442488
00442457: push edi
00442458: movsx edi,ax
0044245b: imul edi,edi,0x80c
00442461: add edi,0x6a9428          ; edi = &cache_file_slots[index]
00442467: call 0x4432b0            ; cache_io_wait_all_requests
0044246c: mov ecx,DWORD PTR [edi]
0044246e: push ecx
0044246f: call DWORD PTR ds:0x63a2f8   ; CloseHandle
00442475: mov ecx,0x203
0044247a: xor eax,eax
0044247c: rep stos DWORD PTR es:[edi],eax
0044247e: mov WORD PTR ds:0x6ac494,0xffff
00442487: pop edi
00442488: mov eax,ds:0x6a8958      ; eax = structure_bsp_data
0044248d: call 0x4431a0            ; structure_bsp_dispose_material_vertex_buffers(eax)
00442492: call 0x442f00            ; model_dispose_vertex_buffers
00442497: mov BYTE PTR ds:0x6a8150,0x0
0044249e: mov DWORD PTR ds:0x87bc14,0x0
004424a5: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
