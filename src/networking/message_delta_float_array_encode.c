// message_delta_float_array_encode  (Ghidra: message_delta_float_array_encode, already named)
// address 0x4e9db0, size 651 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md ("Encodes an array of floats as per-element
// changed-bits plus full 32-bit values for entries that changed beyond an epsilon threshold.");
// message_delta_array_field_encode.c (0x4e95e0, the same reserved-flag-block idiom and the same
// message_delta_field_type +0x58 / +0x60 pair, here with a descriptor that is just {count}).
// register convention: none beyond the stack-recognized four parameters.
// REVIEW PASS 2026-09-20: retranslated against tools/pack.py 0x4e9db0 after three errors in the
// first draft.  (1) The two in-loop seeks are absolute from bit_stream::first_bit (Ghidra reloads
// param_4+8 before each), not relative to the current cursor.  (2) The trailing "nothing changed,
// rewind to the head of the reserved block" seek had been dropped entirely, leaving a dead store
// and an early return.  (3) The baseline branch's chunked write is a goto chain: after a
// successful 0x20-bit write the remaining width is 0 and control jumps straight to the
// accumulate step, so the tail bit_stream_write_bits never runs for a 32-bit element.  The draft
// fell through into it with a width of 0, adding a call the original does not make.
// UNSURE (callers=0 in this batch): bit_stream_write_bits, _chunked and _bit are all called with
// their register operands elided by the decompile, so the value and stream arguments below are
// this file's reconstruction; the baseline branch in particular never advances Ghidra's local_10,
// so whether each element or only the first is written there is not established.

// REGISTRY (2026-09-20): this is the encode half of field-type kind 14, whose .data records
// are named point2d and point3d (see the registry in types/networking.h); the {count} in the
// descriptor is the component count. Its decode counterpart is 0x4ea040.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern uint32_t bit_stream_write_bits(uint32_t bit_count, uint32_t value, bit_stream *stream); // 0x4cfa20
extern int32_t bit_stream_write_bits_chunked(int32_t total_bit_count, uint32_t value,
    bit_stream *stream); // 0x4cf8f0
extern uint32_t bit_stream_write_bit(int32_t bit_value, bit_stream *stream); // 0x4cf9a0, memory module;
    // UNSURE: the stream operand is in a register at this call site and Ghidra drops it;
    // the signature is src/memory/bit_stream_write_bit.c's.

