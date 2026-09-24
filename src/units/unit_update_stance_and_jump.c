// unit_update_stance_and_jump  (Ghidra: FUN_00566de0)
// address 0x566de0, size 1548 bytes
// name confidence: 0.45 (symbols/review_queue.txt candidate; matches functions.md's summary:
//   "Decides and applies a unit's stand/crouch/jump/land animation-state transition based on
//   its current motion and grounded state")   rewrite confidence: 0.3
// evidence: types/units.h unit_data.animation_state (0x2a3, unit_animation_state_unknown_17 is
//   explicitly attributed to this function), .overlays[2] (0x2b2, the third overlay slot this
//   function owns per the header's "0x566de0 owns slot 2"), .animation_state_flags (0x298),
//   .unknown_28c (0x28c); types/objects.h object.current_shield_damage/current_body_damage
//   (0xe8/0xec), object.vitality_flags (0x106), object.parent_object (0x11c), object.type
//   (0xb4), object.animation_frame (0xd2); types/tags.h Unit.soft_ping_threshold/
//   hard_ping_threshold/hard_death_threshold (0x218/0x220/0x228), Unit.soft_ping_interrupt_ticks/
//   hard_ping_interrupt_ticks (0x2c8/0x2ca) -- reused here as generic short thresholds rather
//   than for network prediction, Unit.unit_flags (0x17c, don_t_reface_during_pings = 0x200),
//   Unit.animation_graph (Object.animation_graph.tag_id at 0x44), ModelAnimations.unit_damage
//   (0x3c count / 0x40 pointer) and .animations (0x74 count / 0x78 pointer),
//   ModelAnimationsAnimation.frame_count (0x22) and .main_animation_index (0x42), Biped.biped_flags
//   (has_no_dying_airborne = 0x400, verified via object.type == biped gate).
// register convention: EAX, ECX, EDX, EBX, ESI, EDI, then stack (Ghidra recovered 6 register
//   params and 4 stack params for this callee).
//   // blam-cc: param_1 (EAX) -> unit_index, param_2 (ECX) -> force_ready,
//   //   param_3 (EDX) -> allow_death_reaction, param_4 (EBX) -> suppress_shield_check,
//   //   param_5 (ESI) -> ignore_disoriented, param_6 (EDI) -> force_reaction,
//   //   param_7 (stack) -> turn_angle, param_8 (stack) -> weapon_class_index,
//   //   param_9 (stack) -> fire_trigger_event, param_10 (stack) -> require_still
// UNSURE: parameter names beyond param_1/unit_index are inferred from how each byte gates the
//   branches below; only two of the five call sites in the whole binary pass anything other
//   than a mix of 0/-1 defaults, so the true intent of several flags (especially
//   suppress_shield_check, ignore_disoriented, force_reaction) is not independently confirmed.
// UNSURE: three callee calls in this function reference registers this decompilation could not
//   trace to a write in the caller (unit_animation_state_is_compatible's ECX/DX pair, and the DX weapon-class the
//   `unit_get_weapon_object_index` call implies). They are modelled with the most plausible
//   live value at that point (the unit's tag data pointer / current weapon slot) and flagged
//   inline; a wrong guess here only affects which weapon-holster gate fires, not the animation
//   state or overlay writes that are this function's main effect.
// UNSURE: param_7 is typed `float` by Ghidra (it is compared with ABS() against radian
//   constants to bucket a turn direction into 0..3), but after that bucketing the same stack
//   slot is reused by the compiler to hold a small integer "transition class" (1..3) that feeds
//   `transition_class * 0x2c + weapon_class_index` further down. That reuse is modelled here as
//   two separate locals (`turn_angle`, read-only, and `transition_class`, write-only) instead of
//   reinterpreting one float parameter's bytes, which is behaviourally identical and far clearer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t network_predicted_state_flag; // 0x006f1d20, tested != 0 elsewhere in this module
extern int32_t game_connection_role;         // 0x00719720, 1 = client, 2 = server
extern char *s_stand; // 0x0069fdec "stand"
extern double fabs(double x); // ABS() is a single x87 FABS instruction

