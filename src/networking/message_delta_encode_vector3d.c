// message_delta_encode_vector3d  (Ghidra: message_delta_encode_vector3d, already named)
// address 0x4eabe0, size 744 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md ("Delta-encodes a 3D vector as either a small
// quantized offset from a previous value or a full quantized absolute position."); types/memory.h
// bit_stream.
// register convention: none beyond the stack-recognized four parameters (param_1 is unused).
// UNSURE (extensive, callers=0 in this batch): the explicit single-bit set/clear sequences (an
// inlined "write one flag bit at the current cursor, then manually advance the cursor" idiom,
// distinct from the ordinary bit_stream_write_bit call used elsewhere in this same function) are
// transcribed literally against bit_stream::data; the various DAT_ globals (thresholds, bit
// widths, an absolute-vs-delta mode selector) are named generically and not independently
// re-derived.

#include "tags.h"
#include "memory.h"
#include "math.h"

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern real message_delta_vector3d_delta_epsilon; // 0x0069a2d8
extern real message_delta_vector3d_delta_range; // 0x0069a2d4
extern uint8_t message_delta_vector3d_mode; // 0x0069b350, 0 or 1; UNSURE meaning
extern uint32_t message_delta_vector3d_delta_bits; // 0x0069a2d0
extern uint32_t message_delta_vector3d_absolute_bits_mode0; // 0x0069a2dc
extern uint32_t message_delta_vector3d_absolute_bits_mode1; // 0x0069a2cc

extern double FUN_00623e40(double value); // 0x623e40, CRT helper; UNSURE: exact effect
extern uint8_t bit_stream_write_bit(int32_t bit_value, bit_stream *stream); // 0x4cf9a0,
    // memory module; UNSURE: the stream operand is in a register at this call site
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values

