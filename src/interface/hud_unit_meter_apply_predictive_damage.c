// hud_unit_meter_apply_predictive_damage  (Ghidra: FUN_004b16e0, still unnamed there; named
// here)
// address 0x4b16e0, size 95 bytes
// name confidence: 0.5 (chosen)   rewrite confidence: 0.9 (verified against objdump 0x4b16e0..0x4b173e in the phase-4 review)
// evidence: out/phase4/interface_functions.md "Immediately subtracts a damage amount from a
// local player's cached shield/health meter display, ahead of the next authoritative
// object-state update."; types/game.h player::identifier (+0x00) / local_player_index
// (+0x02) match the two fields read off the player record; types/interface.h
// hud_unit_meter_state::displayed_shield is the field at offset 0 of the block this writes.
// register convention: player datum_index in ECX (in_ECX, index low 16 / salt high 16);
// damage amount as the one recovered stack parameter.
//   // blam-cc: player_index -> ECX, damage -> stack param_1
// UNSURE: the per-local-player stride used here (0x58) does not match
// hud_unit_meter_state's documented size (0x5c); since retail PC has exactly one local
// player, local_player_index is always 0 and the stride is never actually exercised past
// offset 0, so this cannot be resolved from this build alone. The field written (offset 0
// of the block) is displayed_shield either way.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern data_array *player_data;                    // 0x0087a480, stride 0x200 (no types/players.h yet)
extern hud_unit_meter_globals *hud_unit_meters;     // 0x0071942c

// blam-cc: player_index -> ECX, damage -> stack param_1
// Subtracts damage directly from a local player's displayed (smoothed) shield meter value, so
// the HUD shows the hit immediately instead of waiting for the next authoritative object update
// to drive the smoothing in hud_unit_meters_update_for_player.
void hud_unit_meter_apply_predictive_damage(datum_index player_index, float damage)
{
    int16_t index;
    int16_t salt;
    player *p;

    if (player_index == (datum_index)-1) {
        return;
    }
    index = (int16_t)player_index;
    if (index < 0 || index >= player_data->maximum_count) {
        return;
    }
    p = (player *)((uint8_t *)player_data->data + player_data->size * index);
    if (p->identifier == 0) {
        return;
    }
    salt = (int16_t)((uint32_t)player_index >> 16);
    if (salt != 0 && p->identifier != salt) {
        return;
    }
    if (p->local_player_index == -1) {
        return;
    }
    {
        hud_unit_meters->players[p->local_player_index].displayed_shield -= damage;
    }
}

#if 0
Original Ghidra decompilation (0x4b16e0):

void FUN_004b16e0(float param_1)

{
  short *psVar1;
  int iVar2;
  short sVar3;
  int in_ECX;
  short sVar4;

  if (((in_ECX != -1) && (sVar3 = (short)in_ECX, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_0087a480 + 0x20))) {
    psVar1 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3 +
                      *(int *)(DAT_0087a480 + 0x34));
    sVar3 = *psVar1;
    if (((sVar3 != 0) && ((sVar4 = (short)((uint)in_ECX >> 0x10), sVar4 == 0 || (sVar3 == sVar4))))
       && (sVar3 = psVar1[1], sVar3 != -1)) {
      iVar2 = sVar3 * 0x58;
      *(float *)(iVar2 + DAT_0071942c) = *(float *)(iVar2 + DAT_0071942c) - param_1;
    }
  }
  return;
}
#endif
