// message_delta_array_field_encode  (Ghidra: message_delta_array_field_encode, already named)
// address 0x4e95e0, size 512 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md ("Generic array-of-structures field encoder used by
// the message-delta system: encodes each element and writes a per-element changed flag.");
// message_delta_array_field_decode.c (0x4e9330, the counterpart, same reserved-flag-block idiom);
// types/networking.h message_delta_field_binding, whose destination_offset (+0x04) and
// source_offset (+0x08) this function adds to param_3 and param_2 exactly as
// message_delta_encode_field (0x4ec6a0), message_delta_encode_all_fields,
// message_delta_decode_static_fields and message_delta_read_changed_subfields do.
// register convention: none beyond the stack-recognized four parameters.
// REVIEW PASS 2026-09-20: retranslated against tools/pack.py 0x4e95e0 after two errors in the
// first draft.  (1) The element offsets were read one dword too high: Ghidra takes the
// destination offset from fields[i]+0x04 (piVar9[1] in the baseline branch, *piVar9 in the delta
// branch, both landing on +0x04) and the source offset from fields[i]+0x08, while the draft used
// +0x08 for the destination and +0x0c for the source.  (2) Every seek in the element loop is
// absolute from bit_stream::first_bit (Ghidra reloads param_4+8 immediately before each one); the
// draft added the relative positions to the current cursor instead.
// UNSURE (callers=0 in this batch): bit_stream_write_bit is called with one visible argument, so
// its stream operand is not proven to be param_4 here; the fields array is read as
// message_delta_array_field_list, whose entries start at +0x04 rather than the +0x08 of
// message_delta_static_fields.

// REGISTRY (2026-09-20): this is the encode half of field-type kind *9*, not of 0x4e9330
// (kind 8, whose encode half is 0x4e9130). Kind 9 is the compound-record codec: the .data
// registry (see types/networking.h) names its users game_variant, universal_variant,
// network_map and network_player, which is why its descriptor is {count, field bindings}
// rather than {count, element_size, element type}. Its decode counterpart is 0x4e97e0.
// The inherited name is therefore mechanically right but taxonomically misleading.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t bit_stream_write_bit(int32_t bit_value, bit_stream *stream); // 0x4cf9a0, memory module;
    // UNSURE: the stream operand is in a register at this call site and Ghidra drops it;
    // the signature is src/memory/bit_stream_write_bit.c's.

