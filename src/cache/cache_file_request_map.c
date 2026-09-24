// cache_file_request_map  (Ghidra: FUN_00442640; named per types/cache.h's map_download_state
// comment, which cites this function directly)
// address 0x442640, size 212 bytes
// name confidence: 0.55 (out/phase4/cache_functions.md: "Ensures a requested map is either
// already loaded, currently downloading, or gets a load/download attempt started, used when
// joining a multiplayer game")
// rewrite confidence: 0.60 -- see the UNSURE notes below; this is one of the four functions
// out/phase4/cache_types_notes.md item 5 identifies as glue over the foreign, mostly-unresolved
// map_download_state object, kept in-module (not skipped) because the glue logic itself belongs
// to cache, unlike the Win32 retry helpers in item 6.
// evidence: types/cache.h map_download_state (thread 0x960, thread_busy 0x98c) and the globals
// list (map_download_in_progress 0x006ac470); raw disassembly at 0x442640-0x442713 (objdump -d
// -M intel --start-address=0x442640 --stop-address=0x442720 bin/halo.exe), needed because Ghidra
// elided the ESI name argument to every callee and the whole tail past the last documented
// map_download_state field.
// register convention: char *name in ESI (unaff_ESI); uint8_t quit_on_fail is the second
// (stack) parameter Ghidra did recognize (param_1).
// UNSURE: the four globals at 0x00718fac-0x00718fb1 are outside every range
// out/phase4/cache_types_notes.md attributes to this module; reproduced as an opaque block with
// no claimed type or owner. UNSURE: `quit_on_fail`'s exact meaning -- true always calls
// interface_handle_quit_request(); false instead resets that opaque block when its first word is
// -1. Both are preserved literally.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "cache.h"

extern char *strrchr(const char *str, int ch); // 00623bc0 _strrchr
extern void SetThreadPriority(void *thread, int32_t priority);
extern void interface_handle_quit_request(void); // 0x499170

extern int16_t cache_file_find_slot_by_name(char *name); // blam-cc: EDI; 0x443770
extern uint8_t cache_file_download_matches(char *name); // blam-cc: EAX; this module, 0x4432f0
extern void cache_file_download_finish(void); // this module, 0x443540
extern int16_t cache_file_download_status_get(float *progress_out, int32_t unaff_ecx);
    // blam-cc: EAX, ECX; this module, 0x4434a0. Named with a _get suffix: the natural name
    // collides with types/cache.h's own cache_file_download_status enum typedef.
extern uint8_t cache_file_open_by_name(char *name, uint8_t report_fatal_error); // blam-cc: EAX,
    // stack; this module, 0x443360

extern uint8_t map_download_in_progress; // 0x006ac470
extern map_download_state *map_download; // 0x006869c0

extern int16_t unknown_00718fac; // UNSURE, see file header
extern int16_t unknown_00718fae; // UNSURE
extern uint8_t unknown_00718fb0; // UNSURE
extern uint8_t unknown_00718fb1; // UNSURE

// blam-cc: name in ESI
// Makes sure `name` (a map file, basename taken from the last '\\') is available: returns 1 if a
// cache_file_slot for it is already open; otherwise resolves any in-flight download (finishing a
// mismatched one, or reporting not-yet-done / retrying a failed one), and if nothing is
// downloading, starts a fresh open/download attempt. quit_on_fail controls what happens once that
// resolves: true always requests an application quit; false instead resets an unrelated small
// timer-like block the first time it is seen uninitialized.
uint8_t cache_file_request_map(char *name, uint8_t quit_on_fail)
{
    char *slash;
    char *basename;
    short slot_index;
    uint8_t opened;
    int16_t status;
    float progress;

    slash = strrchr(name, '\\');
    basename = (slash != 0) ? slash + 1 : name;

    slot_index = cache_file_find_slot_by_name(basename);
    if (slot_index != -1) {
        return 1;
    }

    if (map_download_in_progress != 0) {
        if (cache_file_download_matches(name) == 0) {
            cache_file_download_finish();
        }
        if (map_download_in_progress != 0) {
            status = cache_file_download_status_get(&progress, 0); // 2nd arg unused: only the
                // unreachable default case in that function reads it
            if (status == 2) {
                goto resolved;
            }
            if (status != 1) {
                return 0;
            }
            cache_file_download_finish();
            return 0;
        }
    }

    map_download->thread_busy = 0;
    SetThreadPriority(map_download->thread, 0);
    opened = cache_file_open_by_name(name, 0);
    if (opened != 0) {
        return 0;
    }

resolved:
    if (quit_on_fail == 0) {
        if (unknown_00718fac == -1) {
            unknown_00718fac = 0x23;
            unknown_00718fae = 0;
            unknown_00718fb0 = 0;
            unknown_00718fb1 = 0;
        }
        return 0;
    }
    interface_handle_quit_request();
    return 0;
}

