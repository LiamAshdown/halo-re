// unit_enter_vehicle_seat  (Ghidra: unit_enter_vehicle_seat, already named)
// address 0x566970, size 652 bytes
// name confidence: 0.7 (already carries this name)   rewrite confidence: 0.2
// evidence: types/tags.h Unit.seats (TagReflexive at 0x2e4/0x2e8, UnitSeat stride 0x11c,
//   marker_name TagString at +0x24), Object.animation_graph (tag+0x44) -> the same
//   +0x0c/+0x10 unit-block chain used by unit_try_set_animation_state (frame_count at
//   ModelAnimationsAnimation+... via the unit-seat animations table, count/pointer at 0x40/
//   0x44); types/objects.h object.parent_object (0x11c), .definition_tag (0x000),
//   .animation_graph/animation_index/animation_frame (0xcc/0xd0/0xd2); types/units.h
//   unit_data.vehicle_seat_index (0x2f0), .actor_index (0x1f4), .animation_state (0x2a3),
//   .current_weapon_index/.desired_weapon_index (0x2f2/0x2f4). unit_seat_is_occupied_by_other
//   (0x566840), unit_set_or_test_seat_and_weapon_label (0x5651e0), unit_get_current_weapon_label
//   (0x56dfd0), unit_ready_desired_weapon (0x56d6e0), object_get_node_local_transform,
//   object_reorient_relative_to_marker (resolved signature: see src/objects/
//   object_reorient_relative_to_marker.c).
// register convention: vehicle index in the recognized parameter, seat index in the second
//   parameter, entering-unit index in EAX.
//   // blam-cc: param_1 -> vehicle_index, param_2 -> seat_index, in_EAX -> unit_index
// UNSURE: object_get_position, matrix4x3_inverse_transform_vector and unit_find_next_zone_permitted_weapon_slot/
//   unit_recompute_seat_occupants/object_offset_node_translation are called with argument counts or outputs Ghidra could not
//   fully recover; reproduced with the visible arguments only, output discarded where the
//   original never captured one. unit_seat_is_occupied_by_other (unit_seat_is_occupied_by_other) is called with
//   only its seat-index and out-pointer parameters visible; self_index/vehicle_index are
//   modelled as this function's own unit_index/vehicle_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// object_get_position (0x4f6900, defined in src/objects/object_get_position.c) writes the
// object position through the pointer in EAX and leaves that same pointer in EAX on return;
// the object index is in ECX. Ghidra binds a different subset of the two operands at each call
// site in this module, so the declaration is left unprototyped.
extern real_point3d *object_get_position();
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags);   // 0x4f6080
extern void matrix4x3_inverse_transform_vector(void *out);                               // 0x4cc010, UNSURE signature
  // real signature (matrix4x3_inverse_transform_vector.c): void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); Ghidra recovered 1 of 3 args at this call site
extern void object_reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name,
                                                uint32_t object_index, char *object_marker_name); // 0x4f6180
extern void unit_recompute_seat_occupants(void);                                                          // 0x56ce30, UNSURE: no traced args
  // real signature (unit_recompute_seat_occupants.c): void unit_recompute_seat_occupants(uint32_t unit_index); Ghidra recovered 0 of 1 args at this call site
extern int16_t unit_find_next_zone_permitted_weapon_slot(int16_t current_weapon_index, uint32_t flag);                // 0x56dba0, UNSURE signature
  // real signature (unit_find_next_zone_permitted_weapon_slot.c): int16_t unit_find_next_zone_permitted_weapon_slot(uint32_t unit_index, int32_t start_slot, int16_t direction); Ghidra recovered 2 of 3 args at this call site
extern void unit_ready_desired_weapon(uint32_t unit_index, uint32_t flag);                // 0x56d6e0, UNSURE: Ghidra bound no arguments here
  // real signature (unit_ready_desired_weapon.c): void unit_ready_desired_weapon(uint32_t unit_index); Ghidra recovered 2 of 1 args at this call site
extern char * unit_get_current_weapon_label(uint32_t unit_index);                               // 0x56dfd0
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label,
                                                     char *weapon_label, uint8_t test_only); // 0x5651e0,
