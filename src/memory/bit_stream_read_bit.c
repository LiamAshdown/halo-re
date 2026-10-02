// bit_stream_read_bit  (Ghidra: bit_stream_read_bit, already named)
// address 0x4cfb80, size 103 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: same bit_stream field layout as bit_stream_write_bit (0x4cf9a0); mirror-image logic.
// register convention: destination bit pointer as the recognized parameter (param_1), stream
// pointer in EDX (in_EDX, unresolved register read).

#include "tags.h"
#include "memory.h"

// blam-cc: destination as the recognized parameter, stream in EDX
// Reads a single bit from a bounds-checked bit stream into *out_bit and advances the stream's
// one-bit cursor. Returns 1 on success, 0 if the stream has no room left.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint32_t bit_stream_read_bit(uint8_t *out_bit, bit_stream *stream)
{
    uint32_t pos;
    uint32_t result;
    uint8_t bit_index;
    uint32_t new_pos;

    pos = (uint32_t)stream->bit_cursor + (uint32_t)stream->byte_cursor * 8;
    result = 0;
    if (stream->first_bit <= pos && pos <= stream->last_bit) {
        bit_index = (uint8_t)stream->bit_cursor;
        *out_bit = (uint8_t)((stream->data[stream->byte_cursor] & (1 << (bit_index & 0x1f))) >>
                              (bit_index & 0x1f));
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
Original Ghidra decompilation (0x4cfb80):

undefined4 bit_stream_read_bit(undefined1 *param_1)

{
  uint uVar1;
  undefined4 uVar2;
  byte bVar3;
  int in_EDX;
  
  uVar1 = *(int *)(in_EDX + 0x10) + *(int *)(in_EDX + 0xc) * 8;
  uVar2 = 0;
  if ((*(uint *)(in_EDX + 8) <= uVar1) && (uVar1 <= *(uint *)(in_EDX + 0x14))) {
    bVar3 = (byte)*(int *)(in_EDX + 0x10);
    *param_1 = (char)((int)((uint)*(byte *)(*(int *)(in_EDX + 4) + *(int *)(in_EDX + 0xc)) &
                           1 << (bVar3 & 0x1f)) >> (bVar3 & 0x1f));
    uVar1 = *(int *)(in_EDX + 0x10) + 1 + *(int *)(in_EDX + 0xc) * 8;
    if (((*(uint *)(in_EDX + 8) <= uVar1) && (uVar1 <= *(uint *)(in_EDX + 0x14))) ||
       (uVar1 == *(int *)(in_EDX + 0x14) + 1U)) {
      *(uint *)(in_EDX + 0x10) = uVar1 & 7;
      *(uint *)(in_EDX + 0xc) = uVar1 >> 3;
    }
    uVar2 = 1;
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
