// string_to_lowercase  (Ghidra: string_to_lowercase, already named)
// address 0x4491e0, size 36 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase2/cseries/00.md; iterates a null-terminated byte string calling _tolower on
// each character in place until the terminating NUL -- a plain in-place ASCII lowercasing loop.
// register convention: EDI = char *string (in_EDI is not visible in the Ghidra decompile because
// Ghidra never recognizes an incoming argument here; the callee prologue reads it straight off
// EDI with no stack access, and both known callers -- src/hs/hs_tokenize_primitive.c and
// src/networking/console_command_bool_get_set.c -- load EDI before calling). The function ends
// with `mov eax,edi; pop esi; ret` (0x449206), so it also returns EAX = the same pointer; Ghidra
// drops that return value, and both known callers ignore it, so it is reproduced here but not
// otherwise relied upon.
//   // blam-cc: EDI -> string

#include "tags.h"
#include "cseries.h"
#include <ctype.h>

// Lowercases a null-terminated string in place (ASCII, via the CRT tolower()) and returns the
// same pointer.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
char *string_to_lowercase(char *string)
    // blam-cc: EDI -> string
{
    char *cursor;

    cursor = string;
    while (*cursor != '\0') {
        *cursor = (char)tolower((uint8_t)*cursor);
        cursor = cursor + 1;
    }
    return string;
}

#if 0
Original Ghidra decompilation (0x4491e0), from tools/pack.py 0x4491e0:

void string_to_lowercase(void)

{
  byte *pbVar1;
  byte bVar2;
  int iVar3;
  byte *unaff_EDI;

  bVar2 = *unaff_EDI;
  while (bVar2 != 0) {
    iVar3 = _tolower((uint)*unaff_EDI);
    *unaff_EDI = (byte)iVar3;
    pbVar1 = unaff_EDI + 1;
    unaff_EDI = unaff_EDI + 1;
    bVar2 = *pbVar1;
  }
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe):
  4491e0: cmp BYTE PTR [edi],0x0
  4491e3: push esi
  4491e4: mov esi,edi
  4491e6: je 0x449206
  4491f0: movzx eax,BYTE PTR [esi]
  4491f3: push eax
  4491f4: call 0x624687            ; _tolower
  4491f9: add esp,0x4
  4491fc: mov BYTE PTR [esi],al
  4491fe: mov al,BYTE PTR [esi+0x1]
  449201: inc esi
  449202: test al,al
  449204: jne 0x4491f0
  449206: mov eax,edi              ; return value = string (dropped by Ghidra)
  449208: pop esi
  449209: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
