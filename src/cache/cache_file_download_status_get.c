// cache_file_download_status_get  (Ghidra: FUN_004434a0; named per types/cache.h's
// cache_file_download_status comment, which cites this function directly -- renamed with a
// _get suffix because the natural name collides with types/cache.h's own
// `cache_file_download_status` enum typedef, which this module's rules forbid editing)
// address 0x4434a0, size 78 bytes
// name confidence: 0.4 (out/phase4/cache_functions.md: "Translates the raw map-download
// progress status into a simplified done/pending/failed code, invalidating the slot's cached
// file time when the download restarts" -- the last clause is not quite what the code does; see
// below)
// rewrite confidence: 0.75
// evidence: types/cache.h cache_file_download_status enum (the raw codes this switches on, 0-4,
// are exactly cache_file_download_poll's return values); types/cache.h globals
// (map_download_slot_index 0x006ac472, cache_file_slots 0x006a9428); raw disassembly (objdump
// -d -M intel --start-address=0x4434a0 --stop-address=0x4434ef bin/halo.exe), needed for the
// jump-table case targets and to confirm the "in_ECX" default case really does read back a
// stack slot the function itself reserved (push ecx) and never writes -- i.e. whatever ECX held
// at entry, not a genuine local.
// register convention: float *progress_out in EAX (forwarded straight through to
// cache_file_download_poll's stack argument); the unreachable default case returns whatever was
// in ECX at entry, reproduced as a second, likely-never-meaningful parameter.
// UNSURE: raw case 2 sets cache_file_slots[map_download_slot_index].file to -1 (the slot's
// HANDLE, offset 0x00) rather than touching last_write_time as out/phase4/cache_functions.md's
// summary suggests; the disassembly is unambiguous that it is offset 0x00. UNSURE: the default
// case is unreachable in practice (cache_file_download_poll only ever returns 0-4), so
// `unaff_ecx`'s value is never observed; preserved literally rather than omitted.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t cache_file_download_poll(float *progress_out); // blam-cc: stack; this module, 0x442720

extern int16_t map_download_slot_index;                  // 0x006ac472
extern cache_file_slot cache_file_slots[k_cache_file_slot_count]; // 0x006a9428

// blam-cc: progress_out in EAX; unaff_ecx in ECX, read only by the unreachable default case
// Polls the download and remaps its raw 0-4 status to a 3-way result cache_file_request_map
// acts on: 2 for "resolved" (failed, cancelled, or reset -- reset additionally invalidates the
// slot this download was writing into by setting its file handle to -1), 0 for idle, 1 for
// still running.
int16_t cache_file_download_status_get(float *progress_out, int32_t unaff_ecx)
{
    int16_t raw;

    raw = cache_file_download_poll(progress_out);
    switch (raw) {
    case 0:
    case 1:
        return 2;
    case 2:
        cache_file_slots[map_download_slot_index].file = (void *)0xffffffff;
        return 2;
    case 3:
        return 0;
    case 4:
        return 1;
    default:
        return (int16_t)unaff_ecx;
    }
}

#if 0
Original Ghidra decompilation (0x4434a0):

undefined4 FUN_004434a0(void)

{
  undefined2 uVar1;
  undefined4 in_ECX;

  uVar1 = FUN_00442720();
  switch(uVar1) {
  case 0:
  case 1:
    return 2;
  case 2:
    *(undefined4 *)(&DAT_006a9428 + DAT_006ac472 * 0x80c) = 0xffffffff;
    return 2;
  case 3:
    return 0;
  case 4:
    return 1;
  default:
    return in_ECX;
  }
}

Raw disassembly (0x4434a0-0x4434ed), objdump -d -M intel --start-address=0x4434a0
--stop-address=0x4434ef bin/halo.exe:

004434a0: push ecx                       ; reserve a stack slot (unaff_ecx, see header note)
004434a1: push eax                       ; progress_out (the real argument to download_poll)
004434a2: call 0x442720                  ; cache_file_download_poll(progress_out)
004434a7: movsx eax,ax
004434aa: add esp,0x4
004434ad: cmp eax,0x4
004434b0: ja 0x4434e9                    ; default: unreachable in practice
004434b2: jmp DWORD PTR [eax*4+0x4434f0] ; jump table, 5 entries for raw codes 0-4
004434b9: mov eax,0x2                    ; case 0, case 1 (shared target)
004434be: pop ecx
004434bf: ret
004434c0: movsx ecx,WORD PTR ds:0x6ac472 ; case 2: map_download_slot_index
004434c7: imul ecx,ecx,0x80c
004434cd: mov DWORD PTR [ecx+0x6a9428],0xffffffff  ; cache_file_slots[slot].file = -1
004434d7: mov eax,0x2
004434dc: pop ecx
004434dd: ret
004434de: xor eax,eax                    ; case 3
004434e0: pop ecx
004434e1: ret
004434e2: mov eax,0x1                    ; case 4
004434e7: pop ecx
004434e8: ret
004434e9: mov eax,DWORD PTR [esp]        ; default: the never-popped "push ecx" slot
004434ec: pop ecx
004434ed: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
