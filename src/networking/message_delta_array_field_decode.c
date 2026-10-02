// message_delta_array_field_decode  (Ghidra: message_delta_array_field_decode, already named)
// address 0x4e9330, size 496 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md ("Generic array-of-structures field decoder used by
// the message-delta system: for each element either copies the unchanged previous value or
// invokes the element type's decode function."); types/memory.h bit_stream (param_4's fields at
// +0x08 first_bit, +0x0c byte_cursor, +0x10 bit_cursor, +0x14 last_bit match exactly);
// types/networking.h message_delta_field_type, whose +0x58 array_descriptor and +0x60
// reserved_bits this function reads and whose +0x54 decode slot it calls per element.
// register convention: none beyond the stack-recognized four parameters.
// REVIEW PASS 2026-09-20: the first draft of this file was wrong in three ways and has been
// retranslated against tools/pack.py 0x4e9330.  (1) Every seek inside the element loop is an
// absolute seek to bit_stream::first_bit + a relative position (Ghidra reloads param_4+8 into
// uVar3 before each one); the draft added those relative positions to the *current* cursor
// instead, so the flag-bit region and the payload region both walked forward.  (2) The running
// flag-bit position starts at (cursor - first_bit) as measured before the reserved block is
// skipped (Ghidra's iVar5), not at the reserved bit count itself.  (3) The guard on the initial
// skip had both of its first two clauses inverted (`advance < 0 || ...` and `advance >= 1 && ...`
// instead of `-1 < advance || ...` and `advance < 1 || ...`).
// UNSURE (callers=0 in this batch, so nothing cross-checks the argument lists): the
// bit_stream argument of bit_stream_read_bit is not visible at this call site and is assumed to
// be param_4; the descriptor's element decode function is called through field_type+0x54 with the
// four arguments Ghidra shows.

// REGISTRY (2026-09-20): this is the decode half of field-type kind 8, whose encode half is
// 0x4e9130. The .data registry (see types/networking.h) names its users: ctf_score_array,
// king_score_array, oddball_score_array, oddball_owner_array, race_score_array,
// slayer_score_array, network_game_players, object_change_colors, game_engine_variant and
// parameters_protocol_array. The "array of structures" reading of the name is confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t bit_stream_read_bit(uint8_t *out_bit, bit_stream *stream); // 0x4cfb80, memory module

// Decodes an array-of-structures field. field_type->array_descriptor holds {count, element_size,
// element field type}. When previous is NULL every element is decoded unconditionally and the
// total bit count is returned. Otherwise the per-element "changed" bits live in a block of
// field_type->reserved_bits reserved at the head of the array and the element payloads follow it,
// so the loop alternates between the two regions with absolute seeks: for each element it seeks
// to the element's flag bit, reads it, seeks back to the payload cursor, and then either copies
// the previous element verbatim or calls the element type's decode. Returns the bits the changed
// elements consumed, plus field_type->reserved_bits when anything changed at all.
int32_t message_delta_array_field_decode(message_delta_field_type *field_type, uint8_t *previous,
    uint8_t *destination, bit_stream *stream)
{
    message_delta_array_descriptor *descriptor;
    int32_t total_bits;
    int32_t index;
    int32_t element_bits;
    uint8_t *src;
    uint8_t *dst;
    uint32_t absolute_bit;
    uint32_t first_bit;
    uint32_t target_bit;
    int32_t reserved_bits;
    int32_t flag_position;   // Ghidra's iVar5/local_4: bits from first_bit to this element's flag
    int32_t data_position;   // Ghidra's iVar6 inside the loop: bits from first_bit to the payload
    uint8_t changed_bit;
    int32_t changed_total;
    uint32_t stride_bytes;
    uint32_t stride_words;
    uint32_t stride_tail;

    descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;
    total_bits = 0;
    changed_total = 0;

    if (previous == 0) {
        if (0 < descriptor->count) {
            for (index = 0; index < descriptor->count; index = index + 1) {
                element_bits = descriptor->field_type->decode(descriptor->field_type, 0,
                    destination + descriptor->element_size * index, stream);
                total_bits = total_bits + element_bits;
            }
        }
        return total_bits;
    }

    absolute_bit = stream->byte_cursor * 8 + stream->bit_cursor;
    reserved_bits = field_type->reserved_bits;
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
        return 0;
    }
    for (index = 0; index < descriptor->count; index = index + 1) {
        src = previous + descriptor->element_size * index;
        dst = destination + descriptor->element_size * index;

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
            stride_bytes = (uint32_t)descriptor->element_size;
            for (stride_words = stride_bytes >> 2; stride_words != 0; stride_words = stride_words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src = src + 4;
                dst = dst + 4;
            }
            for (stride_tail = stride_bytes & 3; stride_tail != 0; stride_tail = stride_tail - 1) {
                *dst = *src;
                src = src + 1;
                dst = dst + 1;
            }
        } else {
            element_bits = descriptor->field_type->decode(descriptor->field_type, src, dst, stream);
            changed_total = changed_total + element_bits;
        }

        flag_position = flag_position + 1;
    }
    if (0 < changed_total) {
        return changed_total + field_type->reserved_bits;
    }
    return changed_total;
}

