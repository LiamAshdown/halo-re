// hex_string_to_uint  (Ghidra: hex_string_to_uint, already named)
// address 0x57d7f0, size 58 bytes
// name confidence: 0.55  rewrite confidence: 0.8
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Parses a
//   hexadecimal digit string (register argument) into an unsigned integer." Only lowercase
//   'a'-'f' and '0'-'9' are recognized (no 'A'-'F'), matching the lower-cased hardware id string
//   shell_detect_hardware_specs 0x57d880 parses GUIDs out of.
// register convention: string pointer in EDX (Ghidra already recognizes it as the "in_EDX"
//   parameter).
// blam-cc: string in EDX (only parameter).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

// Parses consecutive lowercase hex digits ('0'-'9', 'a'-'f') from the given string into an
// unsigned integer, stopping at the first non-hex-digit character. Returns 0 for an empty or
// all-non-hex string.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t hex_string_to_uint(char *string)
{
    char c;
    int32_t value;

    value = 0;
    for (;;) {
        c = *string;
        if (!((c > '/' && c < ':') || (c > '`' && c < 'g'))) {
            break;
        }
        if (c < '0' || c > '9') {
            value = value * 0x10 + (c - 'W'); // 'a'-'f' -> 10-15 (c - 'a' + 10 == c - 0x57)
        } else {
            value = value * 0x10 + (c - '0');
        }
        string++;
    }
    return value;
}

#if 0
Original Ghidra decompilation (0x57d7f0):

int __cdecl hex_string_to_uint(char *in_EDX)

{
  char cVar1;
  int iVar2;
  char *in_EDX_00;

  iVar2 = 0;
  while (((cVar1 = *in_EDX_00, '/' < cVar1 && (cVar1 < ':')) || (('`' < cVar1 && (cVar1 < 'g'))))) {
    if ((cVar1 < '0') || ('9' < cVar1)) {
      iVar2 = iVar2 * 0x10 + -0x57 + (int)cVar1;
      in_EDX_00 = in_EDX_00 + 1;
    }
    else {
      iVar2 = iVar2 * 0x10 + -0x30 + (int)cVar1;
      in_EDX_00 = in_EDX_00 + 1;
    }
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
