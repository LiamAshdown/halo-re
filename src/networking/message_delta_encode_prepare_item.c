// message_delta_encode_prepare_item  (Ghidra: FUN_004ecb60; named per this rewrite)
// address 0x4ecb60, size 160 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: objdump -d -M intel bin/halo.exe @0x4eca36 (the call site in
// message_delta_encode_message): `lea eax,[esp+0x20]` immediately before `call 0x4ecb60`, so this
// receives the encode context at the SAME base address message_delta_encode_message built,
// confirming its offsets +0x04 (message_type), +0x08 (flag) and +0x0c (an unnamed value carried
// in EAX at message_delta_encode_message's own entry) line up with that function's own setup.
// register convention: encode context as the recognized parameter (in_EAX).
// UNSURE: the encode context is a large (at least 0x80-byte), only partially resolved scratch
// block private to the message_delta_encode_* cluster; it is accessed here by raw byte offset
// exactly as decompiled rather than through invented field names, per
// out/phase4/networking_types_notes.md's own admission that this subsystem is only partly
// resolved.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440

// Initializes per-item bit-offset state within the message-delta encode context before encoding
// one item's fields: when the message is flagged, precomputes the item's static-field bit range;
// either way, computes the item's total (static + array) bit range against the message's
// remaining budget.
int32_t message_delta_encode_prepare_item(uint8_t *ctx)
{
    #define CTXW(off) (*(int32_t *)(ctx + (off)))

    CTXW(0x40) = 0;
    CTXW(0x44) = 0;
    if (CTXW(8) == 1) {
        uint32_t bit_offset = (uint32_t)CTXW(0x3c);
        int32_t field_bits = message_delta_definitions[CTXW(4)]->field_bits; // +0x20
        CTXW(0x58) = bit_offset & 7;
        CTXW(0x50) = (int32_t)bit_offset;
        CTXW(0x5c) = (int32_t)(bit_offset - 1) + field_bits;
        CTXW(0x54) = (int32_t)(bit_offset >> 3);
        CTXW(0x48) = 0;
        CTXW(0x4c) = CTXW(0xc);
        CTXW(0x60) = field_bits;
        CTXW(0x40) = CTXW(0x40) + field_bits;
    } else {
        CTXW(0x48) = 0;
        CTXW(0x4c) = 0;
        CTXW(0x50) = 0;
        CTXW(0x54) = 0;
        CTXW(0x58) = 0;
        CTXW(0x5c) = 0;
        CTXW(0x60) = 0;
    }
    {
        uint32_t total_offset = (uint32_t)(CTXW(0x3c) + CTXW(0x40));
        int32_t remaining = CTXW(0x18) - CTXW(0x40);
        CTXW(0x74) = total_offset & 7;
        CTXW(0x64) = 0;
        CTXW(0x6c) = (int32_t)total_offset;
        CTXW(0x68) = CTXW(0xc);
        CTXW(0x70) = (int32_t)(total_offset >> 3);
        CTXW(0x78) = (remaining - 1) + (int32_t)total_offset;
        CTXW(0x7c) = remaining;
    }
    #undef CTXW
    return 1;
}

#if 0
Original Ghidra decompilation (0x4ecb60):

undefined4 FUN_004ecb60(void)

{
  int in_EAX;
  uint uVar1;
  int iVar2;

  *(undefined4 *)(in_EAX + 0x40) = 0;
  *(undefined4 *)(in_EAX + 0x44) = 0;
  if (*(int *)(in_EAX + 8) == 1) {
    uVar1 = *(uint *)(in_EAX + 0x3c);
    iVar2 = *(int *)((&PTR_DAT_0065d440)[*(int *)(in_EAX + 4)] + 0x20);
    *(uint *)(in_EAX + 0x58) = uVar1 & 7;
    *(uint *)(in_EAX + 0x50) = uVar1;
    *(uint *)(in_EAX + 0x5c) = (uVar1 - 1) + iVar2;
    *(uint *)(in_EAX + 0x54) = uVar1 >> 3;
    *(undefined4 *)(in_EAX + 0x48) = 0;
    *(undefined4 *)(in_EAX + 0x4c) = *(undefined4 *)(in_EAX + 0xc);
    *(int *)(in_EAX + 0x60) = iVar2;
    *(int *)(in_EAX + 0x40) = *(int *)(in_EAX + 0x40) + iVar2;
  }
  else {
    *(undefined4 *)(in_EAX + 0x48) = 0;
    *(undefined4 *)(in_EAX + 0x4c) = 0;
    *(undefined4 *)(in_EAX + 0x50) = 0;
    *(undefined4 *)(in_EAX + 0x54) = 0;
    *(undefined4 *)(in_EAX + 0x58) = 0;
    *(undefined4 *)(in_EAX + 0x5c) = 0;
    *(undefined4 *)(in_EAX + 0x60) = 0;
  }
  uVar1 = *(int *)(in_EAX + 0x3c) + *(int *)(in_EAX + 0x40);
  iVar2 = *(int *)(in_EAX + 0x18) - *(int *)(in_EAX + 0x40);
  *(uint *)(in_EAX + 0x74) = uVar1 & 7;
  *(undefined4 *)(in_EAX + 100) = 0;
  *(uint *)(in_EAX + 0x6c) = uVar1;
  *(undefined4 *)(in_EAX + 0x68) = *(undefined4 *)(in_EAX + 0xc);
  *(uint *)(in_EAX + 0x70) = uVar1 >> 3;
  *(uint *)(in_EAX + 0x78) = iVar2 + -1 + uVar1;
  *(int *)(in_EAX + 0x7c) = iVar2;
  return 1;
}
#endif
