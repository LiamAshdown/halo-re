// message_delta_decode_array_field  (Ghidra: FUN_004ec510; named per this rewrite)
// address 0x4ec510, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: objdump -d -M intel bin/halo.exe @0x4ec510: `push eax; call 0x4ed070` passes the
// decode context straight through to message_delta_decode_field_changed_flags; on a
// non-positive result with a baseline (incremental == 0) message and no static fields for this
// message type, treats zero bits as success anyway (an empty array is trivially fully decoded).
// register convention: decode context in EAX (recognized parameter).
// blam-cc: EAX -> context

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440
extern int32_t message_delta_decode_field_changed_flags(void **context); // 0x4ed070, this module

// blam-cc: EAX -> context
// Decodes one array-typed message-delta field: reads its changed-flags/static-field payload via
// message_delta_decode_field_changed_flags and, on success (or on the degenerate "nothing to
// decode" case), accumulates the bit count into state->bits_read and marks state->more_items.
// On failure, rewinds the stream to where it started.
int32_t message_delta_decode_array_field(void **context)
{
    message_delta_decode_state *state;
    int32_t array_bits;
    int32_t treat_as_success;

    state = (message_delta_decode_state *)context[0];
    array_bits = message_delta_decode_field_changed_flags(context);
    treat_as_success = (0 < array_bits);
    if (!treat_as_success && array_bits == 0 && state->incremental == 0) {
        message_delta_definition *definition = message_delta_definitions[state->message_type];
        if (definition->statics->count <= 0) {
            treat_as_success = 1;
        }
    }

    if (treat_as_success) {
        state->bits_read = state->bits_read + array_bits;
        state->more_items = 1;
        return 1;
    }

    {
        bit_stream *stream = (bit_stream *)state->stream;
        int32_t delta = state->start_bit_offset;
        uint32_t target = (uint32_t)stream->first_bit + (uint32_t)delta;
        if ((delta >= 0 || target <= stream->first_bit) &&
            (delta <= 0 || stream->first_bit <= target) &&
            ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
            stream->bit_cursor = target & 7;
            stream->byte_cursor = target >> 3;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ec510):

undefined4 FUN_004ec510(void)

{
  uint uVar1;
  int *piVar2;
  int iVar3;
  uint uVar4;
  int *in_EAX;
  int iVar5;

  piVar2 = (int *)*in_EAX;
  iVar5 = FUN_004ed070();
  if ((iVar5 < 1) &&
     (((iVar5 != 0 || (*piVar2 != 0)) || (0 < **(int **)((&PTR_DAT_0065d440)[piVar2[1]] + 0x1c)))))
  {
    iVar5 = piVar2[5];
    iVar3 = piVar2[4];
    uVar4 = *(uint *)(iVar3 + 8);
    uVar1 = uVar4 + iVar5;
    if ((((-1 < iVar5) || (uVar1 <= uVar4)) && ((iVar5 < 1 || (uVar4 <= uVar1)))) &&
       (((uVar4 <= uVar1 && (uVar1 <= *(uint *)(iVar3 + 0x14))) ||
        (uVar1 == *(int *)(iVar3 + 0x14) + 1U)))) {
      *(uint *)(iVar3 + 0x10) = uVar1 & 7;
      *(uint *)(iVar3 + 0xc) = uVar1 >> 3;
    }
    return 0;
  }
  piVar2[3] = piVar2[3] + iVar5;
  *(undefined1 *)(piVar2 + 7) = 1;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
