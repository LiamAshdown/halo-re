// actor_react_to_seen_target  (Ghidra: actor_react_to_seen_target, renamed)
// address 0x422ec0, size 858 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/ai.h actor.alert_level, actor.awareness_level/vocalization_*/mode/
//   mode_data/vocalization_unknown_3e8; prop.is_unit (0x60)/object_index (0x18)/look_point;
//   types/objects.h object_header; types/units.h unit_data.controlling_player (object+0x218)
//   and unit_data.actor_index (object+0x1f4). Calls actor_queue_search_position (0x421af0),
//   actor_queue_search_and_relay_perception (0x4221f0, already rewritten in this module),
//   teams_are_enemies (0x45bd50), datum_get (0x4d0680) and, outside this rewrite's range,
//   actor_target_data_acquire. Confirmed against bin/halo.exe (0x422ec0..0x422ff1): EAX is the actor_index
//   register argument throughout (Ghidra's own "param_1" name is misleading -- it is really
//   the stack-passed target_prop_index).
//   UNSURE: object+0xb8 (read as one operand of teams_are_enemies) is typed in
//   types/objects.h as `name_index`, which does not obviously fit; kept as a raw offset, as
//   in actor_react_to_flee_point @0x422c00 which has the identical pattern. Likewise
//   player+0x40/+0x44 have no established names (no types/players.h in this repo yet).
// register convention: EAX -> actor_index, stack -> target_prop_index.
//   // blam-cc: EAX -> actor_index, stack -> target_prop_index
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_ai.h"
#include "fn_math.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern data_array *player_data;     // 0x0087a480, stride 0x200 (no types/players.h yet)


extern void * datum_get(datum_index handle, data_array *array); // 0x4d0680
extern int8_t teams_are_enemies(int16_t a, int16_t b); // 0x45bd50, CX/DX


// Variant table paired with the sighted/recognized/directional/danger/flee dialogue families.
extern int16_t actor_dialogue_variant_table_f[]; // 0x00655644

