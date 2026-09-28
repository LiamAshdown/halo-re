// hs_source_buffer_append  (Ghidra: hs_source_buffer_append, already named)
// address 0x4856f0, size 125 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: grows hs_compiled_source (GlobalAlloc/GlobalReAlloc) and appends `length` bytes
// from `text`, NUL-terminating the result; hs_compile's sole caller passes only the text
// pointer explicitly (the byte count travels in EBX and is inferred here to be the same
// source_length hs_compile itself received, which is the only length meaningful at that call
// site).
// register convention: `text` is recognized directly by Ghidra; `length` is unrecognized
// (unaff_EBX), which by the blam-cc convention is the fourth register slot, EBX.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"


extern char *hs_compiled_source;          // 0x006b14c0
extern int32_t hs_compiled_source_length; // 0x006b14bc

// blam-cc: length in EBX
// Appends `length` bytes from `text` to the shared growable hs_compiled_source buffer
// (allocating or growing it as needed) and returns a pointer to the start of the appended
// region, or NULL on allocation failure.
char *hs_source_buffer_append(char *text, uint32_t length)
{
    void *new_buffer;
    char *dest;
    char *result;
    uint32_t words;
    uint32_t tail_bytes;
    uint32_t new_size;

    new_size = (uint32_t)hs_compiled_source_length + 1 + length;
    if (hs_compiled_source == 0) {
        new_buffer = GlobalAlloc(0, new_size);
    } else {
        if (new_size == 0) {
            GlobalFree(hs_compiled_source);
            return 0;
        }
        new_buffer = GlobalReAlloc(hs_compiled_source, new_size, 2);
    }
    if (new_buffer == 0) {
        return 0;
    }
    result = (char *)new_buffer + hs_compiled_source_length;
    dest = result;
    hs_compiled_source = (char *)new_buffer;
    for (words = length >> 2; words != 0; words = words - 1) {
        *(uint32_t *)dest = *(uint32_t *)text;
        text = text + 4;
        dest = dest + 4;
    }
    for (tail_bytes = length & 3; tail_bytes != 0; tail_bytes = tail_bytes - 1) {
        *dest = *text;
        text = text + 1;
        dest = dest + 1;
    }
    hs_compiled_source_length = hs_compiled_source_length + length;
    hs_compiled_source[hs_compiled_source_length] = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x4856f0):

undefined4 * hs_source_buffer_append(undefined4 *param_1)

{
  SIZE_T dwBytes;
  HGLOBAL pvVar1;
  undefined4 *puVar2;
  uint uVar3;
  uint unaff_EBX;
  undefined4 *puVar4;

  dwBytes = DAT_006b14bc + 1 + unaff_EBX;
  if (DAT_006b14c0 == (HGLOBAL)0x0) {
    pvVar1 = GlobalAlloc(0,dwBytes);
  }
  else {
    if (dwBytes == 0) {
      GlobalFree(DAT_006b14c0);
      return (undefined4 *)0x0;
    }
    pvVar1 = GlobalReAlloc(DAT_006b14c0,dwBytes,2);
  }
  if (pvVar1 == (HGLOBAL)0x0) {
    return (undefined4 *)0x0;
  }
  puVar2 = (undefined4 *)((int)pvVar1 + DAT_006b14bc);
  puVar4 = puVar2;
  DAT_006b14c0 = pvVar1;
  for (uVar3 = unaff_EBX >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar4 = *param_1;
    param_1 = param_1 + 1;
    puVar4 = puVar4 + 1;
  }
  for (uVar3 = unaff_EBX & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *(undefined1 *)puVar4 = *(undefined1 *)param_1;
    param_1 = (undefined4 *)((int)param_1 + 1);
    puVar4 = (undefined4 *)((int)puVar4 + 1);
  }
  DAT_006b14bc = DAT_006b14bc + unaff_EBX;
  *(undefined1 *)((int)DAT_006b14c0 + DAT_006b14bc) = 0;
  return puVar2;
}
#endif
