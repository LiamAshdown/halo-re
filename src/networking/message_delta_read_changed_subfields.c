// message_delta_read_changed_subfields  (Ghidra: message_delta_read_changed_subfields, already named)
// address 0x4ed1d0, size 183 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: objdump -d -M intel bin/halo.exe @0x4ed1d0: walks definition->fields[] (the inline
// list at definition+0x28), calling each changed field's decode callback at field_type+0x54
// (the same slot message_delta_decode_static_fields uses) as
// (field_type, changed_offset==0 ? 0 : fields[i].source_offset+changed_offset,
//  fields[i].destination_offset+destination_offset, stream).
// register convention: decode_state in EDI (unaff_EDI, per types/networking.h's own note on
// this function), plus the __cdecl stack parameters changed_flags, changed_offset,
// destination_offset.
// blam-cc: EDI -> state, stack -> changed_flags, changed_offset, destination_offset

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440

typedef int32_t (*message_delta_field_decode_fn)(void *field_type, int32_t changed, int32_t offset, bit_stream *stream);

// blam-cc: EDI -> state, stack -> changed_flags, changed_offset, destination_offset
// Reads only the sub-fields flagged as changed in changed_flags[], invoking each one's decode
// callback with a changed-branch offset (changed_offset, or 0 when there is none) and a
// destination offset (destination_offset). Returns the total bits consumed, or 0 immediately if
// the stream is already out of room, or on the first field that fails to decode.
int32_t message_delta_read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
                                              int32_t changed_offset, int32_t destination_offset)
{
    bit_stream *stream;
    uint32_t position;
    int32_t total_bits;
    int32_t field_count;
    int32_t i;

    stream = state->stream;
    position = stream->bit_cursor + stream->byte_cursor * 8;
    total_bits = 0;
    if (stream->first_bit <= position && position <= stream->last_bit) {
        message_delta_definition *definition = message_delta_definitions[state->message_type];
        field_count = definition->field_count;
        for (i = 0; i < field_count; i++) {
            if (changed_flags[i] != 0) {
                message_delta_field_binding *binding = &definition->fields[i];
                int32_t src_arg = (changed_offset == 0) ? 0 : (binding->source_offset + changed_offset);
                message_delta_field_decode_fn decode =
                    *(message_delta_field_decode_fn *)((uint8_t *)binding->field_type + 0x54);
                int32_t field_bits = decode(binding->field_type, src_arg,
                                             binding->destination_offset + destination_offset, stream);
                if (field_bits < 1) {
                    return 0;
                }
                total_bits = total_bits + field_bits;
            }
        }
    }
    return total_bits;
}

#if 0
Original Ghidra decompilation (0x4ed1d0):

int message_delta_read_changed_subfields(int param_1,int param_2,int param_3)

{
  uint uVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int unaff_EDI;
  int local_c;
  int local_8;

  iVar3 = *(int *)(unaff_EDI + 0x10);
  uVar1 = *(int *)(iVar3 + 0x10) + *(int *)(iVar3 + 0xc) * 8;
  iVar6 = 0;
  local_c = 0;
  if ((*(uint *)(iVar3 + 8) <= uVar1) && (uVar1 <= *(uint *)(iVar3 + 0x14))) {
    iVar3 = *(int *)((&PTR_DAT_0065d440)[*(int *)(unaff_EDI + 4)] + 0x20);
    local_8 = 0;
    if (0 < iVar3) {
      iVar5 = 0;
      do {
        if (*(char *)(local_8 + param_1) != '\0') {
          piVar2 = (int *)((&PTR_DAT_0065d440)[*(int *)(unaff_EDI + 4)] + iVar5 + 0x28);
          iVar6 = *piVar2;
          if (param_2 == 0) {
            iVar4 = 0;
          }
          else {
            iVar4 = piVar2[2] + param_2;
          }
          iVar6 = (**(code **)(iVar6 + 0x54))
                            (iVar6,iVar4,
                             *(int *)((&PTR_DAT_0065d440)[*(int *)(unaff_EDI + 4)] + iVar5 + 0x2c) +
                             param_3,*(undefined4 *)(unaff_EDI + 0x10));
          if (iVar6 < 1) {
            return 0;
          }
          iVar6 = local_c + iVar6;
          local_c = iVar6;
        }
        local_8 = local_8 + 1;
        iVar5 = iVar5 + 0x10;
      } while (local_8 < iVar3);
    }
  }
  return iVar6;
}
#endif
