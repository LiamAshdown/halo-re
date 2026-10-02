// hwreq_parser_get_graphics_device_name  (Ghidra: no function created; the phase-4 types agent
//   carved the stub name "missed_578870" from the vtable evidence)
// address 0x578870, size 14 bytes
// name confidence 0.8, rewrite confidence 0.85
// evidence: out/phase4/shell_types_notes.md: "the vtable accessors 0x578870..0x5788e0
//   (disassembled; they are not Ghidra functions)"; types/shell.h documents hwreq_parser's
//   graphics_device_name field (0x040) as "vtable 0x28", and shell_parse_config_txt "stores ...
//   vtable 0x28 (+0x40) in 0x00722b94" (graphics_device_name). This function reads capacity at
//   ECX+0x58 and the buffer at ECX+0x44, i.e. `this + 0x40` as an `msvc_std_string` (capacity at
//   +0x18, buffer at +0x04 within the string) -- exactly the field at hwreq_parser+0x40. It is
//   the classic MSVC 7.1 `std::string::c_str()`-shaped accessor: return the heap pointer once
//   the string outgrew its inline buffer (capacity > 0xf), otherwise the inline buffer's address.
// register convention: __thiscall (this in ECX), no stack arguments; matches types/shell.h's
//   `hwreq_get_string_fn` (vtable slots 0x28..0x34, 0x3c).
// blam-cc: ECX -> parser.

// VERIFIED against disassembly 0x578870..0x57887e (2026-09-30)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

// hwreq_parser_vtable slot 0x28. Returns a pointer to hwreq_parser->graphics_device_name's
// character data, following the string's own inline-vs-heap-buffer rule.
char *hwreq_parser_get_graphics_device_name(hwreq_parser *parser)
{
    if (parser->graphics_device_name.capacity > 0xf) {
        return (char *)parser->graphics_device_name.buffer.heap_buffer;
    }
    return parser->graphics_device_name.buffer.inline_buffer;
}

#if 0
Original Ghidra decompilation (0x578870):

int missed_578870(void)

{
  int in_ECX;

  if (0xf < *(uint *)(in_ECX + 0x58)) {
    return *(int *)(in_ECX + 0x44);
  }
  return in_ECX + 0x44;
}
#endif
