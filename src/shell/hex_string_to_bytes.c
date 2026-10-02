// hex_string_to_bytes  (Ghidra: hex_string_to_bytes, already named)
// address 0x57d830, size 75 bytes
// name confidence: 0.55  rewrite confidence: 0.8
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Decodes a
//   hexadecimal character string into raw bytes, two hex digits per output byte." Sibling of
//   hex_string_to_uint 0x57d7f0 (same lowercase-only digit test); used by
//   shell_detect_hardware_specs 0x57d880 to decode a sound device's GUID Data4 bytes.
// register convention: objdump confirms dest in ECX, source in EDX ("mov al,[edx]" .. "mov
//   [ecx],al", no register loads before use, so both are live-in).
// blam-cc: dest in ECX, source in EDX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Decodes consecutive lowercase hex digit pairs ('0'-'9', 'a'-'f') from source into raw bytes at
// dest, stopping at the first character that is not a hex digit (so an odd trailing digit, or a
// non-hex character, ends the string without writing a partial byte).
void hex_string_to_bytes(uint8_t *dest, const char *source)
{
    char c;

    for (;;) {
        c = *source;
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            break;
        }
        if (c >= '0' && c <= '9') {
            c = (char)(c - '0');
        } else {
            c = (char)(c - 'W'); // 'a'-'f' -> 10-15
        }
        *dest = (uint8_t)(c << 4);
        source++;

        c = *source;
        if (c >= '0' && c <= '9') {
            *dest = (uint8_t)(*dest + (c - '0'));
        } else {
            *dest = (uint8_t)(*dest + (c - 'W'));
        }
        dest++;
        source++;
    }
}

#if 0
Original Ghidra decompilation (0x57d830):

void hex_string_to_bytes(void)

{
  char cVar1;
  char cVar2;
  char *in_ECX;
  char *in_EDX;

  while (((cVar1 = *in_EDX, '/' < cVar1 && (cVar1 < ':')) || (('`' < cVar1 && (cVar1 < 'g'))))) {
    if ((cVar1 < '0') || ('9' < cVar1)) {
      cVar2 = -0x57;
    }
    else {
      cVar2 = '\0';
    }
    *in_ECX = (cVar1 + cVar2) * '\x10';
    cVar1 = in_EDX[1];
    if ((cVar1 < '0') || ('9' < cVar1)) {
      *in_ECX = *in_ECX + cVar1 + -0x57;
      in_ECX = in_ECX + 1;
      in_EDX = in_EDX + 2;
    }
    else {
      *in_ECX = *in_ECX + cVar1 + -0x30;
      in_ECX = in_ECX + 1;
      in_EDX = in_EDX + 2;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
