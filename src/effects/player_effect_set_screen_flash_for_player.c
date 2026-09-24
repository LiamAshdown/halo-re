// player_effect_set_screen_flash_for_player  (Ghidra: FUN_00456980, still unnamed there)
// address 0x456980, size 75 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/game.h player.local_player_index (+0x02); this module's
//   player_effect_set_screen_flash (0x4578a0) and player_effect_globals.players[1] (+0x000).
//   out/phase4/effects_types_notes.md's misattribution table places this address in the
//   player_effect group, not the "contrail" framing functions.md guessed at phase 2.
// register convention: player datum_index in EAX (in_EAX); a ScreenFlash-shaped descriptor and a
//   duration scale as the two Ghidra-recognized stack parameters (param_1, param_2), though only
//   param_2 (the descriptor) is referenced in the decompiled body.
//   // blam-cc: in_EAX -> player_index, stack -> (duration_scale, descriptor)
// UNSURE: param_1 (duration_scale) is never read in the decompiled body, yet
//   player_effect_set_screen_flash needs a fourth argument this call's visible three do not
//   supply; modeled here as param_1 forwarded through, since a genuinely unused parameter would
//   be unusual for a thin wrapper like this one.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480
extern player_effect_globals *player_effect_globals_pointer; // 0x006f1884

extern void player_effect_set_screen_flash(player_effect *self, player_screen_flash *descriptor,
    float intensity_falloff, float duration_scale); // 0x4578a0, this module

void player_effect_set_screen_flash_for_player(datum_index player_index, float duration_scale,
    player_screen_flash *descriptor) // blam-cc: in_EAX, stack, stack
{
    if (player_index != (datum_index)0xffffffff) {
        player *record = &((player *)player_data->data)[player_index & 0xffff];

        if (record->local_player_index != -1) {
            player_effect_set_screen_flash(
                &player_effect_globals_pointer->players[record->local_player_index],
                descriptor, 1.0f, duration_scale);
        }
    }
}

#if 0
Original Ghidra decompilation (0x456980):

void FUN_00456980(undefined4 param_1,undefined4 param_2)

{
  short sVar1;
  uint in_EAX;

  if ((in_EAX != 0xffffffff) &&
     (sVar1 = *(short *)((in_EAX & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)), sVar1 != -1
     )) {
    FUN_004578a0(sVar1 * 0xec + DAT_006f1884,param_2,0x3f800000);
  }
  return;
}
#endif