#if 0
Original Ghidra decompilation (0x442640):

undefined4 FUN_00442640(char param_1)

{
  undefined *puVar1;
  char cVar2;
  short sVar3;
  char *unaff_ESI;

  _strrchr(unaff_ESI,0x5c);
  sVar3 = cache_file_find_slot_by_name();
  if (sVar3 != -1) {
    return 1;
  }
  if (DAT_006ac470 != '\0') {
    cVar2 = FUN_004432f0();
    if (cVar2 == '\0') {
      FUN_00443540();
    }
    if (DAT_006ac470 != '\0') {
      sVar3 = FUN_004434a0();
      if (sVar3 != 2) {
        if (sVar3 != 1) {
          return 0;
        }
        FUN_00443540();
        return 0;
      }
      goto LAB_004426d8;
    }
  }
  puVar1 = PTR_DAT_006869c0;
  PTR_DAT_006869c0[0x98c] = 0;
  SetThreadPriority(*(HANDLE *)(puVar1 + 0x960),0);
  cVar2 = FUN_00443360(0);
  if (cVar2 != '\0') {
    return 0;
  }
LAB_004426d8:
  if (param_1 == '\0') {
    if (DAT_00718fac == -1) {
      DAT_00718fac = 0x23;
      DAT_00718fae = 0;
      DAT_00718fb0 = 0;
      DAT_00718fb1 = 0;
    }
    return 0;
  }
  interface_handle_quit_request();
  return 0;
}

Raw disassembly (0x442640-0x442713), objdump -d -M intel --start-address=0x442640
--stop-address=0x442720 bin/halo.exe:

00442640: push ecx
00442641: push ebx
00442642: push edi
00442643: push 0x5c
00442645: push esi                  ; esi = name
00442646: xor ebx,ebx
00442648: call 0x623bc0             ; strrchr(name, '\\')
0044264d: add esp,0x8
00442650: cmp eax,ebx
00442652: je 0x442657
00442654: inc eax
00442655: jmp 0x442659
00442657: mov eax,esi
00442659: mov edi,eax               ; edi = basename
0044265b: call 0x443770             ; cache_file_find_slot_by_name(edi)
00442660: or edi,0xffffffff
00442663: cmp ax,di
00442666: je 0x442670
00442668: mov bl,0x1
0044266a: pop edi
0044266b: mov al,bl
0044266d: pop ebx
0044266e: pop ecx
0044266f: ret
00442670: cmp BYTE PTR ds:0x6ac470,bl
00442676: je 0x4426b0
00442678: mov eax,esi
0044267a: call 0x4432f0             ; cache_file_download_matches(esi)
0044267f: test al,al
00442681: jne 0x442688
00442683: call 0x443540             ; cache_file_download_finish
00442688: cmp BYTE PTR ds:0x6ac470,bl
0044268e: je 0x4426b0
00442690: lea eax,[esp+0x8]         ; &progress
00442694: call 0x4434a0             ; cache_file_download_status(eax)
00442699: cmp ax,0x2
0044269d: je 0x4426d8
0044269f: cmp ax,0x1
004426a3: jne 0x44270e
004426a5: call 0x443540             ; cache_file_download_finish
004426aa: pop edi
004426ab: mov al,bl
004426ad: pop ebx
004426ae: pop ecx
004426af: ret
004426b0: mov eax,ds:0x6869c0       ; eax = map_download
004426b5: mov BYTE PTR [eax+0x98c],bl   ; map_download->thread_busy = 0
004426bb: mov eax,DWORD PTR [eax+0x960] ; map_download->thread
004426c1: push ebx
004426c2: push eax
004426c3: call DWORD PTR ds:0x63a308    ; SetThreadPriority(thread, 0)
004426c9: push ebx                  ; report_fatal_error = 0
004426ca: mov eax,esi               ; eax = name
004426cc: call 0x443360             ; cache_file_open_by_name(esi, 0)
004426d1: add esp,0x4
004426d4: test al,al
004426d6: jne 0x44270e
004426d8: cmp BYTE PTR [esp+0x10],bl   ; quit_on_fail
004426dc: je 0x4426e9
004426de: call 0x499170             ; interface_handle_quit_request
004426e3: pop edi
004426e4: mov al,bl
004426e6: pop ebx
004426e7: pop ecx
004426e8: ret
004426e9: cmp WORD PTR ds:0x718fac,di
004426f0: jne 0x44270e
004426f2: mov WORD PTR ds:0x718fac,0x23
004426fb: mov WORD PTR ds:0x718fae,bx
00442702: mov BYTE PTR ds:0x718fb0,bl
00442708: mov BYTE PTR ds:0x718fb1,bl
0044270e: pop edi
0044270f: mov al,bl
00442711: pop ebx
00442712: pop ecx
00442713: ret
#endif
