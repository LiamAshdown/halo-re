// game_engine_player_has_respawn_priority  (Ghidra: FUN_00460e40; renamed per its summary)
// address 0x460e40, size 234 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Determines whether a dead player has priority to
// respawn next under a single-life-per-round game rule"); types/game.h game_variant::unknown_40
// (+0x40, "sanitize normalizes to 0/1"; the 0x006f1cc8 alias list in the header confirms it),
// player::unit (+0x34), player::last_death_tick (+0x84), player::odd_man_out (+0x8c, "cached
// result of 0x460e40" -- this function), player::deaths (+0xae); types/memory.h data_iterator.
// register convention: no register-passed arguments; param_1 is this function's own stack
// parameter (a player index).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data;         // 0x0087a480
extern game_variant game_engine_variant; // 0x006f1c88 (unknown_40 aliased as 0x006f1cc8)
extern int16_t network_game_mode;        // 0x00719720

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// Only meaningful under the single-life-per-round rule (game_engine_variant.unknown_40) and for
// a player that is currently dead. A dedicated client just returns the server-computed cached
// value; the server (or a local game) recomputes it: a dead player with no deaths yet never has
// priority, and otherwise loses priority to any other dead player whose last death was more
// recent, with every tie resolved against the player being tested. The result is cached into
// player::odd_man_out either way.
uint8_t game_engine_player_has_respawn_priority(uint32_t player_index)
{
    player *self = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    uint8_t result = 0;

    if (game_engine_variant.unknown_40 != 0 && self->unit == (datum_index)0xffffffff) {
        result = 1;
        if (network_game_mode == 1) {
            return self->odd_man_out;
        }
        if (self->deaths < 1) {
            result = 0;
        } else {
            data_iterator iter;
            void *element;

            iter.data = player_data;
            iter.next_index = 0;
            iter.index = (datum_index)0xffffffff;

            element = data_iterator_next(&iter);
            if (element != 0) {
                do {
                    player *other = (player *)element;
                    if (other->unit == (datum_index)0xffffffff && other != self &&
                        (self->last_death_tick < other->last_death_tick ||
                         (other->last_death_tick == self->last_death_tick &&
                          (player_index & 0xffff) < 0xffff))) {
                        result = 0;
                    }
                    element = data_iterator_next(&iter);
                } while (element != 0);
                self->odd_man_out = result;
                return result;
            }
        }
        self->odd_man_out = result;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x460e40), from tools/pack.py 0x460e40:

undefined1 FUN_00460e40(uint param_1)

{
  int iVar1;
  undefined1 uVar2;
  int iVar3;
  
  iVar3 = (param_1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  uVar2 = 0;
  if ((DAT_006f1cc8 != '\0') && (*(int *)(iVar3 + 0x34) == -1)) {
    uVar2 = 1;
    if (DAT_00719720 == 1) {
      return *(undefined1 *)(iVar3 + 0x8c);
    }
    if (*(short *)(iVar3 + 0xae) < 1) {
      uVar2 = 0;
    }
    else {
      iVar1 = data_iterator_next();
      if (iVar1 != 0) {
        do {
          if (((*(int *)(iVar1 + 0x34) == -1) && (iVar1 != iVar3)) &&
             ((*(int *)(iVar3 + 0x84) < *(int *)(iVar1 + 0x84) ||
              ((*(int *)(iVar1 + 0x84) == *(int *)(iVar3 + 0x84) && ((param_1 & 0xffff) < 0xffff))))
             )) {
            uVar2 = 0;
          }
          iVar1 = data_iterator_next();
        } while (iVar1 != 0);
        *(undefined1 *)(iVar3 + 0x8c) = uVar2;
        return uVar2;
      }
    }
    *(undefined1 *)(iVar3 + 0x8c) = uVar2;
  }
  return uVar2;
}
#endif
