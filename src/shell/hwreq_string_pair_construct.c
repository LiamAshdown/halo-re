// hwreq_string_pair_construct  (already named; task-provided)
// address 0x57b670, size 111 bytes
// name confidence: 0.5 (already carries this name; standard MSVC 7.1 pair<string,string>
//   converting constructor from two source strings)
// rewrite confidence: 0.55 (standard library code, confirmed against objdump and consistent
//   with string_pair_construct_empty.c's field offsets)
// evidence: types/shell.h hwreq_string_pair (first 0x00, second 0x1c).
// register convention: all three arguments (dest, first_source, second_source) are stack arguments
//   and the callee pops them (ret 0xc); the function returns dest. There is no register argument.
// blam-cc: (all three on the stack)

// VERIFIED against disassembly 0x57b670..0x57b6df (2026-09-30): all three arguments are on the stack (ret 0xc), no register argument
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
