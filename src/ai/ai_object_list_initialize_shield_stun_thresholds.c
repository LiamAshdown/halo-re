// ai_object_list_initialize_shield_stun_thresholds  (Ghidra: FUN_00561ab0; named for this rewrite)
// address 0x561ab0, size 195 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: same object_list walk (types/hs.h, the 0x0087a464 / 0x0087a468 pair every
//   ai_object_list_* function in this directory uses) as its siblings, but unlike
//   ai_object_list_set_unit_flag_800/_800000 it applies to every object in the list, not just
//   bipeds/vehicles (there is no `(1 << type) & 3` filter here), and it skips an object whose
//   object_vitality_flags (types/objects.h) has `_object_health_frozen_bit` set before calling
//   object_initialize_shield_stun_thresholds (0x4ed440, this task's own module, `objects`) on it.
// register convention: object_list_header handle in ECX; two float* stack parameters forwarded
//   unchanged to object_initialize_shield_stun_thresholds's own ESI/EDI (override_max_body_vitality,
//   override_max_shield_vitality) at every qualifying member. Confirmed against objdump
//   0x561ab0..0x561b72: `lea edi,[esp+0x10]; lea esi,[esp+0x14]; call 0x4ed440` right after the
//   vitality-flags test, where [esp+0x10]/[esp+0x14] were freshly copied from this function's
//   own incoming stack arguments just above (`mov edx,[esp+0x18]; mov ecx,[esp+0x14]` before the
//   loop, i.e. copied once, not re-read every iteration -- but the value is the same every
//   iteration regardless).
//   // blam-cc: ECX -> object_list_header_handle, stack -> (override_max_body_vitality, override_max_shield_vitality)
// UNSURE: EAX (the current object index in the walk) is not shown as object_initialize_shield_stun_thresholds's
// FIXED 2026-09-28 (retail-independence loop): the two overrides are float VALUES on the stack, not pointers --
//   objdump 0x561b05..0x561b3c copies them into two locals and passes their addresses (ESI body, EDI
//   shield) to object_initialize_shield_stun_thresholds; the only caller, hs units_set_maximum_vitality
//   (0x47c060), pushes the evaluated real arguments.
//   first argument anywhere in Ghidra's decompile of this function (it has none); this rewrite
//   forwards it because object_initialize_shield_stun_thresholds's own established signature
//   requires EAX = object_index and it is the only live candidate register at the call site,
//   matching the "argument already live in a register, elided by Ghidra" pattern this whole
//   address range exhibits (see e.g. src/units/README.md's own notes on this).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "hs.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;                // 0x008603b0
extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

extern void object_initialize_shield_stun_thresholds(uint32_t object_index,
    float *override_max_body_vitality, float *override_max_shield_vitality); // 0x4ed440, objects module

void ai_object_list_initialize_shield_stun_thresholds(datum_index object_list_header_handle,
    float override_max_body_vitality, float override_max_shield_vitality)
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
        object_header *entry = &((object_header *)object_data->data)[(int16_t)object_index & 0xffff];
        if ((entry->data->vitality_flags & _object_health_frozen_bit) == 0) {
            object_initialize_shield_stun_thresholds((uint32_t)object_index,
                &override_max_body_vitality, &override_max_shield_vitality);
        }

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
Original Ghidra decompilation (0x561ab0):

void FUN_00561ab0(void)

{
  int iVar1;
  uint uVar2;
  uint in_ECX;
  uint local_4;

  iVar1 = DAT_0087a468;
  uVar2 = 0xffffffff;
  if (in_ECX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      uVar2 = 0xffffffff;
      local_4 = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      local_4 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      uVar2 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  while (uVar2 != 0xffffffff) {
    if ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x106) & 4
        ) == 0) {
      object_initialize_shield_stun_thresholds();
    }
    if (local_4 == 0xffffffff) {
      uVar2 = 0xffffffff;
      local_4 = 0xffffffff;
    }
    else {
      uVar2 = local_4 & 0xffff;
      local_4 = *(uint *)(*(int *)(iVar1 + 0x34) + 8 + uVar2 * 0xc);
      uVar2 = *(uint *)(*(int *)(iVar1 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