extern int32_t random_int_range(int16_t min, int16_t max);                              // 0x405320
extern uint32_t weapon_must_be_readied(uint32_t weapon_object_index);                              // 0x4c2ea0, UNSURE signature
extern int32_t animation_choose_random_permutation(int32_t mode);                                               // 0x4d6280, EAX=unit_index implicit
extern void object_delete_teardown(uint32_t object_index);                               // 0x4edc80, EAX=object_index implicit
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask);         // 0x4f6ec0
extern void object_copy_default_node_transforms(uint32_t unit_index);                                           // 0x4f6b70, EAX=unit_index implicit  // real signature (object_copy_default_node_transforms.c): void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); Ghidra recovered 1 of 2 args at this call site
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label, uint8_t test_only); // 0x5651e0
extern uint8_t unit_animation_state_is_compatible(unit_data *unit, int16_t requested_state);                       // 0x565be0, UNSURE registers
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state);      // 0x565f90
extern int32_t unit_pick_random_spawned_actor_count(uint32_t unit_index);                                        // 0x568540, EDI=unit_index implicit
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970
extern char *unit_get_current_weapon_label(uint32_t unit_index, uint8_t param_2);         // 0x56dfd0  // real signature (unit_get_current_weapon_label.c): char * unit_get_current_weapon_label(uint32_t unit_index); Ghidra recovered 2 of 1 args at this call site
extern void unit_release_thrown_grenade(uint32_t object_index, uint8_t apply_throw_fraction);            // 0x56e440
extern void unit_set_custom_animation(TagID animation_graph_tag, int16_t animation_index); // 0x56ebd0, EAX=unit_index implicit  // real signature (unit_set_custom_animation.c): void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); Ghidra recovered 2 of 3 args at this call site
extern void unit_set_throw_aim_direction(uint32_t unit_index);                                            // 0x5704d0, EAX=unit_index implicit  // real signature (unit_set_throw_aim_direction.c): void unit_set_throw_aim_direction(uint32_t object_index, float direction_x, float direction_y); Ghidra recovered 1 of 3 args at this call site

void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction,
                                 uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction,
                                 float turn_angle, int16_t weapon_class_index, int32_t fire_trigger_event,
                                 uint8_t require_still)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    uint8_t should_stand;   // bVar5
    uint8_t is_dead_or_stunned; // bVar10
    int16_t facing_quadrant;    // sVar8

    if (!force_ready) {
        if (!allow_death_reaction) {
            if ((unit_tag->soft_ping_threshold < obj->current_body_damage) ||
                (unit_tag->soft_ping_threshold < obj->current_shield_damage)) {
                should_stand = 1;
            } else {
                should_stand = 0;
            }
            is_dead_or_stunned = unit_tag->hard_ping_threshold < obj->current_body_damage;
            if ((!ignore_disoriented) && (-1 < (int8_t)unit->flags)) {
                goto after_early_flags;
            }
        } else {
            force_ready = 1;
            should_stand = 1;
        }
        is_dead_or_stunned = 0;
    } else {
        allow_death_reaction = 0;
        should_stand = 1;
        if ((unit_tag->hard_death_threshold <= 0.0f) ||
            (obj->current_body_damage <= unit_tag->hard_death_threshold)) {
            is_dead_or_stunned = 0;
            goto after_early_flags;
        }
        is_dead_or_stunned = 1;
    }
