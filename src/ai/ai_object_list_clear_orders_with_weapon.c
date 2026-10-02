// ai_object_list_clear_orders_with_weapon  (Ghidra: ai_object_list_clear_orders_with_weapon; named for this rewrite)
// address 0x432ad0, size 173 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: walks an object_list (types/hs.h), and for each object whose controlling unit
// (unit_data.actor_index, object+0x1f4) has an actor, calls actor_delete (0x427e60, already
// established). The phase-4 summary ("clears its current order/target if it has an active
// weapon object") does not match this code at all -- there is no weapon check anywhere in
// it -- so this rewrite trusts the disassembly over that summary; named generically instead.
// register convention: confirmed by objdump (bin/halo.exe 0x432ad0..0x432b7c): ECX ->
// object_list_header (not EAX, unlike its siblings).
//   // blam-cc: ECX -> object_list_header_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;                // 0x008603b0
extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60

// blam-cc: ECX -> object_list_header_handle
void ai_object_list_clear_orders_with_weapon(datum_index object_list_header_handle)
{
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data + (object_list_header_handle & 0xffff) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        } else {
            object_index = (datum_index)k_datum_index_none;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object_header *header = &((object_header *)object_data->data)[object_index & 0xffff];
        unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);

        if (unit->actor_index != (datum_index)k_datum_index_none) {
            actor_delete(unit->actor_index, 0);
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & 0xffff) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x432ad0):

void FUN_00432ad0(void)

{
  uint uVar1;
  uint uVar2;
  uint in_ECX;

  uVar2 = 0xffffffff;
  if (in_ECX == 0xffffffff) {
    uVar1 = 0xffffffff;
  }
  else {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      uVar2 = 0xffffffff;
      uVar1 = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      uVar1 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      uVar2 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  while (uVar2 != 0xffffffff) {
    if (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 500) != -1)
    {
      actor_delete(0);
    }
    if (uVar1 == 0xffffffff) {
      uVar2 = 0xffffffff;
      uVar1 = 0xffffffff;
    }
    else {
      uVar2 = uVar1 & 0xffff;
      uVar1 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      uVar2 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
