// ai_object_list_update_vitality_fractions  (Ghidra: FUN_00561cb0; named for this rewrite)
// address 0x561cb0, size 155 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: same object_list walk (types/hs.h, the 0x0087a464 / 0x0087a468 pair every
//   ai_object_list_* function in this directory uses) as its siblings, calling
//   unit_update_vitality_fractions (0x561b80, `units`, this task's own module) once per member
//   with the SAME two forwarded float arguments every time.
// register convention: object_list_header handle in EAX; two float stack parameters
//   (body_delta, shield_delta) forwarded unchanged to unit_update_vitality_fractions's own stack
//   parameters at every member. Confirmed against objdump 0x561cb0..0x561d4a: EAX is tested
//   directly at entry (`cmp eax,0xffffffff`), and `mov eax,ecx` at 0x561d02 puts the current
//   walk index (Ghidra's `iVar1`/`in_ECX`, elided from the call) into EAX immediately before
//   `push edi; push ebx; call 0x561b80` -- unit_update_vitality_fractions's own header documents
//   EAX as its object_index parameter, and no other register carries a plausible object index
//   at that call site, so this is Ghidra's usual "argument already live in a register" elision,
//   not a genuinely fixed/ignored argument.
//   // blam-cc: EAX -> object_list_header, stack -> (body_delta, shield_delta)
// UNSURE: unlike ai_object_list_set_unit_flag_800/_800000 and
//   ai_object_list_initialize_shield_stun_thresholds, this function has no per-member type or
//   flag filter at all -- it calls unit_update_vitality_fractions for literally every member of
//   the list without checking `object_header.identifier`/`type` first. Preserved as decompiled;
//   see the identical, unfiltered walk shape in the objdump-only regions the other siblings do
//   filter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "ai.h"

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

extern void unit_update_vitality_fractions(uint32_t unit_index, float body_delta, float shield_delta); // 0x561b80, units module

void ai_object_list_update_vitality_fractions(datum_index object_list_header_handle,
    float body_delta, float shield_delta)
{
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & 0xffff) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        unit_update_vitality_fractions((uint32_t)object_index, body_delta, shield_delta);

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & 0xffff) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x561cb0):

int FUN_00561cb0(undefined4 param_1,undefined4 param_2)

{
  uint in_EAX;
  uint in_ECX;
  int iVar1;
  uint uVar2;

  iVar1 = -1;
  if (in_EAX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      iVar1 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar1 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  if (iVar1 != -1) {
    do {
      unit_update_vitality_fractions(param_1,param_2);
      if (in_ECX == 0xffffffff) {
        iVar1 = -1;
      }
      else {
        uVar2 = in_ECX & 0xffff;
        in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
        iVar1 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
      }
    } while (iVar1 != -1);
    iVar1 = -1;
  }
  return iVar1;
}
#endif
