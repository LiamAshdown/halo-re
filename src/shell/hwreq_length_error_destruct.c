// hwreq_length_error_destruct  (orphan pass 4: FUN_005783b0, no Ghidra name)
// address 0x5783b0, size 11 bytes
// name confidence: 0.5 (MSVC 7.1 std::length_error::~length_error; src/shell/README.md:
//   "0x5783b0 / 0x5783c0 are `length_error` [...] (destructor and scalar deleting destructor)")
// rewrite confidence: 0.55 (trivial: sets the length_error vtable, then reuses
//   hwreq_parse_exception_destruct's shared string-teardown body -- length_error shares
//   logic_error's exact object layout in Dinkumware's hierarchy, just a different vtable)
// evidence: src/shell/README.md / out/phase4/shell_types_notes.md, "Misattributed entries".
//   Same hwreq_parse_exception layout as hwreq_parse_exception_construct.c.
// register convention: ECX = this.
// blam-cc: hwreq_length_error_destruct(hwreq_parse_exception *this /*ECX*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_parse_exception hwreq_parse_exception; // opaque here; see hwreq_parse_exception_construct.c

extern void *length_error_vtable; // 0x0065508c
extern void hwreq_parse_exception_destruct(hwreq_parse_exception *this); // 0x578310, same pass

void hwreq_length_error_destruct(hwreq_parse_exception *this)
{
    *(void **)this = &length_error_vtable;
    hwreq_parse_exception_destruct(this);
}

#if 0
Original Ghidra decompilation (0x5783b0):

void FUN_005783b0(void)

{
  undefined4 *in_ECX;

  *in_ECX = &PTR_FUN_0065508c;
  hwreq_parse_exception_destruct();
  return;
}
#endif
