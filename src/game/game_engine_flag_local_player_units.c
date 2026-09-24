// game_engine_flag_local_player_units  (Ghidra: FUN_0045b590; renamed per
// symbols/review_queue.txt)
// address 0x45b590, size 225 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: types/game.h player::unknown_d0 (0xd0), player::marked_for_deletion (0xd5),
//   player::unit (0x34); types/objects.h object::vitality_flags (0x106, uint16_t); this
//   function only writes the high byte (0x107) with bit 0x20, i.e. vitality_flags bit 0x2000
//   (_object_stunned_bit) -- kept as a raw byte write since the semantic fit for "belongs to the
//   current game_time tick" is not otherwise confirmed.
// register convention: no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_time_globals *game_time;                // 0x006f1d6c
extern data_array *player_data;                      // 0x0087a480
extern data_array *object_data;                       // 0x008603b0
extern int16_t network_game_mode;                      // 0x00719720

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator elided
extern void player_remove(datum_index player_index); // 0x473bb0, UNSURE arg

// Marks datum entries owned by the local viewport's player(s) with a flag, and sets a matching
// bit on their controlled object, when a multiplayer game engine is loaded.
void game_engine_flag_local_player_units(void)
{
    int32_t current_tick;
    player *p;
    object *unit_obj;

    if (current_game_engine != (game_engine_definition *)0) {
        current_tick = game_time->game_time;
        p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
        while (p != (player *)0) {
            if (p->unknown_d0 != k_datum_index_none && p->marked_for_deletion == 0 &&
                (network_game_mode == 1 || current_tick == (int32_t)p->unknown_d0)) {
                p->marked_for_deletion = 1;
                if (p->unit == k_datum_index_none) {
                    player_remove(0xffffffff); // UNSURE: original passes no player handle either (see #if 0)
                } else {
                    unit_obj = ((object_header *)object_data->data)[p->unit & 0xffff].data;
                    *((uint8_t *)unit_obj + 0x107) |= 0x20;
                }
            }
            p = (player *)data_iterator_next((data_iterator *)0); // UNSURE: iterator elided
        }
    }
}

#if 0
Original Ghidra decompilation (0x45b590), from tools/pack.py 0x45b590:

void FUN_0045b590(void)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;

  if (DAT_006f1d20 != 0) {
    iVar2 = *(int *)(DAT_006f1d6c + 0xc);
    iVar3 = data_iterator_next();
    while (iVar3 != 0) {
      if (((*(int *)(iVar3 + 0xd0) != -1) && (*(char *)(iVar3 + 0xd5) == '\0')) &&
         ((DAT_00719720 == 1 || (iVar2 == *(int *)(iVar3 + 0xd0))))) {
        *(undefined1 *)(iVar3 + 0xd5) = 1;
        if (*(uint *)(iVar3 + 0x34) == 0xffffffff) {
          player_remove();
        }
        else {
          pbVar1 = (byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                    (*(uint *)(iVar3 + 0x34) & 0xffff) * 0xc) + 0x107);
          *pbVar1 = *pbVar1 | 0x20;
        }
      }
      iVar3 = data_iterator_next();
    }
  }
  return;
}
#endif
