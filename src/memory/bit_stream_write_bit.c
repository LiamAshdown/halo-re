// bit_stream_write_bit  (Ghidra: bit_stream_write_bit, already named)
// address 0x4cf9a0, size 117 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/memory_types_notes.md "bit_stream (0x18)"; field offsets +4 data,
// +8 first_bit, +0xc byte_cursor, +0x10 bit_cursor, +0x14 last_bit match types/memory.h
// bit_stream exactly, cross-checked against bit_stream_write_bits/read_bit/read_bits.
// (in_EDX, unresolved register read) -- exposed as EDX before EAX's stack-shaped param_1 would
// be wrong, but param_1 here is Ghidra's own recognized parameter, not an in_/unaff_ read, so it
// keeps its declared position; only in_EDX gets a register comment.
// UNSURE: field +0x00 (unknown_00) is never touched by this function or any other in the module.

#include "tags.h"
#include "memory.h"

// Writes a single bit (0 or 1) into a bounds-checked bit stream and advances its one-bit cursor.
// Returns nonzero (with garbage high bits, only the low byte is meaningful to callers) on
// success, or leftover/undefined low byte on a stream-bounds failure -- callers only ever test
// the low byte, so the return type is kept as int for fidelity with the Ghidra signature.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: EDX -> stream, stack -> bit_value
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t bit_stream_write_bit(int32_t bit_value, bit_stream *stream)
{
    int32_t byte_cursor;
    uint32_t pos;
    uint32_t result;
    uint8_t bit_index;
    uint8_t *byte_ptr;
    uint32_t new_pos;

    byte_cursor = stream->byte_cursor;
    pos = (uint32_t)stream->bit_cursor + (uint32_t)byte_cursor * 8;
    result = 0; // UNSURE: Ghidra initializes this from leftover in_EAX & 0xffffff00; callers
                // only read the low byte, which is 0 either way, so 0 is behaviorally identical.
    if (stream->first_bit <= pos && pos <= stream->last_bit) {
        bit_index = (uint8_t)stream->bit_cursor;
        byte_ptr = stream->data + byte_cursor;
        if (bit_value == 1) {
            *byte_ptr = *byte_ptr | (uint8_t)(1 << (bit_index & 0x1f));
        } else {
            *byte_ptr = *byte_ptr & (uint8_t)~(1 << (bit_index & 0x1f));
        }
        new_pos = (uint32_t)stream->bit_cursor + 1 + (uint32_t)stream->byte_cursor * 8;
        if ((stream->first_bit <= new_pos && new_pos <= stream->last_bit) ||
            new_pos == stream->last_bit + 1) {
            stream->bit_cursor = new_pos & 7;
            stream->byte_cursor = new_pos >> 3;
        }
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4cf9a0):

uint bit_stream_write_bit(int param_1)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;
  byte bVar3;
  uint uVar4;
  int in_EDX;
  byte *pbVar5;
  
  iVar1 = *(int *)(in_EDX + 0xc);
  uVar2 = *(int *)(in_EDX + 0x10) + iVar1 * 8;
  uVar4 = in_EAX & 0xffffff00;
  if ((*(uint *)(in_EDX + 8) <= uVar2) && (uVar2 <= *(uint *)(in_EDX + 0x14))) {
    bVar3 = (byte)*(int *)(in_EDX + 0x10);
    if (param_1 == 1) {
      pbVar5 = (byte *)(iVar1 + *(int *)(in_EDX + 4));
      bVar3 = *pbVar5 | '\x01' << (bVar3 & 0x1f);
    }
    else {
      pbVar5 = (byte *)(iVar1 + *(int *)(in_EDX + 4));
      bVar3 = *pbVar5 & ~('\x01' << (bVar3 & 0x1f));
    }
    *pbVar5 = bVar3;
    uVar2 = *(int *)(in_EDX + 0x10) + 1 + *(int *)(in_EDX + 0xc) * 8;
    if (((*(uint *)(in_EDX + 8) <= uVar2) && (uVar2 <= *(uint *)(in_EDX + 0x14))) ||
       (uVar2 == *(int *)(in_EDX + 0x14) + 1U)) {
      uVar4 = uVar2 & 7;
      uVar2 = uVar2 >> 3;
      *(uint *)(in_EDX + 0x10) = uVar4;
      *(uint *)(in_EDX + 0xc) = uVar2;
    }
    uVar4 = CONCAT31((int3)(uVar2 >> 8),1);
  }
  return uVar4;
}
#endif