// Encodes an array of floats. When previous is NULL every element's raw 32 bits are written
// unconditionally. Otherwise the per-element changed bits live in a block of
// field_type->reserved_bits reserved at the head of the array and the values follow it: an
// element whose difference from the previous value exceeds +/-0.0001 has its raw 32 bits written
// and its flag set, everything else only costs the flag. Returns the bits the changed values
// consumed plus field_type->reserved_bits, or rewinds to the head of the reserved block and
// returns 0 when nothing changed.
int32_t message_delta_float_array_encode(message_delta_field_type *field_type, float *previous,
    float *values, bit_stream *stream)
{
    message_delta_scalar_array_descriptor *descriptor;
    int32_t total_bits;
    float *cursor;
    int32_t index;
    int32_t remaining_width;
    uint32_t absolute_bit;
    uint32_t first_bit;
    uint32_t target_bit;
    int32_t reserved_bits;
    int32_t block_position;  // Ghidra's iVar6: bits from first_bit to the reserved flag block
    int32_t flag_position;   // Ghidra's iVar9 in the loop: this element's flag bit
    int32_t data_position;   // Ghidra's iVar7: bits from first_bit to the value cursor
    float delta;
    uint32_t changed_bit;
    int32_t written_bits;
    uintptr_t previous_offset;

    descriptor = (message_delta_scalar_array_descriptor *)field_type->array_descriptor;
    total_bits = 0;
    cursor = values;

    if (previous == 0) {
        if (0 < descriptor->count) {
            for (index = 0; index < descriptor->count; index = index + 1) {
                remaining_width = 0x20;
                while (0x1f < remaining_width) {
                    if (bit_stream_write_bits(0x20, *(uint32_t *)&cursor[index], stream) == 0) {
                        goto accumulate;
                    }
                    remaining_width = remaining_width - 0x20;
                    if (remaining_width < 1) {
                        goto accumulate;
                    }
                }
                if (bit_stream_write_bits(remaining_width, *(uint32_t *)&cursor[index], stream) != 0) {
                    remaining_width = 0;
                }
            accumulate:
                total_bits = total_bits + (0x20 - remaining_width);
            }
        }
        return total_bits;
    }

    previous_offset = (uint8_t *)previous - (uint8_t *)values;
    reserved_bits = field_type->reserved_bits;
    absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
    block_position = (int32_t)(absolute_bit - stream->first_bit);
    target_bit = absolute_bit + reserved_bits;
    if ((reserved_bits > -1 || target_bit <= absolute_bit) &&
        (reserved_bits < 1 || absolute_bit <= target_bit) &&
        ((stream->first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->bit_cursor = target_bit & 7;
        stream->byte_cursor = target_bit >> 3;
    }

    if (0 < descriptor->count) {
        flag_position = block_position;
        for (index = 0; index < descriptor->count; index = index + 1) {
            delta = *(float *)((uint8_t *)cursor + previous_offset) - *cursor;
            if (delta < -0.0001f || 0.0001f < delta) {
                changed_bit = 1;
                written_bits = bit_stream_write_bits_chunked(0x20, *(uint32_t *)cursor, stream);
                total_bits = total_bits + written_bits;
            } else {
                changed_bit = 0;
            }

            first_bit = stream->first_bit;
            data_position = (int32_t)((stream->bit_cursor + stream->byte_cursor * 8) - first_bit);
            target_bit = first_bit + (uint32_t)flag_position;
            if ((flag_position > -1 || target_bit <= first_bit) &&
                (flag_position < 1 || first_bit <= target_bit) &&
                ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                    target_bit == stream->last_bit + 1)) {
                stream->bit_cursor = target_bit & 7;
                stream->byte_cursor = target_bit >> 3;
            }

            bit_stream_write_bit((int32_t)changed_bit, stream);

            first_bit = stream->first_bit;
            target_bit = first_bit + (uint32_t)data_position;
            if ((data_position > -1 || target_bit <= first_bit) &&
                (data_position < 1 || first_bit <= target_bit) &&
                ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                    target_bit == stream->last_bit + 1)) {
                stream->bit_cursor = target_bit & 7;
                stream->byte_cursor = target_bit >> 3;
            }

            cursor = cursor + 1;
            flag_position = flag_position + 1;
        }
        if (0 < total_bits) {
            return total_bits + field_type->reserved_bits;
        }
    }

    first_bit = stream->first_bit;
    target_bit = first_bit + (uint32_t)block_position;
    if ((block_position > -1 || target_bit <= first_bit) &&
        (block_position < 1 || first_bit <= target_bit) &&
        ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
            target_bit == stream->last_bit + 1)) {
        stream->byte_cursor = target_bit >> 3;
        stream->bit_cursor = target_bit & 7;
        return total_bits;
    }
    return total_bits;
}

#if 0
Original Ghidra decompilation (0x4e9db0), from tools/pack.py 0x4e9db0:

