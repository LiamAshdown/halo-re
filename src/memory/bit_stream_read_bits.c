// bit_stream_read_bits  (Ghidra: bit_stream_read_bits, already named)
// address 0x4cfbf0, size 341 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: mirror of bit_stream_write_bits (0x4cfa20); same bit_stream fields and mask tables.
// register convention: bit count as the recognized parameter (param_1), destination uint32_t*
// as the recognized parameter (param_2), stream pointer in EDX (in_EDX).

#include "tags.h"
#include "memory.h"

extern uint8_t bit_mask_keep[9]; // 0x0065c2c0

// blam-cc: bit count and destination as the recognized parameters, stream in EDX
// Reads the low bit_count bits (LSB first) out of the stream into *out_value, merging with
// whatever was already in *out_value above bit position bit_count. Always returns bit_count;
// the stream-bounds failure path leaves *out_value untouched and returns 0.
uint32_t bit_stream_read_bits(uint32_t bit_count, uint32_t *out_value, bit_stream *stream)
{
    int32_t initial_bit_cursor;
    int32_t initial_byte_cursor;
    uint32_t initial_pos;
    uint32_t last_pos;
    uint32_t first_run;   // bits consumed from the initial partial byte (local_10)
    uint32_t accumulator; // bytes read so far, packed LSB-first (local_c)
    uint32_t remaining;   // bits still to copy after the partial-byte step
    uint32_t pos;
    uint32_t chunk;
    uint8_t *byte_cursor_ptr;
    uint32_t low_mask;

    initial_bit_cursor = stream->bit_cursor;
    initial_byte_cursor = stream->byte_cursor;
    initial_pos = (uint32_t)initial_bit_cursor + (uint32_t)initial_byte_cursor * 8;
    last_pos = (bit_count - 1) + initial_pos;
    if (last_pos < stream->first_bit || stream->last_bit < last_pos) {
        return 0;
    }

    accumulator = 0;
    first_run = bit_count;
    if (8U - (uint32_t)initial_bit_cursor <= bit_count) {
        first_run = 8U - (uint32_t)initial_bit_cursor;
    }
    if (first_run == 8) {
        first_run = 0;
        remaining = bit_count;
    } else {
        pos = first_run + initial_pos;
        if ((stream->first_bit <= pos && pos <= stream->last_bit) || pos == stream->last_bit + 1) {
            stream->bit_cursor = pos & 7;
            stream->byte_cursor = pos >> 3;
        }
        remaining = bit_count - first_run;
    }

    accumulator = 0;
    if (remaining != 0) {
        uint8_t *data = stream->data;
        uint8_t *dst = (uint8_t *)&accumulator;
        chunk = remaining;
        do {
            uint32_t take = chunk;
            if (7 < chunk) {
                take = 8;
            }
            *dst = data[stream->byte_cursor] & bit_mask_keep[take];
            pos = take + (uint32_t)stream->byte_cursor * 8 + (uint32_t)stream->bit_cursor;
            dst = dst + 1;
            if ((stream->first_bit <= pos && pos <= stream->last_bit) || pos == stream->last_bit + 1) {
                stream->bit_cursor = pos & 7;
                stream->byte_cursor = pos >> 3;
            }
            chunk = chunk - take;
        } while (chunk != 0);
    }

    low_mask = (bit_count < 0x20) ? (uint32_t)(-1 << (bit_count & 0x1f)) : 0;
    if (first_run == 0) {
        *out_value = (*out_value & low_mask) | accumulator;
        return bit_count;
    }
    byte_cursor_ptr = stream->data + initial_byte_cursor;
    *out_value = ((uint32_t)(*byte_cursor_ptr >> (initial_bit_cursor & 0x1f)) &
                  bit_mask_keep[first_run]) |
                 (*out_value & low_mask) | (accumulator << (first_run & 0x1f));
    return bit_count;
}

#if 0
Original Ghidra decompilation (0x4cfbf0):

uint bit_stream_read_bits(uint param_1,uint *param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int in_EDX;
  uint *puVar4;
  uint uVar5;
  uint uVar6;
  uint local_10;
  uint local_c;
  int local_8;
  int local_4;
  
  local_4 = *(int *)(in_EDX + 0x10);
  local_8 = *(int *)(in_EDX + 0xc);
  iVar1 = local_4 + local_8 * 8;
  uVar5 = (param_1 - 1) + iVar1;
  if ((uVar5 < *(uint *)(in_EDX + 8)) || (*(uint *)(in_EDX + 0x14) < uVar5)) {
    return 0;
  }
  local_c = 0;
  puVar4 = &local_c;
  local_10 = param_1;
  if (8U - local_4 <= param_1) {
    local_10 = 8U - local_4;
  }
  if (local_10 == 8) {
    local_10 = 0;
    uVar5 = param_1;
  }
  else {
    uVar5 = local_10 + iVar1;
    if (((*(uint *)(in_EDX + 8) <= uVar5) && (uVar5 <= *(uint *)(in_EDX + 0x14))) ||
       (uVar5 == *(int *)(in_EDX + 0x14) + 1U)) {
      *(uint *)(in_EDX + 0x10) = uVar5 & 7;
      *(uint *)(in_EDX + 0xc) = uVar5 >> 3;
    }
    uVar5 = param_1 - local_10;
  }
  uVar6 = 0;
  if (uVar5 != 0) {
    iVar1 = *(int *)(in_EDX + 4);
    do {
      uVar6 = uVar5;
      if (7 < uVar5) {
        uVar6 = 8;
      }
      iVar2 = *(int *)(in_EDX + 0xc);
      *(byte *)puVar4 = *(byte *)(iVar1 + iVar2) & (&DAT_0065c2c0)[uVar6];
      uVar3 = uVar6 + iVar2 * 8 + *(int *)(in_EDX + 0x10);
      puVar4 = (uint *)((int)puVar4 + 1);
      if (((*(uint *)(in_EDX + 8) <= uVar3) && (uVar3 <= *(uint *)(in_EDX + 0x14))) ||
         (uVar3 == *(int *)(in_EDX + 0x14) + 1U)) {
        *(uint *)(in_EDX + 0x10) = uVar3 & 7;
        *(uint *)(in_EDX + 0xc) = uVar3 >> 3;
      }
      uVar5 = uVar5 - uVar6;
      uVar6 = local_c;
    } while (uVar5 != 0);
  }
  if (param_1 < 0x20) {
    uVar5 = -1 << ((byte)param_1 & 0x1f);
  }
  else {
    uVar5 = 0;
  }
  if (local_10 == 0) {
    *param_2 = *param_2 & uVar5 | uVar6;
    return param_1;
  }
  *param_2 = (uint)(*(byte *)(*(int *)(in_EDX + 4) + local_8) >> ((byte)local_4 & 0x1f) &
                   (&DAT_0065c2c0)[local_10]) | *param_2 & uVar5 | uVar6 << ((byte)local_10 & 0x1f);
  return param_1;
}
#endif
