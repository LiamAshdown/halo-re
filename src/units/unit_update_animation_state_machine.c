// unit_update_animation_state_machine  (Ghidra: unit_update_animation_state_machine)
// address 0x565420, size 1133 bytes
// name confidence: 0.45 (phase2 candidate)   rewrite confidence: 0.25
// evidence: types/units.h unit_data.seat_command (0x2a6), .unknown_20f (0x20f), .control_flags
//   (0x208, _unit_control_flag_force_alert), .unknown_28b (0x28b), .base_animation_state
//   (0x2a7), .overlays[3] (0x2aa/0x2ae/0x2b2), .animation_state (0x2a3), .unknown_2a4 (0x2a4),
//   .unknown_2a5 (0x2a5), .driver_unit_index (0x324), .vehicle_seat_index (0x2f0);
//   types/objects.h object.parent_object (0x11c), .vitality_flags (0x106,
//   _object_health_frozen_bit), .flags (0x10, _object_at_rest_bit), .type (0xb4), .animation_index
//   (0xd0), .animation_frame (0xd2), .velocity (0x68), .definition_tag (0x000), Object.model
//   (TagDependency at 0x28, tag_id at 0x34); types/tags.h Unit.unit_flags (tag+0x17c,
//   "destroyed_after_dying" bit 0x2), Unit.seats (TagReflexive at 0x2e4/0x2e8, UnitSeat stride
//   0x11c, UnitSeatFlags bit 0 "invisible"), Biped.biped_flags (tag+0x2f4, "has_no_dying_airborne"
//   bit 0x400); types/units.h biped_data.flags (0x4cc, bit 0 grounded).
// UNSURE: the identity and lifetime of the ECX record is not traced back to any caller in this
//   batch (all 14 callers sit outside the address range assigned here); it is modelled as a
//   2-byte {requested_state, extra_flag} pair passed by pointer.
// UNSURE: the root-motion block (animation_state == 0x1b) calls model_animation_get_frame_delta,
//   object_get_world_matrix and matrix4x3_transform_vector with argument counts Ghidra could not
//   recover (the frame-delta output and the world-matrix pointer are passed through hidden
//   stack/register slots this decompilation does not show). The three locals that receive the
//   transformed delta and get added into object->velocity are preserved structurally but
//   default to zero here rather than inventing a plausible-looking call signature.
// UNSURE: unit_reset_ground_adjust_state, unit_pick_random_spawned_actor_count, unit_notify_weapon_removed_dup, object_set_collision_enabled and unit_reset_light_effect are leaf
//   helpers outside this module's address range; their signatures are guessed from call-site
//   argument counts only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern void *unit_base_animation_state_names[6]; // 0x0069fde4, PTR_DAT_0069fde4

extern void model_animation_get_frame_delta(void *model_data);                          // 0x4d4a00, UNSURE
extern void *object_get_world_matrix(void);                                             // 0x4f6a20, UNSURE
  // real signature (object_get_world_matrix.c): real_matrix4x3 * object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out); Ghidra recovered 0 of 2 args at this call site
// matrix4x3_transform_vector (0x4cbe50) transforms the vector in one register by the matrix in
// another and writes the result through the third; Ghidra binds a different subset at each call
// site in this module, so the declaration is left unprototyped.
extern void matrix4x3_transform_vector();
extern void object_delete_teardown(void);                                                // 0x4edc80, UNSURE: no traced args
  // real signature (object_delete_teardown.c): void object_delete_teardown(uint32_t object_index); Ghidra recovered 0 of 1 args at this call site
extern void object_set_collision_enabled(uint32_t not_invisible);                                        // 0x4f6850, UNSURE
  // real signature (object_set_collision_enabled.c): void object_set_collision_enabled(uint32_t object_index, uint8_t enable); Ghidra recovered 1 of 2 args at this call site
extern void object_copy_default_node_transforms(void);                                                          // 0x4f6b70
  // real signature (object_copy_default_node_transforms.c): void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); Ghidra recovered 0 of 2 args at this call site
extern void unit_reset_ground_adjust_state(uint32_t object_index);                                                          // 0x55ad00, UNSURE: no traced args
extern uint8_t unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state); // 0x565be0, ECX = unit+0xa4
extern uint8_t unit_state_allows_control(const uint8_t *animation_block);             // 0x565ca0, ECX = unit+0xa4
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state);      // 0x565f90
extern void unit_pick_random_spawned_actor_count(void);                                                          // 0x568540, UNSURE: no traced args
  // real signature (unit_pick_random_spawned_actor_count.c): int32_t unit_pick_random_spawned_actor_count(uint32_t unit_index); Ghidra recovered 0 of 1 args at this call site
