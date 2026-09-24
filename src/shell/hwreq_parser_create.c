// hwreq_parser_create  (Ghidra: hwreq_parser_create, already named)
// address 0x57b4c0, size 88 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Allocates and
//   default-constructs a new hardware-requirements parser object." objdump: operator_new(0x6b8)
//   (k_hwreq_parser_size), then hwreq_parser_construct 0x579ef0 on success.
// register convention: plain __cdecl, no parameters (objdump: "push ecx" only to align the SEH
//   frame, no argument reads).
// blam-cc: (no arguments)
// UNSURE: the compiler-generated x86 SEH frame is compiler plumbing, not application logic, and
//   is omitted here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void *operator_new(uint32_t size); // 0x6277da CRT
extern hwreq_parser *hwreq_parser_construct(hwreq_parser *this); // 0x579ef0

// Allocates a hardware-requirements parser object and default-constructs it; returns NULL if
// the allocation fails.
hwreq_parser *hwreq_parser_create(void)
{
    hwreq_parser *parser;

    parser = (hwreq_parser *)operator_new(k_hwreq_parser_size);
    if (parser == 0) {
        return 0;
    }
    return hwreq_parser_construct(parser);
}

#if 0
Original Ghidra decompilation (0x57b4c0):

undefined4 hwreq_parser_create(void)

{
  void *pvVar1;
  undefined4 uVar2;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_0063946b;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  pvVar1 = operator_new(0x6b8);
  local_4 = 0;
  if (pvVar1 != (void *)0x0) {
    uVar2 = hwreq_parser_construct(pvVar1);
    ExceptionList = local_c;
    return uVar2;
  }
  ExceptionList = local_c;
  return 0;
}
#endif