// Encodes an array-of-structures field. field_type->array_descriptor holds {count, fields[]}.
// When previous is NULL every element is encoded unconditionally. Otherwise the per-element
// "changed" bits live in a block of field_type->reserved_bits reserved at the head of the array
// and the element payloads follow it: for each element the encoder encodes the payload, seeks
// back to that element's flag bit, writes whether the payload produced any bits, and seeks
// forward again to the payload cursor. If nothing changed at all it rewinds to the start of the
// reserved block, so an unchanged array costs nothing.
int32_t message_delta_array_field_encode(message_delta_field_type *field_type, uint8_t *previous,
    uint8_t *destination, bit_stream *stream)
{
    message_delta_array_field_list *list;
    int32_t total_bits;
    int32_t index;
    int32_t element_bits;
    uint32_t absolute_bit;
    uint32_t first_bit;
    uint32_t target_bit;
    int32_t reserved_bits;
    int32_t block_position;  // Ghidra's iVar6: bits from first_bit to the reserved flag block
    int32_t flag_position;   // Ghidra's iVar7 in the loop: this element's flag bit
    int32_t data_position;   // Ghidra's iVar5: bits from first_bit to the payload cursor
    message_delta_field_binding *field;

    list = (message_delta_array_field_list *)field_type->array_descriptor;
    total_bits = 0;

    if (previous == 0) {
        if (0 < list->count) {
            for (index = 0; index < list->count; index = index + 1) {
                field = &list->fields[index];
                element_bits = field->field_type->encode(field->field_type, 0,
                    destination + field->destination_offset, stream);
                total_bits = total_bits + element_bits;
            }
        }
        return total_bits;
    }

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

    if (0 < list->count) {
        flag_position = block_position;
        for (index = 0; index < list->count; index = index + 1) {
            field = &list->fields[index];
            element_bits = field->field_type->encode(field->field_type,
                previous + field->source_offset, destination + field->destination_offset, stream);
            total_bits = total_bits + element_bits;

            first_bit = stream->first_bit;
            data_position = (int32_t)((stream->bit_cursor + stream->byte_cursor * 8) - first_bit);
            target_bit = (uint32_t)flag_position + first_bit;
            if ((flag_position > -1 || target_bit <= first_bit) &&
                (flag_position < 1 || first_bit <= target_bit) &&
                ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                    target_bit == stream->last_bit + 1)) {
                stream->bit_cursor = target_bit & 7;
                stream->byte_cursor = target_bit >> 3;
            }

            bit_stream_write_bit(0 < element_bits, stream);

            first_bit = stream->first_bit;
            target_bit = first_bit + (uint32_t)data_position;
            if ((data_position > -1 || target_bit <= first_bit) &&
                (data_position < 1 || first_bit <= target_bit) &&
                ((first_bit <= target_bit && target_bit <= stream->last_bit) ||
                    target_bit == stream->last_bit + 1)) {
                stream->bit_cursor = target_bit & 7;
                stream->byte_cursor = target_bit >> 3;
            }

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
Original Ghidra decompilation (0x4e95e0), from tools/pack.py 0x4e95e0:

int message_delta_array_field_encode(int param_1,int param_2,int param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  int *piVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int *piVar9;
  int local_c;

  piVar3 = *(int **)(param_1 + 0x58);
  iVar8 = 0;
  if (param_2 == 0) {
    iVar7 = 0;
    if (0 < *piVar3) {
      piVar9 = piVar3 + 1;
      do {
        iVar6 = (**(code **)(*piVar9 + 0x50))(*piVar9,0,piVar9[1] + param_3,param_4);
        iVar8 = iVar8 + iVar6;
        iVar7 = iVar7 + 1;
        piVar9 = piVar9 + 4;
      } while (iVar7 < *piVar3);
    }
  }
  else {
    iVar7 = *(int *)(param_1 + 0x60);
    uVar1 = *(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8;
    iVar6 = uVar1 - *(int *)(param_4 + 8);
    uVar2 = uVar1 + iVar7;
    if ((((-1 < iVar7) || (uVar2 <= uVar1)) && ((iVar7 < 1 || (uVar1 <= uVar2)))) &&
       (((*(uint *)(param_4 + 8) <= uVar2 && (uVar2 <= *(uint *)(param_4 + 0x14))) ||
        (uVar2 == *(int *)(param_4 + 0x14) + 1U)))) {
      *(uint *)(param_4 + 0x10) = uVar2 & 7;
      *(uint *)(param_4 + 0xc) = uVar2 >> 3;
    }
    local_c = 0;
    if (0 < *piVar3) {
      piVar9 = piVar3 + 2;
      iVar7 = iVar6;
      do {
        iVar4 = (**(code **)(piVar9[-1] + 0x50))
                          (piVar9[-1],piVar9[1] + param_2,*piVar9 + param_3,param_4);
        iVar8 = iVar8 + iVar4;
        uVar2 = *(uint *)(param_4 + 8);
        iVar5 = (*(int *)(param_4 + 0x10) + *(int *)(param_4 + 0xc) * 8) - uVar2;
        uVar1 = iVar7 + uVar2;
        if (((-1 < iVar7) || (uVar1 <= uVar2)) &&
           (((iVar7 < 1 || (uVar2 <= uVar1)) &&
            (((uVar2 <= uVar1 && (uVar1 <= *(uint *)(param_4 + 0x14))) ||
             (uVar1 == *(int *)(param_4 + 0x14) + 1U)))))) {
          *(uint *)(param_4 + 0x10) = uVar1 & 7;
          *(uint *)(param_4 + 0xc) = uVar1 >> 3;
        }
        bit_stream_write_bit(0 < iVar4);
        uVar2 = *(uint *)(param_4 + 8);
        uVar1 = uVar2 + iVar5;
        if (((-1 < iVar5) || (uVar1 <= uVar2)) &&
           (((iVar5 < 1 || (uVar2 <= uVar1)) &&
            (((uVar2 <= uVar1 && (uVar1 <= *(uint *)(param_4 + 0x14))) ||
             (uVar1 == *(int *)(param_4 + 0x14) + 1U)))))) {
          *(uint *)(param_4 + 0x10) = uVar1 & 7;
          *(uint *)(param_4 + 0xc) = uVar1 >> 3;
        }
        local_c = local_c + 1;
        piVar9 = piVar9 + 4;
        iVar7 = iVar7 + 1;
      } while (local_c < *piVar3);
      if (0 < iVar8) {
        return iVar8 + *(int *)(param_1 + 0x60);
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
      return iVar8;
    }
  }
  return iVar8;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
