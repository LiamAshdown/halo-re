// unit_try_start_seat_exit_animation  (Ghidra: FUN_0056c470)
// address 0x56c470, size 461 bytes, name confidence 0.4, rewrite confidence 0.25
// functions.md: "Attempts to start the controlled unit's seat-exit animation sequence,
// returning whether the exit was actually initiated."
// evidence: shares its melee/lunge-style animation swap with unit_try_exit_controlled_seat.c
//   (0x56b5f0) and unit_detach_child_at_named_seat.c (0x56ab50).
// blam-cc: implicit ECX -> object_try_and_get's handle (UNSURE), in_AL -> force_flag,
//   unaff_EDI -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern int32_t game_connection_role; // 0x00719720

extern void actor_notify_weapon_pickup_once(uint32_t object_index);                        // 0x42c370, UNSURE signature
extern int32_t animation_choose_random_permutation(int32_t mode);                              // 0x4d6280
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_for_each_light_attachment(uint32_t object_index, uint32_t flag); // 0x4f9a20, UNSURE signature  // real signature (object_for_each_light_attachment.c): void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); Ghidra recovered 2 of 3 args at this call site
extern uint8_t unit_state_is_scripted_animation(unit_data *unit);                    // 0x565c60, UNSURE signature
extern void unit_notify_weapon_removed(int32_t object_index, int16_t new_state);      // 0x56ab10
extern void unit_dispatch_scripted_event_9(uint32_t param_1);                             // 0x56c370, UNSURE signature  // real signature (unit_dispatch_scripted_event_9.c): void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); Ghidra recovered 1 of 2 args at this call site
extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event); // 0x56c640
extern void unit_set_custom_animation(TagID animation_graph_tag, int16_t animation_index); // 0x56ebd0  // real signature (unit_set_custom_animation.c): void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); Ghidra recovered 2 of 3 args at this call site

uint8_t unit_try_start_seat_exit_animation(uint8_t force_flag, uint32_t unit_index) // blam-cc: in_AL, unaff_EDI
{
    uint8_t result = 0;
    object *self_obj = object_try_and_get(k_datum_index_none /* UNSURE: implicit ECX handle */, _object_mask_unit);
    if (self_obj == (object *)0) {
        return 0;
    }

    if ((game_connection_role != 1) || (force_flag == 1)) {
        datum_index vehicle_index = self_obj->parent_object;
        if ((vehicle_index != k_datum_index_none) && (((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->vehicle_seat_index != -1)) {
            object *parent = ((object_header *)object_data->data)[vehicle_index & 0xffff].data;
            if (parent->type == _object_type_vehicle) {
                unit_detach_from_seat(unit_index, 0, 0, 0); // UNSURE: original calls with no visible args
                return 0;
            }
            if (unit_state_is_scripted_animation(0) == 0) {
                Unit *self_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
                uint8_t *units_block = *(uint8_t **)((uint8_t *)tag_instances[self_tag->base.animation_graph.tag_id.index & 0xffff].data + 0x10);
                uint8_t *seat_block = units_block + ((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->animation_definition_index * 100;
                if ((8 < *(int32_t *)(seat_block + 0x40)) && (*(int16_t *)(*(int32_t *)(seat_block + 0x44) + 0x10) != -1)) {
                    unit_data *vehicle_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
                    if (vehicle_unit->driver_unit_index == unit_index) {
                        unit_notify_weapon_removed((int32_t)vehicle_index, 0);
                    }
                    int16_t new_anim = (int16_t)animation_choose_random_permutation(1);
                    unit_set_custom_animation(self_tag->base.animation_graph.tag_id, new_anim);
                    Object *self_def = (Object *)tag_instances[self_obj->definition_tag & 0xffff].data;
                    if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                        if ((self_obj->flags & 1) != 0) object_for_each_light_attachment(unit_index, 1);
                        if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                            self_obj->flags &= ~1u;                          // object + 0x10, bit 0
                            ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
                        }
                    }
                    ((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->animation_state = 0x1b;
                    actor_notify_weapon_pickup_once(unit_index);
                    result = 1;
                    if (self_obj->network_role == 0) {
                        unit_dispatch_scripted_event_9(0);
                    }
                }
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x56c470):

undefined1 FUN_0056c470(void)

{
  byte *pbVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint *puVar5;
  char in_AL;
  char cVar6;
  uint *puVar7;
  int iVar8;
  undefined4 uVar9;
  uint unaff_EDI;
  undefined1 local_9;

  local_9 = 0;
  puVar7 = (uint *)object_try_and_get(3);
  if (puVar7 == (uint *)0x0) {
    return 0;
  }
  if ((DAT_00719720 != 1) || (in_AL == '\x01')) {
    uVar2 = puVar7[0x47];
    if ((uVar2 != 0xffffffff) && ((short)puVar7[0xbc] != -1)) {
      if ((short)puVar7[0x2d] == 1) {
        FUN_0056c640();
        return 0;
      }
      cVar6 = FUN_00565c60();
      if (cVar6 == '\0') {
        iVar3 = *(int *)((*puVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        iVar8 = (char)puVar7[0xa8] * 100;
        iVar4 = *(int *)(*(int *)((*(uint *)(iVar3 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                        0x10);
        if ((8 < *(int *)(iVar8 + 0x40 + iVar4)) &&
           (*(short *)(*(int *)(iVar8 + iVar4 + 0x44) + 0x10) != -1)) {
          if (*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x324
                       ) == unaff_EDI) {
            FUN_0056ab10();
          }
          iVar4 = DAT_0087bc14;
          uVar9 = FUN_004d6280(1);
          unit_set_custom_animation(*(undefined4 *)(iVar3 + 0x44),uVar9);
          iVar8 = (unaff_EDI & 0xffff) * 0xc;
          puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
          iVar3 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + iVar4);
          if (*(int *)(iVar3 + 0x34) != -1) {
            if ((puVar5[4] & 1) != 0) {
              object_for_each_light_attachment(0,1);
            }
            if (*(int *)(iVar3 + 0x34) != -1) {
              iVar3 = *(int *)(DAT_008603b0 + 0x34);
              puVar5[4] = puVar5[4] & 0xfffffffe;
              pbVar1 = (byte *)(iVar3 + iVar8 + 2);
              *pbVar1 = *pbVar1 | 2;
            }
          }
          *(undefined1 *)((int)puVar7 + 0x2a3) = 0x1b;
          FUN_0042c370();
          local_9 = 1;
          if (puVar7[1] == 0) {
            FUN_0056c370(0);
          }
        }
      }
    }
  }
  return local_9;
}
#endif
