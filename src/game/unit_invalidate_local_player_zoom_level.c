// unit_invalidate_local_player_zoom_level  (Ghidra: FUN_004726f0; renamed, no established name)
// address 0x4726f0, size 80 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/game_types_notes.md item 2: "0x4726f0 / 0x472740 ... Both touch
// `local_player_control + 0x24` [desired_zoom_level], which
// game_engine_init_player_look_state_from_object seeds from unit+0x321 (desired_zoom_level).
// They are the zoom-level accessor pair, not weapon bookkeeping."
// register convention: unit handle in EAX (Ghidra's `in_EAX`).
//   // blam-cc: EAX -> unit

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern data_array *player_data;    // 0x0087a480
extern player_control_globals *player_control_globals_ptr; // 0x006b145c

// blam-cc: EAX -> unit
// If `unit` is controlled by a local player, invalidates that local player's cached
// desired_zoom_level (sets it to -1).
void unit_invalidate_local_player_zoom_level(datum_index unit)
{
    unit_data *u = *(unit_data **)((uint8_t *)object_data->data +
        (uint32_t)(uint16_t)unit * object_data->size + 8);
    datum_index controlling_player = u->controlling_player;

    if (controlling_player != k_datum_index_none) {
        player *p = (player *)((uint8_t *)player_data->data +
            (uint32_t)(uint16_t)controlling_player * sizeof(player));

        if (p->local_player_index != -1) {
            player_control_globals_ptr->local_players[p->local_player_index].desired_zoom_level = -1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4726f0), from tools/pack.py 0x4726f0:

void FUN_004726f0(void)

{
  short sVar1;
  uint uVar2;
  uint in_EAX;

  uVar2 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x218);
  if ((uVar2 != 0xffffffff) &&
     (sVar1 = *(short *)((uVar2 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)), sVar1 != -1)
     ) {
    *(undefined2 *)(sVar1 * 0x40 + 0x34 + DAT_006b145c) = 0xffff;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
