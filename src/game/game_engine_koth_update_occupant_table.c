// game_engine_koth_update_occupant_table  (Ghidra: FUN_0046c320; named per its summary)
// address 0x46c320, size 185 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Removes an index from all slots of the hill-occupant
//   tracking table, then re-adds it under its owning team's slot if the associated object's tag
//   supports it"); types/units.h unit_data.current_weapon_index (0x2f2) / weapons[4] (0x2f8);
//   game_engine_cleanup_stray_items.c's (this batch) identical tag-data-offset-0x308-bit-3 check;
//   object + 0xb8 read as a team index here (the objects.h name_index/team_index conflict
//   PLAN.md flags, same as elsewhere in this batch's CTF/KOTH helpers).
// register convention: index (a player or object handle) in unaff_ESI.
//   // blam-cc: ESI -> index

// CORRECTED (phase 4 review): types/units.h unit_data starts at object + k_unit_data_offset
// (0x1f4), so a unit_data * built straight from the object pointer reads every field 0x1f4
// bytes too low. The cast below adds the extension offset.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_variant game_engine_variant; // 0x006f1c88 (unknown_8c aliased 0x006f1d14)
extern uint32_t king_hill_occupant_table[16]; // 0x006b120c

// blam-cc: ESI -> index
// Outside game_engine_index 1/2, clears `index` from every slot of the 16-entry hill-occupant
// table, then, if `index` is a valid player whose unit's readied weapon's tag has the same
// "protected" flag game_engine_cleanup_stray_items.c checks, re-adds it under that weapon
// object's own team-index slot (object + 0xb8).
void game_engine_koth_update_occupant_table(uint32_t index)
{
    int32_t i;
    player *p;
    datum_index unit;

    if (game_engine_variant.unknown_8c >= 1 && game_engine_variant.unknown_8c <= 2) {
        return;
    }

    for (i = 0; i < 16; i++) {
        if (king_hill_occupant_table[i] == index) {
            king_hill_occupant_table[i] = 0xffffffff;
        }
    }

    p = (player *)((uint8_t *)player_data->data + (index & 0xffff) * sizeof(player));
    unit = p->unit;
    if (unit != (datum_index)0xffffffff) {
        unit_data *unit_obj = (unit_data *)((uint8_t *)
            ((object_header *)object_data->data)[unit & 0xffff].data + k_unit_data_offset);
        int16_t slot = unit_obj->current_weapon_index;
        if (slot != -1) {
            datum_index weapon = unit_obj->weapons[slot];
            if (weapon != (datum_index)0xffffffff) {
                object *weapon_obj = ((object_header *)object_data->data)[weapon & 0xffff].data;
                uint32_t *tag_data = (uint32_t *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
                if ((*(uint32_t *)((uint8_t *)tag_data + 0x308) >> 3 & 1) != 0) {
                    int16_t team = *(int16_t *)((uint8_t *)weapon_obj + 0xb8);
                    king_hill_occupant_table[team] = index;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x46c320), from tools/pack.py 0x46c320:

void FUN_0046c320(void)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  uint *puVar4;
  uint unaff_ESI;

  iVar2 = *(int *)(DAT_0087a480 + 0x34);
  if ((DAT_006f1d14 < 1) || (2 < DAT_006f1d14)) {
    puVar4 = &DAT_006b120c;
    do {
      if (*puVar4 == unaff_ESI) {
        *puVar4 = 0xffffffff;
      }
      puVar4 = puVar4 + 1;
    } while ((int)puVar4 < 0x6b124c);
    uVar3 = *(uint *)((unaff_ESI & 0xffff) * 0x200 + iVar2 + 0x34);
    if (uVar3 != 0xffffffff) {
      iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
      sVar1 = *(short *)(iVar2 + 0x2f2);
      if (((sVar1 != -1) && (uVar3 = *(uint *)(iVar2 + 0x2f8 + sVar1 * 4), uVar3 != 0xffffffff)) &&
         (puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc),
         (*(uint *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 3 & 1) !=
         0)) {
        (&DAT_006b120c)[(short)puVar4[0x2e]] = unaff_ESI;
      }
    }
  }
  return;
}
#endif
