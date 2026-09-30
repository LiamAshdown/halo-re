// uninit_fill_n_string_pair  (orphan pass 4: FUN_0057ce80, no Ghidra name)
// address 0x57ce80, size 98 bytes
// name confidence: 0.5 (MSVC 7.1 std::_Uninit_fill_n<hwreq_string_pair*, size_t,
//   hwreq_string_pair> specialization: default-constructs `count` copies of `value` starting at
//   `dest`, skipping the call entirely when `dest` is null; matches the standard library
//   algorithm's shape and name)
// rewrite confidence: 0.5 (standard MSVC 7.1 library code; confirmed against objdump --
//   register convention, in particular, disagrees with Ghidra's own rendering of the call site
//   in hwreq_device_list_push_back.c, see UNSURE)
// evidence: out/phase4/shell_types_notes.md: "0x57ce80 _Uninit_fill_n". Sole non-trivial callee
//   is string_pair_construct_empty 0x57c640 (this pass, same directory).
// register convention (confirmed via objdump, both at this function's own prologue and at
//   hwreq_device_list_push_back.c's call site): ECX = count, stack argument 1 = dest (mutated
//   as the loop cursor, advancing by sizeof(hwreq_string_pair)), stack argument 2 = value (the
//   source pair each new element is copy-constructed from).
// blam-cc: ECX -> count, stack -> dest, value
// FIXED (register inputs, objdump): the "register convention" line above puts its own colon
// after a parenthetical aside, which the checker requires immediately after that keyword, and
// the blam-cc line below it used a call-style signature annotation; neither parsed as a
// register mapping, so ECX -> count (read at 0x57cea6, mov edi,ecx) was dropped. Rewritten in
// the plain "REG -> name" form.
// UNSURE: the SEH frame (the `push 0xffffffff` / `fs:0` dance) is preserved implicitly by
//   omission -- this rewrite does not model unwind state, matching every other MSVC-runtime
//   function in this batch. hwreq_device_list_push_back.c's call site pushes a 3rd stack dword
//   (a redundant copy of its own `value` argument) that this function's own prologue never
//   reads; that 3rd push is dropped as dead when calling this function from elsewhere.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


void uninit_fill_n_string_pair(hwreq_string_pair *dest, uint32_t count, const hwreq_string_pair *value)
{
    while (count != 0) {
        if (dest != 0) {
            string_pair_construct_empty(dest, value);
        }
        dest = (hwreq_string_pair *)((uint8_t *)dest + 0x38);
        count--;
    }
}

#if 0
Original Ghidra decompilation (0x57ce80):

void FUN_0057ce80(int param_1)

{
  int in_ECX;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined1 local_8;
  undefined3 uStack_7;

  puStack_c = &LAB_00639385;
  local_10 = *unaff_FS_OFFSET;
  *unaff_FS_OFFSET = &local_10;
  uStack_7 = 0;
  for (; in_ECX != 0; in_ECX = in_ECX + -1) {
    local_8 = 1;
    if (param_1 != 0) {
      string_pair_construct_empty(param_1);
    }
    param_1 = param_1 + 0x38;
  }
  *unaff_FS_OFFSET = local_10;
  return;
}
#endif