// blam-cc: EAX -> actor_index, stack -> target_prop_index
// If the target prop is not itself a unit, relays perception/search state to it
// (actor_queue_search_and_relay_perception) and then, depending on who currently controls
// the underlying object, either notifies a hostile controlling player's presence or, for an
// AI-controlled unit whose actor is alert enough, forwards to actor_forward_target_object_reference. If the target
// prop IS a unit, instead directly queues a priority-6 velocity-only search position from it
// (the same shape actor_queue_velocity_search_from_prop @0x4221b0 uses). Either way, finishes
// by queuing category-7 dialogue exactly as actor_queue_sighted_target_dialogue's family does,
// carrying the raw target handle as payload.
void actor_react_to_seen_target(datum_index actor_index, datum_index target_prop_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag;
    prop *target = &((prop *)prop_data->data)[target_prop_index & 0xffff];

    if (target->is_unit == 0) {
        object *tracked = ((object_header *)object_data->data)[target->object_index & 0xffff].data;
        unit_data *unit = (unit_data *)((uint8_t *)tracked + k_unit_data_offset);

        actor_queue_search_and_relay_perception(target_prop_index, actor_index);

        if (unit->controlling_player != (datum_index)k_datum_index_none) {
            uint8_t *player = (uint8_t *)player_data->data + (unit->controlling_player & 0xffff) * 0x200;
            int32_t unknown_40 = *(int32_t *)&((struct player *)player)->observer_target;      // UNSURE: no established name
            int32_t unknown_44 = ((struct player *)player)->observer_state;      // UNSURE: no established name

            if (unknown_40 != -1 && (int32_t)game_time->game_time <= unknown_44 + 0x5a) { // 0x422f6e: jl exits
                object *player_unit = ((object_header *)object_data->data)[unknown_40 & 0xffff].data;
                if (teams_are_enemies(player_unit->owner_team /* UNSURE, see file header */, self->team) != 0) {
                    // 0x422faf: the player's +0x40 object, not the seen unit
                    actor_target_data_acquire(actor_index, (datum_index)unknown_40, k_datum_index_none, k_datum_index_none);
                }
            }
        } else if (unit->actor_index != (datum_index)k_datum_index_none) {
            actor *controller = &((actor *)actor_data->data)[unit->actor_index & 0xffff];
            if (controller->alert_level >= 4) {
                // UNSURE: which actor's target_unit_index this reads (controller vs the
                // outer actor_index) was not independently re-verified with objdump for
                // this call site; controller is used because it is the actor the preceding
                // condition is actually testing.
                actor_forward_target_object_reference(unit->actor_index, actor_index);
            }
        }
    } else {
        actor_queue_search_position(actor_index, 0, 6, (real_vector3d *)&target->look_point,
                                    0xffffffff, 0, 90, target_prop_index, 150, 0);
    }

    actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);

    if (self->awareness_level > 1 && self->vocalization_line < 8 &&
        (self->mode != 11 || self->mode_data.raw[3] != 0)) {
        int16_t recent = self->vocalization_unknown_3e8;

        prop *validated = (prop *)datum_get(target_prop_index, prop_data);
        if (validated != 0) {
            if ((validated->is_unit == 0 && validated->is_vault == 0) ||
                (validated->is_vault != 0 && self->awareness_level > 2)) {
                if (recent > 6) {
                    return;
                }
                if (validated->is_parented == 0 && validated->last_selected_tick != -1 &&
                    (int32_t)game_time->game_time < validated->last_selected_tick + 600) {
                    return;
                }
                validated->last_selected_tick = (int32_t)game_time->game_time;
                validated->priority_weight_spent = (validated->priority_weight_spent <= validated->priority_weight)
                                             ? validated->priority_weight
                                             : validated->priority_weight_spent;
            }

            {
                float wait_scale = (self->awareness_level < 3 || self->alert_level == 0) ? 1.8f : 0.9f;

                if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
                    float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
                    float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
                    wait_scale = random_real_range(min_scale, max_scale) * wait_scale;
                }

                int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f); // ROUND
                if (ticks > 0x7fff) {
                    ticks = 0x7fff;
                }

                self->vocalization_state = (int16_t)ticks;
                self->vocalization_variant = actor_dialogue_variant_table_f[self->alert_level >= 4];
                self->vocalization_line = 7;
                self->vocalization_unknown_54c = 1; // kind = 1 (explicit target)
                self->vocalization_unknown_550 = target_prop_index;
                self->vocalization_unknown_554 = 0;
                self->vocalization_unknown_558 = 0;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x422ec0):

void FUN_00422ec0(uint param_1)

