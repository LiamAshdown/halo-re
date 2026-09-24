// message_delta_decode_message_header  (Ghidra: message_delta_decode_message_header, already named)
// address 0x4ece70, size 504 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: objdump -d -M intel bin/halo.exe @0x4ece70: `push esi` (state) then `mov edx,edi`
// (stream) immediately before `call bit_stream_read_bit`, confirming the "incremental" bit is
// read straight into state+0x00 (state cast to uint8_t*, since bit_stream_read_bit only writes
// one byte -- the dword was zeroed just before); `mov ecx,ebx` / `mov eax,6` /
// `push edi` / `call bit_stream_read_bits_chunked` reads the 6-bit message type into
// state->message_type; the item-count read increments the raw wire value by one into
// state->item_count, matching message_delta_encode_message_header's own item-count bias.
// register convention: stream as the recognized parameter (param_1), decode_state output
// pinned in ESI (unaff_ESI) by the sole caller, message_delta_decode_begin (0x4ec490), across
// the call -- exposed here as an explicit second parameter.
// blam-cc: stack -> stream, ESI -> state
// UNSURE: state+0x20, +0x24 and +0x28 (a per-message item-count bit width, a header-variant tag,
// and a constant 6/3) and state+0x2c (always set to 1) are all written past the end of the
// documented 0x20-byte message_delta_decode_state -- the object message_delta_decode_begin's
// own caller (outside this module) hands in is evidently larger than what
// message_delta_read_changed_subfields alone pins. Accessed here via raw byte offsets rather
// than invented struct fields.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440
extern uint8_t message_delta_item_count_bits[];                 // 0x0065d51f
extern uint8_t message_delta_parameters_enabled;                // 0x0071cfa8
extern int32_t message_delta_parameters_protocol_sequence;      // 0x0071cfac

extern uint32_t bit_stream_read_bit(uint8_t *out_bit, bit_stream *stream);
extern int32_t bit_stream_read_bits_chunked(int32_t total_bit_count, uint32_t *buffer, bit_stream *stream);

