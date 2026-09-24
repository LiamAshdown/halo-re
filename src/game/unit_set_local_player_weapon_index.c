// unit_set_local_player_weapon_index  (Ghidra: FUN_00472100; renamed, no established name)
// address 0x472100, size 83 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md ("Updates the HUD-tracked current weapon index for
// whichever local player owns the given unit"); types/units.h unit_data::controlling_player
// (object+0x218); types/game.h player::local_player_index (+0x02),
// local_player_control::desired_weapon_index (+0x20, i.e. local_players[i]+0x20).
// register convention: unit handle in EAX (Ghidra's `in_EAX`); the new weapon index is this
// function's own recognized stack parameter.
//   // blam-cc: EAX -> unit, stack -> weapon_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *object_headers; // 0x008603b0
extern data_array *player_data;    // 0x0087a480
extern player_control_globals *player_control_globals_ptr; // 0x006b145c

// blam-cc: EAX -> unit, stack -> weapon_index
// If `unit` is controlled by a local player, sets that local player's desired_weapon_index.
void unit_set_local_player_weapon_index(datum_index unit, int16_t weapon_index)
{
    unit_data *u = *(unit_data **)((uint8_t *)object_headers->data +
        (uint32_t)(uint16_t)unit * object_headers->size + 8);
    datum_index controlling_player = u->controlling_player;

    if (controlling_player != k_datum_index_none) {
        player *p = (player *)((uint8_t *)player_data->data +
            (uint32_t)(uint16_t)controlling_player * sizeof(player));

        if (p->local_player_index != -1) {
            player_control_globals_ptr->local_players[p->local_player_index].desired_weapon_index = weapon_index;
        }
    }
}

#if 0
Original Ghidra decompilation (0x472100), from tools/pack.py 0x472100:

void FUN_00472100(undefined2 param_1)

{
  short sVar1;
  uint uVar2;
  uint in_EAX;

  uVar2 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x218);
  if ((uVar2 != 0xffffffff) &&
     (sVar1 = *(short *)((uVar2 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)), sVar1 != -1)
     ) {
    *(undefined2 *)(sVar1 * 0x40 + 0x30 + DAT_006b145c) = param_1;
  }
  return;
}
#endif
