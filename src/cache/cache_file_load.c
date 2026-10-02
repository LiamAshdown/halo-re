// cache_file_load
// address 0x442290, size 401 bytes
// name confidence: 0.85 (already named by Ghidra; matches the cache_file_header validation and
// the tag_header/tag_instances setup documented in types/cache.h and cache_types_notes.md)
// rewrite confidence: 0.90
// evidence: types/cache.h cache_file_header/cache_file_tag_header/cache_file_slot layouts;
// out/phase4/cache_types_notes.md "cache_file_header" and globals sections; raw disassembly at
// 0x442290-0x442420 (objdump -d -M intel --start-address=0x442290 --stop-address=0x442430
// bin/halo.exe) used to recover the EAX path argument, the basename-via-strrchr idiom feeding
// cache_file_find_slot_by_name's EDI argument, and the exact cache_io_request_new call/stack
// layout that Ghidra elided.
// register convention: char *path in EAX (the only argument; Ghidra shows in_EAX but never
// names it as a parameter). The basename computed from it (last '\' + 1, or the whole string if
// none) is passed to cache_file_find_slot_by_name in EDI (unaff_EDI there). The cache_io_request
// completion record built on the stack ({&flag, 0, 0}) is passed to cache_io_request_new by
// pointer in ESI, matching cache_io_completion.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void data_delete_all(data_array *array); // 0x4d0580 memory module
extern int16_t cache_file_find_slot_by_name(char *name); // blam-cc: name in EDI; 0x443770
extern int16_t cache_io_request_new(cache_io_completion *completion, // blam-cc: ESI
    int32_t offset, uint32_t size, void *destination, uint8_t priority,
    uint8_t data_file_index); // 0x442b20
extern void model_load_vertex_buffers(cache_file_tag_header *header); // blam-cc: header in EAX.
    // Ghidra: chimera__on_map_load_client, misattributed
    // (out/phase4/cache_types_notes.md item 2); 0x442d10

extern uint8_t cache_file_loaded;                  // 0x006a8150
extern cache_file_header cache_file_current_header; // 0x006a8154
extern cache_file_tag_header *tag_header;           // 0x006a8954
extern int16_t cache_file_index;                    // 0x006ac494
extern cache_io_request *cache_io_requests;         // 0x006ac4a0
extern cache_file_slot cache_file_slots[k_cache_file_slot_count];         // 0x006a9428
extern data_array *texture_cache_entries;           // 0x006ac538
extern data_array *sound_cache_entries;             // 0x006ac528
extern void *tag_data_base;                         // 0x006ac54c, constant 0x40440000
extern tag_instance *tag_instances;                 // 0x0087bc14
extern void *sound_decode_buffer;                   // 0x006f17ec
extern int32_t sound_decode_buffer_size;            // 0x006f17f0

// Top-level map-file loader. Flushes the texture and sound caches, makes sure the shared sound
// decode scratch buffer is at least 1 MB, finds the already-open cache_file_slot for `path`'s
// basename, zeroes the entire IO request queue, copies that slot's header into the current-map
// global, validates it, blocks on an async read of the tag data block into tag_data_base, wires
// up tag_header/tag_instances, and runs the post-load client fixup. Returns the scenario tag id,
// or k_datum_index_none if the slot's header failed validation.
datum_index cache_file_load(char *path)
{
    char *slash;
    char *basename;
    cache_io_completion completion;
    uint8_t completion_flag;
    cache_file_header *slot_header;
    uint32_t *destination;
    uint32_t *source;
    int32_t i;

    slash = strrchr(path, '\\');
    basename = (slash != 0) ? slash + 1 : path;

    texture_cache_entries->valid = 1;
    data_delete_all(texture_cache_entries);
    sound_cache_entries->valid = 1;
    data_delete_all(sound_cache_entries);

    if (sound_decode_buffer_size < 0x100000) {
        if (sound_decode_buffer != 0) {
            GlobalFree(sound_decode_buffer);
        }
        sound_decode_buffer_size = 0x100000;
        sound_decode_buffer = GlobalAlloc(0, sound_decode_buffer_size);
    }

    cache_file_index = cache_file_find_slot_by_name(basename);

    destination = (uint32_t *)cache_io_requests;
    for (i = 0x1800; i != 0; i--) {
        *destination++ = 0;
    }

    slot_header = &cache_file_slots[cache_file_index].header;
    destination = (uint32_t *)&cache_file_current_header;
    source = (uint32_t *)slot_header;
    for (i = 0x200; i != 0; i--) {
        *destination++ = *source++;
    }

    if (cache_file_current_header.head != k_cache_file_head_signature ||
        cache_file_current_header.foot != k_cache_file_foot_signature ||
        cache_file_current_header.file_size < 0 ||
        cache_file_current_header.file_size > k_cache_file_maximum_size ||
        strlen(cache_file_current_header.name) >= k_cache_file_name_length ||
        cache_file_current_header.version != k_cache_file_version) {
        return (datum_index)0xffffffff;
    }

    completion_flag = 0;
    completion.flag = &completion_flag;
    completion.procedure = 0;
    completion.data = 0;
    cache_io_request_new(&completion, cache_file_current_header.tag_data_offset,
        cache_file_current_header.tag_data_size, tag_data_base, 1, 0);
    while (completion_flag == 0) {
        Sleep(0);
    }

    tag_header = (cache_file_tag_header *)tag_data_base;
    tag_instances = tag_header->tags;
    cache_file_loaded = 1;
    model_load_vertex_buffers(tag_header); // `mov eax,edi` at 0x004423fa: the EAX argument
    return tag_header->scenario_tag;
}

