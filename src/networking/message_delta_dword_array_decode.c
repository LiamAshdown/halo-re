// message_delta_dword_array_decode  (Ghidra: FUN_004ea040; named per this rewrite)
// address 0x4ea040, size 468 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md ("Decode counterpart for a delta-encoded array of
// 4-byte values: copies unchanged elements or decodes changed ones."); decode counterpart of
// message_delta_float_array_encode (0x4e9db0), same message_delta_scalar_array_descriptor at
// message_delta_field_type::array_descriptor and the same reserved-flag-block seek idiom.
// register convention: none beyond the stack-recognized four parameters.
// REVIEW PASS 2026-09-20: retranslated against tools/pack.py 0x4ea040. The first draft advanced
// both in-loop seeks from the current cursor; Ghidra reloads param_4+8 (bit_stream::first_bit)
// immediately before each one, so both are absolute seeks to first_bit + a relative position, and
// the running flag-bit position starts at (cursor - first_bit) measured before the reserved block
// is skipped rather than at the reserved bit count. Unlike its encode counterpart this function
// has no trailing rewind: when nothing changed it simply returns 0 with the cursor left wherever
// the last seek put it.
// UNSURE (callers=0 in this batch): bit_stream_read_bits_chunked is called with only the stream
// visible, so the 0x20 width and the destination slot below are this file's reconstruction,
// mirroring the fixed 32-bit width of the encode side.

// REGISTRY (2026-09-20): this is the decode half of field-type kind 14 (point2d, point3d);
// its encode counterpart is 0x4e9db0. See the registry in types/networking.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern int32_t bit_stream_read_bits_chunked(int32_t total_bit_count, uint32_t *buffer,
    bit_stream *stream); // 0x4cf950
extern uint32_t bit_stream_read_bit(uint8_t *out_bit, bit_stream *stream); // 0x4cfb80

// Decodes an array of 4-byte values. When previous is NULL every element is read unconditionally.
// Otherwise the per-element changed bits sit in a block of field_type->reserved_bits reserved at
// the head of the array and the values follow it: for each element the decoder seeks to that
// element's flag bit, reads it, seeks back to the value cursor and either copies the previous
// dword verbatim or reads a fresh one. Returns the bits the changed values consumed, plus
// field_type->reserved_bits when anything changed at all.
int32_t message_delta_dword_array_decode(message_delta_field_type *field_type, uint32_t *previous,
    uint32_t *destination, bit_stream *stream)
{
    message_delta_scalar_array_descriptor *descriptor;
    int32_t total_bits;
    int32_t index;
    uint32_t absolute_bit;
    uint32_t first_bit;
    uint32_t target_bit;
    int32_t reserved_bits;
    int32_t flag_position;   // Ghidra's iVar8: bits from first_bit to this element's flag
    int32_t data_position;   // Ghidra's iVar6: bits from first_bit to the value cursor
    uint8_t changed_bit;
    uintptr_t previous_offset;
    uint32_t *cursor;
    int32_t decoded_bits;

    descriptor = (message_delta_scalar_array_descriptor *)field_type->array_descriptor;
    total_bits = 0;
    cursor = destination;

    if (previous == 0) {
        if (0 < descriptor->count) {
            for (index = 0; index < descriptor->count; index = index + 1) {
                decoded_bits = bit_stream_read_bits_chunked(0x20, &destination[index], stream);
                total_bits = total_bits + decoded_bits;
            }
        }
        return total_bits;
    }

    reserved_bits = field_type->reserved_bits;
    absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
    flag_position = (int32_t)(absolute_bit - stream->first_bit);
    target_bit = absolute_bit + reserved_bits;
    if ((reserved_bits > -1 || target_bit <= absolute_bit) &&
        (reserved_bits < 1 || absolute_bit <= target_bit) &&
        ((stream->first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->bit_cursor = target_bit & 7;
        stream->byte_cursor = target_bit >> 3;
    }

    if (descriptor->count < 1) {
        return total_bits;
    }
    previous_offset = (uint8_t *)previous - (uint8_t *)destination;
    for (index = 0; index < descriptor->count; index = index + 1) {
        first_bit = stream->first_bit;
        data_position = (int32_t)((stream->bit_cursor + stream->byte_cursor * 8) - first_bit);
        changed_bit = 0;

        target_bit = (uint32_t)flag_position + first_bit;
        if ((flag_position > -1 || target_bit <= first_bit) &&
            (flag_position < 1 || first_bit <= target_bit) &&
            ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                target_bit == stream->last_bit + 1)) {
            stream->bit_cursor = target_bit & 7;
            stream->byte_cursor = target_bit >> 3;
        }

        bit_stream_read_bit(&changed_bit, stream);

        first_bit = stream->first_bit;
        target_bit = first_bit + (uint32_t)data_position;
        if ((data_position > -1 || target_bit <= first_bit) &&
            (data_position < 1 || first_bit <= target_bit) &&
            ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                target_bit == stream->last_bit + 1)) {
            stream->bit_cursor = target_bit & 7;
            stream->byte_cursor = target_bit >> 3;
        }

        if (changed_bit == 0) {
            *cursor = *(uint32_t *)((uint8_t *)cursor + previous_offset);
        } else {
            decoded_bits = bit_stream_read_bits_chunked(0x20, cursor, stream);
            total_bits = total_bits + decoded_bits;
        }

        cursor = cursor + 1;
        flag_position = flag_position + 1;
    }
    if (0 < total_bits) {
        return total_bits + field_type->reserved_bits;
    }
    return total_bits;
}

