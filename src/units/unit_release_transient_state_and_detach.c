// unit_release_transient_state_and_detach  (Ghidra: FUN_00568cb0)
// address 0x568cb0, size 671 bytes
// name confidence: 0.4 (functions.md: "Performs the transient-state cleanup used when a unit
//   fully leaves its seat, also clearing its overlay-animation and weapon-switch fields")
// rewrite confidence: 0.3
// evidence: same fields as unit_release_transient_state.c (0x568610), which this function is
//   almost a byte-for-byte twin of, except it calls unit_detach_from_seat directly to perform the actual
//   seat detach instead of inlining the exit-marker/transform sequence.
// register convention: unit index in param_1, a flag in param_2.
//   // blam-cc: param_1 -> unit_index, param_2 -> is_light_reset (UNSURE name)
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)
extern random_seed random_seed_global;   // 0x00719cd0

extern void actor_attempt_grenade_throw(uint32_t actor_index);          // 0x428ab0
extern void actor_release_from_cluster_or_delete(uint32_t unit_index);                          // 0x428e50, UNSURE signature
extern void player_reset_after_unit_change(uint32_t controlling_player);                  // 0x474e10, UNSURE signature
extern float transition_function_evaluate(int32_t param_1);             // 0x4ccac0, UNSURE signature  // real signature (transition_function_evaluate.c): real transition_function_evaluate(transition_function_t type, real phase); Ghidra recovered 1 of 2 args at this call site
extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event); // 0x56c640, UNSURE signature
extern void unit_detach_reposition_and_nudge(uint32_t unit_index);                          // 0x56ca40, UNSURE signature
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern void unit_drop_object_from_hand(uint32_t unit_index, uint32_t dropped_object_index); // 0x56ed00
extern void unit_drop_inventory_weapons(uint32_t unit_index);           // 0x56f060

void unit_release_transient_state_and_detach(uint32_t unit_index, uint8_t is_light_reset) // blam-cc: param_1, param_2
{
    object *self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)self_obj + k_unit_data_offset);

    if (!is_light_reset) {
        unit->unknown_420 = 0;
        if (unit->controlling_player != k_datum_index_none) {
            player_reset_after_unit_change(unit->controlling_player);
            unit->controlling_player = k_datum_index_none;
        }
        if (unit->actor_index != k_datum_index_none) {
            uint8_t *actor_rec = (uint8_t *)actor_data->data + (unit->actor_index & 0xffff) * 0x724;
            *(int16_t *)((uint8_t *)unit + 0x334) = *(int16_t *)(actor_rec + 0x34);
            *(int16_t *)((uint8_t *)self_obj + 0x336) = *(int16_t *)(actor_rec + 0x3a);
            actor_attempt_grenade_throw(unit->actor_index);
            unit->actor_index = k_datum_index_none;
        }
        if (unit->swarm_actor_index != k_datum_index_none) {
            uint8_t *actor_rec = (uint8_t *)actor_data->data + (unit->swarm_actor_index & 0xffff) * 0x724;
            *(int16_t *)((uint8_t *)unit + 0x334) = *(int16_t *)(actor_rec + 0x34);
            *(int16_t *)((uint8_t *)self_obj + 0x336) = *(int16_t *)(actor_rec + 0x3a);
            actor_release_from_cluster_or_delete(unit_index);
            unit->swarm_actor_index = k_datum_index_none;
        }
        unit->unknown_41c = game_time->game_time;
    } else {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        Unit *unit_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
        if (unit_tag->feign_repeat_chance <= (float)(random_seed_global >> 16) * 1.5259022e-05f) {
            unit->flags &= 0xffffdfff;
        } else {
            unit->flags |= 0x2000;
        }
    }

    unit->flags &= 0xffffffee;
    unit->control_flags = 0;
    if (unit->current_weapon_index != -1) {
        int16_t slot = *(int16_t *)((uint8_t *)self_obj + 0x2f2);
        uint32_t weapon_object_index = 0xffffffff;
        if (slot != -1) {
            weapon_object_index = unit->weapons[slot];
        }
        object *weapon_obj = ((object_header *)object_data->data)[weapon_object_index & 0xffff].data;
        *(int16_t *)((uint8_t *)weapon_obj + 0x230) = 0;
        *(float *)((uint8_t *)weapon_obj + 0x234) = transition_function_evaluate(0);
    }
    unit->flags &= 0xfdffffff; // clears _unit_flag_idle_turn_seeded (0x02000000)

    if (self_obj->parent_object != k_datum_index_none) {
        if (unit->vehicle_seat_index == -1) {
            unit_detach_reposition_and_nudge(unit_index);
        } else {
            unit_detach_from_seat(unit_index, 0, 1, 0);
        }
    }

    unit->pending_speech.priority = 0;
    unit_drop_inventory_weapons(unit_index);
    if (unit->equipment_object_index != k_datum_index_none) {
        unit_drop_object_from_hand(unit_index, unit->equipment_object_index);
        unit->equipment_object_index = k_datum_index_none;
    }
    if (unit->unknown_28c == 0) {
        unit_drop_current_weapon(unit_index, 1);
    }
    unit->overlays[1].animation_index = -1;
    unit->overlays[0].animation_index = -1;
    unit->melee_state = 0;
    if (unit->throwing_grenade_state == 1) {
        unit->throwing_grenade_state = 0;
    }
    return;
}