#if 0
Original Ghidra decompilation (0x442290):

undefined4 cache_file_load(void)

{
  undefined4 *puVar1;
  char *pcVar2;
  char *in_EAX;
  char *pcVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  char local_d;
  char *local_c;
  undefined4 local_8;
  undefined4 local_4;

  _strrchr(in_EAX,0x5c);
  *(undefined1 *)(DAT_006ac538 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_006ac528 + 0x24) = 1;
  data_delete_all();
  if (DAT_006f17f0 < 0x100000) {
    if (DAT_006f17ec != (HGLOBAL)0x0) {
      GlobalFree(DAT_006f17ec);
    }
    DAT_006f17f0 = 0x100000;
    DAT_006f17ec = GlobalAlloc(0,0x100000);
  }
  DAT_006ac494 = cache_file_find_slot_by_name();
  puVar5 = DAT_006ac4a0;
  for (iVar4 = 0x1800; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  puVar5 = (undefined4 *)(&DAT_006a9434 + DAT_006ac494 * 0x80c);
  puVar6 = &DAT_006a8154;
  for (iVar4 = 0x200; puVar1 = DAT_006ac54c, iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar6 = puVar6 + 1;
  }
  if ((((DAT_006a8154 == 0x68656164) && (DAT_006a8950 == 0x666f6f74)) && (-1 < DAT_006a815c)) &&
     (DAT_006a815c < 0x18000001)) {
    pcVar2 = &DAT_006a8174;
    do {
      pcVar3 = pcVar2;
      pcVar2 = pcVar3 + 1;
    } while (*pcVar3 != '\0');
    if ((pcVar3 + -0x6a8174 < &DAT_00000020) && (DAT_006a8158 == 7)) {
      local_c = &local_d;
      local_8 = 0;
      local_4 = 0;
      cache_io_request_new(DAT_006a8164,DAT_006a8168,DAT_006ac54c,1,0);
      while (local_d == '\0') {
        Sleep(0);
      }
      DAT_006a8954 = puVar1;
      DAT_0087bc14 = *puVar1;
      DAT_006a8150 = 1;
      chimera__on_map_load_client();
      return DAT_006a8954[1];
    }
  }
  return 0xffffffff;
}

Raw disassembly (0x442290-0x442420), objdump -d -M intel --start-address=0x442290
--stop-address=0x442430 bin/halo.exe:

00442290: sub esp,0x10
00442293: push ebp
00442294: push esi
00442295: push edi
00442296: mov esi,eax
00442298: push 0x5c
0044229a: push esi
0044229b: call 0x623bc0            ; strrchr(path, '\\')
004422a0: mov edi,eax
004422a2: add esp,0x8
004422a5: test edi,edi
004422a7: je 0x4422ac
004422a9: inc edi                  ; edi = strrchr_result + 1
004422aa: jmp 0x4422ae
004422ac: mov edi,esi              ; edi = path (no '\\' found)
004422ae: mov esi,ds:0x6ac538      ; texture_cache_entries
004422b4: or ebp,0xffffffff
004422b7: mov BYTE PTR [esi+0x24],0x1
004422bb: call 0x4d0580            ; data_delete_all(texture_cache_entries)
004422c0: mov esi,ds:0x6ac528      ; sound_cache_entries
004422c6: mov BYTE PTR [esi+0x24],0x1
004422ca: call 0x4d0580            ; data_delete_all(sound_cache_entries)
004422cf: mov eax,ds:0x6f17f0
004422d4: mov esi,0x100000
004422d9: cmp eax,esi
004422db: jge 0x442301
004422dd: mov eax,ds:0x6f17ec
004422e2: test eax,eax
004422e4: je 0x4422ed
004422e6: push eax
004422e7: call DWORD PTR ds:0x63a0bc   ; GlobalFree
004422ed: push esi
004422ee: push 0x0
004422f0: mov ds:0x6f17f0,esi
004422f6: call DWORD PTR ds:0x63a0b0   ; GlobalAlloc
004422fc: mov ds:0x6f17ec,eax
00442301: call 0x443770            ; cache_file_find_slot_by_name(edi = basename)
00442306: mov edi,ds:0x6ac4a0      ; cache_io_requests
0044230c: mov ecx,0x1800
00442311: mov edx,eax
00442313: xor eax,eax
00442315: rep stos DWORD PTR es:[edi],eax
00442317: movsx eax,dx
0044231a: imul eax,eax,0x80c
00442320: add eax,0x6a9428
00442325: lea esi,[eax+0xc]        ; &cache_file_slots[index].header
00442328: mov ecx,0x200
0044232d: mov edi,0x6a8154         ; &cache_file_current_header
00442332: mov WORD PTR ds:0x6ac494,dx
00442339: rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
0044233b: cmp DWORD PTR ds:0x6a8154,0x68656164
00442345: mov edi,DWORD PTR ds:0x6ac54c   ; edi = tag_data_base
0044234b: jne 0x442418
00442351: cmp DWORD PTR ds:0x6a8950,0x666f6f74
0044235b: jne 0x442418
00442361: mov eax,ds:0x6a815c
00442366: test eax,eax
00442368: jl 0x442418
0044236e: cmp eax,0x18000000
00442373: jg 0x442418
00442379: mov eax,0x6a8174
0044237e: lea edx,[eax+0x1]
00442381: mov cl,BYTE PTR [eax]
00442383: inc eax
00442384: test cl,cl
00442386: jne 0x442381
00442388: sub eax,edx
0044238a: cmp eax,0x1f
0044238d: ja 0x442418
00442393: cmp DWORD PTR ds:0x6a8158,0x7
0044239a: jne 0x442418
0044239c: mov ecx,DWORD PTR ds:0x6a8168   ; tag_data_size
004423a2: mov edx,DWORD PTR ds:0x6a8164   ; tag_data_offset
004423a8: push 0x0                 ; data_file_index
004423aa: push 0x1                 ; priority
004423ac: push edi                 ; destination = tag_data_base
004423ad: push ecx                 ; size
004423ae: lea eax,[esp+0x1f]       ; &completion_flag
004423b2: push edx                 ; offset
004423b3: lea esi,[esp+0x24]       ; &completion (esi arg to cache_io_request_new)
004423b7: mov DWORD PTR [esp+0x24],eax
004423bb: mov DWORD PTR [esp+0x28],0x0
004423c3: mov DWORD PTR [esp+0x2c],0x0
004423cb: call 0x442b20            ; cache_io_request_new
004423d0: mov al,BYTE PTR [esp+0x23]
004423d4: add esp,0x14
004423d7: test al,al
004423d9: jne 0x4423ed
004423db: mov esi,ds:0x63a29c      ; Sleep
004423e1: push 0x0
004423e3: call esi
004423e5: mov al,BYTE PTR [esp+0xf]
004423e9: test al,al
004423eb: je 0x4423e1
004423ed: mov DWORD PTR ds:0x6a8954,edi   ; tag_header = tag_data_base
004423f3: mov eax,DWORD PTR [edi]         ; tag_header->tags
004423f5: mov ds:0x87bc14,eax             ; tag_instances = tag_header->tags
004423fa: mov eax,edi
004423fc: mov BYTE PTR ds:0x6a8150,0x1    ; cache_file_loaded = 1
00442403: call 0x442d10                   ; model_load_vertex_buffers
00442408: mov ecx,DWORD PTR ds:0x6a8954
0044240e: mov eax,DWORD PTR [ecx+0x4]     ; tag_header->scenario_tag
00442411: pop edi
00442412: pop esi
00442413: pop ebp
00442414: add esp,0x10
00442417: ret
00442418: pop edi
00442419: pop esi
0044241a: mov eax,ebp                     ; eax = 0xffffffff
0044241c: pop ebp
0044241d: add esp,0x10
00442420: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
