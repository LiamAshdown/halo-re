// unit_enter_vehicle_seat  (Ghidra: unit_enter_vehicle_seat, already named)
// address 0x566970, size 652 bytes
// name confidence: 0.7 (already carries this name)   rewrite confidence: 0.85
// REWRITTEN from objdump 0x566970..0x566bfb (the draft discarded object_get_position's output, called the marker
//   inverse transform, occupant recompute, weapon slot search, default transforms, permutation and node offset
//   without their arguments). EAX = unit, stack: vehicle, seat.
//   Refused (returns 0) when unit_seat_is_occupied_by_other(EAX unit, EDX vehicle, stack seat, 0) says the seat is
//   taken. Otherwise the unit's position minus the seat marker's world position (object_get_node_local_transform
//   of the vehicle's seat marker, tag seat +0x24) is brought into the marker frame (0x4cc010, EAX = EDX = delta,
//   stack marker +0x38), the unit is attached at the marker (0x4f6180 stack vehicle, marker; ESI unit, EDI ""),
//   takes the seat (+0x2f0) and parent (+0x11c), the vehicle's occupants are recomputed (EAX vehicle), the unit
//   readies the next zone-permitted weapon from its current one and its animation labels become the seat's
//   label (+4) with its weapon, falling back to the seat label alone. When the unit's seat animations (graph
//   units[+0x2a0]) have an enter animation (slot 7), the unit restarts it (default transforms, 6; state 0x1a)
//   and is offset back by the delta so it animates from where it stood (0x4f6c10, 0x4f82b0). An AI-driven unit
//   (+0x1f4) broadcasts 0x24, the weapon switch is validated, and the vehicle's +0x5ac is cleared.
// blam-cc: EAX -> unit_index, stack -> vehicle_index, seat_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX out, ECX object
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t flags); // 0x4f6080, stack
extern void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cc010
extern void object_reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name,
    uint32_t object_index, char *object_marker_name); // 0x4f6180, stack, stack, ESI, EDI
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot,
    int16_t direction); // 0x56dba0, EAX, stack
extern void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force); // 0x56d6e0, stack (unit, force)
extern char *unit_get_current_weapon_label(uint32_t unit_index); // 0x56dfd0, EAX
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label,
    uint8_t apply); // 0x5651e0, EAX, stack
extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); // 0x4f6b70, EAX, DX
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX, DX, stack
extern void object_offset_node_translation(uint32_t object_index, real_vector3d *delta); // 0x4f6c10, EAX, EDX
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0, stack
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a,
    int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, all stack
