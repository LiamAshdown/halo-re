// ai_reference_units_exit_vehicles  (Ghidra: FUN_00433ea0)
// address 0x433ea0, size 1627 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// REWRITTEN from objdump 0x433ea0..0x4344fa (the draft called datum_get, object_try_and_get and the scripted
//   animation test without their register arguments). EAX: the packed ai reference. Every actor it names
//   (0x432650 / 0x4326d0) that still has +0x158 set and a live unit in a vehicle seat -- never on a client --
//   leaves the seat exactly as biped_update's inline sequence does (compared instruction by instruction): a
//   seated vehicle is detached (then a client drops the local player's prediction history), a biped not in a
//   scripted animation plays its seat's slot 8 animation, loses its lights and enters state 0x1b.
// blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "cache.h"
#include "ai.h"
#include "networking.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480
extern int16_t game_connection_role; // 0x00719720: 1 = client
extern game_time_globals *game_time; // 0x006f1d6c
extern network_client_globals *network_client;

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, stack, ECX
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, EDX
extern void actor_notify_weapon_pickup_once(datum_index object_index); // 0x42c370, ECX
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX, ESI
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0 (via 0x696664)
extern void player_update_history_free_all(void *history); // 0x4e6f20
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up,
    real_point3d *position); // 0x4f51c0, stack, EDI position
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t flags); // 0x4f6080
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20, EAX, stack
extern void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index); // 0x55add0, stack, EDI
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack, ECX
extern uint8_t unit_state_is_scripted_animation(unit_data *unit); // 0x565c60, ECX
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, EAX
extern void unit_notify_weapon_removed(int32_t object_index); // 0x56ab10, EAX
extern void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); // 0x56c370, stack, ECX
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX, DX, stack
extern void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); // 0x56ebd0

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define OBJECT_HEADER(h) (((object_header *)object_data->data)[(h) & 0xffff])
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

// 0x5596d3.. / 0x5591a9..: take the unit out of its vehicle seat, keep it where its body was.
static void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = OBJECT_DATA(object_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *nodes = self + *(int16_t *)(self + 0x1f2);
    uint8_t *seat = *(uint8_t **)(TAG_DATA(*(datum_index *)vehicle) + 0x2e8) + *(int16_t *)(self + 0x2f0) * 0x11c;
    uint8_t *model_nodes;
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &marker, 1);
    offset.x = *(float *)(nodes + 0x28) - marker.node_transform.position.x;
    offset.y = *(float *)(nodes + 0x2c) - marker.node_transform.position.y;
    offset.z = *(float *)(nodes + 0x30) - marker.node_transform.position.z;
    model_nodes = *(uint8_t **)(TAG_DATA(*(datum_index *)(TAG_DATA(*(datum_index *)self) + 0x34)) + 0xbc);
    default_translation = *(real_point3d *)(model_nodes + 0x28);
    if (*(datum_index *)(vehicle + 0x324) == object_index && vehicle[0x2a3] != 0x25 &&
        *(datum_index *)(self + 0x11c) != k_datum_index_none) {
        unit_try_set_animation_state(*(datum_index *)(self + 0x11c), 0x25);
    }
    *(datum_index *)(self + 0x32c) = vehicle_index;
    *(int32_t *)(self + 0x330) = game_time->game_time;
    if (*(datum_index *)(self + 0x324) == object_index) {
        *(datum_index *)(self + 0x324) = k_datum_index_none;
    }
    if (*(datum_index *)(self + 0x328) == object_index) {
        *(datum_index *)(self + 0x328) = k_datum_index_none;
    }
    object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + *(float *)(self + 0x5c);
    position.y = offset.y + *(float *)(self + 0x60);
    position.z = offset.z + *(float *)(self + 0x64) - default_translation.z;
    object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = OBJECT_DATA(object_index);

        matrix4x3_multiply((real_matrix4x3 *)(reloaded + *(int16_t *)(reloaded + 0x1f2)),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)(self + 0x74) = basis.forward;
    *(real_vector3d *)(self + 0x80) = basis.up;
    {
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag = TAG_DATA(*(datum_index *)object);

        if (*(int32_t *)(object_tag + 0x34) != -1 && (object[0x10] & 1) != 0) {
            object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)(object_tag + 0x34) != -1) {
            *(uint32_t *)(object + 0x10) &= ~1u;
            OBJECT_HEADER(object_index).flags |= 2;
        }
    }
    *(int16_t *)(self + 0x2f0) = -1;
    self[0x2a7] = 2;
    if (*(datum_index *)(vehicle + 0x324) == object_index) {
        *(datum_index *)(vehicle + 0x324) = k_datum_index_none;
    }
    if (*(datum_index *)(vehicle + 0x328) == object_index) {
        *(datum_index *)(vehicle + 0x328) = k_datum_index_none;
    }
    unit_recompute_seat_occupants(vehicle_index);
    unit_pick_and_ready_next_weapon(object_index);
    {
        int8_t request[2] = { 0x14, 0 };

        unit_update_animation_state_machine(object_index, request);
    }
    *(real_point3d *)(self + *(int16_t *)(self + 0x1ea) + 0x10) = default_translation;
    if (*(int16_t *)(self + 0xb4) == 0) {
        unit_reset_orientation_and_find_position(object_index, vehicle_index); // EDI = the seat parent
    }
    object_recalculate_bounding_radius_recursive(object_index);
    if (unit_all_seats_unoccupied(vehicle_index) == 1) {
        uint8_t *empty = (uint8_t *)object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = game_time->game_time;
        }
    }
    if (game_connection_role == 1) {
        uint8_t *player = (uint8_t *)datum_get(*(datum_index *)(self + 0x218), player_data);

        if (player != 0 && *(int16_t *)(player + 2) == -1) {
            *(int32_t *)(player + 0x180) = 0;
            *(int32_t *)(player + 0x17c) = 0;
            *(int32_t *)(player + 0x1e0) = 0;
            *(int32_t *)(player + 0x1dc) = 0;
        }
    }
}

