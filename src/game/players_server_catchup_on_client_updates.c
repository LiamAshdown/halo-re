// players_server_catchup_on_client_updates  (Ghidra: FUN_004768c0; named after its client-side
// twin players_client_catchup_on_server_updates @0x476d40)
// address 0x4768c0, size 1050 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: objdump -d 0x4768c0..0x476cd9 (Ghidra's decompile mistook the update_server_queues
//   element base for a D3DX string literal and lost every register argument). game_simulate_tick
//   calls it only on a host (network_game_mode == 2) outside the prediction pass. For every
//   remote player (local_player_index == -1) it looks at that player's update_server_queue
//   (update_server_queues @0x006f1d90, element stride 0x64, player_update_queue at +0x28) and,
//   while the queue holds more than server_maximum_queued_client_updates records (0x006887b4,
//   2 in the image) or their references_remaining add up to more than
//   server_maximum_pending_client_update_ticks (0x006887b8, 6), replays one more tick of it:
//   pops the head record (the inline copy of player_update_queue_pop_current @0x479fb0), turns
//   its player_action into a unit_control_data (or, when local_player_globals->input_disabled is
//   set, one seeded from the unit's own desired vectors if no actor or swarm owns it), applies
//   it (unit_apply_control_block @0x5639f0) and runs one simulation step of the unit: its
//   parent's object_update when player_unit_has_parent and network_client_vehicle_ack_enabled,
//   otherwise unit_update then biped_update.
//   The control block built from an action: animation_state 3, aiming_speed 0, the low word of
//   control_flags, weapon/grenade/zoom from the action (the weapon slot is forced to the unit's
//   current one when that weapon must_be_readied, WeaponFlags bit 3 at Weapon +0x308), throttle
//   (x, y, 0), primary_trigger, and the forward vector player_compute_view_forward_vector
//   derives from the action's yaw/pitch in all three of facing/aiming/looking.
//   source_id is the record's field0 once its last reference is consumed, else -1.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// UNSURE: the names of 0x006887b4 / 0x006887b8 (only read here); unit_control_data.unknown_0a
//   is left unwritten on both construction paths, exactly as compiled (stack garbage there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;                     // 0x0087a480
extern data_array *object_data;                     // 0x008603b0
extern tag_instance *tag_instances;                 // 0x0087bc14
extern data_array *update_server_queues;            // 0x006f1d90
extern player_globals *local_player_globals;        // 0x0087a478
extern const real_point3d *global_origin3d_pointer; // 0x00696714
extern uint8_t network_client_vehicle_ack_enabled;  // 0x006894a1
extern int32_t server_maximum_queued_client_updates;      // 0x006887b4 UNSURE name (2)
extern int32_t server_maximum_pending_client_update_ticks; // 0x006887b8 UNSURE name (6)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern uint8_t player_update_queue_pop_current(player_update_record *out,
    player_update_queue *queue); // 0x479fb0, blam-cc: EAX -> out, EBX -> queue
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch,
    real_vector3d *out_forward); // 0x473d70, blam-cc: EAX, ECX, ESI
extern void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control,
    int32_t source_id); // 0x5639f0, blam-cc: EAX -> unit_index, EDX -> control, stack -> source_id
extern uint8_t player_unit_has_parent(datum_index player_handle); // 0x477210, blam-cc: ECX
extern uint8_t object_update(uint32_t object_index); // 0x4f7ef0
extern uint8_t unit_update(uint32_t unit_index);     // 0x5625b0
extern uint32_t biped_update(uint32_t object_index); // 0x5590a0

static object *object_from_index(datum_index object_index)
{
    return ((object_header *)object_data->data)[object_index & 0xffff].data;
}

// Queued client updates (in records) currently waiting in `queue`.
static int32_t player_update_queue_count(const circular_queue *queue)
{
    if (queue->read_index < queue->write_index) {
        return queue->write_index - queue->read_index;
    }
    if (queue->write_index < queue->read_index) {
        return queue->write_index - queue->read_index + queue->capacity;
    }
    return 0;
}

