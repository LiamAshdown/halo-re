// hwreq_string_pair_construct  (already named; task-provided)
// address 0x57b670, size 111 bytes
// name confidence: 0.5 (already carries this name; standard MSVC 7.1 pair<string,string>
//   converting constructor from two source strings)
// rewrite confidence: 0.55 (standard library code, confirmed against objdump and consistent
//   with string_pair_construct_empty.c's field offsets)
// evidence: types/shell.h hwreq_string_pair (first 0x00, second 0x1c).
// register convention: EAX = dest (returned unchanged, matching the standard
//   constructor-return convention), stack arguments = const msvc_std_string *first_source,
//   const msvc_std_string *second_source.
// blam-cc: hwreq_string_pair_construct(hwreq_string_pair *dest /*EAX*/,
//   const msvc_std_string *first_source /*stack*/, const msvc_std_string *second_source /*stack*/)
// UNSURE: FUN_0057b830 (string::assign) is an opaque lib:crt extern, not rewritten here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern msvc_std_string *string_assign_substr(msvc_std_string *this, const msvc_std_string *right,
    uint32_t pos, uint32_t count); // 0x57b830, module=lib:crt, not this pass

hwreq_string_pair *hwreq_string_pair_construct(hwreq_string_pair *dest, const msvc_std_string *first_source,
                                                const msvc_std_string *second_source)
{
    dest->first.capacity = 0xf;
    dest->first.size = 0;
    dest->first.buffer.inline_buffer[0] = 0;
    string_assign_substr(&dest->first, first_source, 0, 0xffffffff);

    dest->second.capacity = 0xf;
    dest->second.size = 0;
    dest->second.buffer.inline_buffer[0] = 0;
    string_assign_substr(&dest->second, second_source, 0, 0xffffffff);

    return dest;
}

#if 0
Original Ghidra decompilation (0x57b670):

int hwreq_string_pair_construct(int param_1,undefined4 param_2,undefined4 param_3)

{
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_00639308;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *(undefined4 *)(param_1 + 0x18) = 0xf;
  *(undefined4 *)(param_1 + 0x14) = 0;
  *(undefined1 *)(param_1 + 4) = 0;
  FUN_0057b830(param_2,0,0xffffffff);
  local_4 = 0;
  *(undefined4 *)(param_1 + 0x34) = 0xf;
  *(undefined4 *)(param_1 + 0x30) = 0;
  *(undefined1 *)(param_1 + 0x20) = 0;
  FUN_0057b830(param_3,0,0xffffffff);
  ExceptionList = local_c;
  return param_1;
}
#endif
