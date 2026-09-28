// unicode_string_list_get_string  (Ghidra: already named; out/phase4/networking_functions.md /
// out/phase2/networking/01.md)
// address 0x4b8d30, size 107 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase4/networking_functions.md ("Looks up the Nth localized string from a
// unicode_string_list tag block (index passed in CX) and returns it in a shared scratch
// buffer."); out/phase4/networking_types_notes.md ("a tag accessor for the unicode_string_list
// tag, which types/tags.h already defines"); modules.json reassigns this address to the game
// module (0.85, "networking types agent: unicode_string_list accessor used by the description
// builder" -- its only caller, multiplayer_game_variant_description_generate @0x4b8da0, is a
// game-variant UI text builder, not yet rewritten). types/tags.h UnicodeStringList /
// UnicodeStringListString / TagDataOffset match the walked layout exactly (0x14-byte stride,
// {size, flags, file_offset, pointer, definition}); types/cache.h tag_instance (+0x14 == data).
// Confirmed against objdump disassembly of bin/halo.exe 0x4b8d30..0x4b8d9b, which pins both
// register arguments and the exact tag-group constant.
// register convention: EAX = path (char *), pushed as tag_lookup's stack argument before EDI is
// set up -- blam-cc: EAX. ECX = index (int16_t; only the low word, aliased through ESI, is ever
// read) -- blam-cc: ECX. The tag group passed to tag_lookup is hardcoded to 'ustr'
// (0x75737472), not a caller argument.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include <wchar.h>

extern tag_instance *tag_instances;            // 0x0087bc14

// 0x00671fac is not an empty string and not a pointer: it holds the characters of
// L"<missing string>", the engine-wide placeholder a failed unicode_string_list lookup
// falls back to. Ghidra prints it as `&PTR_DAT_00671fac` only because its first four
// bytes ("<m") happen to look like a pointer value.
extern wchar_t missing_string_text[];          // 0x00671fac, L"<missing string>"

// Shared single-instance scratch buffer for the decoded string. Also written by
// multiplayer_game_variant_description_generate (0x4b8da0, this function's only caller, which
// also inlines this tag walk five times) and read/written by an unrelated interface-module function (0x4b55d0); this file
// only owns the extern declaration, not the storage.
extern wchar_t unicode_string_list_scratch_buffer; // 0x006b5c58

extern datum_index tag_lookup(tag_group group, char *path);       // 0x442550

// blam-cc: path in EAX, index in ECX (low 16 bits only)
// Looks up the unicode_string_list tag named `path` and returns its `index`'th string, copied
// into the shared scratch buffer. Falls back to the shared L"<missing string>" placeholder when the
// tag cannot be found or `index` is out of range. When a string is found, its last wide character (the tag
// data's own list-entry terminator/padding character) is overwritten with a NUL before the
// copy -- UNSURE why the tag data isn't already terminated one character earlier; this trim is
// unconditional whenever the raw entry size is positive.
wchar_t *unicode_string_list_get_string(char *path, int16_t index)
{
    datum_index tag_id;
    wchar_t *source;
    UnicodeStringList *list;
    UnicodeStringListString *entry;
    int32_t char_count;

    tag_id = tag_lookup(0x75737472, path); // 'ustr'
    source = missing_string_text;

    if (tag_id != k_datum_index_none && index >= 0) {
        list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;
        if (index < (int32_t)list->strings.count) {
            entry = (UnicodeStringListString *)list->strings.pointer + index;
            char_count = (int32_t)entry->string.size;
            if (char_count > 0) {
                source = (wchar_t *)entry->string.pointer;
                source[(char_count / 2) - 1] = 0;
            }
        }
    }

    wcscpy(&unicode_string_list_scratch_buffer, source);
    return &unicode_string_list_scratch_buffer;
}

#if 0
Original Ghidra decompilation (0x4b8d30):

undefined * unicode_string_list_get_string(void)

{
  uint *puVar1;
  int *piVar2;
  uint uVar3;
  short in_CX;
  undefined **_Source;

  uVar3 = tag_lookup();
  _Source = &PTR_DAT_00671fac;
  if (((uVar3 != 0xffffffff) &&
      (piVar2 = *(int **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), -1 < in_CX)) &&
     ((int)in_CX < *piVar2)) {
    puVar1 = (uint *)(piVar2[1] + in_CX * 0x14);
    uVar3 = *puVar1;
    if (0 < (int)uVar3) {
      _Source = (undefined **)puVar1[3];
      *(undefined2 *)((int)_Source + ((uVar3 & 0xfffffffe) - 2)) = 0;
    }
  }
  _wcscpy((wchar_t *)&DAT_006b5c58,(wchar_t *)_Source);
  return &DAT_006b5c58;
}

Disassembly (objdump -d -M intel bin/halo.exe, 0x4b8d30..0x4b8d9b), used to pin the register
arguments Ghidra could not name:

004b8d30:
  56                   push   esi
  57                   push   edi
  50                   push   eax                      ; path arg for tag_lookup (EAX at entry)
  bf 72 74 73 75       mov    edi,0x75737472            ; 'ustr'
  8b f1                mov    esi,ecx                   ; index arg (ECX at entry)
  e8 11 98 f8 ff       call   0x442550                  ; tag_lookup
  83 c4 04             add    esp,0x4
  83 f8 ff             cmp    eax,0xffffffff
  ba ac 1f 67 00       mov    edx,0x671fac
  74 39                je     0x4b8d85
  8b 0d 14 bc 87 00    mov    ecx,DWORD PTR ds:0x87bc14
  25 ff ff 00 00       and    eax,0xffff
  c1 e0 05             shl    eax,0x5
  66 85 f6             test   si,si
  8b 44 08 14          mov    eax,DWORD PTR [eax+ecx*1+0x14]
  7c 22                jl     0x4b8d85
  0f bf ce             movsx  ecx,si
  3b 08                cmp    ecx,DWORD PTR [eax]
  7d 1b                jge    0x4b8d85
  8b 40 04             mov    eax,DWORD PTR [eax+0x4]
  8d 0c 89             lea    ecx,[ecx+ecx*4]
  8d 0c 88             lea    ecx,[eax+ecx*4]
  8b 01                mov    eax,DWORD PTR [ecx]
  85 c0                test   eax,eax
  7e 0c                jle    0x4b8d85
  8b 51 0c             mov    edx,DWORD PTR [ecx+0xc]
  d1 e8                shr    eax,1
  66 c7 44 42 fe 00 00 mov    WORD PTR [edx+eax*2-0x2],0x0
  52                   push   edx
  68 58 5c 6b 00       push   0x6b5c58
  e8 2a ce 16 00       call   0x625bba                  ; _wcscpy
  83 c4 08             add    esp,0x8
  5f                   pop    edi
  b8 58 5c 6b 00       mov    eax,0x6b5c58
  5e                   pop    esi
  c3                   ret
#endif
