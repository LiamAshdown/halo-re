// unit_record_recent_damage_and_react  (Ghidra: FUN_00568230)
// address 0x568230, size 778 bytes
// name confidence: 0.3 (functions.md: "Records a recent damage or contact event into a small
//   per-unit cache, used to avoid repeating an associated response too often")
// rewrite confidence: 0.25
// evidence: types/units.h unit_recent_damage (0x430, four of them, tick/damage/
//   responsible_unit/responsible_player); types/memory.h data_array (0x0087a480 player_data,
//   0x008603b0 object_data); types/objects.h object.vitality_flags (0x106), object.name_index
//   (0xb8, UNSURE -- see below).
// register convention: this unit's index in EAX, the rest on the stack.
//   // blam-cc: in_EAX -> unit_index, param_1 -> damage_amount, param_2 -> response_index,
//   //   param_3 -> allow_broadcast, param_4 -> responsible_player, param_5 -> team_index,
//   //   param_6 -> responsible_object
// UNSURE: param_4 and param_6 are typed `float` by Ghidra because the compiler's `-NaN`
//   (0xffffffff reinterpreted as a float) is how the sentinel "no datum" value is spelled once
//   it shares a register/stack slot with a genuine float; both are really `datum_index`.
// UNSURE: `*(short *)(unit_object + 0xb8)` lands exactly on `object.name_index`
//   (types/objects.h), but is used here as a 0..9 team index into a 10x10 friendly-fire bitmask
//   at 0x006b0b84+0xa4 -- kept as a raw offset rather than asserting it really is the scenario
//   name index for a live unit.
// UNSURE: the tag-side offsets 0x324/0x328 (selected by response_index == 9) and the responsible
//   object's own +0x218 (controlling_player-shaped) test are not named; see the #if 0 block.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern data_array *player_data;     // 0x0087a480
extern tag_instance *tag_instances; // 0x0087bc14
extern hs_game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/hs.h)
extern int32_t network_predicted_state_flag; // 0x006f1d20
extern uint8_t friendly_fire_matrix[100 / 8 + 1]; // 0x006b0b84 + 0xa4, UNSURE exact shape

extern void ai_communication_broadcast(int32_t kind, float responsible_object, uint32_t a, uint32_t b, uint32_t c, uint32_t d, int32_t e); // 0x42d340, UNSURE signature

void unit_record_recent_damage_and_react(uint32_t unit_index, float damage_amount, int16_t response_index,
                                         uint8_t allow_broadcast, uint32_t responsible_player,
                                         int16_t team_index, uint32_t responsible_object) // blam-cc: see header
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    int32_t current_tick = game_time->current_tick;
    uint8_t merged = 0;

    unit_recent_damage *slot = unit->recent_damage;
    for (int32_t i = 4; i != 0; i--, slot++) {
        if (((responsible_player != 0xffffffff) && (slot->responsible_player == responsible_player)) ||
            (slot->responsible_unit == responsible_object)) {
            slot->tick = current_tick;
            merged = 1;
            slot->damage += damage_amount;
        }
    }

    if (!merged) {
        int16_t empty = 0;
        while (unit->recent_damage[empty].tick != -1) {
            empty++;
            if (empty >= 4) {
                empty = -1;
                break;
            }
        }
        int16_t chosen;
        if (empty != -1) {
            chosen = empty;
        } else {
            int16_t oldest_by_damage = 0;
            for (int16_t i = 1; i < 4; i++) {
                if (unit->recent_damage[oldest_by_damage].damage < unit->recent_damage[i].damage) {
                    oldest_by_damage = i;
                }
            }
            int16_t oldest_by_tick = -1;
            for (int16_t i = 0; i < 4; i++) {
                if ((i != oldest_by_damage) &&
                    ((oldest_by_tick == -1) || ((uint32_t)unit->recent_damage[i].tick < (uint32_t)unit->recent_damage[oldest_by_tick].tick))) {
                    oldest_by_tick = i;
                }
            }
            chosen = oldest_by_tick;
        }
        unit->recent_damage[chosen].responsible_unit = responsible_object;
        unit->recent_damage[chosen].responsible_player = responsible_player;
        unit->recent_damage[chosen].damage = damage_amount;
        unit->recent_damage[chosen].tick = current_tick;
    }

    if (!allow_broadcast) {
        return;
    }
    if (team_index == -1) {
        return;
    }

    int16_t self_team = *(int16_t *)((uint8_t *)unit_obj + 0xb8); // UNSURE: object.name_index reused as team
    uint8_t hostile;
    if (network_predicted_state_flag == 0) {
        if ((self_team < 0) || (9 < self_team) || (team_index < 0) || (9 < team_index)) {
            goto broadcast_check;
        }
        int32_t bit_index = team_index + self_team * 10;
        hostile = 1 - ((friendly_fire_matrix[bit_index >> 5] & (1 << (bit_index & 0x1f))) != 0);
    } else {
        hostile = self_team != team_index;
    }
    if (!hostile) {
        return;
    }

