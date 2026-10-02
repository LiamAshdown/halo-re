// message_delta_field_layout_compute_size  (Ghidra: message_delta_field_layout_compute_size, already named)
// address 0x4ec790, size 165 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: objdump -d -M intel bin/halo.exe @0x4ec790: calls message_delta_field_bindings_lazy_init
// once with EBX = definition->statics and once with EBX = &definition->field_count, then sums
// each field type's cached bit size (field_type+0x5c, set by that same lazy-init pass) over both
// lists and writes exactly the five cached fields types/networking.h documents on
// message_delta_definition (header_and_static_bits, item_bits, header_bits, maximum_bits,
// field_bits) plus statics->size_bits and the initialized flag.
// register convention: the message type definition in ESI (unaff_ESI).
// blam-cc: ESI -> definition
// UNSURE: definition->field_count (+0x20) is added directly into header_and_static_bits as if it
// were itself a bit count, alongside the summed static-field sizes; transcribed exactly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440
extern uint8_t message_delta_item_count_bits[];                 // 0x0065d51f
extern uint8_t message_delta_parameters_enabled;                // 0x0071cfa8

extern uint8_t message_delta_field_bindings_lazy_init(message_delta_static_fields *list); // 0x4ec840, this module

// blam-cc: ESI -> definition
// Computes and caches a message type's total encoded size (header, static fields, and array
// fields) for later use during encode/decode, first lazily initializing every field type the
// definition's two field-binding lists reference.
void message_delta_field_layout_compute_size(message_delta_definition *definition)
{
    int32_t fields_bit_sum;
    int32_t statics_bit_sum;
    int32_t i;
    int32_t header_bits;
    int32_t item_count_extra_bits;
    int32_t header_and_static_bits;
    int32_t item_bits;

    message_delta_field_bindings_lazy_init(definition->statics);
    message_delta_field_bindings_lazy_init((message_delta_static_fields *)&definition->field_count);

    fields_bit_sum = 0;
    for (i = 0; i < definition->field_count; i++) {
        fields_bit_sum += *(int32_t *)((uint8_t *)definition->fields[i].field_type + 0x5c);
    }

    statics_bit_sum = 0;
    for (i = 0; i < definition->statics->count; i++) {
        statics_bit_sum += *(int32_t *)((uint8_t *)definition->statics->fields[i].field_type + 0x5c);
    }

    header_bits = 7;
    if (message_delta_parameters_enabled == 1) {
        header_bits = 10;
    }
    item_count_extra_bits = 0;
    if (1 < definition->maximum_items) {
        item_count_extra_bits = message_delta_item_count_bits[definition->maximum_items];
    }

    header_and_static_bits = definition->field_count + statics_bit_sum; // UNSURE: see header note
    item_bits = header_and_static_bits + fields_bit_sum;
    header_bits = item_count_extra_bits + header_bits;

    definition->field_bits = fields_bit_sum;
    definition->statics->size_bits = statics_bit_sum;
    definition->item_bits = item_bits;
    definition->header_and_static_bits = header_and_static_bits;
    definition->header_bits = header_bits;
    definition->maximum_bits = definition->maximum_items * item_bits + header_bits;
    definition->initialized = 1;
}

#if 0
Original Ghidra decompilation (0x4ec790):

void message_delta_field_layout_compute_size(void)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int *unaff_ESI;

  FUN_004ec840();
  FUN_004ec840();
  iVar4 = unaff_ESI[8];
  iVar7 = 0;
  if (0 < iVar4) {
    piVar3 = unaff_ESI + 10;
    do {
      iVar7 = iVar7 + *(int *)(*piVar3 + 0x5c);
      piVar3 = piVar3 + 4;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  iVar4 = *(int *)unaff_ESI[7];
  iVar8 = 0;
  if (0 < iVar4) {
    piVar3 = (int *)unaff_ESI[7] + 2;
    do {
      iVar8 = iVar8 + *(int *)(*piVar3 + 0x5c);
      piVar3 = piVar3 + 4;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  iVar4 = *(int *)((&PTR_DAT_0065d440)[*unaff_ESI] + 0x20);
  iVar2 = unaff_ESI[5];
  iVar6 = 7;
  if (DAT_0071cfa8 == '\x01') {
    iVar6 = 10;
  }
  uVar5 = 0;
  if (1 < iVar2) {
    uVar5 = (uint)(byte)(&DAT_0065d51f)[iVar2];
  }
  iVar1 = iVar4 + iVar8 + iVar7;
  unaff_ESI[9] = iVar7;
  *(int *)(unaff_ESI[7] + 4) = iVar8;
  unaff_ESI[2] = iVar1;
  unaff_ESI[1] = iVar4 + iVar8;
  unaff_ESI[3] = uVar5 + iVar6;
  unaff_ESI[4] = iVar2 * iVar1 + uVar5 + iVar6;
  *(undefined1 *)(unaff_ESI + 6) = 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
