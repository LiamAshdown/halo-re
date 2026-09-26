// message_delta_encode_field  (Ghidra: message_delta_encode_field, already named)
// address 0x4ecde0, size 140 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary; calls a field type's encode callback at
// field_type+0x50 (the encode-side counterpart of the decode callback at +0x54 used throughout
// this subsystem's decode half) and, for a stateless (non-incremental) message, records the
// per-field changed flag by writing a single bit through bit_stream_write_bit.
// register convention: encode (sub-)context in ESI (unaff_ESI, matching
// message_delta_encode_all_fields's own context), a changed-offset value in EAX (in_EAX,
// unresolved register read, forwarded from message_delta_encode_all_fields's own such value),
// field index and type_offset as the two __cdecl stack parameters.
// UNSURE: same context-layout caveat as message_delta_encode_all_fields.c and
// message_delta_encode_prepare_item.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440
extern uint8_t message_delta_field_changed_flags[0x40];         // 0x006b89c0

typedef int32_t (*message_delta_field_encode_fn)(void *field_type, int32_t changed, int32_t offset, void *stream_or_ctx);
extern uint8_t bit_stream_write_bit(int32_t bit_value, bit_stream *stream); // UNSURE: stream argument

// blam-cc: ESI -> ctx, EAX -> changed_offset, stack -> field_index, type_offset
// Encodes one top-level message field via its type-specific callback. For an incremental
// message, reports the field's changed bit through the bit stream; for a stateless message,
// simply reports whether it encoded any bits. Accumulates the field's bit count into the
// context's running total and records its changed flag into the shared scratch array.
uint8_t message_delta_encode_field(int32_t changed_offset, uint8_t *ctx, int32_t field_index, int32_t type_offset)
{
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    message_delta_definition *definition;
    message_delta_field_binding *binding;
    int32_t src_offset;
    int32_t field_bits;
    uint8_t changed;

    definition = message_delta_definitions[CTXD(4)];
    binding = &definition->fields[field_index];
    src_offset = (changed_offset == 0) ? 0 : (binding->source_offset + changed_offset);
    {
        message_delta_field_encode_fn encode =
            *(message_delta_field_encode_fn *)((uint8_t *)binding->field_type + 0x50);
        field_bits = encode(binding->field_type, src_offset, binding->destination_offset + type_offset, ctx + 100);
    }

    changed = 0;
    if (CTXD(8) == 1) {
        if (bit_stream_write_bit(field_bits != 0, 0) != 0) { // UNSURE: stream argument
            changed = 1;
        }
    } else if (0 < field_bits) {
        changed = 1;
    }
    CTXD(0x44) = CTXD(0x44) + field_bits;
    message_delta_field_changed_flags[field_index] = (uint8_t)(field_bits != 0);
    return changed;
    #undef CTXD
}

#if 0
Original Ghidra decompilation (0x4ecde0):

undefined1 message_delta_encode_field(int param_1,int param_2)

{
  int *piVar1;
  char cVar2;
  int in_EAX;
  int iVar3;
  undefined1 uVar4;
  int unaff_ESI;

  piVar1 = (int *)((&PTR_DAT_0065d440)[*(int *)(unaff_ESI + 4)] + param_1 * 0x10 + 0x28);
  uVar4 = 0;
  if (in_EAX == 0) {
    iVar3 = 0;
  }
  else {
    iVar3 = piVar1[2] + in_EAX;
  }
  iVar3 = (**(code **)(*piVar1 + 0x50))(*piVar1,iVar3,piVar1[1] + param_2,unaff_ESI + 100);
  if (*(int *)(unaff_ESI + 8) == 1) {
    uVar4 = 0;
    cVar2 = bit_stream_write_bit(iVar3 != 0);
    if (cVar2 != '\0') {
      uVar4 = 1;
    }
  }
  else if (0 < iVar3) {
    uVar4 = 1;
  }
  *(int *)(unaff_ESI + 0x44) = *(int *)(unaff_ESI + 0x44) + iVar3;
  *(bool *)((int)&DAT_006b89c0 + param_1) = iVar3 != 0;
  return uVar4;
}
#endif