extern void unit_validate_and_clear_weapon_switch(uint32_t unit_index); // 0x5659c0, stack
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern uint8_t unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index,
    uint32_t *out_occupant_index); // 0x566840, EAX self, EDX vehicle, stack seat, out

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index)
{
    uint8_t *unit;
    uint8_t *seat;
    uint8_t *unit_tag;
    uint8_t *unit_seat;
    char *marker_name;
    real_point3d position;
    real_vector3d delta;
    object_marker marker;

    if (unit_seat_is_occupied_by_other(unit_index, seat_index, vehicle_index, 0) == 0) {
        return 0;
    }
    seat = *(uint8_t **)(TAG_DATA(*(datum_index *)OBJECT_DATA(vehicle_index)) + 0x2e8) + seat_index * 0x11c;
    object_get_position(&position, unit_index);
    marker_name = (char *)(seat + 0x24);
    object_get_node_local_transform(vehicle_index, marker_name, &marker, 1);
    delta.i = position.x - marker.node_transform.position.x;
    delta.j = position.y - marker.node_transform.position.y;
    delta.k = position.z - marker.node_transform.position.z;
    matrix4x3_inverse_transform_vector(&delta, &delta, &marker.node_transform);
    object_reorient_relative_to_marker(vehicle_index, marker_name, unit_index, (char *)"");

    unit = OBJECT_DATA(unit_index);
    ((unit_object *)unit)->unit.vehicle_seat_index = seat_index;
    ((unit_object *)unit)->base.parent_object = vehicle_index;
    unit_recompute_seat_occupants(vehicle_index);

    unit = OBJECT_DATA(unit_index);
    ((unit_object *)unit)->unit.desired_weapon_index =
        unit_find_next_zone_permitted_weapon_slot(unit_index, *(uint16_t *)&((unit_object *)unit)->unit.current_weapon_index, 0);
    unit_ready_desired_weapon(unit_index, 1);
    if (unit_set_or_test_seat_and_weapon_label(unit_index, (char *)(seat + 4), unit_get_current_weapon_label(unit_index),
            1) == 0) {
        unit_set_or_test_seat_and_weapon_label(unit_index, (char *)(seat + 4), 0, 1);
    }

    unit_tag = TAG_DATA(*(datum_index *)unit);
    unit_seat = *(uint8_t **)(TAG_DATA(*(datum_index *)(unit_tag + 0x44)) + 0x10) + (int8_t)unit[0x2a0] * 0x64;
    if (*(int32_t *)(unit_seat + 0x40) > 7 && (*(int16_t **)(unit_seat + 0x44))[7] != -1) {
        int16_t enter_animation = (*(int16_t **)(unit_seat + 0x44))[7];
        uint8_t *reloaded;
        int16_t animation;

        object_copy_default_node_transforms(unit_index, 6);
        animation = animation_choose_random_permutation(*(datum_index *)(unit_tag + 0x44), enter_animation, 1);
        reloaded = OBJECT_DATA(unit_index);
        *(datum_index *)(reloaded + 0xcc) = *(datum_index *)(unit_tag + 0x44);
        *(int16_t *)(reloaded + 0xd0) = animation;
        *(int16_t *)(reloaded + 0xd2) = 0;
        unit[0x2a3] = 0x1a;
        object_offset_node_translation(unit_index, &delta);
        object_recalculate_bounding_radius_recursive(unit_index);
    }

    if (*(datum_index *)(OBJECT_DATA(unit_index) + 0x1f4) != k_datum_index_none) {
        ai_communication_broadcast(0x24, unit_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
    }
    unit_validate_and_clear_weapon_switch(unit_index);
    {
        uint8_t *vehicle = (uint8_t *)object_try_and_get(vehicle_index, 2);

        if (vehicle != 0) {
            ((vehicle_object *)vehicle)->vehicle.network_update_tick = -1;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x566970):

undefined4 unit_enter_vehicle_seat(uint param_1,undefined4 param_2)

{
  uint *puVar1;
  char cVar2;
  undefined2 uVar3;
  uint in_EAX;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined1 local_70 [56];
  undefined1 local_38 [56];

  cVar2 = FUN_00566840(param_2,0);
  if (cVar2 != '\0') {
    iVar7 = (in_EAX & 0xffff) * 0xc;
    puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
    iVar6 = (short)param_2 * 0x11c +
            *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                          (param_1 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                             DAT_0087bc14) + 0x2e8);
    object_get_position();
    iVar5 = iVar6 + 0x24;
    object_get_node_local_transform(param_1,iVar5,local_70,1);
    matrix4x3_inverse_transform_vector(local_38);
    object_reorient_relative_to_marker(param_1,iVar5);
    *(short *)(puVar1 + 0xbc) = (short)param_2;
    puVar1[0x47] = param_1;
    FUN_0056ce30();
    iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
    uVar3 = FUN_0056dba0(*(undefined2 *)(iVar5 + 0x2f2),0);
    *(undefined2 *)(iVar5 + 0x2f4) = uVar3;
    unit_ready_desired_weapon(unit_index, 0); // UNSURE: Ghidra bound no arguments at this call site
    iVar6 = iVar6 + 4;
    uVar4 = unit_get_current_weapon_label(1);
    cVar2 = unit_set_or_test_seat_and_weapon_label(iVar6,uVar4);
    if (cVar2 == '\0') {
      unit_set_or_test_seat_and_weapon_label(iVar6,0,1);
    }
    iVar5 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar6 = (char)puVar1[0xa8] * 100 +
            *(int *)(*(int *)((*(uint *)(iVar5 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                    0x10);
    if ((7 < *(int *)(iVar6 + 0x40)) && (*(short *)(*(int *)(iVar6 + 0x44) + 0xe) != -1)) {
      FUN_004f6b70();
      uVar3 = FUN_004d6280(1);
      iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
      *(undefined4 *)(iVar6 + 0xcc) = *(undefined4 *)(iVar5 + 0x44);
      *(undefined2 *)(iVar6 + 0xd0) = uVar3;
      *(undefined2 *)(iVar6 + 0xd2) = 0;
      *(undefined1 *)((int)puVar1 + 0x2a3) = 0x1a;
      FUN_004f6c10();
      object_recalculate_bounding_radius_recursive();
    }
    if (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7) + 500) != -1) {
      ai_communication_broadcast(0x24, unit_index, (datum_index)-1, -1, (datum_index)-1, (datum_index)-1, 0);
    }
    FUN_005659c0();
    iVar5 = object_try_and_get(2);
    if (iVar5 != 0) {
      *(undefined4 *)(iVar5 + 0x5ac) = 0xffffffff;
    }
    return 1;
  }
  return 0;
}
#endif
