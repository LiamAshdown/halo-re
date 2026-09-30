// unit_try_start_seat_exit_animation  (Ghidra: FUN_0056c470)
// address 0x56c470, size 461 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x56c470..0x56c63c (the draft looked up object -1 and called its callees without their
//   arguments). EDI = unit, AL = force. Only a seated unit, and on a client only when forced: a vehicle unit
//   (type 1) is detached (0x56c640 stack unit, 1, force, 1) and 0 returned; any other unit, unless its
//   animation is scripted, starts its seat's exit animation (slot 8; the vehicle's driver leaving sets the
//   vehicle to state 0x25), becomes visible, enters state 0x1b, notifies the AI and returns 1.
// blam-cc: AL -> force_flag, EDI -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"
#include "fn_units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern int16_t network_game_mode; // 0x00719720


extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX, DX, stack
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20, EAX, stack
extern uint8_t unit_state_is_scripted_animation(unit_data *unit); // 0x565c60, ECX
extern void unit_notify_weapon_removed(int32_t object_index); // 0x56ab10, EAX
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX
extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag,
    uint8_t fire_trigger_event); // 0x56c640, stack


#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

uint8_t unit_try_start_seat_exit_animation(uint8_t force_flag, uint32_t unit_index)
{
    uint8_t *self = (uint8_t *)object_try_and_get(unit_index, 3);
    datum_index vehicle_index;
    uint8_t *self_tag;
    datum_index graph;
    uint8_t *seat_block;
    int16_t exit_animation;
    uint8_t *object;
    uint8_t *object_tag;

    if (self == 0) {
        return 0;
    }
    if (network_game_mode == 1 && force_flag != 1) {
        return 0;
    }
    vehicle_index = ((unit_object *)self)->base.parent_object;
    if (vehicle_index == k_datum_index_none || ((unit_object *)self)->unit.vehicle_seat_index == -1) {
        return 0;
    }
    if (((unit_object *)self)->base.type == 1) {
        unit_detach_from_seat(unit_index, 1, force_flag, 1);
        return 0;
    }
    if (unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
        return 0;
    }
    self_tag = TAG_DATA(*(datum_index *)self);
    graph = *(datum_index *)(self_tag + 0x44);
    seat_block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)self[0x2a0] * 0x64;
    if (!(*(int32_t *)(seat_block + 0x40) > 8) || (exit_animation = (*(int16_t **)(seat_block + 0x44))[8]) == -1) {
        return 0;
    }
    if (*(datum_index *)(OBJECT_DATA(vehicle_index) + 0x324) == unit_index) {
        unit_notify_weapon_removed((int32_t)vehicle_index);
    }
    unit_set_custom_animation(unit_index, *(datum_index *)(self_tag + 0x44),
        animation_choose_random_permutation(graph, exit_animation, 1));
    object = OBJECT_DATA(unit_index);
    object_tag = TAG_DATA(*(datum_index *)object);
    if (*(int32_t *)(object_tag + 0x34) != -1) {
        if ((object[0x10] & 1) != 0) {
            object_for_each_light_attachment(unit_index, 0, 1);
        }
        if (*(int32_t *)(object_tag + 0x34) != -1) {
            *(uint32_t *)(object + 0x10) &= ~1u;
            ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 2;
        }
    }
    self[0x2a3] = 0x1b;
    actor_notify_weapon_pickup_once(unit_index);
    if (((unit_object *)self)->base.network_role == 0) {
        unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
    }
    return 1;
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
