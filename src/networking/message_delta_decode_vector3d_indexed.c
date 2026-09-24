// message_delta_decode_vector3d_indexed  (Ghidra: FUN_004eb890; named per this rewrite)
// address 0x4eb890, size 423 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md ("Decodes a 3D vector either as an index into a
// precomputed table or, on failure, as raw interpolated components."); mirrors the two
// bit_stream_read_bits_chunked-times-three plus vector3d_lerp_by_mode_denominator (FUN_004eb370)
// fallback pattern in both its early-out and failure paths.
// register convention: none beyond the stack-recognized four parameters.
// UNSURE (extensive, callers=0 in this batch): the per-bit unary-index decode loop manually reads
// individual bits out of the stream's own byte buffer (bypassing bit_stream_read_bit), and the
// table's field at +0x18 (tested against 1 as a loop-stop condition) and its entries at
// +0x1c (stride 0xc, 3 floats each) are transcribed by raw offset rather than matched to a
// declared struct, since this function's own table layout does not fully agree with
// waypoint_table_quantize_initialize.c's (that one places its point array at +0x24, not +0x1c).

// REGISTRY (2026-09-20): this is the decode half of field-type kind 26, whose .data records
// are named angular_velocity and translational_velocity; its encode counterpart is 0x4eb680.
// See the registry in types/networking.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t bit_stream_read_bits_chunked(int32_t total_bit_count, uint32_t *buffer,
    bit_stream *stream); // 0x4cf950; UNSURE: total_bit_count/buffer not visible at these call
    // sites, only the stream
extern void vector3d_lerp_by_mode_denominator(void *table, real_vector3d *out_point,
    int32_t *ratios); // this module, 0x4eb370; UNSURE: arguments not visible at these call sites

// If mode is nonzero, decodes three raw chunked values into ratios and interpolates directly via
// vector3d_lerp_by_mode_denominator, returning the bits consumed. Otherwise decodes a unary-coded
// index (one bit at a time, directly from the stream's byte buffer) into table's point array
// (stride 0xc, at +0x1c) and copies that entry into destination; if the index never resolves
// (stream exhausted or the table's +0x18 field selects a single-index mode without ever seeing a
// terminating 0 bit), falls back to the same raw chunked-and-interpolated path.
int32_t message_delta_decode_vector3d_indexed(int32_t param_1, int32_t mode, real *destination,
    bit_stream *stream)
{
    uint8_t *table;
    int32_t total_bits;
    int32_t index;
    uint32_t bit_value;
    uint32_t absolute_bit;
    uint32_t bit_cursor_byte;
    int32_t got_bit;
    int32_t ratios[3];
    int32_t a, b, c;
    real *entry;

    table = *(uint8_t **)(param_1 + 0x58);
    total_bits = 0;

    if (mode != 0) {
        (void)bit_stream_read_bits_chunked(0, (uint32_t *)&a, stream);
        (void)bit_stream_read_bits_chunked(0, (uint32_t *)&b, stream);
        (void)bit_stream_read_bits_chunked(0, (uint32_t *)&c, stream);
        ratios[0] = a; ratios[1] = b; ratios[2] = c;
        vector3d_lerp_by_mode_denominator(table, (real_vector3d *)destination, ratios);
        return a + b + c; // UNSURE: matches Ghidra's iVar5+iVar8+iVar6 (the three read results)
    }

    index = -1;
    bit_value = 1;
    do {
        absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
        got_bit = 0;
        if (stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) {
            bit_cursor_byte = stream->bit_cursor;
            bit_value = (uint32_t)(uint8_t)((stream->data[stream->byte_cursor] &
                (1 << (bit_cursor_byte & 0x1f))) >> (bit_cursor_byte & 0x1f));
            absolute_bit = absolute_bit + 1;
            if ((stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) ||
                absolute_bit == stream->last_bit + 1) {
                stream->bit_cursor = absolute_bit & 7;
                stream->byte_cursor = absolute_bit >> 3;
            }
            got_bit = 1;
        }
        index = index + (int32_t)(bit_value & 0xff);
        total_bits = total_bits + got_bit;
    } while (*(int32_t *)(table + 0x18) != 1 && (uint8_t)bit_value != 0);

    if (-1 < index) {
        entry = (real *)(table + 0x1c + index * 0xc);
        destination[0] = entry[0];
        destination[1] = entry[1];
        destination[2] = entry[2];
        return total_bits;
    }

    (void)bit_stream_read_bits_chunked(0, (uint32_t *)&a, stream);
    (void)bit_stream_read_bits_chunked(0, (uint32_t *)&b, stream);
    (void)bit_stream_read_bits_chunked(0, (uint32_t *)&c, stream);
    ratios[0] = a; ratios[1] = b; ratios[2] = c;
    vector3d_lerp_by_mode_denominator(table, (real_vector3d *)destination, ratios);
    return total_bits + a + b + c;
}

#if 0
Original Ghidra decompilation (0x4eb890), from tools/pack.py 0x4eb890:

int FUN_004eb890(int param_1,int param_2,undefined4 *param_3,int param_4)

{
  uint uVar1;
  undefined4 *puVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  byte bVar7;
  int iVar8;

  iVar5 = *(int *)(param_1 + 0x58);
  param_1 = 0;
  if (param_2 != 0) {
    iVar5 = bit_stream_read_bits_chunked(param_4);
    iVar8 = bit_stream_read_bits_chunked(param_4);
    iVar6 = bit_stream_read_bits_chunked(param_4);
    FUN_004eb370();
    return iVar5 + iVar8 + iVar6;
  }
  param_2 = -1;
  uVar4 = 1;
  do {
    uVar1 = *(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8;
    iVar8 = 0;
    if ((*(uint *)(param_4 + 8) <= uVar1) && (uVar3 = *(uint *)(param_4 + 0x14), uVar1 <= uVar3)) {
      bVar7 = (byte)*(int *)(param_4 + 0x10);
      uVar4 = (int)((uint)*(byte *)(*(int *)(param_4 + 4) + *(int *)(param_4 + 0xc)) &
                   1 << (bVar7 & 0x1f)) >> (bVar7 & 0x1f);
      uVar1 = uVar1 + 1;
      if (((*(uint *)(param_4 + 8) <= uVar1) && (uVar1 <= uVar3)) || (uVar1 == uVar3 + 1)) {
        *(uint *)(param_4 + 0x10) = uVar1 & 7;
        *(uint *)(param_4 + 0xc) = uVar1 >> 3;
      }
      iVar8 = 1;
    }
    param_2 = param_2 + (uVar4 & 0xff);
    param_1 = param_1 + iVar8;
  } while ((*(int *)(iVar5 + 0x18) != 1) && ((char)uVar4 != '\0'));
  if (-1 < param_2) {
    puVar2 = (undefined4 *)(iVar5 + 0x1c + param_2 * 0xc);
    *param_3 = *puVar2;
    param_3[1] = puVar2[1];
    param_3[2] = puVar2[2];
    return param_1;
  }
  iVar5 = bit_stream_read_bits_chunked(param_4);
  iVar8 = bit_stream_read_bits_chunked(param_4);
  iVar6 = bit_stream_read_bits_chunked(param_4);
  FUN_004eb370();
  return param_1 + iVar5 + iVar8 + iVar6;
}
#endif