#if 0
Original Ghidra decompilation (0x4ea040), from tools/pack.py 0x4ea040:

int FUN_004ea040(int param_1,uint param_2,undefined4 *param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  int *piVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  int local_10;

  iVar4 = param_4;
  puVar7 = param_3;
  piVar3 = *(int **)(param_1 + 0x58);
  local_10 = 0;
  if (param_2 == 0) {
    iVar8 = 0;
    iVar5 = 0;
    if (0 < *piVar3) {
      do {
        iVar5 = bit_stream_read_bits_chunked(iVar4);
        iVar5 = local_10 + iVar5;
        iVar8 = iVar8 + 1;
        local_10 = iVar5;
      } while (iVar8 < *piVar3);
    }
  }
  else {
    iVar5 = *(int *)(param_1 + 0x60);
    uVar1 = *(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8;
    iVar8 = uVar1 - *(uint *)(param_4 + 8);
    uVar2 = uVar1 + iVar5;
    if ((((-1 < iVar5) || (uVar2 <= uVar1)) && ((iVar5 < 1 || (uVar1 <= uVar2)))) &&
       (((*(uint *)(param_4 + 8) <= uVar2 && (uVar2 <= *(uint *)(param_4 + 0x14))) ||
        (uVar2 == *(int *)(param_4 + 0x14) + 1U)))) {
      *(uint *)(param_4 + 0x10) = uVar2 & 7;
      *(uint *)(param_4 + 0xc) = uVar2 >> 3;
    }
    param_3 = (undefined4 *)0x0;
    iVar5 = local_10;
    if (0 < *piVar3) {
      iVar5 = param_2 - (int)puVar7;
      do {
        uVar2 = *(uint *)(iVar4 + 8);
        iVar6 = (*(int *)(iVar4 + 0x10) + *(int *)(iVar4 + 0xc) * 8) - uVar2;
        param_2 = param_2 & 0xffffff00;
        uVar1 = iVar8 + uVar2;
        if (((-1 < iVar8) || (uVar1 <= uVar2)) &&
           (((iVar8 < 1 || (uVar2 <= uVar1)) &&
            (((uVar2 <= uVar1 && (uVar1 <= *(uint *)(iVar4 + 0x14))) ||
             (uVar1 == *(int *)(iVar4 + 0x14) + 1U)))))) {
          *(uint *)(iVar4 + 0x10) = uVar1 & 7;
          *(uint *)(iVar4 + 0xc) = uVar1 >> 3;
        }
        bit_stream_read_bit(&param_2);
        uVar2 = *(uint *)(iVar4 + 8);
        uVar1 = uVar2 + iVar6;
        if (((-1 < iVar6) || (uVar1 <= uVar2)) &&
           (((iVar6 < 1 || (uVar2 <= uVar1)) &&
            (((uVar2 <= uVar1 && (uVar1 <= *(uint *)(iVar4 + 0x14))) ||
             (uVar1 == *(int *)(iVar4 + 0x14) + 1U)))))) {
          *(uint *)(iVar4 + 0x10) = uVar1 & 7;
          *(uint *)(iVar4 + 0xc) = uVar1 >> 3;
        }
        if ((char)param_2 == '\0') {
          *puVar7 = *(undefined4 *)(iVar5 + (int)puVar7);
        }
        else {
          iVar6 = bit_stream_read_bits_chunked(iVar4);
          local_10 = local_10 + iVar6;
        }
        puVar7 = puVar7 + 1;
        param_3 = (undefined4 *)((int)param_3 + 1);
        iVar8 = iVar8 + 1;
      } while ((int)param_3 < *piVar3);
      iVar5 = local_10;
      if (0 < local_10) {
        return local_10 + *(int *)(param_1 + 0x60);
      }
    }
  }
  return iVar5;
}
#endif
