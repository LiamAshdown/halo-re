// cache_file_open_by_name  (Ghidra: FUN_00443360; named per src/cache/cache_file_request_map.c's
// forward declaration, which already calls it this)
// address 0x443360, size 311 bytes (true extent 0x443360..0x443496 ret inclusive), not the 228 Ghidra reports for
// FUN_00443360 alone -- see the misattribution note below.
// name confidence: 0.45 (out/phase4/cache_functions.md: "Opens a named shared cache map file
// into a free/evicted slot if not already loaded, reporting a fatal error via
// FUN_0057ea70/FUN_00499170 on failure when requested")
// rewrite confidence: 0.70
//
// MISATTRIBUTION FOUND (same class of bug as out/phase4/cache_types_notes.md item 1,
// data_file_read @0x444420): the address range assigned to this batch also contains
// "cache_file_read_request @0x443410, size 83", already named by Ghidra and listed separately
// in out/phase4/cache_functions.md. It is not a real function. Its byte range (0x443410-0x443463)
// is entirely a subset of this function's tail -- the CreateFileA/slot-store/
// cache_file_slot_read_header success path below -- and its reported end (0x443463) is simply
// the "ret" that ends that success path; Ghidra treated the next instruction (the start of this
// function's own report_fatal_error failure path, unreachable except by falling through from
// 0x443360's own branch at 0x4433bf) as a fresh function entry. Confirmed by raw disassembly:
// FUN_00443360's own decompiled body cuts off mid-flow (its call to cache_file_find_oldest_slot
// takes only one visible argument and its CreateFileA path is missing entirely), and the
// instruction bytes at 0x443410 are byte-identical to the tail of the disassembly reproduced
// below starting at the same address. Not rewritten separately; folded into this function, whose
// true extent runs to the real final `ret` at 0x443496.
// evidence: types/cache.h cache_file_slot / cache_file_header / cache_file_slot_category;
// src/cache/cache_file_exists.c, cache_file_find_oldest_slot.c and cache_file_slot_read_header.c
// (already-written siblings whose signatures this call site matches exactly); raw disassembly
// (objdump -d -M intel --start-address=0x443360 --stop-address=0x443463 bin/halo.exe, plus
// --start-address=0x443430 --stop-address=0x443490 for the tail Ghidra folded past its
// truncation), needed because Ghidra dropped the EAX name argument, showed
// cache_file_find_oldest_slot's two arguments as a single call with a bare uninitialized
// local, and elided the report_fatal_error stack parameter from the failure path entirely.
// register convention: char *name in EAX (in_EAX); uint8_t report_fatal_error is the original's
// single recognized stack parameter (param_1).
//
// The cache_file_find_oldest_slot(local_7f8) call Ghidra rendered with one bare, seemingly
// uninitialized argument is not actually reading garbage: the disassembly shows both values
// come from the cache_file_header the immediately preceding cache_file_exists call just filled
// in -- required_size from header.file_size (+0x08), and slot_category from header.unknown_060
// (+0x60, read as a full dword so it also carries unknown_062 in its high 16 bits, which
// cache_file_find_oldest_slot's int16_t parameter then truncates away). This is a second,
// independent confirmation that unknown_060 is the map-type field the published retail layout
// describes; types/cache.h's own field name is left unchanged since this module's convention is
// not to rename fields on inference alone.

// VERIFIED against disassembly 0x443360..0x443496 (2026-09-30): the "Not rewritten separately" note above refers to the
// misattributed cache_file_read_request, which is folded into this function (its tail), not to a gap in this rewrite.
// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void os_platform_identify(void); // 0x5427e0
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70
extern void interface_handle_quit_request(void); // 0x499170

extern int16_t cache_file_find_slot_by_name(char *name); // blam-cc: EDI; 0x443770
extern uint8_t cache_file_exists(char *name, cache_file_header *header_out); // blam-cc: EAX, ESI; 0x442bb0
extern int16_t cache_file_find_oldest_slot(cache_file_slot_category slot_category,
    int32_t required_size); // blam-cc: EAX, stack; 0x4437b0
extern void cache_file_slot_read_header(int32_t slot_index); // blam-cc: EAX; 0x4435e0

extern cache_file_slot cache_file_slots[k_cache_file_slot_count]; // 0x006a9428
extern char map_path_prefix[]; // 0x006f16d8
extern int32_t os_platform; // 0x00721ef0
extern char *rasterizer_shader_file_name; // 0x00722bbc