extern void unit_notify_weapon_removed_dup(void);                                                          // 0x56ab30, UNSURE: no traced args
  // real signature (unit_notify_weapon_removed_dup.c): void unit_notify_weapon_removed_dup(int32_t object_index, int16_t new_state); Ghidra recovered 0 of 2 args at this call site
extern char * unit_get_current_weapon_label(uint32_t unit_index);                                // 0x56dfd0, UNSURE signature
extern void unit_release_thrown_grenade(uint32_t object_index, uint8_t apply_throw_fraction);             // 0x56e440
extern uint16_t unit_reset_light_effect(datum_index effect_index);                                        // 0x56ec10, UNSURE
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label,
                                                     char *weapon_label, uint8_t test_only); // 0x5651e0,
// unit_index in EAX; this matches the definition in unit_set_or_test_seat_and_weapon_label.c.
// The phase-4 review pass corrected the arity (Ghidra binds only the stack arguments at these
// call sites) and the return type (the callee returns a byte, tested in AL).
extern void unit_cause_melee_damage(uint32_t unit_index, uint32_t a2, uint32_t a3, uint32_t a4,
                                     uint32_t a5, uint32_t a6, uint32_t a7);               // UNSURE signature

// FIXED (register inputs, objdump + difftest): the original never reads EAX; unit_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: ECX -> request, stack -> unit_index
uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    int16_t requested_state = request[0]; // UNSURE: see file header
    uint8_t need_retry = 0;
    int32_t skip_animation_request = 0;

    if (obj->parent_object == (datum_index)-1 && (obj->vitality_flags & _object_health_frozen_bit) == 0) {
        int16_t category = -1;
        switch (unit->seat_command) {
        case 0: category = 0; break;
        case 1: case 2: category = 1; break;
        case 3: category = (request[1] != 0) + 2; break;
        case 4: category = 2; break;
        case 5: category = 4; break;
        case 6: category = 5; break;
        default: break;
        }
        if (unit->unknown_20f != -1) {
            category = unit->unknown_20f;
        }
        if ((unit->control_flags & _unit_control_flag_force_alert) != 0) {
            category = 1;
        }
        if (unit->unknown_28b != 0) {
            category = 5;
        }
        if (unit->base_animation_state != category && unit_animation_state_is_compatible((const uint8_t *)unit + 0xa4, category)) {
            char *weapon_label = unit_get_current_weapon_label(1);
            unit_set_or_test_seat_and_weapon_label(unit_index, (char *)unit_base_animation_state_names[category],
                                                    weapon_label, 0);
        }
    }

    if (unit->overlays[2].animation_index != -1 && unit_reset_light_effect(unit_index) == 2) {
        unit->overlays[2].animation_index = -1;
    }

    if (obj->animation_index != -1) {
        int32_t completion = unit_reset_light_effect(unit_index);
        if (completion == 1) {
            switch (unit->animation_state) {
            case 0x1e: case 0x1f: case 0x29:
                unit_cause_melee_damage(unit_index, 0, (uint32_t)-1, (uint32_t)-1, (uint32_t)-1, (uint32_t)-1, 0);
                break;
            case 0x21:
                unit_release_thrown_grenade(unit_index, 0);
                break;
            default:
                break;
            }
        } else if (completion == 2) {
            switch (unit->animation_state) {
            case 0x19: {
                // Three ways to reach the "ready" path (LAB_0056567e/LAB_0056568f in the
                // original); every other combination falls through to the teardown call.
                uint8_t ready;
                if ((*((uint8_t *)obj_tag + 0x17c) & 2) == 0) {
                    ready = 1; // Unit.unit_flags bit 0x2 (destroyed_after_dying) clear
                } else if ((obj->flags & _object_at_rest_bit) == 0) {
                    if (obj->type != 0) {
                        ready = 1; // not a biped: skip the grounded check entirely
                    } else {
                        biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
                        Biped *biped_tag = (Biped *)obj_tag;
                        ready = (biped->flags & 1) != 0 && (biped_tag->biped_flags & 0x400) == 0;
                    }
                } else {
                    ready = 0; // at rest: always tears down
                }

                if (ready) {
                    if (obj->type == 0) {
                        unit_reset_ground_adjust_state(unit_index); // index in a register
                    }
                    unit->animation_state_flags = (uint16_t)(unit->animation_state_flags | 4);
                    obj->animation_frame = obj->animation_frame - 1; // shares switchD_005655fe_caseD_25
                } else {
                    object_delete_teardown();
                    unit_pick_random_spawned_actor_count();
                }
                break;
            }
            case 0x1a: {
                object *parent = ((object_header *)object_data->data)[obj->parent_object & 0xffff].data;
                Object *parent_tag = (Object *)tag_instances[parent->definition_tag & 0xffff].data;
                unit_data *parent_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
                UnitSeat *seat = (UnitSeat *)((uint8_t *)((Unit *)parent_tag)->seats.pointer +
                                               unit->vehicle_seat_index * 0x11c);
                object_set_collision_enabled((~*(uint8_t *)seat) & 1);
                if (parent_unit->driver_unit_index == unit_index) {
                    unit_notify_weapon_removed_dup();
                }
                break;
            }
            case 0x1b: {
                // UNSURE: root-motion delta, see file header
                real_vector3d frame_delta = {0};
                model_animation_get_frame_delta(tag_instances[obj_tag->model.tag_id.index].data);
                void *world_matrix = object_get_world_matrix();
                matrix4x3_transform_vector(world_matrix);
                obj->velocity.i = obj->velocity.i + frame_delta.i;
                obj->velocity.j = obj->velocity.j + frame_delta.j;
                obj->velocity.k = obj->velocity.k + frame_delta.k;
                break;
            }
            case 0x25:
            case 0x26:
                obj->animation_frame = obj->animation_frame - 1;
                break;
            case 0x27:
                requested_state = 0x28;
                need_retry = 1;
                break;
            default:
                break;
            }
            if (!unit_state_allows_control((const uint8_t *)unit + 0xa4)) {
                skip_animation_request = 1;
            }
        }
    }

    if (unit->overlays[0].animation_index != -1 && unit_reset_light_effect(unit_index) == 2) {
        object_copy_default_node_transforms();
        unit->unknown_2a4 = 0;
        unit->overlays[0].animation_index = -1;
    }
    if (unit->overlays[1].animation_index != -1) {
        int32_t completion = unit_reset_light_effect(unit_index);
        if ((completion == 2 || completion == 4) &&
            (unit->animation_state < 3 || 4 < unit->animation_state)) {
            unit->unknown_2a5 = 0;
            unit->overlays[1].animation_index = -1;
        }
    }

    if (skip_animation_request || (requested_state != unit->animation_state &&
                                    unit_animation_state_is_compatible((const uint8_t *)unit + 0xa4, requested_state))) {
        unit_try_set_animation_state(unit_index, requested_state);
    }
    return need_retry;
}

