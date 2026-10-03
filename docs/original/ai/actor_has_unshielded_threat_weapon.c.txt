// actor_has_unshielded_threat_weapon  (Ghidra: actor_has_unshielded_threat_weapon, renamed)
// address 0x428370, size 84 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/ai.h actor.unit_index(0x18); types/objects.h object_header; calls
//   actor_get_threat_weapon_object_index (0x4282c0, already rewritten in this module).
//   Phase-4 summary: "Reports whether a weapon threat exists via actor_get_threat_weapon_object_index, suppressing
//   the result when the associated unit carries a particular status flag (byte+0x107 bit
//   0)." Object+0x107 is the high byte of object.vitality_flags (0x106); bit 0 of that byte
//   is object_vitality_flags bit 0x100, "_object_region_response_100_bit" per types/objects.h.
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index); // 0x4282c0

// blam-cc: EAX -> actor_index
uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t has_weapon = actor_get_threat_weapon_object_index(actor_index) != (datum_index)k_datum_index_none;

    if (has_weapon && self->unit_index != (datum_index)k_datum_index_none) {
        object *unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
        if ((*((uint8_t *)unit_object + 0x107) & 1) != 0) {
            has_weapon = 0;
        }
    }
    return has_weapon;
}

#if 0
Original Ghidra decompilation (0x428370):

bool FUN_00428370(void)

{
  int iVar1;
  uint uVar2;
  uint in_EAX;
  int iVar3;
  bool bVar4;

  iVar1 = *(int *)(DAT_00880360 + 0x34);
  iVar3 = actor_get_threat_weapon_object_index();
  bVar4 = iVar3 != -1;
  if (((bVar4) && (uVar2 = *(uint *)((in_EAX & 0xffff) * 0x724 + iVar1 + 0x18), uVar2 != 0xffffffff)
      ) && ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x107)
            & 1) != 0)) {
    bVar4 = false;
  }
  return bVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
