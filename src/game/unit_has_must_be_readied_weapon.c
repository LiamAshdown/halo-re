// unit_has_must_be_readied_weapon  (Ghidra: FUN_00463300; renamed -- despite its summary, the
// decompilation walks a unit's four weapon slots, not team members)
// address 0x463300, size 148 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md's summary for this address ("Checks whether any of up
// to four registered members of a player team currently satisfy a status bit") does not match
// the actual body, which indexes unit_data's own 4-entry weapon handle array (types/units.h
// k_maximum_weapons_per_unit / the 0x2f8 array) and tests each weapon tag's weapon_flags bit 3
// (must_be_readied, WeaponFlags in types/tags.h) -- the same field this batch's
// game_engine_notify_weapon_ready_state_change (0x462000) already establishes; types/game.h
// player::unit (+0x34); types/objects.h object::definition_tag; types/cache.h tag_instance.
// register convention: a player index in ECX (in_ECX).
//   // blam-cc: ECX -> player_index

// CORRECTED (phase 4 review): types/units.h unit_data starts at object + k_unit_data_offset
// (0x1f4), so a unit_data * built straight from the object pointer reads every field 0x1f4
// bytes too low. The cast below adds the extension offset.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_headers; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: ECX -> player_index
uint8_t unit_has_must_be_readied_weapon(uint32_t player_index)
{
    player *p;
    unit_data *unit;
    int32_t i;

    if (player_index == 0xffffffff) {
        return 0;
    }
    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    if (p->unit == (datum_index)0xffffffff) {
        return 0;
    }

    unit = (unit_data *)((uint8_t *)
        ((object_header *)object_headers->data)[p->unit & 0xffff].data + k_unit_data_offset);

    for (i = 0; i < 4; i++) {
        datum_index weapon = *(datum_index *)((uint8_t *)unit + 0x2f8 + i * 4);
        if (weapon != (datum_index)0xffffffff) {
            object *weapon_obj = ((object_header *)object_headers->data)[weapon & 0xffff].data;
            uint8_t *weapon_tag_data = (uint8_t *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
            if (((*(uint32_t *)(weapon_tag_data + 0x308) >> 3) & 1) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x463300), from tools/pack.py 0x463300:

undefined1 FUN_00463300(void)

{
  uint uVar1;
  undefined1 uVar2;
  uint in_ECX;
  uint *puVar3;
  int iVar4;

  uVar2 = 0;
  if ((in_ECX != 0xffffffff) &&
     (uVar1 = *(uint *)((in_ECX & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)),
     uVar1 != 0xffffffff)) {
    iVar4 = 0;
    puVar3 = (uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) + 0x2f8);
    while ((*puVar3 == 0xffffffff ||
           ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                            (*puVar3 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                               DAT_0087bc14) + 0x308) >> 3 & 1) == 0))) {
      iVar4 = iVar4 + 1;
      puVar3 = puVar3 + 1;
      if (3 < iVar4) {
        return uVar2;
      }
    }
    uVar2 = 1;
  }
  return uVar2;
}
#endif
