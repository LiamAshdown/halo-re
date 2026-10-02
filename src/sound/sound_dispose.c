// sound_dispose  (Ghidra: sound_dispose, already named)
// address 0x549760, size 166 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md summary "Shuts down the sound device and frees all
//   sound/looping-sound/cache-file tables allocated by sound_initialize."; sound_data/
//   looping_sound_data/current_sound_driver/sound_initialized (types/sound.h) and sound_cache_entries/
//   sound_cache (src/cache/cache_reserve_map_memory.c and siblings) match by address; each zeroed
//   block is exactly sizeof(data_array) (0x38, 14 dwords) before being freed with GlobalFree,
//   confirming data_array's own header (not just its bulk data) is what these globals point to.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// Phase-4 review (disassembly appended below): the byte writes are data_array.valid (+0x24) of
//   both tables, not +9 (Ghidra showed an int* index); the other two stores are the sound cache
//   globals sound_cache_initialized (0x006ac534, cleared before the entry table is freed) and
//   sound_cache_base (0x006ac52c, cleared at the end), which the draft had dropped.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data;          // 0x007252c0
extern data_array *looping_sound_data;  // 0x00724a50
extern sound_driver *current_sound_driver;  // 0x00725208
extern uint8_t sound_initialized;       // 0x00725200
extern data_array *sound_cache_entries; // 0x006ac528
extern struct cache *sound_cache;       // 0x006ac530
extern void *sound_cache_base;          // 0x006ac52c, types/cache.h
extern uint8_t sound_cache_initialized; // 0x006ac534, types/cache.h


static void sound_dispose_zero_and_free(void *block, int32_t dword_count)
{
    uint32_t *words = (uint32_t *)block;
    int32_t i;
    for (i = 0; i < dword_count; i++) {
        words[i] = 0;
    }
    GlobalFree(block);
}

// Shuts down the sound device (if initialized) and frees the sound/looping-sound data_array
// headers it owns, then unconditionally frees the sound cache's entry table and cache header.
void sound_dispose(void)
{
    if (sound_initialized != 0) {
        current_sound_driver->dispose();
        sound_data->valid = 0;
        looping_sound_data->valid = 0;
        sound_initialized = 0;
    }

    if (sound_data != (data_array *)0) {
        sound_dispose_zero_and_free(sound_data, 0xe);
    }

    if (looping_sound_data != (data_array *)0) {
        sound_dispose_zero_and_free(looping_sound_data, 0xe);
    }

    sound_cache_initialized = 0;
    sound_dispose_zero_and_free(sound_cache_entries, 0xe);
    sound_dispose_zero_and_free(sound_cache, 0x11);
    sound_cache_base = (void *)0;
}

#if 0
Original Ghidra decompilation (0x549760):

void sound_dispose(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;

  puVar2 = DAT_007252c0;
  puVar3 = DAT_00724a50;
  if (DAT_00725200 != '\0') {
    (**(code **)(DAT_00725208 + 8))();
    puVar2 = DAT_007252c0;
    puVar3 = DAT_00724a50;
    *(undefined1 *)(DAT_007252c0 + 9) = 0;
    *(undefined1 *)(puVar3 + 9) = 0;
    DAT_00725200 = '\0';
  }
  if (puVar2 != (undefined4 *)0x0) {
    puVar3 = puVar2;
    for (iVar1 = 0xe; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    GlobalFree(puVar2);
    puVar3 = DAT_00724a50;
  }
  if (puVar3 != (undefined4 *)0x0) {
    puVar2 = puVar3;
    for (iVar1 = 0xe; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    GlobalFree(puVar3);
  }
  puVar3 = DAT_006ac528;
  DAT_006ac534 = 0;
  puVar2 = DAT_006ac528;
  for (iVar1 = 0xe; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  GlobalFree(puVar3);
  puVar3 = DAT_006ac530;
  puVar2 = DAT_006ac530;
  for (iVar1 = 0x11; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  GlobalFree(puVar3);
  DAT_006ac52c = 0;
  return;
}

Disassembly (0x549760..0x549806, capstone; phase-4 review):

0x549760: mov al, byte ptr [0x725200]
0x549765: push ebx
0x549766: push ebp
0x549767: xor ebx, ebx
0x549769: cmp al, bl
0x54976b: push esi
0x54976c: push edi
0x54976d: je 0x549791
0x54976f: mov eax, dword ptr [0x725208]
0x549774: call dword ptr [eax + 8]
0x549777: mov edx, dword ptr [0x7252c0]
0x54977d: mov ebp, dword ptr [0x724a50]
0x549783: mov byte ptr [edx + 0x24], bl
0x549786: mov byte ptr [ebp + 0x24], bl
0x549789: mov byte ptr [0x725200], bl
0x54978f: jmp 0x54979d
0x549791: mov edx, dword ptr [0x7252c0]
0x549797: mov ebp, dword ptr [0x724a50]
0x54979d: cmp edx, ebx
0x54979f: mov esi, dword ptr [0x63a0bc]
0x5497a5: je 0x5497bb
0x5497a7: xor eax, eax
0x5497a9: mov edi, edx
0x5497ab: mov ecx, 0xe
0x5497b0: push edx
0x5497b1: rep stosd dword ptr es:[edi], eax
0x5497b3: call esi
0x5497b5: mov ebp, dword ptr [0x724a50]
0x5497bb: cmp ebp, ebx
0x5497bd: je 0x5497cd
0x5497bf: xor eax, eax
0x5497c1: mov ecx, 0xe
0x5497c6: mov edi, ebp
0x5497c8: push ebp
0x5497c9: rep stosd dword ptr es:[edi], eax
0x5497cb: call esi
0x5497cd: mov edx, dword ptr [0x6ac528]
0x5497d3: xor eax, eax
0x5497d5: mov edi, edx
0x5497d7: mov ecx, 0xe
0x5497dc: push edx
0x5497dd: mov byte ptr [0x6ac534], bl
0x5497e3: rep stosd dword ptr es:[edi], eax
0x5497e5: call esi
0x5497e7: mov edx, dword ptr [0x6ac530]
0x5497ed: xor eax, eax
0x5497ef: mov edi, edx
0x5497f1: mov ecx, 0x11
0x5497f6: push edx
0x5497f7: rep stosd dword ptr es:[edi], eax
0x5497f9: call esi
0x5497fb: pop edi
0x5497fc: pop esi
0x5497fd: pop ebp
0x5497fe: mov dword ptr [0x6ac52c], ebx
0x549804: pop ebx
0x549805: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
