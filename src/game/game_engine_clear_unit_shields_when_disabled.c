// game_engine_clear_unit_shields_when_disabled  (Ghidra: FUN_0045fd20; renamed from the prior
// one-line summary -- see note below)
// address 0x45fd20, size 102 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md gives a low-confidence (0.3) summary, "Clears a pair of
// per-unit tracking fields (likely recent-damage bookkeeping) each tick when a game option flag
// is enabled" -- but the two fields this function actually zeroes, object+0xdc and object+0xe4,
// are types/objects.h's `maximum_shield_vitality` and `shield_vitality`, not the recent-damage
// pair at +0xf4/+0xf8. Renamed and re-described accordingly: this reads far more like a
// "shields disabled" game-variant option (game_variant::flags bit 3, offset 0x006f1cc0) forcing
// every controlled player's unit to have zero shields, tick after tick.
// register convention: player handle in EAX (in_EAX).
//   // blam-cc: EAX -> player
// UNSURE: bit 3 of game_variant::flags is not named in types/game.h; kept as a raw bit test.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88
extern data_array *player_data;                     // 0x0087a480
extern data_array *object_headers;                  // 0x008603b0

// blam-cc: EAX -> player
// While a multiplayer engine is running and the active variant has flag bit 3 set (UNSURE:
// presumed "shields disabled"), zeroes `player`'s controlled unit's maximum and current shield
// vitality every tick.
void game_engine_clear_unit_shields_when_disabled(datum_index player_handle)
{
    player *p;
    object *unit_obj;

    if (current_game_engine == 0 || player_handle == (datum_index)0xffffffff ||
        (game_engine_variant.flags & 0x08) == 0) {
        return;
    }

    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    if (p->unit == (datum_index)0xffffffff) {
        return;
    }

    unit_obj = ((object_header *)object_headers->data)[p->unit & 0xffff].data;
    unit_obj->shield_vitality = 0.0f;
    unit_obj->maximum_shield_vitality = 0.0f;
}

#if 0
Original Ghidra decompilation (0x45fd20), from tools/pack.py 0x45fd20:

void FUN_0045fd20(void)

{
  uint uVar1;
  int iVar2;
  uint in_EAX;

  if (((DAT_006f1d20 != 0) && (in_EAX != 0xffffffff)) && ((~(byte)(DAT_006f1cc0 >> 3) & 1) == 0)) {
    uVar1 = *(uint *)((in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34);
    if (uVar1 != 0xffffffff) {
      iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
      *(undefined4 *)(iVar2 + 0xe4) = 0;
      *(undefined4 *)(iVar2 + 0xdc) = 0;
    }
  }
  return;
}
#endif
