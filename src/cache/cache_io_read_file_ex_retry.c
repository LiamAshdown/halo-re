// cache_io_read_file_ex_retry  (Ghidra: FUN_00442c70; renamed -- not Bungie-attested, chosen to
// describe what the code does. out/phase4/cache_types_notes.md item 6 classified this as a
// "generic Win32 retry helper" and skipped it; that is only half right -- it is generic in its
// Win32 usage but it is cache-specific in its data, because the ESI object it zero-fills and
// hands to ReadFileEx is a cache_io_request, and the APC it installs is one of this module's own
// two completion routines. Written here so that its two callers stop declaring an invented
// prototype for it.)
// address 0x442c70, size 109 bytes
// name confidence: 0.35   rewrite confidence: 0.8
// evidence: types/cache.h cache_io_request -- the five dwords this function zeroes at ESI+0x00
// through ESI+0x10 are exactly the OVERLAPPED prefix the struct comment describes, and the one
// field it then writes, ESI+0x08, is OVERLAPPED.Offset. The five register/stack arguments were
// recovered from both call sites plus the callee body with objdump -d -M intel
// --start-address=0x442c70 --stop-address=0x442ce0 bin/halo.exe:
//   cache_file_slot_read_header @0x4435e0 (0x443657-0x443679): ESI = a cache_io_request-shaped
//     stack object at frame+0x1c whose completion at +0x24 is the {&header_read_ok, 0, 0} record
//     written at frame+0x40, EBX = 0x800, EDX = 0, EDI = cache_io_completion_routine @0x443b00.
//   cache_io_thread_proc_async @0x443940 (0x4439e1-0x4439f6): ESI = the queued request itself,
//     EBX = request->size, EDX = request->offset, EDI = the queue APC at 0x443ae0.
// register convention: request in ESI, size in EBX, offset in EDX, completion_routine in EDI.
// The three stack parameters Ghidra recognizes are (read_file_ex, file, buffer); read_file_ex is
// the value of the ReadFileEx IAT slot at 0x0063a27c, passed by value and called indirectly, not
// the slot's address.
//
// UNSURE: the retry loop is unbounded. GetLastError's result is fetched and discarded (the
// original never tests it), so a permanently failing read spins here forever at roughly one
// attempt per SleepEx(0) -- reproduced literally rather than "fixed".

#include "win32.h"
#include "tags.h"
#include "cache.h"
#include "fn_cache.h"


// The Win32 ReadFileEx signature, as this function calls it through the pointer it is handed.
typedef int32_t (*read_file_ex_procedure)(void *file, void *buffer, uint32_t bytes_to_read,
    cache_io_request *overlapped, void *completion_routine);

// blam-cc: request in ESI, size in EBX, offset in EDX, completion_routine in EDI
// Issues one overlapped read through ReadFileEx and retries it until it succeeds. Clears the
// request's OVERLAPPED prefix, stamps the file offset into it, then drains any already-queued
// APC with an alertable SleepEx(0) and clears the thread's last-error before each attempt. A
// failed attempt is retried indefinitely; the caller learns the read finished only through the
// completion flag the installed APC sets.
void cache_io_read_file_ex_retry(void *read_file_ex, void *file, void *buffer,
    cache_io_request *request, uint32_t size, uint32_t offset, void *completion_routine)
{
    read_file_ex_procedure read_procedure;
    int32_t started;

    read_procedure = (read_file_ex_procedure)read_file_ex;

    request->internal = 0;
    request->internal_high = 0;
    request->offset = 0;
    request->offset_high = 0;
    request->event = (void *)0;

    request->offset = offset;
    request->offset_high = 0;
    request->event = (void *)0;

    SleepEx(0, 1);
    SetLastError(0);
    started = read_procedure(file, buffer, size, request, completion_routine);

    while (started == 0) {
        GetLastError(); // fetched and discarded by the original
        SleepEx(0, 1);
        SetLastError(0);
        started = read_procedure(file, buffer, size, request, completion_routine);
    }
}

#if 0
Original Ghidra decompilation (0x442c70):

void FUN_00442c70(code *param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 in_EDX;
  code *unaff_EBX;
  undefined4 *unaff_ESI;

  *unaff_ESI = 0;
  unaff_ESI[1] = 0;
  unaff_ESI[2] = 0;
  unaff_ESI[3] = 0;
  unaff_ESI[4] = 0;
  unaff_ESI[2] = in_EDX;
  unaff_ESI[3] = 0;
  unaff_ESI[4] = 0;
  SleepEx(0,1);
  SetLastError(0);
  iVar1 = (*param_1)(param_2,param_3);
  while (iVar1 == 0) {
    GetLastError();
    SleepEx(0,1);
    SetLastError(0);
    iVar1 = (*unaff_EBX)(unaff_ESI,param_3);
  }
  return;
}

Note how badly Ghidra mangles the call: it drops three of the five pushed arguments, and on the
retry it picks EBX -- the size -- as the callee. The raw disassembly is unambiguous.

Raw disassembly (0x442c70-0x442cdc), objdump -d -M intel --start-address=0x442c70
--stop-address=0x442ce0 bin/halo.exe:

00442c70: xor eax,eax
00442c72: mov ecx,esi
00442c74: mov DWORD PTR [ecx],eax           ; request->internal      = 0
00442c76: mov DWORD PTR [ecx+0x4],eax       ; request->internal_high = 0
00442c79: mov DWORD PTR [ecx+0x8],eax       ; request->offset        = 0
00442c7c: push ebp
00442c7d: mov ebp,DWORD PTR [esp+0x10]      ; ebp = arg3 = buffer
00442c81: mov DWORD PTR [ecx+0xc],eax       ; request->offset_high   = 0
00442c84: mov DWORD PTR [ecx+0x10],eax      ; request->event         = 0
00442c87: push 0x1
00442c89: push eax                          ; 0
00442c8a: mov DWORD PTR [esi+0x8],edx       ; request->offset = offset (EDX)
00442c8d: mov DWORD PTR [esi+0xc],eax
00442c90: mov DWORD PTR [esi+0x10],eax
00442c93: call DWORD PTR ds:0x63a290        ; SleepEx(0, TRUE)
00442c99: push 0x0
00442c9b: call DWORD PTR ds:0x63a280        ; SetLastError(0)
00442ca1: mov eax,DWORD PTR [esp+0xc]       ; eax = arg2 = file
00442ca5: push edi                          ; completion_routine
00442ca6: push esi                          ; overlapped == request
00442ca7: push ebx                          ; size
00442ca8: push ebp                          ; buffer
00442ca9: push eax                          ; file
00442caa: call DWORD PTR [esp+0x1c]         ; arg1 == ReadFileEx
00442cae: test eax,eax
00442cb0: jne 0x442cdb                      ; started -> done
00442cb2: call DWORD PTR ds:0x63a284        ; GetLastError(), result discarded
00442cb8: push 0x1
00442cba: push 0x0
00442cbc: call DWORD PTR ds:0x63a290        ; SleepEx(0, TRUE)
00442cc2: push 0x0
00442cc4: call DWORD PTR ds:0x63a280        ; SetLastError(0)
00442cca: mov ecx,DWORD PTR [esp+0xc]       ; file again
00442cce: push edi
00442ccf: push esi
00442cd0: push ebx
00442cd1: push ebp
00442cd2: push ecx
00442cd3: call DWORD PTR [esp+0x1c]
00442cd7: test eax,eax
00442cd9: je 0x442cb2                       ; retry forever
00442cdb: pop ebp
00442cdc: ret
#endif
