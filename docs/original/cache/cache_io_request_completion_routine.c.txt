// cache_io_request_completion_routine  (no Ghidra function; named in
//   cache_io_thread_proc_async.c, which installs it)
// address 0x443ae0, size 21 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: objdump -d 0x443ae0..0x443af4: the ReadFileEx LPOVERLAPPED_COMPLETION_ROUTINE that
//   cache_io_thread_proc_async (0x443940, mov edi,0x443ae0) hands to its read wrapper. It reads
//   only the third argument (the request, whose first 0x14 bytes are the OVERLAPPED), sets
//   *completion.flag (+0x24) to 1 and clears pending (+0x1d) and started (+0x1e); ret 0xc.
// register convention: WINAPI (__stdcall), three stack arguments.
//   // blam-cc: stack -> error_code, stack -> bytes_transferred, stack -> overlapped

#include "tags.h"
#include "memory.h"
#include "cache.h"

// Marks an asynchronous cache read finished: raises the caller's completion flag and frees
// the request slot for the worker thread.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void __stdcall cache_io_request_completion_routine(uint32_t error_code, uint32_t bytes_transferred,
    cache_io_request *overlapped)
{
    *overlapped->completion.flag = 1;
    overlapped->pending = 0;
    overlapped->started = 0;
}

#if 0
No Ghidra function exists at 0x443ae0. Disassembly:
  443ae0: mov eax,[esp+0xc]
  443ae4: mov ecx,[eax+0x24]
  443ae7: mov byte ptr [ecx],1
  443aea: xor cl,cl
  443aec: mov [eax+0x1d],cl
  443aef: mov [eax+0x1e],cl
  443af2: ret 0xc
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