#if 0
Original Ghidra decompilation (0x565420):

undefined2 FUN_00565420(uint param_1)

{
  uint *puVar1;
  int iVar2;
  uint *puVar3;
  undefined2 uVar4;
  bool bVar5;
  char cVar6;
  int iVar7;
  undefined4 uVar8;
  char *in_ECX;
  short sVar9;
  float local_24;
  float local_20;
  float local_1c;
  undefined4 local_14;
  undefined2 local_10;

  iVar7 = (param_1 & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_14 = CONCAT22((short)((uint)DAT_0087bc14 >> 0x10),(short)*in_ECX);
  sVar9 = -1;
  uVar4 = 0;
  local_10 = 0;
  bVar5 = false;
  if ((puVar1[0x47] == 0xffffffff) && ((*(byte *)((int)puVar1 + 0x106) & 4) == 0)) {
    switch(*(undefined1 *)((int)puVar1 + 0x2a6)) {
    case 0:
      sVar9 = 0;
      break;
    case 1:
    case 2:
      sVar9 = 1;
      break;
    case 3:
      sVar9 = (in_ECX[1] != '\0') + 2;
      break;
    case 4:
      sVar9 = 2;
      break;
    case 5:
      sVar9 = 4;
      break;
    case 6:
      sVar9 = 5;
    }
    if (*(char *)((int)puVar1 + 0x20f) != -1) {
      sVar9 = (short)*(char *)((int)puVar1 + 0x20f);
    }
    if ((puVar1[0x82] & 0x200) != 0) {
      sVar9 = 1;
    }
    if (*(char *)((int)puVar1 + 0x28b) != '\0') {
      sVar9 = 5;
    }
    if ((*(char *)((int)puVar1 + 0x2a7) != sVar9) && (cVar6 = FUN_00565be0(), cVar6 != '\0')) {
      uVar8 = unit_get_current_weapon_label(1);
      unit_set_or_test_seat_and_weapon_label((&PTR_DAT_0069fde4)[sVar9],uVar8);
    }
  }
  if ((*(short *)((int)puVar1 + 0x2b2) != -1) && (sVar9 = FUN_0056ec10(param_1), sVar9 == 2)) {
    *(undefined2 *)((int)puVar1 + 0x2b2) = 0xffff;
  }
  if ((short)puVar1[0x34] == -1) goto switchD_005655a9_caseD_20;
  sVar9 = FUN_0056ec10(param_1);
  if (sVar9 == 1) {
    switch(*(undefined1 *)((int)puVar1 + 0x2a3)) {
    case 0x1e:
    case 0x1f:
    case 0x29:
      unit_cause_melee_damage(param_1,0,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0);
      break;
    case 0x21:
      unit_release_thrown_grenade(param_1,0);
    }
    goto switchD_005655a9_caseD_20;
  }
  if (sVar9 != 2) goto switchD_005655a9_caseD_20;
  local_10 = uVar4;
  switch(*(undefined1 *)((int)puVar1 + 0x2a3)) {
  case 0x19:
    if ((*(byte *)((int)iVar2 + 0x17c) & 2) == 0) {
LAB_0056567e:
      if ((short)puVar1[0x2d] == 0) {
        FUN_0055ad00();
      }
LAB_0056568f:
      *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] | 4;
      goto switchD_005655fe_caseD_25;
    }
    if ((puVar1[4] & 0x20) == 0) {
      if ((short)puVar1[0x2d] != 0) goto LAB_0056568f;
      puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
      if (((puVar3[0x133] & 1) != 0) &&
         ((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 0x400) ==
          0)) goto LAB_0056567e;
    }
    object_delete_teardown();
    FUN_00568540();
    break;
  case 0x1a:
    puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar1[0x47] & 0xffff) * 0xc);
    FUN_004f6850(~*(byte *)(*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                    0x2e8) + (short)puVar1[0xbc] * 0x11c) & 1);
    if (puVar3[0xc9] == param_1) {
      FUN_0056ab30();
    }
    break;
  case 0x1b:
    model_animation_get_frame_delta
              (*(undefined4 *)((*(uint *)(iVar2 + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14));
    uVar8 = object_get_world_matrix();
    matrix4x3_transform_vector(uVar8);
    FUN_0056c640(param_1,1,1,1);
    puVar1[0x1a] = (uint)(local_24 + (float)puVar1[0x1a]);
    puVar1[0x1b] = (uint)(local_20 + (float)puVar1[0x1b]);
    puVar1[0x1c] = (uint)(local_1c + (float)puVar1[0x1c]);
    break;
  case 0x25:
  case 0x26:
switchD_005655fe_caseD_25:
    *(short *)((int)puVar1 + 0xd2) = *(short *)((int)puVar1 + 0xd2) + -1;
    break;
  case 0x27:
    local_14 = 0x28;
    local_10 = 1;
  }
  cVar6 = FUN_00565ca0();
  if (cVar6 == '\0') {
    bVar5 = true;
  }
switchD_005655a9_caseD_20:
  if ((*(short *)((int)puVar1 + 0x2aa) != -1) && (sVar9 = FUN_0056ec10(param_1), sVar9 == 2)) {
    FUN_004f6b70();
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
    *(undefined1 *)(iVar2 + 0x2a4) = 0;
    *(undefined2 *)(iVar2 + 0x2aa) = 0xffff;
  }
  if (((*(short *)((int)puVar1 + 0x2ae) != -1) &&
      ((sVar9 = FUN_0056ec10(param_1), sVar9 == 2 || (sVar9 == 4)))) &&
     ((*(char *)((int)puVar1 + 0x2a3) < '\x03' || ('\x04' < *(char *)((int)puVar1 + 0x2a3))))) {
    *(undefined1 *)((int)puVar1 + 0x2a5) = 0;
    *(undefined2 *)((int)puVar1 + 0x2ae) = 0xffff;
  }
  if ((bVar5) ||
     (((short)local_14 != (short)*(char *)((int)puVar1 + 0x2a3) &&
      (cVar6 = FUN_00565be0(), cVar6 != '\0')))) {
    unit_try_set_animation_state(param_1,local_14);
  }
  return local_10;
}
#endif
