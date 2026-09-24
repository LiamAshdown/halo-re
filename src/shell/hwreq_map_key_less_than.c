// hwreq_map_key_less_than  (already named; task-provided)
// address 0x57bbd0, size 39 bytes
// name confidence: 0.5 (already carries this name; standard MSVC 7.1
//   std::less<std::string>::operator() specialized for hwreq_map_node keys)
// rewrite confidence: 0.55 (standard library code; register convention confirmed against
//   objdump, which reads a 4th operand -- ECX -- that Ghidra's own decompilation of this
//   function never showed, see UNSURE)
// evidence: types/shell.h msvc_std_string. string_compare.c (0x57ce10, this pass) is the sole
//   callee, and its own header documents the same discrepancy from the opposite side.
// register convention (confirmed via objdump, overriding Ghidra's incomplete decompile):
//   EAX -> other (the key being looked up), ECX -> this (the current tree node's key).
//   Computes `*this < *other`.
// blam-cc: EAX -> other, ECX -> this
// FIXED (register inputs, objdump): notes were written as a full call signature instead of a
// parseable "REG -> name" mapping, so neither EAX (read at 0x57bbd0) nor ECX (read at 0x57bbe3)
// looked claimed. Body already used both correctly; reworded only.
// UNSURE: Ghidra's own decompilation of this function only shows the EAX-based operand
//   (building the pushed arguments to string_compare) and omits the final `mov eax,[ecx+0x14]`
//   that supplies string_compare's 2nd argument (n1) and implicitly its `this` (ECX). This
//   rewrite is built from the raw disassembly instead.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t string_compare(const msvc_std_string *this, uint32_t n1, uint32_t pos,
    const char *s, uint32_t n2); // 0x57ce10, same pass

uint8_t hwreq_map_key_less_than(const msvc_std_string *this, const msvc_std_string *other)
{
    const char *other_data = (other->capacity < 0x10) ? other->buffer.inline_buffer : (const char *)other->buffer.heap_buffer;
    return string_compare(this, this->size, 0, other_data, other->size) < 0;
}

#if 0
Original Ghidra decompilation (0x57bbd0):

bool hwreq_map_key_less_than(void)

{
  int in_EAX;
  int iVar1;

  if (*(uint *)(in_EAX + 0x18) < 0x10) {
    iVar1 = in_EAX + 4;
  }
  else {
    iVar1 = *(int *)(in_EAX + 4);
  }
  iVar1 = string_compare(0,iVar1,*(undefined4 *)(in_EAX + 0x14));
  return iVar1 < 0;
}
#endif
