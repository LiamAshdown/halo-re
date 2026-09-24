// unit_get_local_player_weapon_index  (Ghidra: unit_get_local_player_weapon_index, already named)
// address 0x4726b0, size 60 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/game_types_notes.md item 1: "it is *not* a weapon lookup. The disassembly
// is `[unit+0x218] -> player handle`, then `mov ax, WORD [player + 2]`, i.e. it returns
// `player::local_player_index`. The correct name is roughly `unit_get_local_player_index`." Kept
// as its already-established name per the task's naming rule (not a Ghidra FUN_ name), but the
// body below returns exactly what that note describes, not a weapon index.
// register convention: unit handle in EAX (Ghidra's `in_EAX`).
//   // blam-cc: EAX -> unit

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *object_headers; // 0x008603b0
extern data_array *player_data;    // 0x0087a480

// blam-cc: EAX -> unit
// Returns the local_player_index of the player controlling `unit`, or -1 if it has no
// controlling player. See the header note: despite the name, this does not return a weapon
// index.
int32_t unit_get_local_player_weapon_index(datum_index unit)
{
    unit_data *u = *(unit_data **)((uint8_t *)object_headers->data +
        (uint32_t)(uint16_t)unit * object_headers->size + 8);
    datum_index controlling_player = u->controlling_player;

    if (controlling_player != k_datum_index_none) {
        player *p = (player *)((uint8_t *)player_data->data +
            (uint32_t)(uint16_t)controlling_player * sizeof(player));
        return p->local_player_index;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4726b0), from tools/pack.py 0x4726b0:

undefined4 unit_get_local_player_weapon_index(void)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;

  uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x218);
  if (uVar1 != 0xffffffff) {
    iVar2 = (uVar1 & 0xffff) * 0x200;
    return CONCAT22((short)((uint)iVar2 >> 0x10),
                    *(undefined2 *)(iVar2 + 2 + *(int *)(DAT_0087a480 + 0x34)));
  }
  return 0xffffffff;
}
#endif
