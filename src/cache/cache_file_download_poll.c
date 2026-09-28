// cache_file_download_poll  (Ghidra: FUN_00442720; named per types/cache.h's cache_file_header
// comment "Return codes of cache_file_download_poll @0x442720 (raw) and the simplified codes
// cache_file_download_status @0x4434a0 maps them to.")
// address 0x442720, size 273 bytes
// name confidence: 0.45 (out/phase4/cache_functions.md: "Polls a background map-download's
// semaphores/state and returns a status code while writing the current download progress
// fraction (0..1) to the output pointer")
// rewrite confidence: 0.70 -- one of the four map_download_state glue functions
// out/phase4/cache_types_notes.md item 5 keeps in-module; the raw return code's bit-2 case (the
// `~status_flags` computation below) has no documented meaning, only the literal bit operation.
// evidence: types/cache.h map_download_state fields (status_flags 0x908, thread_busy 0x98c,
// thread 0x960, queued_file_count 0x110, finished_event 0x958, progress_event 0x95c,
// progress 0xaa4); raw disassembly (objdump -d -M intel --start-address=0x442720
// --stop-address=0x442840 bin/halo.exe), needed because Ghidra left the float constant compares
// as raw DAT_00672ac0/DAT_00672ac4 (0.0f and 1.0f, confirmed image constants used the same way
// throughout the math module, e.g. src/math/vector3d_randomize_direction.c).
// register convention: no register arguments; progress_out (float*) is the single stack
// parameter Ghidra recognized as param_1.
// UNSURE: the raw code returned when the download ended without being cancelled -- 0 or 2
// depending on bit 2 of status_flags -- has no established meaning beyond the literal bit test;
// preserved exactly. UNSURE: the "running" vs "idle-but-not-finished" values 4 and 3 from
// `4 - (finished_event signaled)` look swapped relative to cache_file_download_status's enum
// names, but that is exactly what the code computes.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "cache.h"

extern void __stdcall Sleep(uint32_t milliseconds); // 0x0063a29c IAT
extern uint32_t __stdcall WaitForSingleObject(void *handle, uint32_t timeout_ms); // 0x0063a310 IAT

extern map_download_state *map_download; // 0x006869c0

// blam-cc: progress_out is the recognized stack parameter (param_1); no register arguments
// Polls the multiplayer map downloader and returns a raw status code, writing the current
// progress fraction to *progress_out (clamped to [0, 1]) when it is available. If the download
// has ended (status_flags nonzero) or never started (no thread), returns 1 for a cancelled
// download or a 0/2 code derived from the remaining status_flags bits, with *progress_out forced
// to 0. If nothing is queued, returns 3 with *progress_out forced to 0. Otherwise waits (with no
// timeout) on the finished/progress events and returns 4 or 3 depending on whether the download
// thread has finished, leaving *progress_out untouched if the progress event was not signaled.
int16_t cache_file_download_poll(float *progress_out)
{
    uint32_t status_flags;
    uint32_t finished_signaled;
    uint32_t progress_ready;
    int16_t code;
    uint32_t raw;
    float progress;

    status_flags = map_download->status_flags;
    if (map_download->thread_busy != 0) {
        Sleep(0x10);
    }

    if (status_flags != 0 || map_download->thread == 0) {
        if ((status_flags & 2) != 0) {
            *progress_out = 0.0f;
            return 1;
        }
        *progress_out = 0.0f;
        raw = (uint8_t)(~(uint8_t)status_flags);
        raw = raw >> 1;
        return (int16_t)(raw & 2);
    }

    if (map_download->queued_file_count < 1) {
        *progress_out = 0.0f;
        return 3;
    }

    finished_signaled = WaitForSingleObject(map_download->finished_event, 0);
    code = (int16_t)(4 - (finished_signaled != 0));

    progress_ready = WaitForSingleObject(map_download->progress_event, 0);
    if (progress_ready == 0) {
        progress = map_download->progress;
        if (progress < 0.0f) {
            *progress_out = 0.0f;
        } else if (1.0f < progress) {
            *progress_out = 1.0f;
        } else {
            *progress_out = progress;
        }
    }
    return code;
}

#if 0
Original Ghidra decompilation (0x442720):

ushort FUN_00442720(undefined4 *param_1)

