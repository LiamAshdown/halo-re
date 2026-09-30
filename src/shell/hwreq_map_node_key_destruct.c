// hwreq_map_node_key_destruct  (orphan pass 4: FUN_0057cde0, no Ghidra name)
// address 0x57cde0, size 34 bytes
// name confidence: 0.5 (destructs the msvc_std_string embedded at hwreq_map_node.key, offset
//   0xc -- the field offsets +0x10/+0x20/+0x24 are exactly 0xc + the string's own
//   buffer/size/capacity offsets 0x4/0x14/0x18; out/phase4/shell_types_notes.md calls this
//   "node key _Tidy")
// rewrite confidence: 0.55 (standard MSVC 7.1 library code, same shape as hwreq_string_destruct.c)
// evidence: types/shell.h hwreq_map_node (key at 0x0c, a msvc_std_string).
// register convention: ESI = node (unaff_ESI; live-in from the caller, consistent with how
//   hwreq_string_destruct.c also receives its `this` in ESI).
// blam-cc: hwreq_map_node_key_destruct(hwreq_map_node *node /*ESI*/)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


void hwreq_map_node_key_destruct(hwreq_map_node *node)
{
    if (node->key.capacity > 0xf) {
        free((void *)node->key.buffer.heap_buffer);
    }
    node->key.capacity = 0xf;
    node->key.size = 0;
    node->key.buffer.inline_buffer[0] = 0;
}

#if 0
Original Ghidra decompilation (0x57cde0):

void FUN_0057cde0(void)

{
  int unaff_ESI;

  if (0xf < *(uint *)(unaff_ESI + 0x24)) {
    _free(*(void **)(unaff_ESI + 0x10));
  }
  *(undefined4 *)(unaff_ESI + 0x24) = 0xf;
  *(undefined4 *)(unaff_ESI + 0x20) = 0;
  *(undefined1 *)(unaff_ESI + 0x10) = 0;
  return;
}
#endif