// blam-cc: stack -> stream, ESI -> state
// Decodes the header of a message-delta message: the "incremental" bit, the 6-bit message type,
// under protocol v2 a parameters-in-progress bit and a 2-bit rolling sequence number, and
// (whenever the message type allows more than one item) a variable-width item count biased by
// one. Returns the total header bit count on success, 0 on any decode or range failure.
int32_t message_delta_decode_message_header(bit_stream *stream, message_delta_decode_state *state)
{
    uint32_t position;
    int32_t header_bits;
    uint8_t incremental_read_ok;
    uint8_t message_type_read_ok;
    uint8_t sequence_bit;
    uint8_t item_count_ok;
    int32_t sequence_value;
    uint8_t *raw_state = (uint8_t *)state; // for the fields past the documented struct

    header_bits = 7;
    if (message_delta_parameters_enabled == 1) {
        header_bits = 10;
    }
    position = stream->bit_cursor + stream->byte_cursor * 8 + 3 + header_bits;
    if (position < stream->first_bit || stream->last_bit < position) {
        return 0;
    }

    state->incremental = 0;
    state->message_type = 0;
    state->item_count = 0;
    sequence_bit = 0;
    sequence_value = 0;

    incremental_read_ok = (uint8_t)bit_stream_read_bit((uint8_t *)state, stream);
    *(int32_t *)(raw_state + 0x2c) = 1; // UNSURE: past the documented struct
    message_type_read_ok = incremental_read_ok != 0 && state->incremental >= 0 && state->incremental < 2;

    message_type_read_ok = (uint8_t)(bit_stream_read_bits_chunked(6, (uint32_t *)&state->message_type, stream) != 0) && message_type_read_ok;
    header_bits = 7;
    *(int32_t *)(raw_state + 0x28) = 6; // UNSURE: past the documented struct

    {
        uint8_t range_ok = state->message_type >= 0 && state->message_type <= 0x37 && message_type_read_ok;

        if (message_delta_parameters_enabled == 1) {
            incremental_read_ok = (uint8_t)bit_stream_read_bit(&sequence_bit, stream);
            message_type_read_ok = incremental_read_ok != 0 && range_ok;
            incremental_read_ok = (uint8_t)(bit_stream_read_bits_chunked(2, (uint32_t *)&sequence_value, stream) != 0);
            range_ok = incremental_read_ok != 0 && message_type_read_ok;
            header_bits = 10;
            *(int32_t *)(raw_state + 0x24) = 3; // UNSURE: past the documented struct
        } else {
            *(int32_t *)(raw_state + 0x24) = 0; // UNSURE: past the documented struct
        }

        if (range_ok) {
            message_delta_definition *definition = message_delta_definitions[state->message_type];
            int32_t maximum_items = definition->maximum_items;
            if (maximum_items < 2) {
                state->item_count = 1;
                *(int32_t *)(raw_state + 0x20) = 0; // UNSURE: past the documented struct
            } else {
                int32_t item_bits = message_delta_item_count_bits[maximum_items];
                item_count_ok = (uint8_t)(bit_stream_read_bits_chunked(item_bits, (uint32_t *)&state->item_count, stream) != 0);
                header_bits += item_bits;
                state->item_count += 1;
                *(int32_t *)(raw_state + 0x20) = item_bits; // UNSURE: past the documented struct
                range_ok = state->item_count >= 1 && state->item_count <= maximum_items && item_count_ok;
            }
        }

        if ((message_delta_parameters_enabled != 1 || sequence_value == message_delta_parameters_protocol_sequence ||
             sequence_bit != 0) && range_ok) {
            return header_bits;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ece70):

int message_delta_decode_message_header(int param_1)

{
  int *piVar1;
  int *piVar2;
  uint uVar3;
  byte bVar4;
  bool bVar5;
  bool bVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int *unaff_ESI;
  char local_9;
  int local_8;
  int local_4;

  iVar7 = 7;
  if (DAT_0071cfa8 == '\x01') {
    iVar7 = 10;
  }
  uVar3 = *(int *)(param_1 + 0x10) + *(int *)(param_1 + 0xc) * 8 + 3 + iVar7;
  if ((*(uint *)(param_1 + 8) <= uVar3) && (uVar3 <= *(uint *)(param_1 + 0x14))) {
    piVar1 = unaff_ESI + 1;
    piVar2 = unaff_ESI + 2;
    *unaff_ESI = 0;
    *piVar1 = 0;
    *piVar2 = 0;
    local_9 = '\0';
    local_4 = 0;
    iVar7 = bit_stream_read_bit();
    unaff_ESI[0xb] = 1;
    if ((*unaff_ESI < 0) || ((1 < *unaff_ESI || (bVar5 = true, iVar7 == 0)))) {
      bVar5 = false;
    }
    iVar7 = FUN_004cf950(param_1);
    if ((iVar7 == 0) || (!bVar5)) {
      bVar5 = false;
    }
    else {
      bVar5 = true;
    }
    local_8 = 7;
    unaff_ESI[10] = 6;
    if (((*piVar1 < 0) || (0x37 < *piVar1)) || (bVar6 = true, !bVar5)) {
      bVar6 = false;
    }
    if (DAT_0071cfa8 == '\x01') {
      iVar7 = bit_stream_read_bit(&local_9);
      if ((iVar7 == 0) || (bVar5 = true, !bVar6)) {
        bVar5 = false;
      }
      iVar7 = FUN_004cf950(param_1);
      if ((iVar7 == 0) || (bVar6 = true, !bVar5)) {
        bVar6 = false;
      }
      local_8 = 10;
      unaff_ESI[9] = 3;
    }
    else {
      unaff_ESI[9] = 0;
    }
    if (bVar6) {
      iVar7 = *(int *)((&PTR_DAT_0065d440)[*piVar1] + 0x14);
      if (iVar7 < 2) {
        *piVar2 = 1;
        unaff_ESI[8] = 0;
      }
      else {
        bVar4 = (&DAT_0065d51f)[iVar7];
        iVar8 = FUN_004cf950(param_1);
        local_8 = local_8 + (uint)bVar4;
        iVar9 = *piVar2 + 1;
        *piVar2 = iVar9;
        unaff_ESI[8] = (uint)bVar4;
        if (((iVar9 < 1) || (iVar7 < iVar9)) || (iVar8 == 0)) {
          bVar6 = false;
        }
        else {
          bVar6 = true;
        }
      }
    }
    if ((((DAT_0071cfa8 != '\x01') || (local_4 == DAT_0071cfac)) || (local_9 != '\0')) && (bVar6)) {
      return local_8;
    }
  }
  return 0;
}
#endif