// Encodes a 3D position (values) either as a small quantized delta from previous (if the distance
// between them is within range and each axis's delta fits the configured delta bit width), or as
// a full quantized absolute position otherwise. previous may be NULL to force the absolute path.
int32_t message_delta_encode_vector3d(int32_t unused, real *previous, real *values,
    bit_stream *stream)
{
    real delta[3];
    uint8_t sign[3];
    int32_t total_bits;
    int32_t i;
    uint8_t out_of_range;
    uint32_t absolute_bit;
    uint32_t level_count;
    real level_count_as_float;
    double scaled;
    uint32_t quantized;
    int32_t written_bits;
    uint32_t absolute_bits;

    total_bits = 0;
    if (previous != 0) {
        delta[0] = values[0] - previous[0];
        delta[1] = values[1] - previous[1];
        delta[2] = values[2] - previous[2];
        if (sqrt(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]) <=
            message_delta_vector3d_delta_epsilon) {
            return 0;
        }
        out_of_range = 0;
        for (i = 0; i < 3; i = i + 1) {
            sign[i] = 0;
            if (delta[i] < 0.0f) {
                sign[i] = 1;
                delta[i] = -delta[i];
            }
            if (message_delta_vector3d_delta_range < delta[i]) {
                out_of_range = 1;
            }
        }
        if (!out_of_range && message_delta_vector3d_mode == 1) {
            // Explicit "clear the mode bit" write: flips bit_cursor's bit in stream->data to 0,
            // then manually advances the cursor by one bit.
            absolute_bit = stream->byte_cursor * 8 + stream->bit_cursor;
            total_bits = 0;
            if (stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) {
                stream->data[stream->byte_cursor] &= ~(1 << (stream->bit_cursor & 0x1f));
                absolute_bit = stream->bit_cursor + 1 + stream->byte_cursor * 8;
                if ((stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) ||
                    absolute_bit == stream->last_bit + 1) {
                    stream->bit_cursor = absolute_bit & 7;
                    stream->byte_cursor = absolute_bit >> 3;
                }
                total_bits = 1;
            }
            for (i = 0; i < 3; i = i + 1) {
                total_bits = total_bits + (bit_stream_write_bit(sign[i], stream) != 0);
                level_count = (1u << (message_delta_vector3d_delta_bits & 0x1f)) - 1;
                level_count_as_float = (real)(int32_t)level_count;
                if ((int32_t)level_count < 0) {
                    level_count_as_float = level_count_as_float + 4.2949673e+09f;
                }
                scaled = FUN_00623e40((double)(level_count_as_float *
                    (delta[i] / message_delta_vector3d_delta_range) + 0.5));
                quantized = (uint32_t)(int32_t)scaled;
                if (level_count < quantized) {
                    quantized = level_count;
                }
                written_bits = bit_stream_write_bits_chunked(stream, &quantized,
                    (int32_t)message_delta_vector3d_delta_bits); // 0x4ead86: ECX = &the quantized value
                total_bits = total_bits + written_bits;
            }
            return total_bits;
        }

        absolute_bit = stream->bit_cursor + stream->byte_cursor * 8;
        total_bits = 0;
        if (stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) {
            stream->data[stream->byte_cursor] |= (uint8_t)(1 << (stream->bit_cursor & 0x1f));
            absolute_bit = stream->bit_cursor + 1 + stream->byte_cursor * 8;
            if ((stream->first_bit <= absolute_bit && absolute_bit <= stream->last_bit) ||
                absolute_bit == stream->last_bit + 1) {
                stream->bit_cursor = absolute_bit & 7;
                stream->byte_cursor = absolute_bit >> 3;
            }
            total_bits = 1;
        }
    }

    absolute_bits = message_delta_vector3d_mode == 0 ? message_delta_vector3d_absolute_bits_mode0
                                                       : message_delta_vector3d_absolute_bits_mode1;
    level_count = (1u << (absolute_bits & 0x1f)) - 1;
    level_count_as_float = (real)(int32_t)level_count;
    if ((int32_t)level_count < 0) {
        level_count_as_float = level_count_as_float + 4.2949673e+09f;
    }
    for (i = 0; i < 3; i = i + 1) {
        scaled = FUN_00623e40((double)(level_count_as_float * (values[i] - -5000.0) * 0.0001 + 0.5));
        quantized = (uint32_t)(int32_t)scaled;
        if (level_count < quantized) {
            quantized = level_count;
        }
        written_bits = bit_stream_write_bits_chunked(stream, &quantized, (int32_t)absolute_bits); // 0x4eae98
        total_bits = total_bits + written_bits;
    }
    return total_bits;
}

#if 0
Original Ghidra decompilation (0x4eabe0), from tools/pack.py 0x4eabe0:

int message_delta_encode_vector3d(undefined4 param_1,float *param_2,float *param_3,int param_4)

