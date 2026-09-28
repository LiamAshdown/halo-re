// unit_update_animation_state_machine  (Ghidra: unit_update_animation_state_machine)
// address 0x565420, size 1133 bytes
// name confidence: 0.45 (phase2 candidate)   rewrite confidence: 0.85
// REWRITTEN (objdump 0x565420..0x56588c, jump tables 0x565890/0x5658ac+0x5658b8/0x5658c4+0x5658dc; the draft passed no
//   or wrong arguments to most callees and crashed in the animation advance). ECX = the 2-byte request {requested
//   state, flag}, [esp+4] = the unit. Returns a word that is 1 only when a finished animation in state 0x27 asks for
//   state 0x28.
//   A. Unparented, living units pick a base animation state from the seat command (+0x2a6: 0 -> 0, 1/2 -> 1,
//      3 -> 2 + flag, 4 -> 2, 5 -> 4, 6 -> 5, else -1), overridden by the scripted seat byte (+0x20f unless -1),
//      control flag 0x200 (-> 1) and a nonzero +0x28b (-> 5). When it differs from the current base state (+0x2a7)
//      and unit_animation_state_is_compatible(ECX = +0x298, DX = requested) allows it:
//      unit_set_or_test_seat_and_weapon_label(EAX unit, stack: unit_base_animation_state_names[state],
//      unit_get_current_weapon_label(EAX unit, stack 1), 1) -- the 1 pushed for the first call stays on the stack.
//   B. The third overlay (+0x2b2) advances in the unit tag's graph (+0x44) via the animation advance at 0x56ec10
//      (ECX slot, EAX graph, stack unit); 2 (finished) clears it.
//   C. The base animation (+0xd0, graph +0xcc) advances: 1 (event frame) in states 0x1e/0x1f/0x29 causes melee
//      damage (stack: unit, 0, -1, -1, -1, -1, 0) and in 0x21 releases the grenade (stack: unit, 0); 2 (finished)
//      handles 0x19 (dying: delete a "destroyed after dying" unit (tag +0x17c bit 1) that is at rest, or is a biped
//      that is airborne or whose biped tag has flag 0x400; otherwise reset a biped's ground adjust, set +0x298 bit 2
//      and step the frame back), 0x1a (seat entered: collision on unless the seat is invisible, and a driver closes
//      the vehicle), 0x1b (root motion: the frame delta through the world matrix added to the velocity, after a
//      detach from the seat), 0x25/0x26 (step the frame back) and 0x27 (return 1, request 0x28); after any finish a
//      state that unit_state_allows_control refuses forces the request through.
//   D. Overlay +0x2aa finishing copies the default node transforms (EAX unit, DX 6) and clears +0x2a4 and the slot.
//   E. Overlay +0x2ae finishing (2 or 4) outside states 3..4 clears +0x2a5 and the slot.
//   F. A forced request, or a request differing from the state (+0x2a3) that is compatible, goes to
//      unit_try_set_animation_state(stack: unit, request).
// blam-cc: ECX -> request, stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern char *unit_base_animation_state_names[6]; // 0x0069fde4

extern uint8_t unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state); // 0x565be0, ECX, DX
extern uint8_t unit_state_allows_control(const uint8_t *animation_block); // 0x565ca0, ECX
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern char *unit_get_current_weapon_label(uint32_t unit_index); // 0x56dfd0, EAX (the stack 1 is not read)
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label,
    uint8_t test_only); // 0x5651e0, EAX, stack
extern uint16_t unit_reset_light_effect(void *state, uint32_t animation_graph_tag_index, datum_index object_index);
    // 0x56ec10 (the animation slot advance), ECX, EAX, stack
extern void unit_release_thrown_grenade(uint32_t object_index, uint8_t apply_throw_fraction); // 0x56e440
extern void unit_cause_melee_damage(uint32_t unit_index, uint8_t suppress_effect, uint32_t target_object_index,
    int16_t damage_param4, int16_t damage_param5, int16_t damage_param6, uint32_t damage_param7); // 0x56f2d0
extern void object_delete_teardown(uint32_t object_index); // 0x4edc80, EAX
extern int32_t unit_pick_random_spawned_actor_count(uint32_t unit_index); // 0x568540, EDI
extern void unit_reset_ground_adjust_state(uint32_t object_index); // 0x55ad00, EAX
extern void model_animation_get_frame_delta(int16_t frame, void *animation, real_vector3d *out, void *model);
    // 0x4d4a00, ECX, EDX, EBX, stack
extern real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out); // 0x4f6a20, EAX, EDI
extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cbe50, EAX, EDX, stack
extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag,
    uint8_t fire_trigger_event); // 0x56c640
extern void object_set_collision_enabled(uint32_t object_index, uint8_t enable); // 0x4f6850, EAX, stack
extern void unit_notify_weapon_removed_dup(int32_t object_index); // 0x56ab30, EAX
extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); // 0x4f6b70, EAX, DX

