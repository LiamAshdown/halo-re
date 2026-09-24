// message_delta_decode_compound_field_staged  (Ghidra: FUN_004ec670; named per this rewrite)
// address 0x4ec670, size 142 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: objdump -d -M intel bin/halo.exe @0x4ec670: allocates a 2048-byte stack scratch
// buffer and decodes into it via message_delta_read_changed_subfields, passing the scratch
// buffer itself as both the destination offset and (only when state->incremental == 1) the
// changed-branch offset -- `(incremental != 1) - 1 & scratch`, i.e. scratch when incremental==1,
// otherwise 0. Nothing in this function copies the scratch buffer back out anywhere, so its
// purpose beyond letting the subfield callbacks stage data during the decode call is unresolved.
// register convention: decode context in EAX.
// blam-cc: EAX -> context
// UNSURE: the scratch buffer is discarded when the function returns; whether callers rely on a
// side effect written through it (e.g. a field type that keeps its own pointer into it) can't be
// established from this function alone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
                                                      int32_t changed_offset, int32_t destination_offset); // 0x4ed1d0, this module

// blam-cc: EAX -> context
// Decodes a compound message-delta field through a local 2048-byte scratch buffer instead of a
// caller-supplied destination, using the buffer as the changed-branch offset too when the
// message is incremental (state->incremental == 1). Returns 1 on success, 0 on failure (with the
// usual stream rewind).
uint8_t message_delta_decode_compound_field_staged(void **context)
{
    message_delta_decode_state *state;
    uint8_t scratch[0x800];
    int32_t changed_offset;
    int32_t bits;

    state = (message_delta_decode_state *)context[0];
    changed_offset = (state->incremental == 1) ? (int32_t)(int32_t)scratch : 0;
    bits = message_delta_read_changed_subfields(state, (uint8_t *)context + 4, changed_offset,
                                                 (int32_t)(int32_t)scratch);
    if (bits == 0 && state->incremental == 0) {
        bit_stream *stream = state->stream;
        int32_t delta = state->unknown_14;
        uint32_t target = (uint32_t)stream->first_bit + (uint32_t)delta;
        if ((delta >= 0 || target <= stream->first_bit) &&
            (delta <= 0 || stream->first_bit <= target) &&
            ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
            stream->bit_cursor = target & 7;
            stream->byte_cursor = target >> 3;
        }
        return 0;
    }
    state->changed = 1;
    state->bits_read = state->bits_read + bits;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4ec670):

undefined4 FUN_004ec670(void)

{
  uint uVar1;
  int *piVar2;
  int iVar3;
  uint uVar4;
  int *in_EAX;
  int iVar5;
  undefined1 local_800 [2048];

  piVar2 = (int *)*in_EAX;
  iVar5 = message_delta_read_changed_subfields
                    (in_EAX + 1,(*piVar2 != 1) - 1 & (uint)local_800,local_800);
  if ((iVar5 == 0) && (*piVar2 == 0)) {
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
  *(undefined1 *)((int)piVar2 + 0x1d) = 1;
  piVar2[3] = piVar2[3] + iVar5;
  return 1;
}
#endif
