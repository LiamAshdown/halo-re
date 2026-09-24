// game_engine_end_game_sequence_stage2  (Ghidra: FUN_004670f0; named per
// out/phase4/game_functions.md: "Second stage of the countdown sequence: sets state 2 with a
// 5-second timer and clears a per-unit flag for all players.")
// address 0x4670f0, size 135 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: types/game.h game_engine_state (_game_engine_state_ended == 2, "5.0 s timer, set by
//   the second countdown stage" -- this function), game_engine_post_game_fade (0x0087aa0c,
//   "ramps 0 -> 1"), game_engine_end_game_timer (0x0087aa08); object+0x106 is types/objects.h
//   object::vitality_flags, bit 0x0020 of which is not named by that enum (the nearest named
//   bits are 0x0010 and 0x0080). VERIFIED against the disassembly (objdump -d -M intel
//   --start-address=0x4670f0 --stop-address=0x467180): Ghidra drops the local data_iterator this
//   function builds on its own stack and passes in EDI (src/memory/data_iterator_next.c
//   documents that real convention), same construction (and same dead fourth "iter"-XOR dword)
//   as game_engine_reset_all_unit_grenade_counts.c (0x467de0, this batch).
// register convention: no parameters.
// UNSURE: which vitality_flags bit 0x0020 is; types/objects.h leaves it unnamed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern game_engine_state game_engine_state_value;    // 0x0087aa10
extern float game_engine_end_game_timer;             // 0x0087aa08
extern float game_engine_post_game_fade;             // 0x0087aa0c
extern data_array *player_data;                      // 0x0087a480
extern data_array *object_headers;                   // 0x008603b0

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator

// Enters the "ended" end-of-game state: resets the post-game fade, starts the 5-second timer,
// and, for every player with a unit, sets an unresolved object vitality flag (bit 0x0020) on it.
void game_engine_end_game_sequence_stage2(void)
{
    data_iterator iterator;
    uint32_t unused_checksum; // UNSURE: written, never read back (see header)
    player *p;

    game_engine_post_game_fade = 0.0f;
    game_engine_state_value = _game_engine_state_ended;
    game_engine_end_game_timer = 5.0f;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)0xffffffff;
    unused_checksum = (uint32_t)player_data ^ 0x69746572;

    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (p->unit != (datum_index)0xffffffff) {
            object *unit_obj = ((object_header *)object_headers->data)[p->unit & 0xffff].data;
            unit_obj->vitality_flags = unit_obj->vitality_flags | 0x0020; // UNSURE: unnamed bit
        }
        p = (player *)data_iterator_next(&iterator);
    }

    (void)unused_checksum;
}

#if 0
Original Ghidra decompilation (0x4670f0), from tools/pack.py 0x4670f0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004670f0(void)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;

  DAT_0087aa0c = 0;
  DAT_0087aa10 = 2;
  _DAT_0087aa08 = 0x40a00000;
  iVar3 = data_iterator_next();
  iVar2 = DAT_008603b0;
  while (iVar3 != 0) {
    if (*(uint *)(iVar3 + 0x34) != 0xffffffff) {
      pbVar1 = (byte *)(*(int *)(*(int *)(iVar2 + 0x34) + 8 +
                                (*(uint *)(iVar3 + 0x34) & 0xffff) * 0xc) + 0x106);
      *pbVar1 = *pbVar1 | 0x20;
    }
    iVar3 = data_iterator_next();
  }
  return;
}
#endif
