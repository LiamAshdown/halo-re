// cache_io_completion_routine  (Ghidra: cache_io_completion_routine, already named)
// address 0x443b00, size 41 bytes
// name confidence: 0.75   rewrite confidence: 0.90
// evidence: types/cache.h cache_io_request/cache_io_completion: the completion sub-record sits
// at +0x24 of the request, which is exactly the OVERLAPPED-sized offset this APC indexes from;
// out/phase4/cache_types_notes.md's cache_io_request section names this function directly and
// documents the embedded-record relationship.
// register convention: none; this is a real Win32 FileIOCompletionRoutine callback (__stdcall,
// already fully recognized by Ghidra).

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "cache.h"

// APC-based completion routine passed to overlapped ReadFileEx calls. `overlapped` is really the
// cache_io_request the read was issued against; runs the request's optional per-request
// completion procedure (passing it the embedded completion record), then marks the completion
// flag so any waiter sees the read as finished.
void cache_io_completion_routine(uint32_t error_code, uint32_t bytes_transferred, cache_io_request *overlapped)
{
    if (overlapped->completion.procedure != (void *)0) {
        overlapped->completion.procedure(&overlapped->completion);
        *overlapped->completion.flag = 1;
        return;
    }
    *overlapped->completion.flag = 1;
    return;
}

#if 0
Original Ghidra decompilation (0x443b00):

void cache_io_completion_routine(ulong error_code,ulong bytes_transferred,void *overlapped)

{
  if (*(code **)((int)overlapped + 0x28) != (code *)0x0) {
    (**(code **)((int)overlapped + 0x28))((int)overlapped + 0x24);
    **(undefined1 **)((int)overlapped + 0x24) = 1;
    return;
  }
  **(undefined1 **)((int)overlapped + 0x24) = 1;
  return;
}
#endif
