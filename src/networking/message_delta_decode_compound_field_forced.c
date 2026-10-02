// message_delta_decode_compound_field_forced  (Ghidra: FUN_004ec600; named per this rewrite)
// address 0x4ec600, size 109 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: objdump -d -M intel bin/halo.exe @0x4ec600: `mov edi,[eax]` (state), `push ecx` /
// `push edx` / `push eax+4` / `call message_delta_read_changed_subfields`, i.e. this variant
// forwards both a changed-branch offset (EDX) and a destination offset (ECX) instead of hardcoding
// 0 for the former like message_delta_decode_compound_field does; a stack `force` byte at
// [esp+0x8] additionally lets the caller treat the field as changed even when nothing was.
// register convention: decode context in EAX, destination offset in ECX, changed-branch offset
// in EDX, force flag on the stack.
// blam-cc: EAX -> context, ECX -> destination, EDX -> changed_offset, stack -> force

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
                                                      int32_t changed_offset, int32_t destination_offset); // 0x4ed1d0, this module

// blam-cc: EAX -> context, ECX -> destination, EDX -> changed_offset, stack -> force
// Variant of message_delta_decode_compound_field that also forwards a changed-branch offset and
// can be forced to report success (bits == 0 && force) without touching the stream, matching a
// field that had nothing to decode but must still be treated as present.
uint8_t message_delta_decode_compound_field_forced(void **context, void *destination, int32_t changed_offset,
                                                    uint8_t force)
{
    message_delta_decode_state *state;
    int32_t bits;

    state = (message_delta_decode_state *)context[0];
    bits = message_delta_read_changed_subfields(state, (uint8_t *)context + 4, changed_offset,
                                                 (int32_t)(int32_t)destination);
    state->bits_read = state->bits_read + bits;
    if (bits == 0 && force == 0) {
        bit_stream *stream = (bit_stream *)state->stream;
        int32_t delta = state->start_bit_offset;
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
    return 1;
}

#if 0
Original Ghidra decompilation (0x4ec600):

undefined4 FUN_004ec600(char param_1)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  int *in_EAX;
  int iVar4;

  iVar2 = *in_EAX;
  iVar4 = message_delta_read_changed_subfields(in_EAX + 1);
  *(int *)(iVar2 + 0xc) = *(int *)(iVar2 + 0xc) + iVar4;
  if ((iVar4 == 0) && (param_1 == '\0')) {
    iVar4 = *(int *)(iVar2 + 0x14);
    iVar2 = *(int *)(iVar2 + 0x10);
    uVar3 = *(uint *)(iVar2 + 8);
    uVar1 = uVar3 + iVar4;
    if ((((-1 < iVar4) || (uVar1 <= uVar3)) && ((iVar4 < 1 || (uVar3 <= uVar1)))) &&
       (((uVar3 <= uVar1 && (uVar1 <= *(uint *)(iVar2 + 0x14))) ||
        (uVar1 == *(int *)(iVar2 + 0x14) + 1U)))) {
      *(uint *)(iVar2 + 0x10) = uVar1 & 7;
      *(uint *)(iVar2 + 0xc) = uVar1 >> 3;
    }
    return 0;
  }
  *(undefined1 *)(iVar2 + 0x1d) = 1;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
