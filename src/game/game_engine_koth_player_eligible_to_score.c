// game_engine_koth_player_eligible_to_score  (Ghidra: FUN_0046ce10; named per its summary)
// address 0x46ce10, size 143 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Determines whether a player is currently eligible to
//   score hill time, using different rules for team versus non-team hill configurations");
//   game_engine_variant::unknown_8c aliased 0x006f1d14; player::unit (0x34), player_data
//   (0x0087a480); unit_find_weapon_index_by_flag (0x570520, already committed,
//   src/units/unit_find_weapon_index_by_flag.c) blam-cc: called here with only the flag_bit
//   argument (3) visible, unit_index forwarded from a register this function's own body never
//   assigns -- modeled as the player's own controlled unit, which is the only unit handle in
//   scope here.
// register convention: object handle in param_1; player index in param_2 (both ordinary stack
//   parameters per Ghidra's own signature).
// UNSURE: the exact meaning of "unit_find_weapon_index_by_flag(unit, 3) == 0" as the
//   ineligibility test; transcribed literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern data_array *object_data; // 0x008603b0
extern data_array *player_data;    // 0x0087a480
extern game_variant game_engine_variant; // 0x006f1c88 (unknown_8c aliased 0x006f1d14)

extern void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player,
    int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast); // 0x460c10, blam-cc: BL broadcast
extern uint16_t unit_find_weapon_index_by_flag(uint32_t unit_index, uint8_t flag_bit); // 0x570520

uint8_t game_engine_koth_player_eligible_to_score(uint32_t object_handle, uint32_t player_index)
{
    object *obj = ((object_header *)object_data->data)[object_handle & 0xffff].data;

    if (game_engine_variant.unknown_8c > 0 && game_engine_variant.unknown_8c < 3) {
        game_engine_broadcast_kill_feed_by_relationship(player_index, 0x20, 0x21, 0x22, player_index, 0); // BL = 0 at 0x46ce4e
        return 1;
    }

    {
        player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
        uint8_t eligible = 1;
        if (p->unit != (datum_index)0xffffffff) {
            uint16_t found = unit_find_weapon_index_by_flag((uint32_t)p->unit, 3);
            eligible = 1 - (found != 0);
            if (eligible != 0) {
                *(uint32_t *)((uint8_t *)obj + 0x22c) |= 0x40;
            }
        }
        return eligible;
    }
}

#if 0
Original Ghidra decompilation (0x46ce10), from tools/pack.py 0x46ce10:

char FUN_0046ce10(uint param_1,uint param_2)

{
  uint *puVar1;
  int iVar2;
  char cVar3;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  cVar3 = '\x01';
  if ((0 < DAT_006f1d14) && (DAT_006f1d14 < 3)) {
    FUN_00460c10(param_2,0x20,0x21,0x22,param_2);
    return '\x01';
  }
  if (*(int *)((param_2 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34) != -1) {
    cVar3 = FUN_00570520(3);
    cVar3 = '\x01' - (cVar3 != '\0');
    if (cVar3 != '\0') {
      puVar1 = (uint *)(iVar2 + 0x22c);
      *puVar1 = *puVar1 | 0x40;
    }
  }
  return cVar3;
}
#endif
