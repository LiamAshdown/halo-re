// hwreq_parse_exception_construct  (orphan pass 4: FUN_005782b0, no Ghidra name)
// address 0x5782b0, size 95 bytes
// name confidence: 0.55 (this is the constructor half of hwreq_parse_exception_destruct
//   0x578310, an already-named function in this same batch; MSVC 7.1's std::logic_error(const
//   string&) constructor, specialized to whatever "hwreq_parse_exception"/logic_error subclass
//   this vtable belongs to -- the hwreq parser is the only user of this exception hierarchy in
//   this module, see src/shell/README.md's "Misattributed entries" note)
// rewrite confidence: 0.55 (this is standard MSVC 7.1 Dinkumware library code, not custom Blam
//   logic; the object layout, SEH frame push and string-copy call are confirmed against
//   objdump; C++ exception machinery (the /GX SEH frame, FUN_00627dc1 = _EH_prolog3,
//   FUN_00638e74/0x00638eb4 = _Xlen/_Xran) is preserved as opaque calls rather than reimplemented)
// evidence: types/shell.h msvc_std_string (buffer 0x04, size 0x14, capacity 0x18 relative to
//   the string's own base); src/shell/README.md / out/phase4/shell_types_notes.md: "5782b0
//   std::logic_error::logic_error(const string &), 578310 ~logic_error". objdump confirms the
//   embedded string sits at ECX+0xc (a `lea ecx,[esi+0xc]` before every field write and before
//   the call to 0x57b830), giving the exception object layout: vtable 0x00, an 8-byte
//   unused-here pair at 0x04/0x08 (the classic Dinkumware `exception::_Dofree` bool + `_What`
//   char* used only by the `exception(const char*)` constructor path, never taken here), then
//   the embedded `std::string _Str` at 0x0c (size 0x28 total, matching 578310's own
//   `*(int*)(in_ECX+0x24) > 0xf` capacity test at relative +0x24, i.e. absolute object+0xc+0x18).
// register convention (confirmed via objdump): ECX = this (ret 0x4 confirms one stack
//   argument), stack argument = const msvc_std_string *message. Returns `this` in EAX (the
//   compiler's standard constructor-return convention), consistent with objdump's
//   `mov eax,esi` before the epilogue.
// blam-cc: hwreq_parse_exception_construct(hwreq_parse_exception *this /*ECX*/, const msvc_std_string *message /*stack*/)
// UNSURE: FUN_00627dc1 (the SEH frame prolog, `_EH_prolog3` in the CRT) and FUN_0057b830
//   (`std::string::assign(const string&, size_t, size_t)`, module=lib:crt per pack.py -- not
//   assigned to this pass) are declared as opaque externs rather than rewritten; both are
//   generic MSVC 7.1 runtime code, not Blam logic.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_parse_exception {
    uint32_t vtable;         // 0x00
    uint32_t dofree;         // 0x04, UNSURE: Dinkumware exception::_Dofree bool + padding, unused by this constructor
    uint32_t legacy_what;    // 0x08, UNSURE: Dinkumware exception::_What char*, unused by this constructor
    msvc_std_string message; // 0x0c
} hwreq_parse_exception; // size 0x28

extern void *logic_error_vtable; // 0x00655080
extern void seh_prolog3(void); // 0x627dc1, UNSURE: opaque CRT `_EH_prolog3`
extern msvc_std_string *string_assign_substr(msvc_std_string *this, const msvc_std_string *right,
    uint32_t pos, uint32_t count); // 0x57b830, module=lib:crt, not this pass

hwreq_parse_exception *hwreq_parse_exception_construct(hwreq_parse_exception *this, const msvc_std_string *message)
{
    seh_prolog3(); // UNSURE: SEH frame setup, opaque

    this->vtable = (uint32_t)&logic_error_vtable;
    this->message.size = 0;
    this->message.capacity = 0xf;
    this->message.buffer.inline_buffer[0] = 0;

    string_assign_substr(&this->message, message, 0, 0xffffffff);

    return this;
}

#if 0
Original Ghidra decompilation (0x5782b0):

void FUN_005782b0(undefined4 param_1)

{
  undefined4 *in_ECX;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_00639328;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  FUN_00627dc1();
  local_4 = 0;
  *in_ECX = &PTR_FUN_00655080;
  in_ECX[8] = 0;
  in_ECX[9] = 0xf;
  *(undefined1 *)(in_ECX + 4) = 0;
  FUN_0057b830(param_1,0,0xffffffff);
  ExceptionList = local_c;
  return;
}
#endif
