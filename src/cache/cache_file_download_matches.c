// cache_file_download_matches  (Ghidra: FUN_004432f0; named per the reference in
// types/cache.h's globals list: "map_download_name 0x006ac474 basename compared by
// cache_file_download_matches", and per src/cache/cache_file_request_map.c's forward
// declaration which already uses this name)
// address 0x4432f0, size 106 bytes
// name confidence: 0.3 (out/phase4/cache_functions.md: "When a map download is pending,
// compares the basename of a given path against the name of the map currently being
// downloaded")
// rewrite confidence: 0.55
// evidence: types/cache.h globals (map_download_slot_index 0x006ac472, map_download_name
// 0x006ac474); raw disassembly (objdump -d -M intel --start-address=0x4432f0
// --stop-address=0x44335c bin/halo.exe), needed because Ghidra rendered the inlined byte-compare
// loop with an unresolved `in_EAX` and a `sbb`-based comparison result that only ever gets
// tested against zero; the disassembly shows the "not idle" exit at 0x443356 is a plain
// `xor al,al; ret`, confirming the idle case returns 0, not the `(uint)in_EAX & 0xffffff00`
// expression Ghidra printed for it.
// register convention: char *name in EAX (in_EAX).

#include "crt.h"
#include "tags.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern int16_t map_download_slot_index; // 0x006ac472, 0xffff (-1) when idle
extern char map_download_name[0x20];    // 0x006ac474

// blam-cc: name in EAX
// Returns 1 if the basename of `name` (the text after the last '\\', or all of `name` if there
// is none) matches map_download_name, the file currently being downloaded. Returns 0 if nothing
// is downloading (map_download_slot_index == -1) or the names differ.
uint8_t cache_file_download_matches(char *name)
{
    char *slash;
    char *basename;
    char *a;
    char *b;
    uint8_t ca;
    uint8_t cb;

    if (map_download_slot_index == -1) {
        return 0;
    }

    slash = strrchr(name, '\\');
    basename = (slash != 0) ? slash + 1 : name;

    a = map_download_name;
    b = basename;
    for (;;) {
        ca = (uint8_t)*a;
        cb = (uint8_t)*b;
        if (ca != cb) {
            return 0;
        }
        if (ca == 0) {
            return 1;
        }
        a = a + 1;
        b = b + 1;
    }
}

#if 0
Original Ghidra decompilation (0x4432f0):

uint FUN_004432f0(void)

{
  byte bVar1;
  byte *in_EAX;
  char *pcVar2;
  byte *pbVar3;
  bool bVar4;

  if (DAT_006ac472 != -1) {
    pcVar2 = _strrchr((char *)in_EAX,0x5c);
    if (pcVar2 != (char *)0x0) {
      in_EAX = (byte *)(pcVar2 + 1);
    }
    pbVar3 = &DAT_006ac474;
    do {
      bVar1 = *pbVar3;
      bVar4 = bVar1 < *in_EAX;
      if (bVar1 != *in_EAX) {
LAB_00443348:
        in_EAX = (byte *)((1 - (uint)bVar4) - (uint)(bVar4 != 0));
        goto LAB_0044334d;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar3[1];
      bVar4 = bVar1 < in_EAX[1];
      if (bVar1 != in_EAX[1]) goto LAB_00443348;
      pbVar3 = pbVar3 + 2;
      in_EAX = in_EAX + 2;
    } while (bVar1 != 0);
    in_EAX = (byte *)0x0;
LAB_0044334d:
    if (in_EAX == (byte *)0x0) {
      return 1;
    }
  }
  return (uint)in_EAX & 0xffffff00;
}

Raw disassembly (0x4432f0-0x443359), objdump -d -M intel --start-address=0x4432f0
--stop-address=0x44335c bin/halo.exe:

004432f0: cmp word ptr ds:0x6ac472,0xffff    ; map_download_slot_index
004432f8: push esi
004432f9: mov esi,eax                        ; esi = name
004432fb: je 0x443356                        ; idle: fall straight to "return 0"
004432fd: push 0x5c
004432ff: push esi
00443300: call 0x623bc0                      ; strrchr(name, '\\')
00443305: add esp,0x8
00443308: test eax,eax
0044330a: je 0x44330f
0044330c: inc eax                            ; basename = slash + 1
0044330d: jmp 0x443311
0044330f: mov eax,esi                        ; basename = name
00443311: mov esi,eax                        ; esi = basename
00443313: mov eax,0x6ac474                   ; eax = &map_download_name
00443318: push ebx
00443320: mov dl,[eax]
00443322: mov bl,[esi]
00443324: mov cl,dl
00443326: cmp dl,bl
00443328: jne 0x443348
0044332a: test cl,cl
0044332c: je 0x443344                        ; both NUL: equal
0044332e: mov dl,[eax+0x1]
00443331: mov bl,[esi+0x1]
00443334: mov cl,dl
00443336: cmp dl,bl
00443338: jne 0x443348
0044333a: add eax,0x2
0044333d: add esi,0x2
00443340: test cl,cl
00443342: jne 0x443320
00443344: xor eax,eax                        ; equal
00443346: jmp 0x44334d
00443348: sbb eax,eax
0044334a: sbb eax,0xffffffff                 ; not equal (nonzero)
0044334d: test eax,eax
0044334f: pop ebx
00443350: jne 0x443356
00443352: mov al,0x1                         ; equal: return 1
00443354: pop esi
00443355: ret
00443356: xor al,al                          ; idle or not equal: return 0
00443358: pop esi
00443359: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