// blam-cc: name in EAX, report_fatal_error is the recognized stack parameter
// Ensures a cache_file_slot for `name`'s basename exists and is open: returns 1 immediately if
// one is already open. Otherwise reads the map file's header via cache_file_exists; if that
// fails, optionally raises a fatal error (using the full `name`, not the basename) and asks the
// application to quit, then returns 0. On success, picks the least-recently-used eligible slot
// (category and required size taken from the freshly-read header), zero-fills its header area,
// opens "<map_path_prefix>maps\\<basename>.map" (overlapped IO if the platform supports it),
// stores the resulting handle in the slot, re-reads the slot's header through
// cache_file_slot_read_header, and returns 1.
uint8_t cache_file_open_by_name(char *name, uint8_t report_fatal_error)
{
    char *slash;
    char *basename;
    int16_t slot_index;
    cache_file_header header;
    char path[264];
    void *file;
    uint32_t flags_and_attributes;
    uint32_t *destination;
    int32_t i;

    slash = strrchr(name, '\\');
    basename = (slash != 0) ? slash + 1 : name;

    slot_index = cache_file_find_slot_by_name(basename);
    if (slot_index != -1) {
        return 1;
    }

    if (cache_file_exists(basename, &header) == 0) {
        if (report_fatal_error != 0) {
            rasterizer_shader_file_name = name;
            shell_display_fatal_error_dialog(0x89, 0x7e, 1);
            interface_handle_quit_request();
        }
        return 0;
    }

    slot_index = cache_file_find_oldest_slot(
        (cache_file_slot_category)header.map_type, header.file_size);

    destination = (uint32_t *)&cache_file_slots[slot_index].header;
    for (i = 0x200; i != 0; i--) {
        *destination++ = 0;
    }

    sprintf(path, "%s%s%s.map", map_path_prefix, "maps\\", basename);

    flags_and_attributes = 0x48000080;
    if (os_platform == 0) {
        os_platform_identify();
    }
    if (os_platform < 3) {
        flags_and_attributes = 0x8000080;
    }

    file = CreateFileA(path, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)((void *)0), 4, flags_and_attributes, (void *)0);
    cache_file_slots[slot_index].file = file;
    cache_file_slot_read_header(slot_index);
    return 1;
}

#if 0
Original Ghidra decompilation (0x443360):

undefined4 FUN_00443360(char param_1)