static uint8_t *state_machine_object(uint32_t object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request)
{
    uint8_t *unit = state_machine_object(unit_index);
    uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    int16_t requested = request[0];
    uint16_t result = 0;
    uint8_t force = 0;
    uint16_t advance;

    if (((unit_object *)unit)->base.parent_object == k_datum_index_none && (unit[0x106] & 4) == 0) {
        int16_t base_state = -1;

        switch ((int8_t)unit[0x2a6]) {
        case 0: base_state = 0; break;
        case 1: case 2: base_state = 1; break;
        case 3: base_state = (int16_t)(2 + (request[1] != 0)); break;
        case 4: base_state = 2; break;
        case 5: base_state = 4; break;
        case 6: base_state = 5; break;
        default: break;
        }
        if (unit[0x20f] != 0xff) {
            base_state = (int8_t)unit[0x20f];
        }
        if (((unit_object *)unit)->unit.control_flags & 0x200) {
            base_state = 1;
        }
        if (unit[0x28b] != 0) {
            base_state = 5;
        }
        if ((int8_t)unit[0x2a7] != base_state && unit_animation_state_is_compatible(unit + 0x298, requested)) {
            char *weapon_label = unit_get_current_weapon_label(unit_index);

            unit_set_or_test_seat_and_weapon_label(unit_index, unit_base_animation_state_names[base_state],
                weapon_label, 1);
        }
    }

    if (*(int16_t *)(unit + 0x2b2) != -1 &&
        unit_reset_light_effect(unit + 0x2b2, *(uint32_t *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id, unit_index) == 2) {
        *(int16_t *)(unit + 0x2b2) = -1;
    }

    if (((unit_object *)unit)->base.animation_index != -1) {
        advance = unit_reset_light_effect(unit + 0xd0, *(uint32_t *)&((unit_object *)unit)->base.animation_graph, unit_index);
        if (advance == 1) {
            switch ((int8_t)unit[0x2a3]) {
            case 0x1e: case 0x1f: case 0x29:
                unit_cause_melee_damage(unit_index, 0, 0xffffffff, -1, -1, -1, 0);
                break;
            case 0x21:
                unit_release_thrown_grenade(unit_index, 0);
                break;
            default:
                break;
            }
        } else if (advance == 2) {
            switch ((int8_t)unit[0x2a3]) {
            case 0x19: {
                uint8_t delete_now = 0;

                if (unit_tag[0x17c] & 2) {
                    if (unit[0x10] & 0x20) {
                        delete_now = 1;
                    } else if (((unit_object *)unit)->base.type == 0) {
                        uint8_t *biped = state_machine_object(unit_index);
                        uint8_t *biped_tag = (uint8_t *)tag_instances[*(datum_index *)biped & 0xffff].data;

                        if (!(biped[0x4cc] & 1) || (*(uint32_t *)(biped_tag + 0x2f4) & 0x400)) {
                            delete_now = 1;
                        }
                    }
                }
                if (delete_now) {
                    object_delete_teardown(unit_index);
                    unit_pick_random_spawned_actor_count(unit_index);
                    break;
                }
                if (((unit_object *)unit)->base.type == 0) {
                    unit_reset_ground_adjust_state(unit_index);
                }
                unit[0x298] |= 4;
                ((unit_object *)unit)->base.animation_frame -= 1;
                break;
            }
            case 0x1a: {
                datum_index parent_index = ((unit_object *)unit)->base.parent_object;
                uint8_t *parent = state_machine_object(parent_index);
                uint8_t *parent_tag = (uint8_t *)tag_instances[*(datum_index *)parent & 0xffff].data;
                uint8_t seat_flags = *(*(uint8_t **)(parent_tag + 0x2e8) + ((unit_object *)unit)->unit.vehicle_seat_index * 0x11c);

                object_set_collision_enabled(unit_index, (uint8_t)(~seat_flags & 1));
                if (*(datum_index *)(parent + 0x324) == unit_index) {
                    unit_notify_weapon_removed_dup((int32_t)((unit_object *)unit)->base.parent_object);
                }
                break;
            }
            case 0x1b: {
                uint8_t *animations =
                    *(uint8_t **)((uint8_t *)tag_instances[((unit_object *)unit)->base.animation_graph & 0xffff].data + 0x78);
                void *model = tag_instances[*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id & 0xffff].data;
                real_vector3d delta;
                real_matrix4x3 world;
                real_matrix4x3 *matrix;

                model_animation_get_frame_delta(((unit_object *)unit)->base.animation_frame,
                    animations + ((unit_object *)unit)->base.animation_index * 0xb4, &delta, model);
                matrix = object_get_world_matrix(unit_index, &world);
                matrix4x3_transform_vector(&delta, &delta, matrix);
                unit_detach_from_seat(unit_index, 1, 1, 1);
                ((unit_object *)unit)->base.velocity.i = delta.i + ((unit_object *)unit)->base.velocity.i;
                ((unit_object *)unit)->base.velocity.j = delta.j + ((unit_object *)unit)->base.velocity.j;
                ((unit_object *)unit)->base.velocity.k = delta.k + ((unit_object *)unit)->base.velocity.k;
                break;
            }
            case 0x25: case 0x26:
                ((unit_object *)unit)->base.animation_frame -= 1;
                break;
            case 0x27:
                result = 1;
                requested = 0x28;
                break;
            default:
                break;
            }
            if (!unit_state_allows_control(unit + 0x298)) {
                force = 1;
            }
        }
    }

    if (*(int16_t *)(unit + 0x2aa) != -1 &&
        unit_reset_light_effect(unit + 0x2aa, *(uint32_t *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id, unit_index) == 2) {
        uint8_t *reloaded;

        object_copy_default_node_transforms(unit_index, 6);
        reloaded = state_machine_object(unit_index);
        reloaded[0x2a4] = 0;
        *(int16_t *)(reloaded + 0x2aa) = -1;
    }

    if (*(int16_t *)(unit + 0x2ae) != -1) {
        advance = unit_reset_light_effect(unit + 0x2ae, *(uint32_t *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id, unit_index);
        if ((advance == 2 || advance == 4) && ((int8_t)unit[0x2a3] < 3 || (int8_t)unit[0x2a3] > 4)) {
            unit[0x2a5] = 0;
            *(int16_t *)(unit + 0x2ae) = -1;
        }
    }

    if (force || (requested != (int8_t)unit[0x2a3] && unit_animation_state_is_compatible(unit + 0x298, requested))) {
        unit_try_set_animation_state(unit_index, requested);
    }
    return result;
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