// unit_index in EAX; this matches the definition in unit_set_or_test_seat_and_weapon_label.c.
// The phase-4 review pass corrected the arity (Ghidra binds only the stack arguments at these
// call sites) and the return type (the callee returns a byte, tested in AL).
extern void object_copy_default_node_transforms(void);                                                          // 0x4f6b70
  // real signature (object_copy_default_node_transforms.c): void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); Ghidra recovered 0 of 2 args at this call site
extern int16_t animation_choose_random_permutation(uint32_t flag);                                              // 0x4d6280
extern void object_offset_node_translation(void);                                                          // 0x4f6c10, UNSURE: no traced args
  // real signature (object_offset_node_translation.c): void object_offset_node_translation(uint32_t object_index, real_vector3d *delta); Ghidra recovered 0 of 2 args at this call site
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index);         // 0x4f82b0, index in a register
extern void ai_communication_broadcast(int32_t line_id);                                 // 0x42d340
extern void unit_validate_and_clear_weapon_switch(uint32_t unit_index);                                           // unit_validate_and_clear_weapon_switch, 0x5659c0
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask);          // 0x4f6ec0
extern uint8_t unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index,
                                               uint32_t *out_occupant_index);             // 0x566840

uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index) // blam-cc: see file header
{
    if (unit_seat_is_occupied_by_other(unit_index, seat_index, vehicle_index, 0) == 0) {
        return 0;
    }

    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    Unit *vehicle_tag = (Unit *)tag_instances[((object_header *)object_data->data)[vehicle_index & 0xffff].data->definition_tag & 0xffff].data;
    UnitSeat *seat = (UnitSeat *)((uint8_t *)vehicle_tag->seats.pointer + seat_index * 0x11c);

    object_get_position(vehicle_index); // UNSURE: output discarded, see file header

    object_marker marker = {0};
    object_get_node_local_transform(vehicle_index, seat->marker_name.string, &marker, 1);
    matrix4x3_inverse_transform_vector(&marker); // UNSURE
    object_reorient_relative_to_marker(vehicle_index, seat->marker_name.string, unit_index, 0);

    unit->vehicle_seat_index = seat_index;
    unit_obj->parent_object = (datum_index)vehicle_index;
    unit_recompute_seat_occupants();

    unit->desired_weapon_index = unit_find_next_zone_permitted_weapon_slot(unit->current_weapon_index, 0);
    unit_ready_desired_weapon(unit_index, 0); // UNSURE: Ghidra bound no arguments at this call site

    char *weapon_label = unit_get_current_weapon_label(1);
    if (unit_set_or_test_seat_and_weapon_label(unit_index, seat->label.string, weapon_label, 0) == 0) {
        unit_set_or_test_seat_and_weapon_label(unit_index, seat->label.string, 0, 1);
    }

    Object *obj_tag = (Object *)tag_instances[unit_obj->definition_tag & 0xffff].data;
    void *graph = tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)((uint8_t *)graph + 0x10);
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);

    if ((int32_t)unit_seat->animations.count > 7 &&
        *(int16_t *)((uint8_t *)unit_seat->animations.pointer + 0xe) != -1) {
        object_copy_default_node_transforms();
        int16_t instance = animation_choose_random_permutation(1);
        unit_obj->animation_graph = *(datum_index *)&obj_tag->animation_graph.tag_id;
        unit_obj->animation_index = instance;
        unit_obj->animation_frame = 0;
        unit->animation_state = 0x1a;
        object_offset_node_translation();
        object_recalculate_bounding_radius_recursive(unit_index); // UNSURE: register-carried
    }

    if (unit->actor_index != (datum_index)-1) {
        ai_communication_broadcast(0x24);
    }
    unit_validate_and_clear_weapon_switch(unit_index);

    object *player = object_try_and_get((datum_index)2, 2); // UNSURE: literal `2` reproduced as both args, see original
    if (player != 0) {
        *(int32_t *)((uint8_t *)player + 0x5ac) = -1;
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
      ai_communication_broadcast(0x24);
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