{
  char cVar1;
  short sVar2;
  char *in_EAX;
  char *pcVar3;
  HANDLE pvVar4;
  int iVar5;
  char *pcVar6;
  undefined4 *puVar7;
  DWORD dwFlagsAndAttributes;
  char local_900 [264];
  int local_7f8;

  pcVar3 = _strrchr(in_EAX,0x5c);
  pcVar6 = in_EAX;
  if (pcVar3 != (char *)0x0) {
    pcVar6 = pcVar3 + 1;
  }
  _strrchr(in_EAX,0x5c);
  sVar2 = cache_file_find_slot_by_name();
  if (sVar2 != -1) {
    return 1;
  }
  cVar1 = cache_file_exists();
  if (cVar1 != '\0') {
    sVar2 = cache_file_find_oldest_slot(local_7f8);
    puVar7 = (undefined4 *)(&DAT_006a9434 + sVar2 * 0x80c);
    for (iVar5 = 0x200; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    _sprintf(local_900,"%s%s%s.map",&DAT_006f16d8,"maps\\",pcVar6);
    dwFlagsAndAttributes = 0x48000080;
    if (DAT_00721ef0 == 0) {
      os_platform_identify();
    }
    if (DAT_00721ef0 < 3) {
      dwFlagsAndAttributes = 0x8000080;
    }
    pvVar4 = CreateFileA(local_900,0x80000000,1,(LPSECURITY_ATTRIBUTES)0x0,4,dwFlagsAndAttributes,
                         (HANDLE)0x0);
    *(HANDLE *)(&DAT_006a9428 + sVar2 * 0x80c) = pvVar4;
    FUN_004435e0();
    return 1;
  }
  if (param_1 != '\0') {
    DAT_00722bbc = in_EAX;
    shell_display_fatal_error_dialog(0x89,0x7e,1);
    interface_handle_quit_request();
  }
  return 0;
}

Raw disassembly (0x443360-0x443462), objdump -d -M intel --start-address=0x443360
--stop-address=0x443463 bin/halo.exe, plus the tail at --start-address=0x443430
--stop-address=0x443490 (the two overlap at 0x443430-0x443462):

00443360: sub esp,0x900
00443366: push ebx
00443367: push ebp
00443368: push edi
00443369: mov ebx,eax                 ; ebx = name (kept for the whole function)
0044336b: push 0x5c
0044336d: push ebx
0044336e: call 0x623bc0               ; strrchr(name, '\\')            [1st call, -> ebp/basename]
00443373: mov ebp,eax
00443375: add esp,0x8
00443378: test ebp,ebp
0044337a: je 0x44337f
0044337c: inc ebp
0044337d: jmp 0x443381
0044337f: mov ebp,ebx
00443381: push 0x5c
00443383: push ebx
00443384: call 0x623bc0               ; strrchr(name, '\\')            [2nd call, redundant, -> edi]
00443389: add esp,0x8
0044338c: test eax,eax
0044338e: je 0x443393
00443390: inc eax
00443391: jmp 0x443395
00443393: mov eax,ebx
00443395: mov edi,eax
00443397: call 0x443770               ; cache_file_find_slot_by_name(edi=basename)
0044339c: cmp ax,0xffff
004433a0: je 0x4433ae
004433a2: pop edi
004433a3: pop ebp
004433a4: mov al,0x1
004433a6: pop ebx
004433a7: add esp,0x900
004433ad: ret
004433ae: push esi
004433af: lea esi,[esp+0x110]         ; esi = &header (local cache_file_header)
004433b6: mov eax,ebp                 ; eax = basename
004433b8: call 0x442bb0               ; cache_file_exists(basename, &header)
004433bd: test al,al
004433bf: je 0x443463                 ; not found: fail path
004433c5: mov eax,[esp+0x118]         ; = header.file_size (header+0x08)
004433cc: push eax
004433cd: mov eax,[esp+0x174]         ; = header.unknown_060 (header+0x60, full dword)
004433d4: call 0x4437b0               ; cache_file_find_oldest_slot(category=eax, size=[stack])
004433d9: push ebp
004433da: push 0x65fd7c               ; "maps\\"
004433df: mov ecx,0x200
004433e4: push 0x6f16d8               ; map_path_prefix
004433e9: push 0x65fd70               ; "%s%s%s.map"
004433ee: mov ebx,eax                 ; ebx = slot_index
004433f0: movsx esi,bx
004433f3: imul esi,esi,0x80c
004433f9: add esi,0x6a9428            ; esi = &cache_file_slots[slot_index]
004433ff: xor eax,eax
00443401: lea edi,[esi+0xc]           ; &slot.header
00443404: rep stos DWORD PTR es:[edi],eax   ; zero-fill 0x200 dwords (0x800 bytes)
00443406: lea ecx,[esp+0x24]          ; &path[0]
0044340a: push ecx
0044340b: call 0x623693               ; sprintf(path, "%s%s%s.map", map_path_prefix, "maps\\", basename)
00443410: mov eax,ds:0x721ef0         ; os_platform
00443415: add esp,0x18
00443418: test eax,eax
0044341a: mov edi,0x48000080
0044341f: jne 0x443426
00443421: call 0x5427e0               ; os_platform_identify()
00443426: cmp DWORD PTR ds:0x721ef0,0x3
0044342d: jge 0x443434
0044342f: mov edi,0x8000080
00443434: push 0x0                    ; template_file = NULL
00443436: push edi                    ; flags_and_attributes
00443437: push 0x4                    ; creation_disposition = OPEN_ALWAYS
00443439: push 0x0                    ; security = NULL
0044343b: push 0x1                    ; share = 1
0044343d: push 0x80000000             ; access = GENERIC_READ
00443442: lea edx,[esp+0x28]          ; &path[0]
00443446: push edx
00443447: call DWORD PTR ds:0x63a2b8  ; CreateFileA
0044344d: mov [esi],eax               ; cache_file_slots[slot_index].file = handle
0044344f: mov eax,ebx                 ; eax = slot_index
00443451: call 0x4435e0               ; cache_file_slot_read_header(slot_index)
00443456: pop esi
00443457: pop edi
00443458: pop ebp
00443459: mov al,0x1
0044345b: pop ebx
0044345c: add esp,0x900
00443462: ret
00443463: mov al,[esp+0x914]          ; = report_fatal_error, the original [E+4] stack argument
0044346a: test al,al
0044346c: je 0x44348a
0044346e: push 0x1
00443470: push 0x7e
00443472: push 0x89
00443477: mov ds:0x722bbc,ebx         ; shell_fatal_error_argument = name (full, not basename)
0044347d: call 0x57ea70               ; shell_display_fatal_error_dialog(0x89, 0x7e, 1)
00443482: add esp,0xc
00443485: call 0x499170               ; interface_handle_quit_request()
0044348a: pop esi
0044348b: pop edi
0044348c: pop ebp
0044348d: xor al,al
0044348f: pop ebx
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
