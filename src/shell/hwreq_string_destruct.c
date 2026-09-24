// hwreq_string_destruct  (already named; task-provided)
// address 0x57b800, size 34 bytes
// name confidence: 0.5 (already carries this name; standard MSVC 7.1
//   std::basic_string<char>::~basic_string(), out/phase4/shell_types_notes.md calls this
//   "string _Tidy")
// rewrite confidence: 0.6 (trivial, standard library code, confirmed against objdump)
// evidence: types/shell.h msvc_std_string (buffer 0x04, size 0x14, capacity 0x18).
// register convention: ESI = this (unaff_ESI; live-in from the caller, not a recognized
//   parameter -- string destructors are frequently inlined/tail-called this way under LTCG).
// blam-cc: hwreq_string_destruct(msvc_std_string *this /*ESI*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void _free(void *ptr); // 0x6277e8

void hwreq_string_destruct(msvc_std_string *this)
{
    if (this->capacity > 0xf) {
        _free((void *)this->buffer.heap_buffer);
    }
    this->capacity = 0xf;
    this->size = 0;
    this->buffer.inline_buffer[0] = 0;
}

#if 0
Original Ghidra decompilation (0x57b800):

void hwreq_string_destruct(void)

{
  int unaff_ESI;

  if (0xf < *(uint *)(unaff_ESI + 0x18)) {
    _free(*(void **)(unaff_ESI + 4));
  }
  *(undefined4 *)(unaff_ESI + 0x18) = 0xf;
  *(undefined4 *)(unaff_ESI + 0x14) = 0;
  *(undefined1 *)(unaff_ESI + 4) = 0;
  return;
}
#endif
