// string_pair_construct_empty  (already named; task-provided)
// address 0x57c640, size 110 bytes
// name confidence: 0.5 (already carries this name; every caller in this batch --
//   uninit_fill_n_string_pair 0x57ce80 and uninit_copy_string_pair 0x57cf50, this pass -- passes
//   a reference to a default-constructed hwreq_string_pair as the source, so in practice this
//   always constructs an empty pair even though the code itself is a generic copy constructor)
// rewrite confidence: 0.55 (standard MSVC 7.1 std::pair<string,string> copy constructor;
//   confirmed against objdump)
// evidence: types/shell.h hwreq_string_pair (first 0x00, second 0x1c, each a msvc_std_string).
//   Sole caller of FUN_0057b830 (string::assign(const string&, pos, count), lib:crt, not this
//   pass) confirms the same pattern as hwreq_string_pair_construct.c.
// register convention (confirmed via objdump, uninit_fill_n_string_pair.c's call site): ECX =
//   const hwreq_string_pair *source, stack argument = hwreq_string_pair *dest. Returns `dest`
//   in EAX (standard constructor-return convention).
// blam-cc: ECX -> source, stack -> dest
// FIXED (register inputs, objdump): note phrasing only -- rewritten from the call-style
// "f(x /*ECX*/)" comment the checker cannot parse into "ECX -> source, stack -> dest".

// VERIFIED against disassembly 0x57c640..0x57c6ae (2026-09-30): ECX=source, stack=dest, ret 4
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern msvc_std_string *string_assign_substr(msvc_std_string *self, const msvc_std_string *right,
    uint32_t pos, uint32_t count); // 0x57b830, module=lib:crt, not this pass

hwreq_string_pair *string_pair_construct_empty(hwreq_string_pair *dest, const hwreq_string_pair *source)
{
    dest->first.capacity = 0xf;
    dest->first.size = 0;
    dest->first.buffer.inline_buffer[0] = 0;
    string_assign_substr(&dest->first, &source->first, 0, 0xffffffff);

    dest->second.capacity = 0xf;
    dest->second.size = 0;
    dest->second.buffer.inline_buffer[0] = 0;
    string_assign_substr(&dest->second, &source->second, 0, 0xffffffff);

    return dest;
}

#if 0
Original Ghidra decompilation (0x57c640):

int string_pair_construct_empty(int param_1)

{
  int in_ECX;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  local_c = *unaff_FS_OFFSET;
  local_4 = 0xffffffff;
  puStack_8 = &LAB_00639308;
  *unaff_FS_OFFSET = &local_c;
  *(undefined4 *)(param_1 + 0x18) = 0xf;
  *(undefined4 *)(param_1 + 0x14) = 0;
  *(undefined1 *)(param_1 + 4) = 0;
  FUN_0057b830(in_ECX,0,0xffffffff);
  local_4 = 0;
  *(undefined4 *)(param_1 + 0x34) = 0xf;
  *(undefined4 *)(param_1 + 0x30) = 0;
  *(undefined1 *)(param_1 + 0x20) = 0;
  FUN_0057b830(in_ECX + 0x1c,0,0xffffffff);
  *unaff_FS_OFFSET = local_c;
  return param_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
