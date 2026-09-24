// hud_unit_sounds_update  (Ghidra: FUN_004afee0, renamed in the phase-4 review)
// address 0x4afee0, size 557 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4afee0..0x4b010c in the phase-4 review (the first rewrite
// took a local player index and read 0x006f187c as an array). EAX is a player record: the
// meter record is the one of its local player index, the unit is player::unit or, without
// one, hud_unit_meter_state::last_unit. The HUD sounds of the unit new hud interface (Unit
// tag +0x2a8/+0x2ac, second entry in split screen) are latched by condition bits: 0x01 unit
// object flags bit 12, 0x02 shield dropping, 0x04 shield low (0 < shield < 0.25), 0x08 shield
// empty (only with a displayed shield, a live player per 0x462c10 and script flag bit 2 clear),
// 0x10 health low (< 0.25), 0x20 object flags bit 2, 0x40 health dropping by less than
// 0.1875, 0x80 health dropping by at least 0.1875 (script flag bit 0 clear). A dead or
// flag-bit-2 (+0x10) unit forgets last_unit and stops everything; so do a disabled HUD and a
// cinematic (byte 9 of 0x006f187c). Callers: hud_update_player (HUD enabled byte) and
// hud_unit_meters_update_for_player (0x4b0160) during a cinematic.
// register convention: EAX player; one stack argument (a byte).
//   // blam-cc: player -> EAX
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"

extern tag_instance *tag_instances;            // 0x0087bc14
extern player_globals *local_player_globals;   // 0x0087a478
extern hud_unit_meter_globals *hud_unit_meters; // 0x0071942c
extern uint8_t *cinematic_globals; // 0x006f187c, UNSURE name; byte +9 suppresses HUD messages

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern datum_index local_player_to_player_index(int16_t local_player_index); // 0x474d30, blam-cc: AX
extern uint8_t game_engine_object_flag_bit3_clear(datum_index player_index); // 0x462c10, UNSURE: a player alive / playing test
extern void hud_unit_sounds_play(uint32_t active_mask, const TagReflexive *sounds, int32_t *handles,
                                 uint16_t *playing); // 0x4afd30

// blam-cc: player -> EAX
void hud_unit_sounds_update(player *p, uint8_t hud_enabled)
{
    hud_unit_meter_state *state = &hud_unit_meters->players[p->local_player_index];
    datum_index unit_index = p->unit;
    uint8_t *unit;
    Unit *unit_tag;
    UnitHUDInterface *hud;
    int32_t choice;
    int32_t last;
    datum_index hud_tag;
    uint32_t mask;

    if (unit_index == (datum_index)-1) {
        unit_index = state->last_unit;
    }
    unit = (uint8_t *)object_try_and_get(unit_index, 3);
    if (unit == 0) {
        return;
    }
    unit_tag = (Unit *)tag_instances[*(datum_index *)unit & 0xffff].data;
    choice = (int16_t)(local_player_globals->local_player_count > 1);
    last = (int32_t)*(uint32_t *)((uint8_t *)unit_tag + 0x2a8) - 1;
    if (choice > last) {
        choice = last;
    }
    if ((int16_t)choice < 0) {
        return;
    }
    hud_tag = *(datum_index *)(*(uint8_t **)((uint8_t *)unit_tag + 0x2ac) + (int16_t)choice * 0x30 + 0xc);
    if (hud_tag == (datum_index)-1) {
        return;
    }
    hud = (UnitHUDInterface *)tag_instances[hud_tag & 0xffff].data;

    mask = 0;
    if ((unit[0x10] & 4) != 0 || !(*(float *)(unit + 0xe0) > 0.0f)) {
        state->last_unit = (datum_index)-1;
    } else if (hud_enabled != 0 && cinematic_globals[9] == 0) {
        float shield = *(float *)(unit + 0xe4);
        float health = *(float *)(unit + 0xe0);

        if (state->displayed_shield != -1.0f && game_engine_object_flag_bit3_clear(local_player_to_player_index(p->local_player_index)) != 0 &&
            (hud_unit_meters->flags & 4) == 0) {
            mask = (*(uint16_t *)(unit + 0x106) >> 12) & 1;
            if (state->displayed_shield > shield) {
                mask |= 2;
            }
            if (shield < 0.25f && shield > 0.0f) {
                mask |= 4;
            }
            if (shield == 0.0f) {
                mask |= 8;
            }
        }
        if ((hud_unit_meters->flags & 1) == 0) {
            if (health < 0.25f) {
                mask |= 0x10;
            }
            if ((unit[0x106] & 4) != 0) {
                mask |= 0x20;
            }
            if (state->displayed_health > health && state->displayed_health - health < 0.1875f) {
                mask |= 0x40;
            }
            if (!(state->displayed_health - health < 0.1875f)) {
                mask |= 0x80;
            }
        }
    }
    hud_unit_sounds_play(mask, &hud->sounds, state->sound_handles, &state->sounds_playing);
}

