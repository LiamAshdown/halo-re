// fill_string_pair_range  (orphan pass 4: FUN_0057cda0, no Ghidra name)
// address 0x57cda0, size 52 bytes
// name confidence: 0.5 (MSVC 7.1 std::fill(hwreq_string_pair *first, hwreq_string_pair *last,
//   const hwreq_string_pair &value): copy-assigns `value` into every already-constructed
//   element of [first, last), via string::assign rather than a constructor, distinguishing it
//   from uninit_fill_n_string_pair.c which constructs new elements)
// rewrite confidence: 0.5 (standard MSVC 7.1 library code; confirmed against objdump)
// evidence: out/phase4/shell_types_notes.md groups this with 0x57cde0 as "node key _Tidy", but
//   the 0x38-byte stride (hwreq_string_pair's size) and the two string::assign calls per
//   element (first, then first+0x1c = second) match std::fill over a vector<hwreq_string_pair>
//   range, not a single map-node key string; see hwreq_map_node_key_destruct.c (0x57cde0, this
//   pass) for the actual node-key operation.
// register convention (confirmed via objdump): EAX = first, stack argument = last, EBX = value
//   (const hwreq_string_pair *, a genuine live-in register parameter, constant through the
//   whole loop).
// blam-cc: EAX -> first, EBX -> value, stack -> last
// FIXED (register inputs, objdump): EAX/EBX (read at 0x57cda6/0x57cdad) were already C
//   parameters (first/value) but the old "blam-cc" note was a prose function signature that the
//   checker's "REG -> name" parser could not read; reworded.
// UNSURE: FUN_0057b830 (string::assign) is an opaque lib:crt extern defined elsewhere.

// VERIFIED against disassembly 0x57cda0..0x57cdd4 (2026-09-30): EAX=first, EBX=value, stack=last, stride 0x38, second string at +0x1c
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

// blam-cc: EAX -> first, EBX -> value, stack -> last
void fill_string_pair_range(hwreq_string_pair *first, hwreq_string_pair *last, const hwreq_string_pair *value)
{
    while (first != last) {
        string_assign_substr(&first->first, &value->first, 0, 0xffffffff);
        string_assign_substr(&first->second, &value->second, 0, 0xffffffff);
        first = (hwreq_string_pair *)((uint8_t *)first + 0x38);
    }
}

#if 0
Original Ghidra decompilation (0x57cda0):

void FUN_0057cda0(int param_1)

{
  int in_EAX;
  int unaff_EBX;

  if (in_EAX != param_1) {
    do {
      FUN_0057b830(unaff_EBX,0,0xffffffff);
      FUN_0057b830(unaff_EBX + 0x1c,0,0xffffffff);
      in_EAX = in_EAX + 0x38;
    } while (in_EAX != param_1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
