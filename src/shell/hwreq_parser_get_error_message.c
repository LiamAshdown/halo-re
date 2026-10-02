// hwreq_parser_get_error_message  (Ghidra: no function created; the phase-4 types agent carved the stub
//   name "missed_5788e0" from the vtable evidence)
// address 0x5788e0, size 14 bytes
// name confidence 0.8, rewrite confidence 0.85
// evidence: out/phase4/shell_types_notes.md: "the vtable accessors 0x578870..0x5788e0
//   (disassembled; they are not Ghidra functions)"; types/shell.h documents hwreq_parser's
//   error_message field (0x024) as "vtable 0x3c". This function reads capacity at ECX+0x3c and
//   the buffer at ECX+0x28, i.e. `this + 0x24` as an `msvc_std_string` (capacity at +0x18,
//   buffer at +0x04 within the string) -- exactly the field at hwreq_parser+0x24. Same shape as
//   hwreq_parser_get_graphics_device_name.c: return the heap pointer once the string outgrew its
//   inline buffer (capacity > 0xf), otherwise the inline buffer's address.
// register convention: __thiscall (this in ECX), no stack arguments; matches types/shell.h's
//   `hwreq_get_string_fn` (vtable slots 0x28..0x34, 0x3c).
// blam-cc: ECX -> parser.

// VERIFIED against disassembly 0x5788e0..0x5788ee (2026-09-30)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

// hwreq_parser_vtable slot 0x3c. Returns a pointer to hwreq_parser->error_message's character
// data, following the string's own inline-vs-heap-buffer rule.
char *hwreq_parser_get_error_message(hwreq_parser *parser)
{
    if (parser->error_message.capacity > 0xf) {
        return (char *)parser->error_message.buffer.heap_buffer;
    }
    return parser->error_message.buffer.inline_buffer;
}

#if 0
Original Ghidra decompilation (0x5788e0):

int missed_5788e0(void)

{
  int in_ECX;

  if (0xf < *(uint *)(in_ECX + 0x3c)) {
    return *(int *)(in_ECX + 0x28);
  }
  return in_ECX + 0x28;
}
#endif