broadcast_check:
    {
        object *responsible_obj = (object *)0;
        if ((responsible_player != 0xffffffff) &&
            (*(uint32_t *)((uint8_t *)player_data->data + (responsible_player & 0xffff) * 0x200 + 0x34) != 0xffffffff)) {
            uint32_t controlled_unit = *(uint32_t *)((uint8_t *)player_data->data + (responsible_player & 0xffff) * 0x200 + 0x34);
            responsible_obj = ((object_header *)object_data->data)[controlled_unit & 0xffff].data;
        }
        if (responsible_obj == (object *)0) {
            object *by_index = (object *)0;
            if ((responsible_object != 0xffffffff) && (0 <= (int16_t)responsible_object) &&
                ((int16_t)responsible_object < object_data->maximum_count)) {
                object_header *hdr = (object_header *)object_data->data + (int16_t)responsible_object;
                if ((hdr->identifier != 0) &&
                    (((int16_t)(responsible_object >> 16) == 0) || (hdr->identifier == (int16_t)(responsible_object >> 16)))) {
                    by_index = hdr->data;
                }
            }
            if ((by_index != (object *)0) && ((_object_mask_unit & (1 << (by_index->type & 0x1f))) != 0)) {
                responsible_obj = by_index;
            }
            if (responsible_obj == (object *)0) {
                return;
            }
        }

        uint8_t *responsible_def = (uint8_t *)tag_instances[responsible_obj->definition_tag & 0xffff].data;
        // UNSURE: the -1.0f sentinel below is `-NAN`'s bit pattern (0xffffffff) reinterpreted;
        // `broadcast_datum` carries a datum_index throughout even though the original spells it
        // as a float once it shares a register with genuine floats.
        uint32_t broadcast_datum = responsible_object;
        float indirect = response_index == 9 ? *(float *)(responsible_def + 0x324) : *(float *)(responsible_def + 0x328);
        if (*(uint32_t *)&indirect != 0xffffffff) {
            broadcast_datum = *(uint32_t *)&indirect;
        }
        object *target_obj = ((object_header *)object_data->data)[broadcast_datum & 0xffff].data;

        if ((target_obj->vitality_flags & _object_health_frozen_bit) == 0) {
            int32_t tick = game_time->current_tick;
            if ((unit->ai_communication_tick == -1) || (unit->ai_communication_tick + 0x78 < tick)) {
                unit->ai_communication_count = 0;
            }
            unit->ai_communication_count++;
            unit->ai_communication_tick = tick;
            unit_data *target_unit = (unit_data *)((uint8_t *)target_obj + k_unit_data_offset);
            int16_t threshold = (target_unit->controlling_player != k_datum_index_none) ? 5 : 3;
            if (threshold <= unit->ai_communication_count) {
                ai_communication_broadcast(1, *(float *)&broadcast_datum, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0);
                unit->ai_communication_count = 0;
            }
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x568230):

void FUN_00568230(float param_1,short param_2,char param_3,float param_4,short param_5,float param_6
                 )

{
  float fVar1;
  bool bVar2;
  uint in_EAX;
  float *pfVar3;
  int iVar4;
  short sVar5;
  float fVar6;
  short sVar7;
  short sVar8;
  int iVar9;
  uint *puVar10;
  int iVar11;
  char cVar12;

  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  fVar6 = *(float *)(DAT_006f1d6c + 0xc);
  bVar2 = false;
  pfVar3 = (float *)(iVar4 + 0x434);
  iVar9 = 4;
  do {
    if (((param_4 != -NAN) && (pfVar3[2] == param_4)) || (pfVar3[1] == param_6)) {
      pfVar3[-1] = fVar6;
      bVar2 = true;
      *pfVar3 = param_1 + *pfVar3;
    }
    pfVar3 = pfVar3 + 4;
    iVar9 = iVar9 + -1;
  } while (iVar9 != 0);
  if (!bVar2) {
    sVar7 = 0;
    do {
      if (*(int *)((sVar7 + 0x43) * 0x10 + iVar4) == -1) {
        if (sVar7 != -1) goto LAB_00568333;
        break;
      }
      sVar7 = sVar7 + 1;
    } while (sVar7 < 4);
    sVar8 = 0;
    sVar7 = 1;
    pfVar3 = (float *)(iVar4 + 0x444);
    do {
      if (*(float *)(sVar8 * 0x10 + 0x434 + iVar4) < *pfVar3) {
        sVar8 = sVar7;
      }
      sVar7 = sVar7 + 1;
      pfVar3 = pfVar3 + 4;
    } while (sVar7 < 4);
    sVar7 = -1;
    sVar5 = 0;
    puVar10 = (uint *)(iVar4 + 0x430);
    do {
      if ((sVar5 != sVar8) &&
         ((sVar7 == -1 || (*puVar10 < *(uint *)((sVar7 + 0x43) * 0x10 + iVar4))))) {
        sVar7 = sVar5;
      }
      sVar5 = sVar5 + 1;
      puVar10 = puVar10 + 4;
    } while (sVar5 < 4);
LAB_00568333:
    iVar9 = sVar7 * 0x10 + iVar4;
    *(float *)(iVar9 + 0x438) = param_6;
    *(float *)(iVar9 + 0x43c) = param_4;
    *(float *)(iVar9 + 0x434) = param_1;
    *(float *)((sVar7 + 0x43) * 0x10 + iVar4) = fVar6;
  }
  iVar9 = DAT_008603b0;
  if (param_3 == '\0') {
    return;
  }
  if (param_5 == -1) {
    return;
  }
  sVar7 = *(short *)(iVar4 + 0xb8);
  if (DAT_006f1d20 == 0) {
    if (((sVar7 < 0) || (9 < sVar7)) || ((param_5 < 0 || (9 < param_5)))) goto LAB_005683dd;
    iVar4 = (int)param_5 + sVar7 * 10;
    cVar12 = '\x01' - ((1 << ((byte)iVar4 & 0x1f) &
                       *(uint *)(DAT_006b0b84 + 0xa4 + (iVar4 >> 5) * 4)) != 0);
  }
  else {
    cVar12 = sVar7 != param_5;
  }
  if (cVar12 == '\0') {
    return;
  }
LAB_005683dd:
  if (((param_4 == -NAN) ||
      (fVar6 = *(float *)(((uint)param_4 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34),
      fVar6 == -NAN)) ||
     (iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)fVar6 & 0xffff) * 0xc), iVar4 == 0
     )) {
    iVar11 = 0;
    if (((param_6 != -NAN) && (sVar7 = SUB42(param_6,0), -1 < sVar7)) &&
       (sVar7 < *(short *)(DAT_008603b0 + 0x20))) {
      iVar4 = (int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar7;
      sVar7 = *(short *)(iVar4 + *(int *)(DAT_008603b0 + 0x34));
      if ((sVar7 != 0) && ((sVar8 = (short)((uint)param_6 >> 0x10), sVar8 == 0 || (sVar7 == sVar8)))
         ) {
        iVar11 = iVar4 + *(int *)(DAT_008603b0 + 0x34);
      }
    }
    iVar4 = 0;
    if ((iVar11 != 0) && ((1 << (*(byte *)(iVar11 + 3) & 0x1f) & 3U) != 0)) {
      iVar4 = *(int *)(iVar11 + 8);
    }
    fVar6 = param_6;
    if (iVar4 == 0) {
      return;
    }
  }
  if (param_2 == 9) {
    fVar1 = *(float *)(iVar4 + 0x324);
  }
  else {
    fVar1 = *(float *)(iVar4 + 0x328);
  }
  if (fVar1 != -NAN) {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)fVar1 & 0xffff) * 0xc);
    fVar6 = fVar1;
  }
  if ((*(byte *)(iVar4 + 0x106) & 4) == 0) {
    iVar11 = *(int *)(DAT_006f1d6c + 0xc);
    if ((*(int *)(iVar4 + 0x42c) == -1) || (*(int *)(iVar4 + 0x42c) + 0x78 < iVar11)) {
      *(undefined2 *)(iVar4 + 0x42a) = 0;
    }
    *(short *)(iVar4 + 0x42a) = *(short *)(iVar4 + 0x42a) + 1;
    *(int *)(iVar4 + 0x42c) = iVar11;
    if ((short)((ushort)(*(int *)(*(int *)(*(int *)(iVar9 + 0x34) + 8 + ((uint)fVar6 & 0xffff) * 0xc
                                          ) + 0x218) != -1) * 2 + 3) <= *(short *)(iVar4 + 0x42a)) {
      ai_communication_broadcast(1,fVar6,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0);
      *(undefined2 *)(iVar4 + 0x42a) = 0;
    }
  }
  return;
}
#endif