after_early_flags:
    if (force_reaction) {
        should_stand = 1;
        is_dead_or_stunned = 1;
    }
    if (weapon_class_index == -1) {
        weapon_class_index = 0;
    }

    if (0.7853982f <= (float)fabs((double)turn_angle)) {
        if ((float)fabs((double)turn_angle) <= 2.159845f) {
            facing_quadrant = 1;
            if (turn_angle <= 0.0f) {
                facing_quadrant = 2;
            }
        } else {
            facing_quadrant = 0;
        }
    } else {
        facing_quadrant = 3;
    }

    if ((network_predicted_state_flag != 0) && (weapon_class_index == 2) && is_dead_or_stunned && force_ready) {
        facing_quadrant = 1;
    }
    if (require_still && !should_stand && !force_ready) {
        return;
    }

    ModelAnimations *graph = (ModelAnimations *)tag_instances[unit_tag->base.animation_graph.tag_id.index & 0xffff].data;

    if ((!is_dead_or_stunned) && (!force_ready)) {
        // fallback: refresh the third overlay slot (0x2b2/0x2b4) with a fresh idle animation
        // when the one it is holding has run past the soft_ping_threshold-reused frame limit.
        if ((unit->overlays[2].animation_index != -1) &&
            (unit->overlays[2].frame <= unit_tag->soft_ping_interrupt_ticks)) {
            return;
        }
        int32_t new_overlay = animation_choose_random_permutation(1);
        if (new_overlay == -1) {
            return;
        }
        unit->overlays[2].animation_index = (int16_t)new_overlay;
        unit->overlays[2].frame = 0;
        return;
    }

    int8_t new_state = force_ready ? 0x19 : 0x17;
    int16_t transition_class;
    if (!force_ready) {
        transition_class = 1;
        // UNSURE: the original decompilation shows this call with no visible arguments; ECX and
        // DX are presumed to carry the unit's tag data and its current animation_state.
        uint32_t compatible = unit_animation_state_is_compatible((unit_data *)unit_tag, unit->animation_state);
        if (compatible != 0) goto class_assigned;
        should_stand = 0;
    } else {
        transition_class = (int16_t)is_dead_or_stunned + 2;
class_assigned:
        should_stand = 1;
    }

    if ((unit->animation_state == 0x17) && (unit_tag->hard_ping_interrupt_ticks < obj->animation_frame)) {
        should_stand = 1;
    }
    if (!force_ready) {
        if (obj->vitality_flags & _object_health_frozen_bit) {
            should_stand = 0;
        }
        if (obj->parent_object != k_datum_index_none) {
            return;
        }
    }
    if (!should_stand) {
        return;
    }

    if (force_ready) {
        char *weapon_label = unit_get_current_weapon_label(unit_index, 1);
        // UNSURE: unit_index and the test_only flag are the implicit EAX/(4th) arguments of
        // unit_set_or_test_seat_and_weapon_label; only the two stack arguments are visible here.
        unit_set_or_test_seat_and_weapon_label(unit_index, s_stand, weapon_label, 0);
    }

    if ((new_state == 0x19) && (obj->type == _object_type_biped) &&
        (((biped_data *)((uint8_t *)obj + k_unit_object_size))->flags & 0x1) &&
        ((((Biped *)unit_tag)->biped_flags & 0x400) == 0)) {
        new_state = 0x18;
        if (unit_try_set_animation_state(unit_index, 0x18) != 0) {
            goto fire_trigger;
        }
    }

    int32_t chosen_animation = animation_choose_random_permutation(1);
    int16_t chosen_animation_index = (int16_t)chosen_animation;
    if (chosen_animation_index == -1) {
        if (force_ready) {
            unit->animation_state_flags = (unit->animation_state_flags & 0xfff7) | 4;
            if (unit_tag->base.flags & 0x2) {
                object_delete_teardown(unit_index);
                unit_pick_random_spawned_actor_count(unit_index);
            }
        }
        goto fire_trigger;
    }

    if (unit->animation_state == 0x21) {
        unit_release_thrown_grenade(unit_index, 1);
    }
    object_copy_default_node_transforms(unit_index);
    unit->animation_state = new_state;
    unit_set_custom_animation(unit_tag->base.animation_graph.tag_id, chosen_animation_index);
    unit->animation_state_flags = (uint16_t)((uint8_t)unit->animation_state_flags | 1) | (unit->animation_state_flags & 0xff00);

    if (force_ready) {
        if ((!suppress_shield_check) && (!allow_death_reaction)) {
            if (game_connection_role != 0) {
                // UNSURE: unit_get_weapon_object_index's slot argument (CX) is not visible in the
                // decompilation; the current weapon slot is the only value that makes sense here.
                datum_index weapon_index = unit_get_weapon_object_index(unit_index, unit->current_weapon_index);
                object *weapon_obj = object_try_and_get(weapon_index, _object_mask_weapon);
                if ((weapon_obj != (object *)0) && (weapon_must_be_readied(weapon_index) == 1)) {
                    goto clear_overlay_timer;
                }
            }
            ModelAnimationsAnimation *anim_array = (ModelAnimationsAnimation *)graph->animations.pointer;
            int16_t frame_count = anim_array[chosen_animation_index].frame_count;
            // UNSURE: random_int_range (0x405320) takes its "max" on the stack and its "min" in
            // an ECX this decompilation never shows being set at this call site (see the file
            // header). Only the low 16 bits of the caller's packed CONCAT22 expression reach the
            // callee's stack slot, which is reproduced exactly here as max_frames; min_frames is
            // left at the callee's own implicit-register default (modelled as 0).
            int16_t max_frames = (int16_t)((uint16_t)(frame_count >> 1) + (uint16_t)(frame_count >> 2));
            int8_t roll = (int8_t)random_int_range(0, max_frames);
            unit->unknown_28c = roll;
            if (roll < 2) {
                roll = 1;
            }
            unit->unknown_28c = roll;
        } else {
clear_overlay_timer:
            unit->unknown_28c = 0;
        }
    }

    if (facing_quadrant != 0) {
        int32_t damage_index = transition_class * 0x2c + (int32_t)weapon_class_index;
        int16_t mapped_animation;
        if ((damage_index < 0) || (graph->unit_damage.count <= (uint32_t)damage_index)) {
            mapped_animation = -1;
        } else {
            mapped_animation = ((int16_t *)graph->unit_damage.pointer)[damage_index];
        }
        ModelAnimationsAnimation *anim_array2 = (ModelAnimationsAnimation *)graph->animations.pointer;
        int16_t main_animation_index = anim_array2[chosen_animation_index].main_animation_index;
        if (main_animation_index == mapped_animation) {
            facing_quadrant = 0;
        }
    }
    if (force_ready) {
        if (facing_quadrant == 3) {
            unit->animation_state_flags |= 0x8;
        } else {
            unit->animation_state_flags &= 0xfff7;
        }
    }

