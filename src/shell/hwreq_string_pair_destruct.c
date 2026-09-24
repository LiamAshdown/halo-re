// hwreq_string_pair_destruct  (Ghidra: hwreq_string_pair_destruct, already named)
// address 0x5785b0, size 122 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: frees each string's heap buffer only when capacity > 0xf (the MSVC 7.1 std::string
// inline-vs-heap test shell.h documents on msvc_std_string), then resets both to the empty
// inline state; matches hwreq_string_pair's first/second layout exactly (0x00 / 0x1c).
// register convention: pair pointer is the recognized stack parameter (thiscall folded to a
// normal parameter by Ghidra).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void _free(void *memory);

// Destructs a heap-embedded pair of strings, releasing any out-of-line buffers each string owns.
void hwreq_string_pair_destruct(hwreq_string_pair *pair)
{
    if (pair->second.capacity > k_msvc_string_inline_capacity) {
        _free((void *)pair->second.buffer.heap_buffer);
    }
    pair->second.capacity = k_msvc_string_inline_capacity;
    pair->second.size = 0;
    pair->second.buffer.inline_buffer[0] = 0;

    if (pair->first.capacity > k_msvc_string_inline_capacity) {
        _free((void *)pair->first.buffer.heap_buffer);
    }
    pair->first.size = 0;
    pair->first.capacity = k_msvc_string_inline_capacity;
    pair->first.buffer.inline_buffer[0] = 0;
}

#if 0
Original Ghidra decompilation (0x5785b0):


void hwreq_string_pair_destruct(int param_1)

{
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_00639308;
  local_c = ExceptionList;
  local_4 = 0;
  ExceptionList = &local_c;
  if (0xf < *(uint *)(param_1 + 0x34)) {
    ExceptionList = &local_c;
    _free(*(void **)(param_1 + 0x20));
  }
  *(undefined4 *)(param_1 + 0x34) = 0xf;
  *(undefined4 *)(param_1 + 0x30) = 0;
  *(undefined1 *)(param_1 + 0x20) = 0;
  local_4 = 0xffffffff;
  if (0xf < *(uint *)(param_1 + 0x18)) {
    _free(*(void **)(param_1 + 4));
  }
  *(undefined4 *)(param_1 + 0x14) = 0;
  *(undefined4 *)(param_1 + 0x18) = 0xf;
  *(undefined1 *)(param_1 + 4) = 0;
  ExceptionList = local_c;
  return;
}
#endif
