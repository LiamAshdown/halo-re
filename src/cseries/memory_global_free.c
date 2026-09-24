// memory_global_free  (Ghidra: memory_global_free, already named)
// address 0x449380, size 8 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase2/cseries/00.md; direct passthrough to GlobalFree(handle) with the handle
// taken from an implicit register argument.
// register convention: EAX = handle (in_EAX). Both callers, 0x54194c and 0x541956, load the
// handle into EAX immediately before calling.
//   // blam-cc: EAX -> handle

#include "tags.h"
#include "cseries.h"

extern void *GlobalFree(void *handle); // Win32, 0x0063a0bc import thunk

// Thin wrapper freeing a block previously returned by memory_global_alloc via GlobalFree.
void *memory_global_free(void *handle)
    // blam-cc: EAX -> handle
{
    return GlobalFree(handle);
}

#if 0
Original Ghidra decompilation (0x449380), from tools/pack.py 0x449380:

void memory_global_free(void)

{
  HGLOBAL in_EAX;

  GlobalFree(in_EAX);
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe):
  449380: push eax
  449381: call DWORD PTR ds:0x63a0bc    ; GlobalFree(handle)
  449387: ret
#endif
