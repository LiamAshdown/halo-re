// memory_global_alloc  (Ghidra: memory_global_alloc, already named)
// address 0x449370, size 10 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase2/cseries/00.md; direct passthrough to GlobalAlloc(0, size) with the size
// taken from an implicit register argument and no fixed-flag/moveable handling beyond flags=0.
// register convention: EAX = size (in_EAX). The only caller, 0x541271, sets EAX to strlen()+1
// right before the call. Returns EAX = the GlobalAlloc handle (the function performs no move
// after the call, so the Win32 return value passes straight through).
//   // blam-cc: EAX -> size

#include "tags.h"
#include "cseries.h"

extern void *GlobalAlloc(uint32_t flags, uint32_t size); // Win32, 0x0063a0b0 import thunk

// Thin wrapper allocating a fixed (GMEM_FIXED, flags=0) block of `size` bytes via GlobalAlloc.
void *memory_global_alloc(uint32_t size)
    // blam-cc: EAX -> size
{
    return GlobalAlloc(0, size);
}

#if 0
Original Ghidra decompilation (0x449370), from tools/pack.py 0x449370:

void memory_global_alloc(void)

{
  SIZE_T in_EAX;

  GlobalAlloc(0,in_EAX);
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe):
  449370: push eax
  449371: push 0x0
  449373: call DWORD PTR ds:0x63a0b0    ; GlobalAlloc(0, size)
  449379: ret
#endif