fire_trigger:
    if ((fire_trigger_event != 0) && ((unit_tag->unit_flags & 0x200) == 0) && (obj->type == _object_type_biped) &&
        (obj->parent_object == k_datum_index_none) && (is_dead_or_stunned || force_ready)) {
        switch (facing_quadrant) {
        case 0:
            unit_set_throw_aim_direction(unit_index);
            return;
        case 1:
            unit_set_throw_aim_direction(unit_index);
            return;
        case 2:
            break;
        case 3:
            break;
        }
        unit_set_throw_aim_direction(unit_index);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x566de0):

void FUN_00566de0(uint param_1,char param_2,char param_3,char param_4,char param_5,char param_6,
                 float param_7,short param_8,int param_9,char param_10)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  bool bVar5;
  char cVar6;
  char cVar7;
  short sVar8;
  undefined4 uVar9;
  bool bVar10;
  short sVar11;
  short sVar12;
  int iVar13;

  iVar13 = (param_1 & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar13);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_2 == '\0') {
    if (param_3 == '\0') {
      if ((*(float *)(iVar2 + 0x218) < (float)puVar1[0x3b]) ||
         (*(float *)(iVar2 + 0x218) < (float)puVar1[0x3a])) {
        bVar5 = true;
      }
      else {
        bVar5 = false;
      }
      bVar10 = *(float *)(iVar2 + 0x220) < (float)puVar1[0x3b];
      if ((param_5 == '\0') && (-1 < (char)puVar1[0x81])) goto LAB_00566eca;
    }
    else {
      param_2 = '\x01';
      bVar5 = true;
    }
LAB_00566ec4:
    bVar10 = false;
  }
  else {
    param_3 = '\0';
    bVar5 = true;
    if ((*(float *)(iVar2 + 0x228) <= 0.0) || ((float)puVar1[0x3b] <= *(float *)(iVar2 + 0x228)))
    goto LAB_00566ec4;
    bVar10 = true;
  }
