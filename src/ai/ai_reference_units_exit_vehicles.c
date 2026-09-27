// ai_reference_units_exit_vehicles  (Ghidra: ai_reference_units_exit_vehicles; named for this rewrite)
// address 0x433ea0, size 1627 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: phase-4 summary ("iterates active squad members and updates each unit's
//   weapon-holster/attach state, including transform, light attachments, and holster
//   animation"). What the code actually does is take every actor a packed ai reference names
//   that is both riding something (object + 0x11c parent, object + 0x2f0 seat) and has an
//   active_unit_index, and get it out of that seat: a vehicle-type rider is detached and
//   re-placed at the seat marker's transform, a biped rider is made to play the seat's exit
//   custom animation instead.
// register convention: EAX -> packed_reference (consumed by the iterator pair at 0x432650 /
//   0x4326d0, which Ghidra renders argument-less here).
//   // blam-cc: EAX -> packed_reference
//
// UNSURE (high) - this is the least certain rewrite of this batch, and most of its body
// belongs to the units and objects modules rather than to ai:
//  - Every unit-side and object-side offset here (0x11c parent, 0x1ea, 0x1f2, 0x2a3, 0x2a7,
//    0x2f0..0x2f3, 0x324, 0x32c, 0x330, 0x5ac, 0x17c/0x180/0x1dc/0x1e0) is used raw; none of
//    them is defined in types/units.h or types/objects.h yet.
//  - Ten of the calls (matrix4x3_multiply through the function pointer at 0x00696664,
//    object_for_each_light_attachment, unit_recompute_seat_occupants, unit_pick_and_ready_next_weapon, unit_update_animation_state_machine,
//    unit_reset_orientation_and_find_position, unit_all_seats_unoccupied, datum_get, unit_notify_weapon_removed, animation_choose_random_permutation) are shown by Ghidra
//    with fewer arguments than they take, or with none at all. They are declared below to
//    match this call site only.
//  - DAT_0087a480 (player_data) is saved and restored around the whole loop for no visible
//    reason; preserved.
//  - network_game_mode (0x00719720) == 1 gates two separate blocks, which reads as
//    "dedicated server / client only".
//
// Because of the above this file is a structural translation, not a semantic recovery.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "cache.h"
#include "ai.h"

extern data_array *object_data;      // 0x008603b0
extern int16_t network_game_mode;    // 0x00719720
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern void (*matrix4x3_multiply_ptr)(void *a, void *b, void *out); // 0x00696664
extern data_array *player_data;      // 0x0087a480
extern int32_t object_control_local_player_b; // 0x0071c2d8

extern void ai_reference_actor_iterator_new(uint32_t packed_reference,
    ai_reference_actor_iterator *out_iterator);                             // 0x432650
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0
extern void actor_notify_weapon_pickup_once(void);                 // 0x42c370, no visible argument
extern void *datum_get(void);                                     // 0x4d0680, no visible argument
extern uint32_t animation_choose_random_permutation(int32_t which);                      // 0x4d6280
extern void player_update_history_free_all(uint32_t player);      // 0x4e6f20
extern void object_set_position_and_orientation(datum_index object_index,
    real_vector3d *forward, real_vector3d *up, real_point3d *position); // 0x4f51c0
extern void object_get_node_local_transform(datum_index object_index, void *marker,
    void *out_transform, int32_t flag);                           // 0x4f6080