{
  undefined4 uVar1;
  short sVar2;
  short sVar3;
  undefined2 uVar4;
  uint uVar5;
  char cVar6;
  uint in_EAX;
  int iVar7;
  int iVar8;
  int iVar9;
  float fVar10;
  float local_1c;
  float local_18;
  float local_14;
  undefined4 local_10;
  undefined4 local_8;
  undefined4 local_4;

  iVar7 = (param_1 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (*(char *)(iVar7 + 0x60) == '\0') {
    iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar7 + 0x18) & 0xffff) * 0xc);
    FUN_004221f0();
    uVar5 = *(uint *)(iVar7 + 0x218);
    if (uVar5 == 0xffffffff) {
      uVar5 = *(uint *)(iVar7 + 500);
      if ((uVar5 != 0xffffffff) &&
         (3 < *(short *)((uVar5 & 0xffff) * 0x724 + 0x6e + *(int *)(DAT_00880360 + 0x34)))) {
        FUN_00428420();
      }
    }
    else {
      iVar7 = (uVar5 & 0xffff) * 0x200;
      if (((*(int *)(iVar7 + 0x40 + *(int *)(DAT_0087a480 + 0x34)) != -1) &&
          (*(int *)(DAT_006f1d6c + 0xc) <=
           *(int *)(iVar7 + *(int *)(DAT_0087a480 + 0x34) + 0x44) + 0x5a)) &&
         (cVar6 = FUN_0045bd50(), cVar6 != '\0')) {
        FUN_0041f7d0();
      }
    }
  }
  else {
    FUN_00421af0(0xffffffff,0,0x5a,param_1,0x96,0);
  }
  iVar9 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar2 = *(short *)(iVar9 + 0x6a);
  iVar7 = *(int *)((*(uint *)(iVar9 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_10 = CONCAT22(local_10._2_2_,1);
  if ((((1 < sVar2) && (*(short *)(iVar9 + 0x544) < 8)) &&
      ((sVar3 = *(short *)(iVar9 + 1000), *(short *)(iVar9 + 0x6c) != 0xb ||
       (*(char *)(iVar9 + 0x9f) != '\0')))) && (iVar8 = datum_get(), iVar8 != 0)) {
    if (((*(char *)(iVar8 + 0x60) == '\0') && (*(char *)(iVar8 + 0x127) == '\0')) ||
       ((*(char *)(iVar8 + 0x127) != '\0' && (2 < sVar2)))) {
      if (6 < sVar3) {
        return;
      }
      if (((*(char *)(iVar8 + 0x12e) == '\0') && (*(int *)(iVar8 + 0x5c) != -1)) &&
         (*(int *)(DAT_006f1d6c + 0xc) < *(int *)(iVar8 + 0x5c) + 600)) {
        return;
      }
      *(int *)(iVar8 + 0x5c) = *(int *)(DAT_006f1d6c + 0xc);
      if (*(float *)(iVar8 + 0x58) <= *(float *)(iVar8 + 0x54)) {
        uVar1 = *(undefined4 *)(iVar8 + 0x54);
      }
      else {
        uVar1 = *(undefined4 *)(iVar8 + 0x58);
      }
      *(undefined4 *)(iVar8 + 0x58) = uVar1;
    }
    local_1c = 0.9;
    if ((*(short *)(iVar9 + 0x6a) < 3) || (*(short *)(iVar9 + 0x6e) == 0)) {
      local_1c = 1.8;
    }
    if ((*(float *)(iVar7 + 0xd4) != 0.0) || (*(float *)(iVar7 + 0xd8) != 0.0)) {
      if (*(float *)(iVar7 + 0xd4) <= 0.5) {
        local_14 = 0.5;
      }
      else {
        local_14 = *(float *)(iVar7 + 0xd4);
      }
      if (*(float *)(iVar7 + 0xd8) <= 2.0) {
        local_18 = *(float *)(iVar7 + 0xd8);
      }
      else {
        local_18 = 2.0;
      }
      fVar10 = random_real_range(local_14,local_18);
      local_1c = fVar10 * local_1c;
    }
    iVar7 = (int)ROUND(local_1c * 30.0);
    if (0x7fff < iVar7) {
      iVar7 = 0x7fff;
    }
    uVar4 = *(undefined2 *)(&DAT_00655644 + (uint)(3 < *(short *)(iVar9 + 0x6e)) * 2);
    *(short *)(iVar9 + 0x548) = (short)iVar7;
    *(undefined2 *)(iVar9 + 0x546) = uVar4;
    *(undefined2 *)(iVar9 + 0x544) = 7;
    *(undefined4 *)(iVar9 + 0x54c) = local_10;
    *(uint *)(iVar9 + 0x550) = param_1;
    *(undefined4 *)(iVar9 + 0x554) = local_8;
    *(undefined4 *)(iVar9 + 0x558) = local_4;
  }
  return;
}

Ground truth from objdump (bin/halo.exe @ 0x422ec0..0x422ff1) confirming EAX = actor_index
throughout (not param_1) and the exact register setup for every truncated call; see file
header.
#endif
