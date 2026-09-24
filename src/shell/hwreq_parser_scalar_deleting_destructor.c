// hwreq_parser_scalar_deleting_destructor  (Ghidra: FUN_005786a0; chosen name, see shell.h's
// vtable comment: "scalar_deleting_destructor 0x04 0x5786a0")
// address 0x5786a0, size 24 bytes
// name confidence: 0.4 (Ghidra) / 0.7 (this name)   rewrite confidence: 0.85
// evidence: vtable slot 1 of hwreq_parser_vtable; the standard MSVC "destroy, then free(this)"
// scalar deleting destructor shape, calling hwreq_parser_destruct (0x57a010, this module).
// register convention: thiscall, this in ECX (in_ECX in Ghidra).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void hwreq_parser_destruct(hwreq_parser *parser); // 0x0057a010
extern void _free(void *memory);

// Scalar deleting destructor for the hardware-requirements parser object: destructs it via
// hwreq_parser_destruct and frees its storage.
void hwreq_parser_scalar_deleting_destructor(hwreq_parser *this_parser)
{
    if (this_parser != 0) {
        hwreq_parser_destruct(this_parser);
        _free(this_parser);
    }
}

#if 0
Original Ghidra decompilation (0x5786a0):


void FUN_005786a0(void)

{
  void *in_ECX;
  
  if (in_ECX != (void *)0x0) {
    hwreq_parser_destruct(in_ECX);
    _free(in_ECX);
  }
  return;
}
#endif
