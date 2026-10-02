// hwreq_parse_exception_copy_construct  (orphan pass 4: FUN_0057bc20, no Ghidra name)
// address 0x57bc20, size 101 bytes
// name confidence: 0.4 (copy constructor counterpart to hwreq_parse_exception_construct.c
//   0x5782b0: calls the Dinkumware `exception` base copy constructor, then sets the same
//   logic_error vtable (0x00655080) that 0x5782b0 sets, then copies the embedded message
//   string from `other`'s +0xc offset -- i.e. `logic_error(const logic_error&)`, not an
//   out_of_range constructor despite Ghidra typing its parameter `exception *`)
// rewrite confidence: 0.5 (standard MSVC 7.1 library code; confirmed against the
//   decompilation and consistent with hwreq_parse_exception_construct.c's object layout)
// evidence: types/shell.h msvc_std_string; hwreq_parse_exception_construct.c's
//   hwreq_parse_exception layout (message string embedded at +0xc). The vtable stored,
//   0x00655080, is the same logic_error vtable hwreq_parse_exception_construct.c sets, not
//   0x0065508c (length_error) or 0x00655098 (out_of_range).
// register convention: ECX = this, stack argument = const hwreq_parse_exception *other.
// blam-cc: hwreq_parse_exception_copy_construct(hwreq_parse_exception *this /*ECX*/, const hwreq_parse_exception *other /*stack*/)
// UNSURE: `exception::exception(this, other)` (0x627dd2, the Dinkumware base-class copy
//   constructor) and FUN_0057b830 (string::assign) are opaque externs defined in another file.

// VERIFIED against disassembly 0x57bc20..0x57bc85 (2026-09-30): returns this (mov eax,esi; ret 4)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

typedef struct hwreq_parse_exception {
    uint32_t vtable;
    uint32_t dofree;
    uint32_t legacy_what;
    msvc_std_string message;
} hwreq_parse_exception; // size 0x28, see hwreq_parse_exception_construct.c

extern void *logic_error_vtable; // 0x00655080
extern void exception_copy_construct(hwreq_parse_exception *self, const hwreq_parse_exception *other); // 0x627dd2, UNSURE: opaque Dinkumware `exception::exception(const exception&)`
extern msvc_std_string *string_assign_substr(msvc_std_string *self, const msvc_std_string *right,
    uint32_t pos, uint32_t count); // 0x57b830, module=lib:crt, not this pass

hwreq_parse_exception *hwreq_parse_exception_copy_construct(hwreq_parse_exception *self, const hwreq_parse_exception *other)
{
    exception_copy_construct(self, other); // UNSURE: opaque base-class copy constructor
    self->vtable = (uint32_t)&logic_error_vtable;
    self->message.capacity = 0xf;
    self->message.size = 0;
    self->message.buffer.inline_buffer[0] = 0;
    string_assign_substr(&self->message, &other->message, 0, 0xffffffff);
    return self;
}

#if 0
Original Ghidra decompilation (0x57bc20):

void FUN_0057bc20(exception *param_1)

{
  exception *in_ECX;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_00639328;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  exception::exception(in_ECX,param_1);
  local_4 = 0;
  *(undefined ***)in_ECX = &PTR_FUN_00655080;
  *(undefined4 *)(in_ECX + 0x24) = 0xf;
  *(undefined4 *)(in_ECX + 0x20) = 0;
  in_ECX[0x10] = (exception)0x0;
  FUN_0057b830(param_1 + 0xc,0,0xffffffff);
  ExceptionList = local_c;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
