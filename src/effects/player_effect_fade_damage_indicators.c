// player_effect_fade_damage_indicators  (Ghidra: FUN_00457220, still unnamed there; named
//   directly by out/phase4/effects_types_notes.md: "player_effect_fade_damage_indicators 0x457220
//   fades the four indicator bytes up to 255")
// address 0x457220, size 84 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: types/effects.h player_effect.damage_indicator_alpha[4] (+0xe4); src/objects/
//   glow_update.c and src/effects/decal_update_fade.c establish game_tick_globals (0x006f1d6c)
//   with a current tick at +0x0c and a per tick delta at +0x10 (int16).
// register convention: local player index in the low 16 bits of EAX (in_AX); an output pointer
//   in EDX that receives the pre-fade snapshot of the four indicator bytes, which lets the
//   caller (player_effect_send_network_update, 0x456bc0) tell whether anything changed.
//   // blam-cc: in_AX -> local_player_index, in_EDX -> out_previous_indicators

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern player_effect_globals *player_effect_globals_pointer; // 0x006f1884
extern int32_t *game_tick_globals;                            // 0x006f1d6c; +0x0c current game
                                    // tick, +0x10 tick delta (int16), per src/objects/glow_update.c

// Fades one local player's four directional damage indicators upward toward 255 by the current
// tick delta, and hands the caller the pre-fade byte values so it can detect a change.
void player_effect_fade_damage_indicators(int16_t local_player_index, uint32_t *out_previous_indicators)
{
    player_effect *self = &player_effect_globals_pointer->players[local_player_index];
    uint8_t *indicators = self->damage_indicator_alpha;
    int16_t delta = *(int16_t *)((uint8_t *)game_tick_globals + 0x10);
    int i;

    *out_previous_indicators = *(uint32_t *)indicators;

    for (i = 0; i < 4; i++) {
        if (indicators[i] != 0) {
            int32_t faded = (int32_t)indicators[i] + delta;
            if (faded > 0xfe) {
                faded = 0xff;
            }
            indicators[i] = (uint8_t)faded;
        }
    }
}

#if 0
Original Ghidra decompilation (0x457220):

void FUN_00457220(void)

{
  short in_AX;
  int iVar1;
  byte *pbVar2;
  int iVar3;
  undefined4 *in_EDX;
  int iVar4;

  iVar1 = in_AX * 0xec + DAT_006f1884;
  pbVar2 = (byte *)(iVar1 + 0xe4);
  *in_EDX = *(undefined4 *)(iVar1 + 0xe4);
  iVar1 = DAT_006f1d6c;
  iVar4 = 4;
  do {
    if (*pbVar2 != 0) {
      iVar3 = (uint)*pbVar2 + (int)*(short *)(iVar1 + 0x10);
      if (0xfe < iVar3) {
        iVar3 = 0xff;
      }
      *pbVar2 = (byte)iVar3;
    }
    pbVar2 = pbVar2 + 1;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  return;
}
#endif