{
  uint uVar1;
  DWORD DVar2;
  ushort uVar3;

  uVar1 = *(uint *)(PTR_DAT_006869c0 + 0x908);
  if (PTR_DAT_006869c0[0x98c] != '\0') {
    Sleep(0x10);
  }
  if ((uVar1 != 0) || (*(int *)(PTR_DAT_006869c0 + 0x960) == 0)) {
    if ((uVar1 & 2) != 0) {
      *param_1 = 0;
      return 1;
    }
    *param_1 = 0;
    return (byte)~(byte)uVar1 >> 1 & 2;
  }
  if (*(int *)(PTR_DAT_006869c0 + 0x110) < 1) {
    *param_1 = 0;
    return 3;
  }
  DVar2 = WaitForSingleObject(*(HANDLE *)(PTR_DAT_006869c0 + 0x958),0);
  uVar3 = 4 - (DVar2 != 0);
  DVar2 = WaitForSingleObject(*(HANDLE *)(PTR_DAT_006869c0 + 0x95c),0);
  if (DVar2 == 0) {
    if (*(float *)(PTR_DAT_006869c0 + 0xaa4) < 0.0) {
      *param_1 = 0;
      return uVar3;
    }
    if (1.0 < *(float *)(PTR_DAT_006869c0 + 0xaa4)) {
      *param_1 = 0x3f800000;
      return uVar3;
    }
    *param_1 = *(undefined4 *)(PTR_DAT_006869c0 + 0xaa4);
  }
  return uVar3;
}

Raw disassembly (0x442720-0x442830), objdump -d -M intel --start-address=0x442720
--stop-address=0x442840 bin/halo.exe:

00442720: mov eax,ds:0x6869c0
00442725: mov cl,[eax+0x98c]              ; thread_busy
0044272b: test cl,cl
0044272d: push ebx
0044272e: mov ebx,[eax+0x908]              ; status_flags
00442734: push ebp
00442735: mov ebp,[esp+0xc]                ; progress_out
00442739: je 0x442748
0044273b: push 0x10
0044273d: call dword ptr ds:0x63a29c       ; Sleep(0x10)
00442743: mov eax,ds:0x6869c0
00442748: test ebx,ebx
0044274a: jne 0x442809
00442750: mov ecx,[eax+0x960]              ; thread
00442756: test ecx,ecx
00442758: je 0x442809
0044275e: mov ecx,[eax+0x110]              ; queued_file_count
00442764: test ecx,ecx
00442766: jle 0x4427fa
0044276c: mov eax,[eax+0x958]              ; finished_event
00442772: push esi
00442773: push edi
00442774: mov edi,ds:0x63a310              ; &WaitForSingleObject
0044277a: push ebx
0044277b: push eax
0044277c: call edi
0044277e: mov ecx,ds:0x6869c0
00442784: mov edx,[ecx+0x95c]              ; progress_event
0044278a: mov esi,eax
0044278c: neg esi
0044278e: push ebx
0044278f: sbb esi,esi
00442791: push edx
00442792: add esi,0x4                      ; esi = 4 - (finished != 0)
00442795: call edi
00442797: test eax,eax
00442799: jne 0x4427f2                     ; progress_event not signaled: skip the write
0044279b: mov ecx,ds:0x6869c0
004427a1: fld dword ptr [ecx+0xaa4]        ; progress
004427a7: fcomp dword ptr ds:0x672ac0      ; 0.0f
004427ad: fnstsw ax
004427af: test ah,0x5
004427b2: jp 0x4427c5
004427b4: fld dword ptr ds:0x672ac0        ; 0.0f
004427ba: pop edi
004427bb: mov ax,si
004427be: fstp dword ptr [ebp+0x0]
004427c1: pop esi
004427c2: pop ebp
004427c3: pop ebx
004427c4: ret
004427c5: fld dword ptr [ecx+0xaa4]
004427cb: fcomp dword ptr ds:0x672ac4      ; 1.0f
004427d1: fnstsw ax
004427d3: test ah,0x41
004427d6: jne 0x4427e9
004427d8: fld dword ptr ds:0x672ac4        ; 1.0f
004427de: pop edi
004427df: mov ax,si
004427e2: fstp dword ptr [ebp+0x0]
004427e5: pop esi
004427e6: pop ebp
004427e7: pop ebx
004427e8: ret
004427e9: fld dword ptr [ecx+0xaa4]
004427ef: fstp dword ptr [ebp+0x0]
004427f2: pop edi
004427f3: mov ax,si
004427f6: pop esi
004427f7: pop ebp
004427f8: pop ebx
004427f9: ret
004427fa: mov dword ptr [ebp+0x0],0x0
00442801: pop ebp
00442802: mov eax,0x3
00442807: pop ebx
00442808: ret
00442809: test bl,0x2
0044280c: je 0x44281d
0044280e: mov dword ptr [ebp+0x0],0x0
00442815: pop ebp
00442816: mov eax,0x1
0044281b: pop ebx
0044281c: ret
0044281d: not bl
0044281f: movzx eax,bl
00442822: mov dword ptr [ebp+0x0],0x0
00442829: shr eax,1
0044282b: pop ebp
0044282c: and eax,0x2
0044282f: pop ebx
00442830: ret
#endif