// Replays remote players' queued control updates on the host until each player's backlog is
// within the allowed number of records and ticks.
void players_server_catchup_on_client_updates(void)
{
    data_iterator player_iter;
    player *plr;

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_data ^ k_data_iterator_signature;

    for (plr = (player *)data_iterator_next(&player_iter); plr != 0;
         plr = (player *)data_iterator_next(&player_iter)) {
        player_update_queue *queue;

        if (plr->local_player_index != -1) {
            continue;
        }
        queue = &((update_server_queue *)update_server_queues->data)[player_iter.index & 0xffff].queue;

        for (;;) {
            player_update_record record;
            player_action action;
            int32_t source_id;
            object *unit_obj;
            unit_data *unit;
            unit_control_data control;

            if (player_update_queue_count(&queue->queue) <= server_maximum_queued_client_updates) {
                int32_t pending_ticks = 0;
                int32_t i = queue->queue.read_index;
                int32_t write_index = queue->queue.write_index;

                while (i != write_index) {
                    pending_ticks += ((player_update_record *)queue->queue.records[i])->references_remaining;
                    i = (i + 1) % k_player_update_history_count; // idiv by the literal 0x78
                }
                if (pending_ticks <= server_maximum_pending_client_update_ticks) {
                    break;
                }
            }

            player_update_queue_pop_current(&record, queue);
            action = record.action;
            source_id = record.references_remaining != 0 ? -1 : (int32_t)record.field0;

            if (plr->unit == (datum_index)-1) {
                continue;
            }
            unit_obj = object_from_index(plr->unit);
            unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
            if ((unit->flags & 0x40) == 0) { // UNSURE: unnamed unit_flags bit 6
                continue;
            }

            if (local_player_globals->input_disabled == 0) {
                if (unit->current_weapon_index != -1) {
                    datum_index weapon_index = unit->weapons[unit->current_weapon_index];
                    if (weapon_index != (datum_index)-1) {
                        Weapon *weapon = (Weapon *)tag_instances[object_from_index(weapon_index)->definition_tag & 0xffff].data;
                        if ((weapon->weapon_flags & 0x8) != 0) { // must_be_readied
                            action.weapon_index = unit->current_weapon_index;
                        }
                    }
                }
                control.control_flags = (uint16_t)action.control_flags;
                player_compute_view_forward_vector(player_iter.index, &action.desired_yaw, &control.aiming_vector);
                control.facing_vector = control.aiming_vector;
                control.looking_vector = control.aiming_vector;
                control.throttle.i = action.throttle_x;
                control.throttle.j = action.throttle_y;
                control.throttle.k = 0.0f;
                control.primary_trigger = action.primary_trigger;
                control.weapon_index = action.weapon_index;
                control.grenade_index = action.grenade_index;
                control.zoom_level = action.zoom_level;
                control.animation_state = 3;
                control.aiming_speed = 0;
                unit_apply_control_block(plr->unit, &control, source_id);
            } else if (unit->swarm_actor_index == (datum_index)-1 && unit->actor_index == (datum_index)-1) {
                control.weapon_index = -1;
                control.grenade_index = -1;
                control.zoom_level = -1;
                control.throttle = *(const real_vector3d *)global_origin3d_pointer;
                control.facing_vector = unit->desired_facing_vector;
                control.aiming_vector = unit->desired_aiming_vector;
                control.looking_vector = unit->desired_looking_vector;
                control.animation_state = 3;
                control.aiming_speed = 0;
                control.control_flags = 0;
                control.primary_trigger = 0.0f;
                unit_apply_control_block(plr->unit, &control, source_id);
            }

            if (player_unit_has_parent(player_iter.index) && network_client_vehicle_ack_enabled != 0) {
                object_update(unit_obj->parent_object);
            } else {
                unit_update(plr->unit);
                biped_update(plr->unit);
            }
        }
    }
}

#if 0
Disassembly outline (0x4768c0..0x476cd9); the Ghidra decompile is in out/halo_decompiled.c and
reads the update_server_queues element as an offset into the string at 0x662xxx.
  4768c6 iterator over player_data (inline data_iterator_new), data_iterator_next(EDI=&iter)
  476910 skip players with local_player_index != -1
  47691b ebx = update_server_queues->data + (iter.index & 0xffff) * 0x64 + 0x28
  476938 count = write - read (wrapping by capacity); if count <= [0x6887b4]:
         sum records[i]->references_remaining for i = read .. write (mod 0x78); <= [0x6887b8] -> next player
  47699a record.field0 = record.references_remaining = -1; inline pop_current; action = record + 0xc
         ebp = references_remaining ? -1 : field0
  476a1d unit = player->unit; -1 -> 476938; !(unit flags & 0x40) -> 476938
  476a57 local_player_globals->unknown_11 == 0:
         must_be_readied weapon -> action.weapon_index = unit->current_weapon_index
         player_compute_view_forward_vector(EAX=iter.index, ECX=&action.desired_yaw, ESI=&control+0x28)
         block {3, 0, (short)flags, weapon, grenade, zoom, ?, throttle x y 0, trigger, fwd, fwd, fwd}
  476b81 else if actor_index == -1 && swarm_actor_index == -1:
         block {3, 0, 0, -1, -1, -1, ?, *global_origin3d_pointer, 0, desired facing/aiming/looking}
  476c6e unit_apply_control_block(EAX=unit, EDX=&block, push source_id)
  476c77 player_unit_has_parent(ECX=iter.index) && [0x6894a1] ? object_update(unit->parent_object)
         : unit_update(player->unit), biped_update(player->unit); -> 476938
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