#if 0
Original Ghidra decompilation (0x568cb0):

void FUN_00568cb0(uint param_1,char param_2)

{
  uint *puVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  float10 fVar7;

  iVar6 = (param_1 & 0xffff) * 0xc;
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  if (param_2 == '\0') {
    *(undefined2 *)(puVar3 + 0x108) = 0;
    if (puVar3[0x86] != 0xffffffff) {
      FUN_00474e10(puVar3[0x86]);
      puVar3[0x86] = 0xffffffff;
    }
    uVar5 = puVar3[0x7d];
    if (uVar5 != 0xffffffff) {
      iVar4 = (uVar5 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      *(undefined2 *)(puVar3 + 0xcd) = *(undefined2 *)(iVar4 + 0x34);
      *(undefined2 *)((int)puVar3 + 0x336) = *(undefined2 *)(iVar4 + 0x3a);
      actor_attempt_grenade_throw(uVar5);
      puVar3[0x7d] = 0xffffffff;
    }
    if (puVar3[0x7e] != 0xffffffff) {
      iVar4 = (puVar3[0x7e] & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      *(undefined2 *)(puVar3 + 0xcd) = *(undefined2 *)(iVar4 + 0x34);
      *(undefined2 *)((int)puVar3 + 0x336) = *(undefined2 *)(iVar4 + 0x3a);
      FUN_00428e50(param_1);
      puVar3[0x7e] = 0xffffffff;
    }
    puVar3[0x107] = *(uint *)(DAT_006f1d6c + 0xc);
  }
  else {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    if (*(float *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x248) <=
        (float)(random_seed_global >> 0x10) * 1.5259022e-05) {
      puVar3[0x81] = puVar3[0x81] & 0xffffdfff;
    }
    else {
      puVar3[0x81] = puVar3[0x81] | 0x2000;
    }
  }
  puVar3[0x81] = puVar3[0x81] & 0xffffffee;
  puVar3[0x82] = 0;
  if (*(short *)((int)puVar3 + 0x2f2) != -1) {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
    sVar2 = *(short *)(iVar4 + 0x2f2);
    uVar5 = 0xffffffff;
    if (sVar2 != -1) {
      uVar5 = *(uint *)(iVar4 + 0x2f8 + sVar2 * 4);
    }
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc);
    *(undefined2 *)(iVar4 + 0x230) = 0;
    fVar7 = (float10)transition_function_evaluate(0);
    *(float *)(iVar4 + 0x234) = (float)fVar7;
  }
  puVar1 = (uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6) + 0x204);
  *puVar1 = *puVar1 & 0xfdffffff;
  if (puVar3[0x47] != 0xffffffff) {
    if ((short)puVar3[0xbc] == -1) {
      FUN_0056ca40();
    }
    else {
      FUN_0056c640(param_1,0,1,0);
    }
  }
  *(undefined2 *)(puVar3 + 0xee) = 0;
  unit_drop_inventory_weapons(param_1);
  iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  iVar4 = *(int *)(iVar6 + 0x318);
  if (iVar4 != -1) {
    unit_drop_object_from_hand(param_1,iVar4);
    *(undefined4 *)(iVar6 + 0x318) = 0xffffffff;
  }
  if ((char)puVar3[0xa3] == '\0') {
    unit_drop_current_weapon(param_1,1);
  }
  *(undefined2 *)((int)puVar3 + 0x2ae) = 0xffff;
  *(undefined2 *)((int)puVar3 + 0x2aa) = 0xffff;
  *(undefined1 *)((int)puVar3 + 0x289) = 0;
  if (*(char *)((int)puVar3 + 0x28d) == '\x01') {
    *(undefined1 *)((int)puVar3 + 0x28d) = 0;
  }
  return;
}
#endif