// 0x559505 / 0x559a59: a client drops the prediction history of a local player's unit.
static void biped_free_local_player_history(uint8_t *self)
{
    datum_index player_index = *(datum_index *)(self + 0x218);
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    uint8_t *player;

    if (game_connection_role != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= *(int16_t *)((uint8_t *)player_data + 0x20)) {
        return;
    }
    player = (uint8_t *)player_data->data + *(int16_t *)((uint8_t *)player_data + 0x22) * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || *(int16_t *)(player + 2) == -1) {
        return;
    }
    if (network_client != 0) {
        player_update_history_free_all(*(void **)&network_client->update_history);
    }
}

void ai_reference_units_exit_vehicles(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *actor_record;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    for (actor_record = ai_reference_actor_iterator_next(&iterator); actor_record != 0;
         actor_record = ai_reference_actor_iterator_next(&iterator)) {
        datum_index unit_index = *(datum_index *)((uint8_t *)actor_record + 0x18);
        int16_t index = (int16_t)unit_index;
        int16_t salt = (int16_t)(unit_index >> 16);
        uint8_t *header;
        uint8_t *self;
        datum_index vehicle_index;

        if (*(datum_index *)((uint8_t *)actor_record + 0x158) == k_datum_index_none ||
            unit_index == k_datum_index_none || index < 0 || index >= object_data->maximum_count) {
            continue;
        }
        header = (uint8_t *)object_data->data + object_data->size * index;
        if (*(int16_t *)header == 0 || (salt != 0 && *(int16_t *)header != salt) ||
            ((1u << (header[3] & 0x1f)) & 3) == 0) {
            continue;
        }
        self = *(uint8_t **)(header + 0x8);
        if (self == 0 || game_connection_role == 1 ||
            (vehicle_index = *(datum_index *)(self + 0x11c)) == k_datum_index_none ||
            *(int16_t *)(self + 0x2f0) == -1) {
            continue;
        }
        if (*(int16_t *)(self + 0xb4) == 1) {
            uint8_t *me = OBJECT_DATA(unit_index);

            if (*(datum_index *)(me + 0x11c) != k_datum_index_none && *(int16_t *)(me + 0x2f0) != -1) {
                biped_detach_from_seat(unit_index, *(datum_index *)(me + 0x11c));
            }
            biped_free_local_player_history(me);
        } else if (!unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
            uint8_t *self_tag = TAG_DATA(*(datum_index *)self);
            datum_index graph = *(datum_index *)(self_tag + 0x44);
            uint8_t *seat_block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)self[0x2a0] * 0x64;

            if (*(int32_t *)(seat_block + 0x40) > 8 && (*(int16_t **)(seat_block + 0x44))[8] != -1) {
                int16_t exit_animation = (*(int16_t **)(seat_block + 0x44))[8];
                uint8_t *object;
                uint8_t *object_tag;

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
                        OBJECT_HEADER(unit_index).flags |= 2;
                    }
                }
                self[0x2a3] = 0x1b;
                actor_notify_weapon_pickup_once(unit_index);
                if (*(int32_t *)(self + 4) == 0) {
                    unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
                }
            }
        }
    }
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
