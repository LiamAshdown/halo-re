// hwreq_parse_exception_scalar_deleting_destruct  (orphan pass 4: FUN_00578390, no Ghidra name)
// address 0x578390, size 30 bytes
// name confidence: 0.5 (standard MSVC "scalar deleting destructor" thunk: calls the real
//   destructor, then conditionally frees `this` -- the compiler-generated pattern for every
//   polymorphic class's vtable `delete` slot)
// rewrite confidence: 0.6 (trivial, standard MSVC thunk shape; confirmed against the
//   decompilation, register convention matches every other scalar deleting destructor in this
//   batch: 5783c0, 5783f0)
// evidence: src/shell/README.md: "0x578390 [is] its scalar deleting destructor", for
//   hwreq_parse_exception_destruct 0x578310 (this pass, same directory).
// register convention: ECX = this; the low bit of a stack byte parameter selects whether to
//   free `this` after destructing it (the standard MSVC `unsigned char __formal` deleting-dtor
//   flag).
// blam-cc: hwreq_parse_exception_scalar_deleting_destruct(hwreq_parse_exception *this /*ECX*/, uint8_t free_flag /*stack*/)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_parse_exception hwreq_parse_exception; // opaque here; see hwreq_parse_exception_construct.c

extern void hwreq_parse_exception_destruct(hwreq_parse_exception *this); // 0x578310, same pass

void hwreq_parse_exception_scalar_deleting_destruct(hwreq_parse_exception *this, uint8_t free_flag)
{
    hwreq_parse_exception_destruct(this);
    if (free_flag & 1) {
        free(this);
    }
}

#if 0
Original Ghidra decompilation (0x578390):

void FUN_00578390(byte param_1)

{
  void *in_ECX;

  hwreq_parse_exception_destruct();
  if ((param_1 & 1) != 0) {
    _free(in_ECX);
  }
  return;
}
#endif
