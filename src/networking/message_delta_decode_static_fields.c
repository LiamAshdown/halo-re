// message_delta_decode_static_fields  (Ghidra: FUN_004ed290; named per this rewrite)
// address 0x4ed290, size 117 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: objdump -d -M intel bin/halo.exe @0x4ed290: walks definition->statics->fields[],
// calling each field type's decode callback at field_type+0x54 (the same slot
// message_delta_read_changed_subfields calls) as (field_type, 0, fields[i].destination_offset + offset,
// stream), stopping and returning 0 on the first non-positive result.
// register convention: message type index in EBX (unaff_EBX), stream and offset as the two
// __cdecl stack parameters (param_1, param_2).
// blam-cc: EBX -> message_type, stack -> stream, offset

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440

typedef int32_t (*message_delta_field_decode_fn)(void *field_type, int32_t changed, int32_t offset, bit_stream *stream);

// blam-cc: EBX -> message_type, stack -> stream, offset
// Decodes every unconditional (non-optional) static field of a message type into the
// destination at `offset`, always passing "not changed" (0) to each field's decode callback.
// Returns the total bits consumed, or 0 on the first field that fails to decode.
int32_t message_delta_decode_static_fields(int32_t message_type, bit_stream *stream, int32_t offset)
{
    message_delta_definition *definition;
    message_delta_static_fields *statics;
    int32_t total_bits;
    int32_t i;
    message_delta_field_binding *binding;
    int32_t field_bits;

    definition = message_delta_definitions[message_type];
    statics = definition->statics;
    total_bits = 0;
    if (0 < statics->count) {
        for (i = 0; i < statics->count; i++) {
            message_delta_field_decode_fn decode;

            binding = &statics->fields[i];
            decode = *(message_delta_field_decode_fn *)((uint8_t *)binding->field_type + 0x54);
            field_bits = decode(binding->field_type, 0, binding->destination_offset + offset, stream);
            if (field_bits < 1) {
                return 0;
            }
            total_bits = total_bits + field_bits;
        }
    }
    return total_bits;
}

#if 0
Original Ghidra decompilation (0x4ed290):

int FUN_004ed290(undefined4 param_1,int param_2)

{
  undefined *puVar1;
  int iVar2;
  int unaff_EBX;
  int iVar3;
  int iVar4;
  int local_4;

  puVar1 = (&PTR_DAT_0065d440)[unaff_EBX];
  local_4 = 0;
  if (0 < **(int **)(puVar1 + 0x1c)) {
    iVar3 = 0;
    iVar4 = 0;
    while (iVar2 = *(int *)(*(int *)((&PTR_DAT_0065d440)[unaff_EBX] + 0x1c) + 8 + iVar3),
          iVar2 = (**(code **)(iVar2 + 0x54))
                            (iVar2,0,*(int *)(*(int *)((&PTR_DAT_0065d440)[unaff_EBX] + 0x1c) +
                                              iVar3 + 0xc) + param_2,param_1), 0 < iVar2) {
      local_4 = local_4 + iVar2;
      iVar4 = iVar4 + 1;
      iVar3 = iVar3 + 0x10;
      if (**(int **)(puVar1 + 0x1c) <= iVar4) {
        return local_4;
      }
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
