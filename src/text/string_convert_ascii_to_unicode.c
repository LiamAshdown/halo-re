// string_convert_ascii_to_unicode  (Ghidra: FUN_00557990; renamed here)
// address 0x557990, size 66 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/text_types_notes.md corrects the phase 2 summary: "The phase 2
//   summary of 0x557990 says 'length-prefixed'. That is wrong: in_EAX + 2 + (i-1)*2 is
//   just dst[i], a plain NUL-terminated copy." Every write address in the decompile is a
//   +2-offset artifact of Ghidra's pre-decremented loop index (iVar3 = iVar3 - 1 before
//   its first use as an array index), not a 2-byte header in the destination buffer.
// register convention: EAX = dst (uint16_t*, in_EAX), EBX = source (char*, unaff_EBX),
//   EDI = capacity in bytes (unaff_EDI). No stack arguments (Ghidra's fully-unrecognized
//   "FUN_00557990(void)").

#include "tags.h"
#include "memory.h"
#include "text.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX=dst, EBX=source, EDI=capacity_bytes
// Widens source (narrow, byte-per-character) into dst (UTF-16), NUL-terminated, copied
// back-to-front. Copies min(strlen(source), capacity_bytes / 2 - 1) characters.
// Quirk kept from the binary: with capacity_bytes < 2 the clamp makes count -1, the
// capacity test (count * 2 + 2 = 0) still passes, and the NUL lands at dst[-1]
// (0x5579b6 mov [eax+ecx*2+2],0 with ecx = -2). The early-out at 0x5579cf is only
// reachable if strlen * 2 + 2 wraps. EAX is never changed on the success path, so the
// caller sees dst (0 on the early-out) in EAX; the source may have returned it.
// FIXED (objdump 0x557990..0x5579d1): returns EAX -- dst, or 0 when even a truncated copy does not fit (0x5579cf);
// the terminator is written before the copy runs backwards. Parameters ordered as every caller declares them (the
// registers are bound by name: EAX dst, EDI capacity in bytes, EBX source).
uint16_t *string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source)
{
    int32_t count;
    int32_t i;

    count = (int32_t)strlen(source);
    if (capacity_bytes < (uint32_t)count * 2 + 2) {
        count = (int32_t)(capacity_bytes >> 1) - 1;
    }
    if ((uint32_t)count * 2 + 2 > capacity_bytes) {
        return 0;
    }
    dst[count] = 0;
    for (i = count - 1; i >= 0; i--) {
        dst[i] = (uint16_t)(uint8_t)source[i];
    }
    return dst;
}

#if 0
Original Ghidra decompilation (0x557990):

void FUN_00557990(void)

{
  char cVar1;
  int in_EAX;
  char *pcVar2;
  int iVar3;
  int iVar4;
  char *unaff_EBX;
  uint unaff_EDI;

  pcVar2 = unaff_EBX;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  iVar3 = (int)pcVar2 - (int)(unaff_EBX + 1);
  if (unaff_EDI < iVar3 * 2 + 2U) {
    iVar3 = (unaff_EDI >> 1) - 1;
  }
  if (iVar3 * 2 + 2U <= unaff_EDI) {
    iVar3 = iVar3 + -1;
    *(undefined2 *)(in_EAX + 2 + iVar3 * 2) = 0;
    if (-1 < iVar3) {
      do {
        iVar4 = iVar3 + -1;
        *(ushort *)(in_EAX + 2 + iVar4 * 2) = (ushort)(byte)unaff_EBX[iVar3];
        iVar3 = iVar4;
      } while (-1 < iVar4);
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