{
  float fVar1;
  bool bVar2;
  char cVar3;
  uint uVar4;
  int iVar5;
  byte *pbVar6;
  int iVar7;
  uint uVar8;
  undefined4 local_24;
  int local_20;
  float local_1c;
  uint local_18;
  float local_14 [4];

  iVar7 = 0;
  local_20 = 0;
  if (param_2 != (float *)0x0) {
    local_14[0] = *param_3 - *param_2;
    local_14[1] = param_3[1] - param_2[1];
    local_14[2] = param_3[2] - param_2[2];
    if (SQRT(local_14[0] * local_14[0] + local_14[1] * local_14[1] + local_14[2] * local_14[2]) <=
        _DAT_0069a2d8) {
      return 0;
    }
    bVar2 = false;
    do {
      fVar1 = local_14[iVar7];
      *(undefined1 *)((int)&local_24 + iVar7) = 0;
      if (fVar1 < 0.0) {
        fVar1 = local_14[iVar7];
        *(undefined1 *)((int)&local_24 + iVar7) = 1;
        local_14[iVar7] = -fVar1;
      }
      if (_DAT_0069a2d4 < local_14[iVar7]) {
        bVar2 = true;
      }
      iVar7 = iVar7 + 1;
    } while (iVar7 < 3);
    if ((!bVar2) && (DAT_0069b350 == 1)) {
      iVar7 = *(int *)(param_4 + 0xc);
      uVar8 = *(int *)(param_4 + 0x10) + iVar7 * 8;
      local_20 = 0;
      if ((*(uint *)(param_4 + 8) <= uVar8) && (uVar8 <= *(uint *)(param_4 + 0x14))) {
        *(byte *)(iVar7 + *(int *)(param_4 + 4)) =
             *(byte *)(iVar7 + *(int *)(param_4 + 4)) &
             ~('\x01' << ((byte)*(int *)(param_4 + 0x10) & 0x1f));
        uVar8 = *(int *)(param_4 + 0x10) + 1 + *(int *)(param_4 + 0xc) * 8;
        if (((*(uint *)(param_4 + 8) <= uVar8) && (uVar8 <= *(uint *)(param_4 + 0x14))) ||
           (uVar8 == *(int *)(param_4 + 0x14) + 1U)) {
          *(uint *)(param_4 + 0x10) = uVar8 & 7;
          *(uint *)(param_4 + 0xc) = uVar8 >> 3;
        }
        local_20 = 1;
      }
      iVar7 = 0;
      do {
        cVar3 = bit_stream_write_bit();
        local_20 = local_20 + (uint)(cVar3 != '\0');
        uVar8 = (1 << ((byte)DAT_0069a2d0 & 0x1f)) - 1;
        fVar1 = (float)(int)uVar8;
        if ((int)uVar8 < 0) {
          fVar1 = fVar1 + 4.2949673e+09;
        }
        local_18 = uVar8;
        FUN_00623e40((double)(fVar1 * (local_14[iVar7] / _DAT_0069a2d4) + 0.5));
        uVar4 = __ftol();
        if (uVar8 < uVar4) {
          uVar4 = uVar8;
        }
        local_1c = (float)(-(uint)(uVar4 != 0) & uVar4);
        iVar5 = bit_stream_write_bits_chunked();
        local_20 = local_20 + iVar5;
        iVar7 = iVar7 + 1;
      } while (iVar7 < 3);
      return local_20;
    }
    uVar8 = *(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8;
    local_20 = 0;
    if ((*(uint *)(param_4 + 8) <= uVar8) && (uVar8 <= *(uint *)(param_4 + 0x14))) {
      pbVar6 = (byte *)(*(int *)(param_4 + 0xc) + *(int *)(param_4 + 4));
      *pbVar6 = *pbVar6 | '\x01' << ((byte)*(int *)(param_4 + 0x10) & 0x1f);
      uVar8 = *(int *)(param_4 + 0x10) + 1 + *(int *)(param_4 + 0xc) * 8;
      if (((*(uint *)(param_4 + 8) <= uVar8) && (uVar8 <= *(uint *)(param_4 + 0x14))) ||
         (uVar8 == *(int *)(param_4 + 0x14) + 1U)) {
        *(uint *)(param_4 + 0x10) = uVar8 & 7;
        *(uint *)(param_4 + 0xc) = uVar8 >> 3;
      }
      local_20 = 1;
    }
  }
  if (DAT_0069b350 == 0) {
    local_24 = DAT_0069a2dc;
  }
  else {
    local_24 = DAT_0069a2cc;
  }
  iVar7 = 0;
  uVar8 = (1 << ((byte)local_24 & 0x1f)) - 1;
  local_1c = (float)(int)uVar8;
  local_18 = uVar8;
  if ((int)uVar8 < 0) {
    local_1c = local_1c + 4.2949673e+09;
  }
  do {
    FUN_00623e40((double)(local_1c * (param_3[iVar7] - -5000.0) * 0.0001 + 0.5));
    local_18 = __ftol();
    if (uVar8 < local_18) {
      local_18 = uVar8;
    }
    local_18 = -(uint)(local_18 != 0) & local_18;
    iVar5 = bit_stream_write_bits_chunked();
    local_20 = local_20 + iVar5;
    iVar7 = iVar7 + 1;
  } while (iVar7 < 3);
  return local_20;
}
#endif