int message_delta_float_array_encode(int param_1,int param_2,float *param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  int *piVar3;
  float fVar4;
  char cVar5;
  int iVar6;
  int iVar7;
  undefined4 uVar8;
  int iVar9;
  int local_14;
  float *local_10;
  int local_8;

  piVar3 = *(int **)(param_1 + 0x58);
  local_14 = 0;
  local_10 = param_3;
  if (param_2 == 0) {
    iVar9 = 0;
    if (0 < *piVar3) {
      do {
        iVar6 = 0x20;
        while (0x1f < iVar6) {
          cVar5 = bit_stream_write_bits(0x20);
          if ((cVar5 == '\0') || (iVar6 = iVar6 + -0x20, iVar6 < 1)) goto LAB_004ea008;
        }
        cVar5 = bit_stream_write_bits(iVar6);
        if (cVar5 != '\0') {
          iVar6 = 0;
        }
LAB_004ea008:
        local_14 = local_14 + (0x20 - iVar6);
        iVar9 = iVar9 + 1;
      } while (iVar9 < *piVar3);
    }
  }
  else {
    iVar9 = *(int *)(param_1 + 0x60);
    uVar1 = *(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8;
    iVar6 = uVar1 - *(uint *)(param_4 + 8);
    uVar2 = uVar1 + iVar9;
    if ((((-1 < iVar9) || (uVar2 <= uVar1)) && ((iVar9 < 1 || (uVar1 <= uVar2)))) &&
       (((*(uint *)(param_4 + 8) <= uVar2 && (uVar2 <= *(uint *)(param_4 + 0x14))) ||
        (uVar2 == *(int *)(param_4 + 0x14) + 1U)))) {
      *(uint *)(param_4 + 0x10) = uVar2 & 7;
      *(uint *)(param_4 + 0xc) = uVar2 >> 3;
    }
    local_8 = 0;
    if (0 < *piVar3) {
      iVar9 = iVar6;
      do {
        fVar4 = *(float *)((param_2 - (int)param_3) + (int)local_10) - *local_10;
        if ((fVar4 < -0.0001) || (0.0001 < fVar4)) {
          uVar8 = 1;
          iVar7 = bit_stream_write_bits_chunked(0x20);
          local_14 = local_14 + iVar7;
        }
        else {
          uVar8 = 0;
        }
        uVar2 = *(uint *)(param_4 + 8);
        iVar7 = (*(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8) - uVar2;
        uVar1 = uVar2 + iVar9;
        if ((((-1 < iVar9) || (uVar1 <= uVar2)) && ((iVar9 < 1 || (uVar2 <= uVar1)))) &&
           (((uVar2 <= uVar1 && (uVar1 <= *(uint *)(param_4 + 0x14))) ||
            (uVar1 == *(int *)(param_4 + 0x14) + 1U)))) {
          *(uint *)(param_4 + 0x10) = uVar1 & 7;
          *(uint *)(param_4 + 0xc) = uVar1 >> 3;
        }
        bit_stream_write_bit(uVar8);
        uVar2 = *(uint *)(param_4 + 8);
        uVar1 = uVar2 + iVar7;
        if (((-1 < iVar7) || (uVar1 <= uVar2)) &&
           (((iVar7 < 1 || (uVar2 <= uVar1)) &&
            (((uVar2 <= uVar1 && (uVar1 <= *(uint *)(param_4 + 0x14))) ||
             (uVar1 == *(int *)(param_4 + 0x14) + 1U)))))) {
          *(uint *)(param_4 + 0x10) = uVar1 & 7;
          *(uint *)(param_4 + 0xc) = uVar1 >> 3;
        }
        local_10 = local_10 + 1;
        local_8 = local_8 + 1;
        iVar9 = iVar9 + 1;
      } while (local_8 < *piVar3);
      if (0 < local_14) {
        return local_14 + *(int *)(param_1 + 0x60);
      }
    }
    uVar2 = *(uint *)(param_4 + 8);
    uVar1 = uVar2 + iVar6;
    if (((-1 < iVar6) || (uVar1 <= uVar2)) &&
       (((iVar6 < 1 || (uVar2 <= uVar1)) &&
        (((uVar2 <= uVar1 && (uVar1 <= *(uint *)(param_4 + 0x14))) ||
         (uVar1 == *(int *)(param_4 + 0x14) + 1U)))))) {
      *(uint *)(param_4 + 0xc) = uVar1 >> 3;
      *(uint *)(param_4 + 0x10) = uVar1 & 7;
      return local_14;
    }
  }
  return local_14;
}
#endif
