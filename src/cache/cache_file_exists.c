// cache_file_exists
// address 0x442bb0, size 186 bytes
// name confidence: 0.75 (already named by Ghidra; matches out/phase4/cache_types_notes.md's
// "cache_file_header" section, which cites this function as one of the three identical header
// validators)
// rewrite confidence: 0.75
// evidence: types/cache.h cache_file_header; out/phase4/cache_types_notes.md's five-part
// validation description, byte-identical to cache_file_load and cache_file_slot_read_header;
// raw disassembly at 0x442bb0-0x442c69 (objdump -d -M intel --start-address=0x442bb0
// --stop-address=0x442c70 bin/halo.exe) used to recover the EAX name argument (the third %s
// Ghidra's sprintf call dropped) and to confirm the two string literals at 0x65fd70
// ("%s%s%s.map") and 0x65fd7c ("maps\\").
// register convention: char *name in EAX; cache_file_header *header_out in ESI (unaff_ESI) --
// the caller supplies the 0x800-byte buffer the header is read into, this function does not
// allocate one itself.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "cache.h"


extern char map_path_prefix[]; // 0x006f16d8

// VERIFIED against disassembly 0x442bb0..0x442c69 (2026-09-30): the "%s%s%s.map" path build (prefix 0x6f16d8, "maps\\", name), the
//   CreateFileA/ReadFile arguments, the five header checks (0x800 bytes, 'head', 'foot' at +0x7fc, size in [0, 0x18000000],
//   name < 0x20, version 7) and the bl return match.
// blam-cc: name in EAX, header_out in ESI
// Builds "<map_path_prefix>maps\\<name>.map", opens it, reads its 0x800-byte header into
// *header_out, and runs the same five-part validation cache_file_load and
// cache_file_slot_read_header use. Returns 1 if the file exists and its header is valid, 0
// otherwise (including when the file cannot be opened at all).
uint8_t cache_file_exists(char *name, cache_file_header *header_out)
{
    char path[256];
    void *file;
    uint32_t bytes_read;
    uint8_t valid;
    char *name_scan;

    valid = 0;
    sprintf(path, "%s%s%s.map", map_path_prefix, "maps\\", name);
    file = CreateFileA(path, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)((void *)0), 3, 0, (void *)0);
    if (file != (void *)0xffffffff) {
        if (ReadFile(file, header_out, k_cache_file_header_size, (LPDWORD)(&bytes_read), (LPOVERLAPPED)((void *)0)) != 0 &&
            bytes_read == k_cache_file_header_size &&
            header_out->head == k_cache_file_head_signature &&
            header_out->foot == k_cache_file_foot_signature &&
            header_out->file_size >= 0 && header_out->file_size < k_cache_file_maximum_size + 1) {
            name_scan = header_out->name;
            while (*name_scan != '\0') {
                name_scan++;
            }
            if ((uint32_t)(name_scan - header_out->name) < k_cache_file_name_length &&
                header_out->version == k_cache_file_version) {
                valid = 1;
            }
        }
        CloseHandle(file);
    }
    return valid;
}

#if 0
Original Ghidra decompilation (0x442bb0):

undefined1 cache_file_exists(void)

{
  int iVar1;
  HANDLE hFile;
  BOOL BVar2;
  int *piVar3;
  undefined1 uVar4;
  int *unaff_ESI;
  DWORD local_104;
  char local_100 [256];
  undefined1 uVar5;

  uVar5 = 0;
  uVar4 = 0;
  _sprintf(local_100,"%s%s%s.map",&DAT_006f16d8,"maps\\");
  hFile = CreateFileA(local_100,0x80000000,1,(LPSECURITY_ATTRIBUTES)0x0,3,0,(HANDLE)0x0);
  if (hFile != (HANDLE)0xffffffff) {
    BVar2 = ReadFile(hFile,unaff_ESI,0x800,&local_104,(LPOVERLAPPED)0x0);
    uVar4 = uVar5;
    if (((((BVar2 != 0) && (local_104 == 0x800)) && (*unaff_ESI == 0x68656164)) &&
        ((unaff_ESI[0x1ff] == 0x666f6f74 && (-1 < unaff_ESI[2])))) && (unaff_ESI[2] < 0x18000001)) {
      piVar3 = unaff_ESI + 8;
      do {
        iVar1 = *piVar3;
        piVar3 = (int *)((int)piVar3 + 1);
      } while ((char)iVar1 != '\0');
      if (((uint)((int)piVar3 - ((int)unaff_ESI + 0x21)) < 0x20) && (unaff_ESI[1] == 7)) {
        uVar4 = 1;
      }
    }
    CloseHandle(hFile);
  }
  return uVar4;
}

Raw disassembly (0x442bb0-0x442c69), objdump -d -M intel --start-address=0x442bb0
--stop-address=0x442c70 bin/halo.exe:

00442bb0: sub esp,0x104
00442bb6: push ebx
00442bb7: push edi
00442bb8: push eax                  ; eax = name (the third %s Ghidra's call dropped)
00442bb9: push 0x65fd7c             ; "maps\\"
00442bbe: push 0x6f16d8             ; map_path_prefix
00442bc3: lea ecx,[esp+0x18]        ; ecx = &path[0]
00442bc7: push 0x65fd70             ; "%s%s%s.map"
00442bcc: push ecx
00442bcd: xor bl,bl                 ; valid = 0
00442bcf: call 0x623693             ; sprintf(path, "%s%s%s.map", map_path_prefix, "maps\\", name)
00442bd4: add esp,0x14
00442bd7: push 0x0
00442bd9: push 0x0
00442bdb: push 0x3
00442bdd: push 0x0
00442bdf: push 0x1
00442be1: push 0x80000000
00442be6: lea edx,[esp+0x24]        ; &path[0]
00442bea: push edx
00442beb: call DWORD PTR ds:0x63a2b8   ; CreateFileA
00442bf1: mov edi,eax
00442bf3: cmp edi,0xffffffff
00442bf6: je 0x442c5f
00442bf8: push 0x0
00442bfa: lea eax,[esp+0xc]         ; &bytes_read
00442bfe: push eax
00442bff: push 0x800
00442c04: push esi                  ; esi = header_out (caller-supplied buffer)
00442c05: push edi
00442c06: call DWORD PTR ds:0x63a2d8   ; ReadFile
00442c0c: test eax,eax
00442c0e: je 0x442c58
00442c10: cmp DWORD PTR [esp+0x8],0x800
00442c18: jne 0x442c58
00442c1a: cmp DWORD PTR [esi],0x68656164      ; header_out->head
00442c20: jne 0x442c58
00442c22: cmp DWORD PTR [esi+0x7fc],0x666f6f74 ; header_out->foot
00442c2c: jne 0x442c58
00442c2e: mov eax,DWORD PTR [esi+0x8]         ; header_out->file_size
00442c33: jl 0x442c58
00442c35: cmp eax,0x18000000
00442c3a: jg 0x442c58
00442c3c: lea eax,[esi+0x20]                  ; &header_out->name[0]
00442c42..442c47: strlen loop
00442c4b: cmp eax,0x1f
00442c4e: ja 0x442c58
00442c50: cmp DWORD PTR [esi+0x4],0x7         ; header_out->version
00442c54: jne 0x442c58
00442c56: mov bl,0x1                          ; valid = 1
00442c58: push edi
00442c59: call DWORD PTR ds:0x63a2f8   ; CloseHandle
00442c5f: pop edi
00442c60: mov al,bl
00442c62: pop ebx
00442c63: add esp,0x104
00442c69: ret
#endif
