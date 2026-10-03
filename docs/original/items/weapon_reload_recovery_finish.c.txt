// weapon_reload_recovery_finish  (Ghidra: FUN_004c4940; named from
// out/phase4/items_functions.md, "Runs the post-reload recovery step for a trigger, invoking a
// holder-type-specific finish routine")
// address 0x4c4940, size 116 bytes
// VERIFIED against disassembly 0x4c4940..0x4c49b4 (2026-09-30)
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump 0x4c4940..0x4c49b3; plays overheat_detonation then deletes (FIXED tag))
// evidence: types/objects.h object.network_role (0/3 dispatch matches every other
//   object_delete_unparented/object_delete_recursive pairing in this codebase).
// register convention: item index in EBX (unaff_EBX); there is no other argument (the callers push nothing;
// the old trigger_index parameter was spurious and has been removed).
// blam-cc: EBX -> item_index
// EDI for weapon_play_trigger_tag_effect = weapon tag +0x390 (overheat_detonation.tag_id), 0x4c496f.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, real scale_a,
    real scale_b); // 0x4c47d0, ECX item, EDI tag, stack (scale_a, scale_b)
extern void object_delete_unparented(datum_index object_index); // 0x4f5aa0
extern void object_delete_recursive(datum_index object_index, uint8_t recurse_siblings); // 0x4f59d0

// Finishes an overcharged trigger's recovery by consuming (deleting) the item itself, since it
// is presumably a single-use weapon/equipment charge.
void weapon_reload_recovery_finish(datum_index item_index)
{
    object *item_obj;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    // FIXED (0x4c496f): EDI = the weapon tag's overheat_detonation.tag_id (+0x390); the draft passed -1
    weapon_play_trigger_tag_effect(item_index,
        *(datum_index *)&((Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data)->overheat_detonation.tag_id, 0, 0);

    if (item_obj->network_role == 0) {
        object_delete_unparented(item_index);
    } else if (item_obj->network_role != 3) {
        return;
    }
    object_delete_recursive(item_index, 0);
}

#if 0
Original Ghidra decompilation (0x4c4940):

void FUN_004c4940(void)

{
  int iVar1;
  uint unaff_EBX;

  weapon_play_trigger_tag_effect(0,0);
  iVar1 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc) + 4);
  if (iVar1 == 0) {
    object_delete_unparented();
  }
  else if (iVar1 != 3) {
    return;
  }
  object_delete_recursive();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
