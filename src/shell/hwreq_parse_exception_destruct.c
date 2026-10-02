// hwreq_parse_exception_destruct  (already named; task-provided)
// address 0x578310, size 110 bytes
// name confidence: 0.6 (already carries this name; matches the body exactly -- the destructor
//   half of hwreq_parse_exception_construct 0x5782b0, this pass, same directory)
// rewrite confidence: 0.55 (standard MSVC 7.1 std::logic_error::~logic_error; confirmed against
//   objdump, same layout as the constructor)
// evidence: see hwreq_parse_exception_construct.c for the full struct-layout evidence
//   (shared object type hwreq_parse_exception, embedded msvc_std_string at +0xc).
// register convention: ECX = this (no return value visible in Ghidra's own signature; the SEH
//   frame pop is the only tail activity).
// blam-cc: hwreq_parse_exception_destruct(hwreq_parse_exception *this /*ECX*/)
// UNSURE: FUN_00627e1c (`exception::~exception`, the Dinkumware base-class destructor) and
//   `free` are opaque CRT calls defined elsewhere.

// VERIFIED against disassembly 0x578310..0x57837e (2026-09-30): vtable reset, string cleared, base dtor
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_parse_exception {
    uint32_t vtable;
    uint32_t dofree;
    uint32_t legacy_what;
    msvc_std_string message;
} hwreq_parse_exception; // size 0x28, see hwreq_parse_exception_construct.c

extern void *logic_error_vtable; // 0x00655080
extern void exception_destruct(hwreq_parse_exception *self); // 0x627e1c, UNSURE: opaque Dinkumware `exception::~exception`

void hwreq_parse_exception_destruct(hwreq_parse_exception *self)
{
    self->vtable = (uint32_t)&logic_error_vtable;

    if (self->message.capacity > 0xf) {
        free((void *)self->message.buffer.heap_buffer);
    }
    self->message.capacity = 0xf;
    self->message.size = 0;
    self->message.buffer.inline_buffer[0] = 0;

    exception_destruct(self);
}

#if 0
Original Ghidra decompilation (0x578310):

void hwreq_parse_exception_destruct(void)

{
  exception *in_ECX;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  puStack_8 = &LAB_00639328;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined ***)in_ECX = &PTR_FUN_00655080;
  local_4 = 0;
  if (0xf < *(uint *)(in_ECX + 0x24)) {
    _free(*(void **)(in_ECX + 0x10));
  }
  *(undefined4 *)(in_ECX + 0x24) = 0xf;
  *(undefined4 *)(in_ECX + 0x20) = 0;
  in_ECX[0x10] = (exception)0x0;
  local_4 = 0xffffffff;
  exception::~exception(in_ECX);
  ExceptionList = local_c;
  return;
}
#endif