#if 0
Original Ghidra decompilation (0x4e9330), from tools/pack.py 0x4e9330:

int message_delta_array_field_decode(int param_1,int param_2,int param_3,int param_4)

{
  int *piVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  char local_15;
  int local_14;
  int local_10;
  undefined4 *local_c;
  undefined4 *local_8;
  int local_4;

  piVar1 = *(int **)(param_1 + 0x58);
  iVar6 = 0;
  local_14 = 0;
  if (param_2 == 0) {
    iVar5 = 0;
    if (0 < *piVar1) {
      do {
        iVar2 = (**(code **)(piVar1[2] + 0x54))(piVar1[2],0,piVar1[1] * iVar5 + param_3,param_4);
        iVar6 = iVar6 + iVar2;
        iVar5 = iVar5 + 1;
      } while (iVar5 < *piVar1);
    }
    return iVar6;
  }
  uVar4 = *(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8;
  iVar6 = *(int *)(param_1 + 0x60);
  iVar5 = uVar4 - *(uint *)(param_4 + 8);
  uVar3 = uVar4 + iVar6;
  if ((((-1 < iVar6) || (uVar3 <= uVar4)) && ((iVar6 < 1 || (uVar4 <= uVar3)))) &&
     (((*(uint *)(param_4 + 8) <= uVar3 && (uVar3 <= *(uint *)(param_4 + 0x14))) ||
      (uVar3 == *(int *)(param_4 + 0x14) + 1U)))) {
    *(uint *)(param_4 + 0x10) = uVar3 & 7;
    *(uint *)(param_4 + 0xc) = uVar3 >> 3;
  }
  local_10 = 0;
  if (*piVar1 < 1) {
    local_14 = 0;
  }
  else {
    do {
      local_8 = (undefined4 *)(piVar1[1] * local_10 + param_2);
      local_c = (undefined4 *)(piVar1[1] * local_10 + param_3);
      uVar3 = *(uint *)(param_4 + 8);
      iVar6 = (*(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8) - uVar3;
      local_15 = '\0';
      uVar4 = iVar5 + uVar3;
      if (((-1 < iVar5) || (uVar4 <= uVar3)) &&
         (((iVar5 < 1 || (uVar3 <= uVar4)) &&
          (((uVar3 <= uVar4 && (uVar4 <= *(uint *)(param_4 + 0x14))) ||
           (uVar4 == *(int *)(param_4 + 0x14) + 1U)))))) {
        *(uint *)(param_4 + 0x10) = uVar4 & 7;
        *(uint *)(param_4 + 0xc) = uVar4 >> 3;
      }
      local_4 = iVar5;
      bit_stream_read_bit(&local_15);
      uVar3 = *(uint *)(param_4 + 8);
      uVar4 = uVar3 + iVar6;
      if (((-1 < iVar6) || (uVar4 <= uVar3)) &&
         (((iVar6 < 1 || (uVar3 <= uVar4)) &&
          (((uVar3 <= uVar4 && (uVar4 <= *(uint *)(param_4 + 0x14))) ||
           (uVar4 == *(int *)(param_4 + 0x14) + 1U)))))) {
        *(uint *)(param_4 + 0x10) = uVar4 & 7;
        *(uint *)(param_4 + 0xc) = uVar4 >> 3;
      }
      if (local_15 == '\0') {
        uVar4 = piVar1[1];
        for (uVar3 = uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
          *local_c = *local_8;
          local_8 = local_8 + 1;
          local_c = local_c + 1;
        }
        for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
          *(undefined1 *)local_c = *(undefined1 *)local_8;
          local_8 = (undefined4 *)((int)local_8 + 1);
          local_c = (undefined4 *)((int)local_c + 1);
        }
      }
      else {
        iVar6 = (**(code **)(piVar1[2] + 0x54))(piVar1[2],local_8,local_c,param_4);
        local_14 = local_14 + iVar6;
        local_4 = iVar5;
      }
      local_10 = local_10 + 1;
      iVar5 = local_4 + 1;
    } while (local_10 < *piVar1);
    if (0 < local_14) {
      return local_14 + *(int *)(param_1 + 0x60);
    }
  }
  return local_14;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
