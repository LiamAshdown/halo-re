// unit_apply_fall_damage  (Ghidra: already named unit_apply_fall_damage)
// address 0x55e4f0, size 444 bytes
// name confidence: 0.55 (already carried this name; matches functions.md's summary)
// rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x55e4f0..0x55e6ab)
// evidence: damage_data (types/objects.h, size 0x54) matches the locally built record
//   field-for-field (damage_effect_tag 0x00, responsible_player/_object 0x08/0x0c -1,
//   team_index 0x10, location_cluster_index 0x18, random_blend 0x40, multiplier 0x44,
//   unknown_4c 0x4c); object_apply_damage / damage_data_initialize signatures verified against
//   src/objects/object_apply_damage.c and src/objects/object_update_vitality_and_regeneration.c.
//   Biped.biped_flags bit 2 (0x2f4) gates whether a unit is exempt (already "invulnerable to
//   falling" per the sign test on that byte).
// UNSURE: the fall-damage table at global_globals+0x18c (offsets 0x8c/0x90/0x94/0x1c/0x38) is
//   not identified against any documented struct (units.h only names the +0x18c/+0x190 slots as
//   "grenade tables", which does not obviously match this usage) -- kept as raw offsets.
//   FUN_00474db0's role (looked up when a "delete on out-of-bounds" flag is set) is UNSURE.
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;
extern uint8_t unit_updates_suppressed; // 0x0071c419
extern uint8_t DAT_0087abc1;        // UNSURE global (cheat/debug toggle)
extern game_engine_definition *current_game_engine;  // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern void damage_data_initialize(damage_data *dd, datum_index damage_effect_tag); // 0x4ed990
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index,
                                 int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0
extern int32_t player_index_from_unit_index(uint32_t unit_index); // UNSURE module
extern void object_delete(uint32_t object_index);   // 0x4f5bd0, UNSURE exact signature

// Applies scaled fall damage to a unit when its downward velocity (param_2, positive) exceeds
// the tag-defined safe threshold, unless the unit is exempt (unattended and the Biped tag's own
// exemption bit is set, or updates are suppressed, or it's a non-local-player unit while the
// multiplayer-fall-damage toggle is off). Below the harmful threshold, applies the instant
// "out of bounds" damage effect (and deletes the unit if it's still marked for deferred delete);
// otherwise applies a blended damage effect scaled by how far past the harmful threshold the
// velocity is.
void unit_apply_fall_damage(uint32_t object_index, float fall_speed)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t *fall_table = (uint8_t *)global_globals->falling_damage.pointer;
    uint32_t exempt;

    exempt = ((unit->flags & 0x1000) == 0 && (int8_t)tag->biped_flags >= 0) ? 0 : 1;
    if (unit_updates_suppressed != 0) {
        exempt = 1;
    }

    if (DAT_0087abc1 == 0 || unit->controlling_player == k_datum_index_none) {
        if (fall_speed <= *(float *)(fall_table + 0x90)) {
            if ((tag->biped_flags & 4) == 0 && obj->velocity.k < -*(float *)(fall_table + 0x8c)) {
                if (!exempt && (obj->vitality_flags & 4) == 0) {
                    damage_data dd;
                    damage_data_initialize(&dd, *(datum_index *)(fall_table + 0x38));
                    object_apply_damage(&dd, object_index, -1, -1, -1, 0);
                }
                if (current_game_engine == 0 && (obj->flags & 0x200000) != 0) {
                    if (player_index_from_unit_index(object_index) == -1) {
                        object_delete(object_index);
                    }
                }
            }
        } else if (!exempt) {
            damage_data dd = {0};
            float harmless = *(float *)(fall_table + 0x90);
            float harmful = *(float *)(fall_table + 0x94);
            float blend = (fall_speed - harmless) / (harmful - harmless);

            dd.damage_effect_tag = *(datum_index *)(fall_table + 0x1c);
            dd.responsible_player = k_datum_index_none;
            dd.responsible_object = k_datum_index_none;
            dd.team_index = -1;
            dd.location_cluster_index = -1;
            dd.material_type = -1;
            dd.multiplier = 1.0f;
            dd.random_blend = (blend < 0.0f) ? 0.0f : (blend > 1.0f ? 1.0f : blend);

            object_apply_damage(&dd, object_index, -1, -1, -1, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x55e4f0):

void unit_apply_fall_damage(uint param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  uint *puVar4;
  bool bVar5;
  int iVar6;
  int iVar7;
  undefined4 extraout_EDX;
  undefined4 *puVar8;
  undefined4 local_5c [4];
  undefined2 local_4c;
  undefined2 local_44;
  float local_1c;
  undefined4 local_18;
  undefined2 local_10;

  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar6 = *(int *)(DAT_00746fa0 + 0x18c);
  iVar7 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((puVar4[0x81] & 0x1000) == 0) && (-1 < *(char *)(iVar7 + 0x2f4))) {
    bVar5 = false;
  }
  else {
    bVar5 = true;
  }
  if (DAT_0071c419 == '\x01') {
    bVar5 = true;
  }
  if ((DAT_0087abc1 == '\0') || (puVar4[0x86] == 0xffffffff)) {
    if (param_2 <= *(float *)(iVar6 + 0x90)) {
      if (((*(byte *)(iVar7 + 0x2f4) & 4) == 0) && ((float)puVar4[0x1c] < -*(float *)(iVar6 + 0x8c))
         ) {
        if ((!bVar5) && ((*(byte *)((int)puVar4 + 0x106) & 4) == 0)) {
          damage_data_initialize(*(undefined4 *)(iVar6 + 0x38));
          object_apply_damage(extraout_EDX,param_1,0xffffffff,0xffffffff,0xffffffff,0);
        }
        if ((DAT_006f1d20 == 0) && ((puVar4[4] & 0x200000) != 0)) {
          iVar6 = FUN_00474db0(param_1);
          if (iVar6 == -1) {
            object_delete();
          }
        }
      }
    }
    else if (!bVar5) {
      fVar1 = *(float *)(iVar6 + 0x90);
      fVar2 = *(float *)(iVar6 + 0x94);
      fVar3 = *(float *)(iVar6 + 0x90);
      puVar8 = local_5c;
      for (iVar7 = 0x15; iVar7 != 0; iVar7 = iVar7 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      local_1c = (param_2 - fVar1) / (fVar2 - fVar3);
      local_5c[0] = *(undefined4 *)(iVar6 + 0x1c);
      local_10 = 0xffff;
      local_5c[2] = 0xffffffff;
      local_5c[3] = 0xffffffff;
      local_4c = 0xffff;
      local_44 = 0xffff;
      local_18 = 0x3f800000;
      if (0.0 <= local_1c) {
        if (1.0 < local_1c) {
          local_1c = 1.0;
        }
      }
      else {
        local_1c = 0.0;
      }
      object_apply_damage(local_5c,param_1,0xffffffff,0xffffffff,0xffffffff,0);
      return;
    }
  }
  return;
}
#endif
