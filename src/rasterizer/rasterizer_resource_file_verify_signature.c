// rasterizer_resource_file_verify_signature  (Ghidra: FUN_00519980, unnamed; named per
// out/phase4/rasterizer_types_notes.md, which already refers to this function by this name from
// rasterizer_load_file_and_verify's evidence)
// address 0x519980, size 107 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: rejects buffers under 0x22 bytes, then computes a 33 byte reference signature (via
//   two callees outside this session's range) and compares it against the buffer's trailing 33
//   bytes.
// register convention: buffer size in in_EAX, buffer pointer in unaff_EBX.
//   // blam-cc: in_EAX -> size, unaff_EBX -> buffer
// UNSURE: FUN_00618350/FUN_0061a730's real roles (outside this session's range) -- the second
//   almost certainly fills the 33 byte local_24 reference signature (likely a build date/version
//   stamp), the first's effect is not resolved; both are called with no visible arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void FUN_00618350(void); // 0x618350, UNSURE
extern void FUN_0061a730(void); // 0x61a730, UNSURE: likely fills a reference signature buffer

// blam-cc: in_EAX -> size, unaff_EBX -> buffer
// Rejects undersized buffers, then compares the last 33 bytes of `buffer` against a freshly
// computed reference signature.
uint8_t rasterizer_resource_file_verify_signature(uint8_t *buffer, uint32_t size)
{
    char reference[36]; // local_24, only the first 33 bytes are meaningfully compared
    const char *tail;
    const char *ref;
    int32_t remaining;
    uint8_t matches;

    if (size < 0x22) {
        return (uint8_t)(size & 0xffffff00);
    }

    FUN_00618350(); // UNSURE
    FUN_0061a730(); // UNSURE: presumably fills `reference`

    matches = 1;
    remaining = 0x21;
    tail = (const char *)(buffer - 0x21 + size);
    ref = reference;
    do {
        if (remaining == 0) {
            break;
        }
        remaining = remaining - 1;
        matches = (*tail == *ref);
        tail++;
        ref++;
    } while (matches);

    return matches;
}

#if 0
Original Ghidra decompilation (0x519980):

uint FUN_00519980(void)

{
  uint in_EAX;
  int iVar1;
  int unaff_EBX;
  char *pcVar2;
  char *pcVar3;
  bool bVar4;
  char local_24 [36];

  if (in_EAX < 0x22) {
    return in_EAX & 0xffffff00;
  }
  FUN_00618350();
  FUN_0061a730();
  bVar4 = true;
  iVar1 = 0x21;
  pcVar2 = (char *)(unaff_EBX + -0x21 + in_EAX);
  pcVar3 = local_24;
  do {
    if (iVar1 == 0) break;
    iVar1 = iVar1 + -1;
    bVar4 = *pcVar2 == *pcVar3;
    pcVar2 = pcVar2 + 1;
    pcVar3 = pcVar3 + 1;
  } while (bVar4);
  return (uint)bVar4;
}
#endif
