// uninit_copy_string_pair  (orphan pass 4: FUN_0057cf50, no Ghidra name)
// address 0x57cf50, size 102 bytes
// name confidence: 0.5 (MSVC 7.1 std::_Uninit_copy<hwreq_string_pair*, hwreq_string_pair*>
//   specialization: copy-constructs [source_begin, source_end) into dest, returning
//   dest + (source_end - source_begin))
// rewrite confidence: 0.5 (standard MSVC 7.1 library code; confirmed against objdump)
// evidence: out/phase4/shell_types_notes.md: "0x57cf50 _Uninit_copy". Same
//   string_pair_construct_empty 0x57c640 callee (this pass) as uninit_fill_n_string_pair.c,
//   confirming the same per-element copy-construct call.
// register convention (confirmed via objdump): ECX = source_begin, stack argument 1 =
//   source_end, stack argument 2 = dest (mutated as the loop cursor). Returns the final dest
//   cursor (one past the last constructed element) in EAX.
// blam-cc: ECX -> source_begin, stack -> source_end, dest
// FIXED (register inputs, objdump): note phrasing only -- rewritten from the call-style
// "f(x /*ECX*/)" comment the checker cannot parse into "ECX -> source_begin, stack -> ...".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern hwreq_string_pair *string_pair_construct_empty(hwreq_string_pair *dest, const hwreq_string_pair *source); // 0x57c640, same pass

hwreq_string_pair *uninit_copy_string_pair(hwreq_string_pair *source_begin, hwreq_string_pair *source_end, hwreq_string_pair *dest)
{
    while (source_begin != source_end) {
        if (dest != 0) {
            string_pair_construct_empty(dest, source_begin);
        }
        dest = (hwreq_string_pair *)((uint8_t *)dest + 0x38);
        source_begin = (hwreq_string_pair *)((uint8_t *)source_begin + 0x38);
    }
    return dest;
}

#if 0
Original Ghidra decompilation (0x57cf50):

int FUN_0057cf50(int param_1,int param_2)

{
  int in_ECX;
  undefined4 *unaff_FS_OFFSET;
  undefined4 local_10;
  undefined1 *puStack_c;
  undefined1 local_8;
  undefined3 uStack_7;

  puStack_c = &LAB_00639435;
  local_10 = *unaff_FS_OFFSET;
  *unaff_FS_OFFSET = &local_10;
  uStack_7 = 0;
  for (; in_ECX != param_1; in_ECX = in_ECX + 0x38) {
    local_8 = 1;
    if (param_2 != 0) {
      string_pair_construct_empty(param_2);
    }
    param_2 = param_2 + 0x38;
  }
  *unaff_FS_OFFSET = local_10;
  return param_2;
}
#endif
