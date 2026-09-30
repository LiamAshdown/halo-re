// destroy_range_string_pair  (orphan pass 4: FUN_0057be00, no Ghidra name)
// address 0x57be00, size 22 bytes
// name confidence: 0.5 (MSVC 7.1 std::_Destroy_range(hwreq_string_pair *first,
//   hwreq_string_pair *last): destructs every element in [first, last) without freeing the
//   backing storage)
// rewrite confidence: 0.55 (standard library code, trivial loop, confirmed against objdump)
// evidence: out/phase4/shell_types_notes.md: "0x57be00 _Destroy_range". Callee
//   hwreq_string_pair_destruct 0x5785b0 (out/phase4/shell_types_notes.md: "0x57b670 / 0x5785b0
//   pair constructor / destruct"), not in this pass's address list.
// register convention: EAX = first, EDI = last (both live-in registers).
// blam-cc: destroy_range_string_pair(hwreq_string_pair *first /*EAX*/, hwreq_string_pair *last /*EDI*/)
// UNSURE: hwreq_string_pair_destruct (0x5785b0) is an opaque extern, not this pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


void destroy_range_string_pair(hwreq_string_pair *first, hwreq_string_pair *last)
{
    while (first != last) {
        hwreq_string_pair_destruct(first);
        first = (hwreq_string_pair *)((uint8_t *)first + 0x38);
    }
}

#if 0
Original Ghidra decompilation (0x57be00):

void FUN_0057be00(void)

{
  int in_EAX;
  int unaff_EDI;

  for (; in_EAX != unaff_EDI; in_EAX = in_EAX + 0x38) {
    hwreq_string_pair_destruct(in_EAX);
  }
  return;
}
#endif
