// player_weapon_locality_for_object  (Ghidra: FUN_00453a10, still unnamed there; phase-2's
//   "particle_system_..." framing in functions.md is wrong -- this walks the player table, not
//   particle systems, and is called only from the dead particle_system_resolve_local_players
//   0x454080)
// address 0x453a10, size 249 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: types/game.h player (local_player_index +0x02, unit +0x34); types/units.h unit
//   (current_weapon_index +0x2f2, weapons[4] +0x2f8); types/objects.h object_header (type +0x03,
//   size 0x0c, data +0x08), object_type_mask (_object_mask_biped | _object_mask_vehicle == 3).
// register convention: none -- the single argument (a weapon object index to search for) is a
//   Ghidra-recognized stack parameter.
// UNSURE: the combined 32-bit nonzero test on object_header+0x04 (cluster_index and block_size
//   together) is kept literally rather than split into two named field reads. UNSURE: the raw
//   disassembly (objdump -d -M intel, 0x453a10..0x453a3a) builds the on-stack data_iterator with
//   an extra `xor eax,0x69746572` folded into one of its fields before the first
//   data_iterator_next call; this function is unreachable dead code (its only caller,
//   particle_system_resolve_local_players 0x454080, is itself unused per functions.md), so the
//   iterator is modeled here the same way every other data_iterator user in this codebase
//   builds one (data = player_data, next_index = 0, index = k_datum_index_none) rather than
//   reverse-engineering that one instruction's effect.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "units.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// Walks every live player looking for one whose unit is a biped or vehicle currently holding
// `weapon_object_index` as its current weapon. Returns 1 if found and that player is a local
// player, -1 if found and it is not, or 0 if no player is holding that weapon.
int32_t player_weapon_locality_for_object(datum_index weapon_object_index)
{
    data_iterator iterator;
    player *record;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)0xffffffff;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    record = (player *)data_iterator_next(&iterator);
    while (record != (player *)0) {
        datum_index unit_index = record->unit;

        if (unit_index != (datum_index)0xffffffff) {
            int16_t index = (int16_t)unit_index;

            if (index >= 0 && index < object_data->maximum_count) {
                object_header *header = &((object_header *)object_data->data)[index];

                if (header->identifier != 0) {
                    int16_t salt = (int16_t)(unit_index >> 16);

                    if ((salt == 0 || header->identifier == salt) &&
                        ((1 << (header->type & 0x1f)) & 3) != 0 &&
                        *(int32_t *)&header->cluster_index != 0) {
                        unit_data *held_unit =
                            (unit_data *)((object_header *)object_data->data)[unit_index & 0xffff].data;
                        int16_t current_weapon = held_unit->current_weapon_index;
                        datum_index current_weapon_object = (datum_index)0xffffffff;

                        if (current_weapon != -1) {
                            current_weapon_object = held_unit->weapons[current_weapon];
                        }
                        if (weapon_object_index == current_weapon_object) {
                            return (record->local_player_index != -1) ? 1 : -1;
                        }
                    }
                }
            }
        }

        record = (player *)data_iterator_next(&iterator);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x453a10):

int FUN_00453a10(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  short *psVar5;
  int iVar6;

  iVar3 = data_iterator_next();
  iVar2 = DAT_008603b0;
  if (iVar3 == 0) {
    return 0;
  }
  do {
    iVar1 = *(int *)(iVar3 + 0x34);
    if (((iVar1 != -1) && (sVar4 = (short)iVar1, -1 < sVar4)) && (sVar4 < *(short *)(iVar2 + 0x20)))
    {
      psVar5 = (short *)((int)*(short *)(iVar2 + 0x22) * (int)sVar4 + *(int *)(iVar2 + 0x34));
      if ((((*psVar5 != 0) &&
           ((sVar4 = (short)((uint)iVar1 >> 0x10), sVar4 == 0 || (*psVar5 == sVar4)))) &&
          ((1 << (*(byte *)((int)psVar5 + 3) & 0x1f) & 3U) != 0)) && (*(int *)(psVar5 + 4) != 0)) {
        iVar1 = *(int *)(*(int *)(iVar2 + 0x34) + 8 + (*(uint *)(iVar3 + 0x34) & 0xffff) * 0xc);
        sVar4 = *(short *)(iVar1 + 0x2f2);
        iVar6 = -1;
        if (sVar4 != -1) {
          iVar6 = *(int *)(iVar1 + 0x2f8 + sVar4 * 4);
        }
        if (param_1 == iVar6) {
          return (uint)(*(short *)(iVar3 + 2) != -1) * 2 + -1;
        }
      }
    }
    iVar3 = data_iterator_next();
    if (iVar3 == 0) {
      return 0;
    }
  } while( true );
}
#endif
