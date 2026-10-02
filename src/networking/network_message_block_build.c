// network_message_block_build  (Ghidra: FUN_00440350, still unnamed -> renamed)
// address 0x440350, size 86 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("allocates or reuses a small heap
// buffer, encodes a length+flags header in its first word, and copies the supplied data
// after the header"); out/phase2/networking/00.md and `python tools/pack.py 0x440350`.
// All seven callers live outside this module (around the message-delta / test-message code
// near 0x4e93xx) and always immediately reassign their own variable from the call result,
// e.g. `local_608 = (ushort *)FUN_00440350(local_608);`, which is why the buffer pointer is
// treated here as an implicit EAX return despite Ghidra's void prototype (same situation as
// src/memory/circular_buffer_new.c).
// register convention: existing buffer pointer or NULL in EAX, source data pointer in ECX,
// flags (2 bits used) in DL (low byte of EDX), byte length on the stack (param_1).
// FIXED (register inputs, objdump): EDX/DL (read at 0x44035c, "mov bl,dl") carries flags; the
// blam-cc wording ("flag bits in DL") didn't match the flags parameter name, so it wasn't
// recognized as claimed. Reworded only; flags was already wired up correctly.
// UNSURE: the header encoding `((flags & 3) | (length + 2) * 4) << 2` is preserved verbatim;
// its consumer (some other module's bit-packed record format) is not recovered here.
// UNSURE: when buffer is already non-NULL its capacity is never checked against length, so a
// caller must already guarantee it is large enough.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


// blam-cc: existing buffer (or NULL) in EAX, source pointer in ECX, flags in DL, byte
// length in param_1 (stack)
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length)
{
    uint32_t *dest;
    uint32_t count;

    if (buffer == 0) {
        buffer = (uint16_t *)GlobalAlloc(0, (uint32_t)(int16_t)(length + 2));
        if (buffer == 0) {
            return 0;
        }
    }
    *buffer = (uint16_t)(int16_t)(((flags & 3) | (length + 2) * 4) << 2);
    if (source != 0) {
        dest = (uint32_t *)(buffer + 1);
        for (count = (length & 0xffff) >> 2; count != 0; count--) {
            *dest = *source;
            source++;
            dest++;
        }
        for (length = length & 3; length != 0; length--) {
            *(uint8_t *)dest = *(uint8_t *)source;
            source = (uint32_t *)((int)source + 1);
            dest = (uint32_t *)((int)dest + 1);
        }
    }
    return buffer;
}

#if 0
Original Ghidra decompilation (0x440350):

void FUN_00440350(uint param_1)

{
  undefined2 *in_EAX;
  undefined4 *in_ECX;
  uint uVar1;
  byte in_DL;
  undefined4 *puVar2;

  if ((in_EAX == (undefined2 *)0x0) &&
     (in_EAX = GlobalAlloc(0,(int)(short)(param_1 + 2)), in_EAX == (undefined2 *)0x0)) {
    return;
  }
  *in_EAX = (short)(((uint)(in_DL & 3) | (param_1 + 2) * 4) << 2);
  if (in_ECX != (undefined4 *)0x0) {
    puVar2 = (undefined4 *)(in_EAX + 1);
    for (uVar1 = (param_1 & 0xffff) >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
      *puVar2 = *in_ECX;
      in_ECX = in_ECX + 1;
      puVar2 = puVar2 + 1;
    }
    for (param_1 = param_1 & 3; param_1 != 0; param_1 = param_1 - 1) {
      *(undefined1 *)puVar2 = *(undefined1 *)in_ECX;
      in_ECX = (undefined4 *)((int)in_ECX + 1);
      puVar2 = (undefined4 *)((int)puVar2 + 1);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
