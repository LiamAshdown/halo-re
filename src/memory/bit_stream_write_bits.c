// bit_stream_write_bits  (Ghidra: bit_stream_write_bits, already named)
// address 0x4cfa20, size 348 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/memory_types_notes.md "bit_stream (0x18)" and "mask tables" sections;
// field offsets match bit_stream exactly; mask tables read directly out of the image at
// 0x0065c2b4 (bit_mask_clear[8] = 0xff<<i) and 0x0065c2c0 (bit_mask_keep[9] = (1<<i)-1, entry 8
// = 0xff). DAT_0065c2c8 is bit_mask_keep[8], indexed with a negative offset in the original.
// register convention: bit count as the recognized parameter (param_1), value to write in EDX
// (in_EDX), stream pointer in ESI (unaff_ESI).

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"

extern uint8_t bit_mask_clear[8]; // 0x0065c2b4, entry i = (uint8_t)(0xff << i)
extern uint8_t bit_mask_keep[9];  // 0x0065c2c0, entry i = (uint8_t)((1 << i) - 1), entry 8 = 0xff

// blam-cc: bit count as the recognized parameter, value in EDX, stream in ESI
// Writes the low bit_count bits of `value` (LSB first) into the stream at its current cursor,
// merging into partially-filled bytes with the mask tables above. bit_count must fit in the
// stream's remaining bounds as one contiguous run; on success the cursor is advanced by
// bit_count bits and 1 is returned, otherwise the stream is left untouched and 0 is returned.
uint8_t bit_stream_write_bits(uint32_t bit_count, uint32_t value, bit_stream *stream)
{
    int32_t initial_bit_cursor;
    uint32_t last_pos;
    uint32_t result;
    uint32_t written;
    uint8_t merge_mask;
    uint8_t *byte_ptr;
    uint8_t low_byte;
    uint32_t chunk;
    uint32_t pos;

    initial_bit_cursor = stream->bit_cursor;
    last_pos = (uint32_t)initial_bit_cursor + (uint32_t)stream->byte_cursor * 8 - 1 + bit_count;
    result = 0;
    if (stream->first_bit <= last_pos && last_pos <= stream->last_bit) {
        written = 0;
        if (initial_bit_cursor != 0) {
            // Fill the remainder of the current partial byte first.
            uint32_t room_in_byte = 8 - (uint32_t)initial_bit_cursor;
            if (bit_count < room_in_byte) {
                merge_mask = bit_mask_keep[bit_count];
                written = bit_count;
            } else {
                merge_mask = bit_mask_keep[8 - initial_bit_cursor]; // (&bit_mask_keep[8])[-initial_bit_cursor]
                written = room_in_byte;
            }
            byte_ptr = stream->data + stream->byte_cursor;
            low_byte = (uint8_t)value;
            value = value >> (written & 0x1f);
            *byte_ptr = (uint8_t)(~(merge_mask << (initial_bit_cursor & 0x1f)) & *byte_ptr) |
                        (uint8_t)((low_byte & merge_mask) << (initial_bit_cursor & 0x1f));
            pos = written + (uint32_t)stream->byte_cursor * 8 + (uint32_t)stream->bit_cursor;
            if ((stream->first_bit <= pos && pos <= stream->last_bit) ||
                pos == stream->last_bit + 1) {
                stream->bit_cursor = pos & 7;
                stream->byte_cursor = pos >> 3;
            }
        }
        while (written < bit_count) {
            chunk = bit_count - written;
            if (chunk < 8) {
                byte_ptr = stream->data + stream->byte_cursor;
                *byte_ptr = (bit_mask_clear[chunk] & *byte_ptr) |
                            (bit_mask_keep[chunk] & (uint8_t)value);
                written = written + chunk;
                value = value >> (chunk & 0x1f);
                pos = chunk + (uint32_t)stream->byte_cursor * 8 + (uint32_t)stream->bit_cursor;
                if ((stream->first_bit <= pos && pos <= stream->last_bit) ||
                    pos == stream->last_bit + 1) {
                    stream->bit_cursor = pos & 7;
                    stream->byte_cursor = pos >> 3;
                }
            } else {
                stream->data[stream->byte_cursor] = (uint8_t)value;
                pos = (uint32_t)stream->bit_cursor + 8 + (uint32_t)stream->byte_cursor * 8;
                value = value >> 8;
                written = written + 8;
                if ((stream->first_bit <= pos && pos <= stream->last_bit) ||
                    pos == stream->last_bit + 1) {
                    stream->bit_cursor = pos & 7;
                    stream->byte_cursor = pos >> 3;
                }
            }
        }
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4cfa20):

uint bit_stream_write_bits(uint param_1)

{
  int iVar1;
  byte bVar2;
  byte bVar3;
  uint in_EAX;
  uint uVar4;
  uint uVar5;
  uint in_EDX;
  uint uVar6;
  byte *pbVar7;
  int unaff_ESI;
  
  iVar1 = *(int *)(unaff_ESI + 0x10);
  uVar6 = iVar1 + *(int *)(unaff_ESI + 0xc) * 8 + -1 + param_1;
  uVar4 = in_EAX & 0xffffff00;
  if ((*(uint *)(unaff_ESI + 8) <= uVar6) && (uVar6 <= *(uint *)(unaff_ESI + 0x14))) {
    uVar6 = 0;
    if (iVar1 != 0) {
      uVar6 = -iVar1 + 8;
      if (param_1 < uVar6) {
        bVar3 = (&DAT_0065c2c0)[param_1];
        uVar6 = param_1;
      }
      else {
        bVar3 = (&DAT_0065c2c8)[-iVar1];
      }
      pbVar7 = (byte *)(*(int *)(unaff_ESI + 4) + *(int *)(unaff_ESI + 0xc));
      bVar2 = (byte)in_EDX;
      in_EDX = in_EDX >> ((byte)uVar6 & 0x1f);
      *pbVar7 = ~(bVar3 << ((byte)iVar1 & 0x1f)) & *pbVar7 | (bVar2 & bVar3) << ((byte)iVar1 & 0x1f)
      ;
      uVar4 = uVar6 + *(int *)(unaff_ESI + 0xc) * 8 + *(int *)(unaff_ESI + 0x10);
      if (((*(uint *)(unaff_ESI + 8) <= uVar4) && (uVar4 <= *(uint *)(unaff_ESI + 0x14))) ||
         (uVar4 == *(int *)(unaff_ESI + 0x14) + 1U)) {
        uVar5 = uVar4 & 7;
        uVar4 = uVar4 >> 3;
        *(uint *)(unaff_ESI + 0x10) = uVar5;
        *(uint *)(unaff_ESI + 0xc) = uVar4;
      }
    }
    while (uVar6 < param_1) {
      uVar4 = param_1 - uVar6;
      if (uVar4 < 8) {
        pbVar7 = (byte *)(*(int *)(unaff_ESI + 0xc) + *(int *)(unaff_ESI + 4));
        *pbVar7 = (&DAT_0065c2b4)[uVar4] & *pbVar7 | (&DAT_0065c2c0)[uVar4] & (byte)in_EDX;
        uVar6 = uVar6 + uVar4;
        in_EDX = in_EDX >> ((byte)uVar4 & 0x1f);
        uVar4 = uVar4 + *(int *)(unaff_ESI + 0xc) * 8 + *(int *)(unaff_ESI + 0x10);
        if (((*(uint *)(unaff_ESI + 8) <= uVar4) && (uVar4 <= *(uint *)(unaff_ESI + 0x14))) ||
           (uVar4 == *(int *)(unaff_ESI + 0x14) + 1U)) {
          uVar5 = uVar4 & 7;
          uVar4 = uVar4 >> 3;
          *(uint *)(unaff_ESI + 0x10) = uVar5;
          *(uint *)(unaff_ESI + 0xc) = uVar4;
        }
      }
      else {
        *(byte *)(*(int *)(unaff_ESI + 0xc) + *(int *)(unaff_ESI + 4)) = (byte)in_EDX;
        uVar4 = *(int *)(unaff_ESI + 0x10) + 8 + *(int *)(unaff_ESI + 0xc) * 8;
        in_EDX = in_EDX >> 8;
        uVar6 = uVar6 + 8;
        if (((*(uint *)(unaff_ESI + 8) <= uVar4) && (uVar4 <= *(uint *)(unaff_ESI + 0x14))) ||
           (uVar4 == *(int *)(unaff_ESI + 0x14) + 1U)) {
          uVar5 = uVar4 & 7;
          uVar4 = uVar4 >> 3;
          *(uint *)(unaff_ESI + 0x10) = uVar5;
          *(uint *)(unaff_ESI + 0xc) = uVar4;
        }
      }
    }
    uVar4 = CONCAT31((int3)(uVar4 >> 8),1);
  }
  return uVar4;
}
#endif