#if 0
Original Ghidra decompilation (0x4afee0):

void FUN_004afee0(char param_1)

{
  int iVar1;
  char cVar2;
  int in_EAX;
  uint *puVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  uint extraout_EDX;
  float *pfVar8;

  pfVar8 = (float *)(*(short *)(in_EAX + 2) * 0x58 + DAT_0071942c);
  puVar3 = (uint *)object_try_and_get(3);
  if (puVar3 != (uint *)0x0) {
    iVar1 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar5 = *(int *)(iVar1 + 0x2a8) + -1;
    iVar6 = (int)(short)(ushort)(1 < *(short *)(DAT_0087a478 + 0xc));
    if (iVar6 <= iVar5) {
      iVar5 = iVar6;
    }
    if ((-1 < (short)iVar5) &&
       (uVar7 = *(uint *)((short)iVar5 * 0x30 + 0xc + *(int *)(iVar1 + 0x2ac)), uVar7 != 0xffffffff)
       ) {
      iVar1 = *(int *)((uVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      uVar7 = 0;
      if (((puVar3[4] & 4) != 0) || ((float)puVar3[0x38] <= 0.0)) {
        pfVar8[7] = -NAN;
      }
      else if ((param_1 != '\0') && (*(char *)(DAT_006f187c + 9) == '\0')) {
        if (*pfVar8 != -1.0) {
          uVar4 = local_player_to_player_index();
          cVar2 = FUN_00462c10(uVar4);
          uVar7 = extraout_EDX;
          if ((cVar2 != '\0') && ((*(byte *)(DAT_0071942c + 0x58) & 4) == 0)) {
            uVar7 = (*(ushort *)((int)puVar3 + 0x106) & 0x1000) >> 0xc;
            if ((float)puVar3[0x39] < *pfVar8) {
              uVar7 = uVar7 | 2;
            }
            if (((float)puVar3[0x39] < 0.25) && (0.0 < (float)puVar3[0x39])) {
              uVar7 = uVar7 | 4;
            }
            if ((float)puVar3[0x39] == 0.0) {
              uVar7 = uVar7 | 8;
            }
          }
        }
        if ((*(byte *)(DAT_0071942c + 0x58) & 1) == 0) {
          if (0.25 <= (float)puVar3[0x38]) {
            uVar7 = uVar7 & 0xffffffef;
          }
          else {
            uVar7 = uVar7 | 0x10;
          }
          if ((*(byte *)((int)puVar3 + 0x106) & 4) == 0) {
            uVar7 = uVar7 & 0xffffffdf;
          }
          else {
            uVar7 = uVar7 | 0x20;
          }
          if ((pfVar8[1] <= (float)puVar3[0x38]) || (0.1875 <= pfVar8[1] - (float)puVar3[0x38])) {
            uVar7 = uVar7 & 0xffffffbf;
          }
          else {
            uVar7 = uVar7 | 0x40;
          }
          if (pfVar8[1] - (float)puVar3[0x38] < 0.1875) {
            uVar7 = uVar7 & 0xffffff7f;
          }
          else {
            uVar7 = uVar7 | 0x80;
          }
        }
      }
      FUN_004afd30(uVar7,iVar1 + 0x3c0,pfVar8 + 10,pfVar8 + 9);
    }
  }
  return;
}
#endif