LAB_00566eca:
  if (param_6 != '\0') {
    bVar5 = true;
    bVar10 = true;
  }
  if (param_8 == -1) {
    param_8 = 0;
  }
  if (0.7853982 <= ABS(param_7)) {
    if (ABS(param_7) <= 2.159845) {
      sVar8 = 1;
      if (param_7 <= 0.0) {
        sVar8 = 2;
      }
    }
    else {
      sVar8 = 0;
    }
  }
  else {
    sVar8 = 3;
  }
  if ((((DAT_006f1d20 != 0) && (param_8 == 2)) && (bVar10)) && (param_2 != '\0')) {
    sVar8 = 1;
  }
  if (((param_10 != '\0') && (!bVar5)) && (param_2 == '\0')) {
    return;
  }
  iVar3 = *(int *)((*(uint *)(iVar2 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((!bVar10) && (param_2 == '\0')) {
    if ((*(short *)((int)puVar1 + 0x2b2) != -1) &&
       ((short)puVar1[0xad] <= *(short *)(iVar2 + 0x2c8))) {
      return;
    }
    sVar8 = FUN_004d6280(1);
    if (sVar8 == -1) {
      return;
    }
    *(short *)((int)puVar1 + 0x2b2) = sVar8;
    *(undefined2 *)(puVar1 + 0xad) = 0;
    return;
  }
  cVar7 = (param_2 != '\0') * '\x02' + '\x17';
  if (param_2 == '\0') {
    param_7._0_2_ = 1;
    cVar6 = FUN_00565be0();
    sVar12 = 1;
    if (cVar6 != '\0') goto LAB_00567031;
    bVar5 = false;
  }
  else {
    sVar12 = bVar10 + 2;
LAB_00567031:
    param_7._0_2_ = sVar12;
    bVar5 = true;
  }
  if ((*(char *)((int)puVar1 + 0x2a3) == '\x17') &&
     (*(short *)(iVar2 + 0x2ca) < *(short *)((int)puVar1 + 0xd2))) {
    bVar5 = true;
  }
  if (param_2 == '\0') {
    if ((*(byte *)((int)puVar1 + 0x106) & 4) != 0) {
      bVar5 = false;
    }
    if (puVar1[0x47] != 0xffffffff) {
      return;
    }
  }
  if (!bVar5) {
    return;
  }
  if (param_2 != '\0') {
    uVar9 = unit_get_current_weapon_label(1);
    unit_set_or_test_seat_and_weapon_label(PTR_s_stand_0069fdec,uVar9);
  }
  if ((((cVar7 == '\x19') && ((short)puVar1[0x2d] == 0)) &&
      (puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar13), (puVar4[0x133] & 1) != 0))
     && ((*(uint *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 0x400) == 0
        )) {
    cVar7 = '\x18';
    cVar6 = unit_try_set_animation_state(param_1,0x18);
    if (cVar6 != '\0') goto LAB_00567314;
  }
  uVar9 = FUN_004d6280(1);
  sVar12 = (short)uVar9;
  if (sVar12 == -1) {
    if ((param_2 != '\0') &&
       (*(ushort *)(puVar1 + 0xa6) = (ushort)puVar1[0xa6] & 0xfff7 | 4,
       (*(byte *)(iVar2 + 0x17c) & 2) != 0)) {
      object_delete_teardown();
      FUN_00568540();
    }
    goto LAB_00567314;
  }
  if (*(char *)((int)puVar1 + 0x2a3) == '!') {
    unit_release_thrown_grenade(param_1,1);
  }
  FUN_004f6b70();
  *(char *)((int)puVar1 + 0x2a3) = cVar7;
  unit_set_custom_animation(*(undefined4 *)(iVar2 + 0x44),uVar9);
  *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] | 1;
  if (param_2 != '\0') {
    if ((param_4 == '\0') && (param_3 == '\0')) {
      if (DAT_00719720 != 0) {
        unit_get_weapon_object_index();
        iVar13 = object_try_and_get(4);
        if ((iVar13 != 0) && (cVar7 = FUN_004c2ea0(), cVar7 == '\x01')) goto LAB_00567261;
      }
      sVar11 = *(short *)(sVar12 * 0xb4 + *(int *)(iVar3 + 0x78) + 0x22);
      cVar7 = random_int_range(CONCAT22(sVar11 >> 0xf,sVar11 >> 1) + (uint)(ushort)(sVar11 >> 2));
      *(char *)(puVar1 + 0xa3) = cVar7;
      if (cVar7 < '\x02') {
        cVar7 = '\x01';
      }
      *(char *)(puVar1 + 0xa3) = cVar7;
    }
    else {
LAB_00567261:
      *(undefined1 *)(puVar1 + 0xa3) = 0;
    }
  }
  if (sVar8 != 0) {
    iVar13 = param_7._0_2_ * 0x2c + (int)param_8;
    if ((iVar13 < 0) || (*(int *)(iVar3 + 0x3c) <= iVar13)) {
      sVar11 = -1;
    }
    else {
      sVar11 = *(short *)(*(int *)(iVar3 + 0x40) + iVar13 * 2);
    }
    if (*(short *)(sVar12 * 0xb4 + *(int *)(iVar3 + 0x78) + 0x42) == sVar11) {
      sVar8 = 0;
    }
  }
  if (param_2 != '\0') {
    if (sVar8 == 3) {
      *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] | 8;
    }
    else {
      *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] & 0xf7;
    }
  }
LAB_00567314:
  if ((((param_9 != 0) && ((*(uint *)(iVar2 + 0x17c) & 0x200) == 0)) && ((short)puVar1[0x2d] == 0))
     && ((puVar1[0x47] == 0xffffffff && ((bVar10 || (param_2 != '\0')))))) {
    switch(sVar8) {
    case 0:
      FUN_005704d0();
      return;
    case 1:
      FUN_005704d0();
      return;
    case 2:
      break;
    case 3:
    }
    FUN_005704d0();
  }
  return;
}
#endif