extern void object_snap_to_parent_marker_and_detach(datum_index object_index);  // 0x4f6610, not yet rewritten
extern object *object_try_and_get(uint32_t type_mask);            // 0x4f6ec0
extern void object_recalculate_bounding_radius_recursive(datum_index object_index); // 0x4f82b0
extern void object_for_each_light_attachment(int32_t a, int32_t b); // 0x4f9a20
extern void unit_reset_orientation_and_find_position(datum_index object_index);               // 0x55add0, not yet rewritten
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack unit, ECX request
static const int8_t k_unit_exit_seat_request[2] = {0x14, 0}; // every caller builds these two bytes on its stack
extern uint8_t unit_state_is_scripted_animation(void);            // 0x565c60, no visible argument
extern void unit_try_set_animation_state(datum_index unit_index, int32_t state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(void);                                // 0x566910, no visible argument
extern void unit_notify_weapon_removed(void);                                   // 0x56ab10, no visible argument
extern void unit_dispatch_scripted_event_9(int32_t a);                              // 0x56c370, not yet rewritten
extern void unit_recompute_seat_occupants(void);                                   // 0x56ce30, no visible argument
extern void unit_pick_and_ready_next_weapon(void);                                   // 0x56d6a0, no visible argument
extern void unit_set_custom_animation(datum_index unit_index, uint32_t animation); // 0x56ebd0

// blam-cc: EAX -> packed_reference
void ai_reference_units_exit_vehicles(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *a;
    data_array *saved_player_data;
    object_header *header;
    object_header *headers;
    uint8_t *rider;
    uint8_t *vehicle;
    uint8_t *root;
    uint8_t *seat_transform;
    uint8_t *unit_tag;
    uint8_t *seat_tag;
    uint8_t *player_record;
    datum_index rider_index;
    datum_index vehicle_index;
    datum_index owner_index;
    int16_t identifier;
    int16_t salt;
    int32_t seat_offset;
    uint32_t saved_position[3];
    uint8_t transform[116];
    uint8_t node_transform[116];
    uint32_t animation;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    saved_player_data = player_data;

    while (a != 0) {
        player_data = saved_player_data;

        if (a->active_unit_index != (datum_index)k_datum_index_none &&
            a->unit_index != (datum_index)k_datum_index_none) {

            rider_index = a->unit_index;
            header = 0;
            headers = (object_header *)object_data->data;
            identifier = (int16_t)rider_index;
            if (0 <= identifier && identifier < object_data->maximum_count) {
                object_header *candidate = &headers[identifier];
                salt = (int16_t)(rider_index >> 0x10);
                if (candidate->identifier != 0 &&
                    (salt == 0 || candidate->identifier == salt)) {
                    header = candidate;
                }
            }

            if (header != 0 && ((1 << (header->type & 0x1f)) & 3) != 0 &&
                (rider = (uint8_t *)header->data) != 0 &&
                network_game_mode != 1 &&
                *(datum_index *)(rider + 0x11c) != (datum_index)k_datum_index_none &&
                *(int16_t *)(rider + 0x2f0) != -1) {

                if (*(int16_t *)(rider + 0xb4) == _object_type_vehicle) {
                    rider = (uint8_t *)headers[rider_index & 0xffff].data;
                    vehicle_index = *(datum_index *)(rider + 0x11c);

                    if (vehicle_index != (datum_index)k_datum_index_none &&
                        *(int16_t *)(rider + 0x2f0) != -1) {

                        vehicle = (uint8_t *)headers[vehicle_index & 0xffff].data;

                        // the seat marker transform out of the vehicle's model
                        object_get_node_local_transform(vehicle_index,
                            (uint8_t *)tag_instances[*(datum_index *)vehicle & 0xffff].data +
                                0x2e8, node_transform, 1);

                        unit_tag = (uint8_t *)tag_instances[*(datum_index *)rider & 0xffff].data;
                        seat_tag = (uint8_t *)tag_instances[
                            *(datum_index *)(unit_tag + 0x34) & 0xffff].data;
                        seat_transform = (uint8_t *)(*(uint32_t *)(seat_tag + 0xbc));

                        saved_position[0] = *(uint32_t *)(seat_transform + 0x28);
                        saved_position[1] = *(uint32_t *)(seat_transform + 0x2c);
                        saved_position[2] = *(uint32_t *)(seat_transform + 0x30);

                        if (*(datum_index *)(vehicle + 0x324) == rider_index &&
                            *(uint8_t *)(vehicle + 0x2a3) != 0x25 &&
                            *(datum_index *)(rider + 0x11c) !=
                                (datum_index)k_datum_index_none) {
                            unit_try_set_animation_state(*(datum_index *)(rider + 0x11c), 0x25);
                        }

                        *(datum_index *)(rider + 0x32c) = vehicle_index;
                        *(int32_t *)(rider + 0x330) = game_time->game_time;
                        if (*(datum_index *)(rider + 0x324) == rider_index) {
                            *(datum_index *)(rider + 0x324) = (datum_index)k_datum_index_none;
                        }
                        if (*(datum_index *)(rider + 0x328) == rider_index) {
                            *(datum_index *)(rider + 0x328) = (datum_index)k_datum_index_none;
                        }

                        object_snap_to_parent_marker_and_detach(rider_index);
                        object_set_position_and_orientation(rider_index, 0, 0, 0);

                        root = (uint8_t *)headers[rider_index & 0xffff].data;
                        (*matrix4x3_multiply_ptr)(root + *(int16_t *)(root + 0x1f2),
                                                  seat_transform + 0x68, transform);
                        *(uint32_t *)(rider + 0x74) = *(uint32_t *)(transform + 0x04);
                        *(uint32_t *)(rider + 0x78) = *(uint32_t *)(transform + 0x08);
                        *(uint32_t *)(rider + 0x7c) = *(uint32_t *)(transform + 0x0c);
                        *(uint32_t *)(rider + 0x80) = *(uint32_t *)(transform + 0x1c);
                        *(uint32_t *)(rider + 0x84) = *(uint32_t *)(transform + 0x20);
                        *(uint32_t *)(rider + 0x88) = *(uint32_t *)(transform + 0x24);

                        root = (uint8_t *)headers[rider_index & 0xffff].data;
                        unit_tag = (uint8_t *)tag_instances[
                            *(datum_index *)root & 0xffff].data;
                        if (*(int32_t *)(unit_tag + 0x34) != -1) {
                            if ((*(uint32_t *)(root + 0x10) & 1) != 0) {
                                object_for_each_light_attachment(0, 1);
                            }
                            if (*(int32_t *)(unit_tag + 0x34) != -1) {
                                *(uint32_t *)(root + 0x10) =
                                    *(uint32_t *)(root + 0x10) & 0xfffffffe;
                                headers[rider_index & 0xffff].flags =
                                    (uint16_t)(headers[rider_index & 0xffff].flags | 2);
                            }
                        }

                        *(int16_t *)(rider + 0x2f0) = -1;
                        *(uint8_t *)(rider + 0x2a7) = 2;
                        if (*(datum_index *)(vehicle + 0x324) == rider_index) {
                            *(datum_index *)(vehicle + 0x324) = (datum_index)k_datum_index_none;
                        }
                        if (*(datum_index *)(vehicle + 0x328) == rider_index) {
                            *(datum_index *)(vehicle + 0x328) = (datum_index)k_datum_index_none;
                        }

                        unit_recompute_seat_occupants();
                        unit_pick_and_ready_next_weapon();
                        unit_update_animation_state_machine(rider_index, k_unit_exit_seat_request);

                        {
                            uint32_t *marker = (uint32_t *)(rider + 0x10 +
                                *(int16_t *)(rider + 0x1ea));
                            marker[0] = saved_position[0];
                            marker[1] = saved_position[1];
                            marker[2] = saved_position[2];
                        }

                        if (*(int16_t *)(rider + 0xb4) == _object_type_biped) {
                            unit_reset_orientation_and_find_position(rider_index);
                        }
                        object_recalculate_bounding_radius_recursive(rider_index);

                        if (unit_all_seats_unoccupied() == 1) {
                            object *local = object_try_and_get(2);
                            if (local != 0) {
                                *(int32_t *)((uint8_t *)local + 0x5ac) = game_time->game_time;
                            }
                        }
                    }

                    if (network_game_mode == 1) {
                        player_record = (uint8_t *)datum_get();
                        if (player_record != 0 && *(int16_t *)(player_record + 2) == -1) {
                            *(uint32_t *)(player_record + 0x180) = 0;
                            *(uint32_t *)(player_record + 0x17c) = 0;
                            *(uint32_t *)(player_record + 0x1e0) = 0;
                            *(uint32_t *)(player_record + 0x1dc) = 0;
                        } else {
                            owner_index = *(datum_index *)(rider + 0x218);
                            if (owner_index != (datum_index)k_datum_index_none) {
                                identifier = (int16_t)owner_index;
                                if (0 <= identifier &&
                                    identifier < saved_player_data->maximum_count) {
                                    int16_t *slot = (int16_t *)((uint8_t *)
                                        saved_player_data->data +
                                        (int32_t)saved_player_data->size * identifier);
                                    salt = (int16_t)(owner_index >> 0x10);
                                    if (*slot != 0 && (salt == 0 || *slot == salt) &&
                                        slot[1] != -1 && object_control_local_player_b != 0) {
                                        player_update_history_free_all(
                                            *(uint32_t *)(object_control_local_player_b + 0xf48));
                                    }
                                }
                            }
                        }
                    }
                } else if (unit_state_is_scripted_animation() == 0) {
                    unit_tag = (uint8_t *)tag_instances[*(datum_index *)rider & 0xffff].data;
                    seat_tag = (uint8_t *)(*(uint32_t *)((uint8_t *)tag_instances[
                        *(datum_index *)(unit_tag + 0x44) & 0xffff].data + 0x10));
                    seat_offset = (int32_t)*(int8_t *)(rider + 0x2a0) * 100;

                    if (8 < *(int32_t *)(seat_tag + seat_offset + 0x40) &&
                        *(int16_t *)(*(uint32_t *)(seat_tag + seat_offset + 0x44) + 0x10) != -1) {

                        vehicle = (uint8_t *)headers[
                            *(datum_index *)(rider + 0x11c) & 0xffff].data;
                        if (*(datum_index *)(vehicle + 0x324) == rider_index) {
                            unit_notify_weapon_removed();
                        }
                        animation = animation_choose_random_permutation(1);
                        unit_set_custom_animation(*(datum_index *)(unit_tag + 0x44), animation);

                        root = (uint8_t *)headers[rider_index & 0xffff].data;
                        unit_tag = (uint8_t *)tag_instances[*(datum_index *)root & 0xffff].data;
                        if (*(int32_t *)(unit_tag + 0x34) != -1) {
                            if ((*(uint32_t *)(root + 0x10) & 1) != 0) {
                                object_for_each_light_attachment(0, 1);
                            }
                            if (*(int32_t *)(unit_tag + 0x34) != -1) {
                                *(uint32_t *)(root + 0x10) =
                                    *(uint32_t *)(root + 0x10) & 0xfffffffe;
                                headers[rider_index & 0xffff].flags =
                                    (uint16_t)(headers[rider_index & 0xffff].flags | 2);
                            }
                        }

                        *(uint8_t *)(rider + 0x2a3) = 0x1b;
                        actor_notify_weapon_pickup_once();
                        if (*(uint32_t *)(rider + 0x04) == 0) {
                            unit_dispatch_scripted_event_9(0);
                        }
                    }
                }
            }
        }
        a = ai_reference_actor_iterator_next(&iterator);
        player_data = saved_player_data;
    }
    player_data = saved_player_data;
}

#if 0
Original Ghidra decompilation (0x433ea0) -- the full listing is available via
`python tools/pack.py 0x433ea0`; the shape reproduced above is:

void FUN_00433ea0(void)
{
  FUN_00432650();
  iVar11 = FUN_004326d0();
  iVar18 = DAT_0087a480;
  do {
    if (iVar11 == 0) { DAT_0087a480 = iVar18; return; }
    DAT_0087a480 = iVar18;
    if ((*(int *)(iVar11 + 0x158) != -1) && (uVar3 = *(uint *)(iVar11 + 0x18), uVar3 != 0xffffffff))
    {
      ... datum_get-style inline lookup of uVar3 in DAT_008603b0 ...
      if ((((psVar17 != (short *)0x0) && ((1 << (*(byte *)((int)psVar17 + 3) & 0x1f) & 3U) != 0)) &&
          (puVar4 = *(uint **)(psVar17 + 4), puVar4 != (uint *)0x0)) &&
         (((DAT_00719720 != 1 && (uVar5 = puVar4[0x47], uVar5 != 0xffffffff)) &&
          ((short)puVar4[0xbc] != -1)))) {
        if ((short)puVar4[0x2d] == 1) {
          ... the vehicle-rider detach path: object_get_node_local_transform,
              unit_try_set_animation_state(.., 0x25), FUN_004f6610,
              object_set_position_and_orientation, matrix4x3_multiply through
              PTR_matrix4x3_multiply_00696664, object_for_each_light_attachment,
              FUN_0056ce30 / FUN_0056d6a0 / FUN_00565420 / FUN_0055add0,
              object_recalculate_bounding_radius_recursive, FUN_00566910 /
              object_try_and_get(2), then the DAT_00719720 == 1 player-history block ...
        }
        else {
          cVar10 = FUN_00565c60();
          if (cVar10 == '\0') {
            ... the biped-rider exit-animation path: FUN_0056ab10, FUN_004d6280(1),
                unit_set_custom_animation, the same light-attachment block,
                *(undefined1 *)((int)puVar4 + 0x2a3) = 0x1b, FUN_0042c370,
                FUN_0056c370(0) ...
          }
        }
      }
    }
    iVar11 = FUN_004326d0();
    iVar18 = DAT_0087a480;
  } while( true );
}
#endif
