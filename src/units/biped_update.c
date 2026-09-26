// biped_update  (Ghidra: unit_update, renamed)
// address 0x5590a0, size 3460 bytes
// name confidence: 0.85   rewrite confidence: 0.3
// evidence: out/phase4/units_types_notes.md "Misattributed functions": the object_type_definition
//   vtable read straight out of .data (0x0069bfdc) puts this address in the *biped* row's
//   +0x34 ("update") column, not the unit row's -- the real unit_update is 0x5625b0, outside
//   this batch. This is the single highest-value correction the notes call out for the module,
//   so this rewrite uses the corrected name even though Ghidra still reports the old one.
// register convention: object index recognized as a normal (stack) parameter; every callee this
//   function invokes with literally no visible arguments in Ghidra's decompile (FUN_0042c370,
//   FUN_00492730, FUN_004c2ee0, FUN_004c4b50, unit_state_is_scripted_animation, unit_all_seats_unoccupied, unit_notify_weapon_removed,
//   unit_recompute_seat_occupants, unit_pick_and_ready_next_weapon, unit_get_weapon_object_index, unit_evaluate_flee_reaction) is declared and
//   called with no arguments here too, to avoid inventing a binding this decompilation does not
//   show; every register-passed input actually needed is the object index itself, which x86
//   calling conventions would leave live in EAX/ECX across most of these calls without any
//   visible reload.
// UNSURE (see body comments for detail): the exact ECX index object_try_and_get(3) receives in
//   the vehicle-seat branch (modeled here as the biped's parent, i.e. the vehicle, re-validated
//   after unit_evaluate_flee_reaction may have changed things); the CONCAT12/CONCAT22 byte-splicing around the
//   melee-timer countdown (lines ~427-443 of the original) has been algebraically simplified --
//   see detach_and_realign_to_parent_seat's melee-timer analogue is absent, this is in the tail
//   section instead, simplified to `biped->unknown_506 = biped->unknown_505 - (int8_t)second_roll`
//   after confirming the CONCAT construction reduces to exactly that. local_8/local_7 in the
//   original are written on one early-exit path (parent.type==0) but never read on any reachable
//   path afterward (the LAB_00559dd2 tail never touches them, and the function always returns 1
//   regardless), so they are omitted here as dead.
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)
// reconciled: R32 follow-up: local extern player_control_globals (0x0071c2d8) renamed network_client (networking.h name) because game.h is now included and owns the player_control_globals typedef
// reconciled: R26 object.unknown_018 -> network_position_valid (0x018)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480, players module, stride 0x200 (types/units.h)
extern int32_t game_connection_role; // 0x00719720: 1 = client, 2 = server (types/units.h)
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)
extern uint8_t *network_client; // 0x0071c2d8 (networking.h network_client; renamed from network_client, which collides with game.h's typedef), +0xf48 is the prediction history (types/units.h)
extern uint8_t DAT_006893cc;         // UNSURE: unresolved global, gates the "falling out of a
                                      // vehicle" reposition below
extern uint8_t unit_updates_suppressed; // 0x0071c419, types/units.h
extern real_vector3d *global_forward3d_pointer; // 0x00696718: indirect pointer to math.h's
                                             // global_forward3d (0x0065c20c), per this function's
                                             // own "globals referenced" list
extern real_point3d *global_origin3d_pointer;   // 0x00696714, math.h global_origin3d_pointer

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place, returns length, ECX (verified elsewhere in this module)
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0, verified in src/math
extern void * datum_get(datum_index handle, data_array *array);      // 0x4d0680, memory module
extern void player_update_history_free_all(void *history);          // 0x4e6f20, game module
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward,
                                                 real_vector3d *up, real_point3d *position); // 0x4f51c0, position in EDI
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); // 0x4f9a20, object_index in EAX
extern void unit_recalculate_position(uint32_t object_index); // 0x558eb0
extern void unit_reset_orientation_and_find_position(uint32_t object_index); // 0x55add0, this batch: reset orientation basis / find spawn position
extern void biped_integrate_movement_with_collision(uint32_t object_index, int8_t *state); // 0x55cfd0, this batch: full per-tick movement
extern void unit_evaluate_flee_reaction(uint32_t object_index); // 0x55e2d0, this batch: vehicle flee/evade evaluation, no visible args
extern void unit_check_fell_off_level(uint32_t object_index); // 0x55e4a0, this batch: fallen-below-level check, object_index implicit
extern void biped_check_evade_reaction(uint32_t object_index); // 0x55e190, this batch: evasive reaction check
extern void biped_update_idle_basis(uint32_t object_index, uint8_t *state_out); // 0x55e840, this batch: idle basis refresh selection
extern void biped_apply_idle_fidget(uint32_t object_index, uint8_t *state_out); // 0x55e940, this batch: idle fidget impulse
extern void biped_advance_frame_counter_trigger(uint32_t object_index, char *state_out); // 0x55eb90, this batch: animation frame trigger
extern void biped_trigger_on_velocity_threshold(uint32_t object_index); // 0x55ec20, this batch: velocity-threshold trigger
extern uint32_t unit_snap_to_min_ground_height(uint32_t object_index); // 0x55ecf0, this batch: snap to min ground height
extern void biped_update_facing(uint32_t object_index, int8_t *out_animation_state); // 0x55b7c0, EAX, stack
  // real signature (biped_update_facing.c): void biped_update_facing(uint32_t object_index, int8_t *out_animation_state); Ghidra recovered 1 of 2 args at this call site
extern void unit_update_footstep_and_idle_triggers(uint32_t unit_index); // 0x560410, next batch: seat/turret angle-limit trigger
extern void unit_update_up_vector(Biped *biped_tag, object *obj); // 0x560800, next batch: level up-vector toward target
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, next batch: seat-transition dispatcher; returns a status this
                                                  // rewrite discards where Ghidra shows a used result (see body)
extern uint8_t unit_state_is_scripted_animation(unit_data *unit); // 0x565c60, next batch: uninterruptible/special-move state test
extern void unit_start_seat_overlay_animation_a(uint32_t unit_index, int16_t command); // 0x565e00, next batch: seat-control overlay starter
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index); // 0x566910, next batch: seat-occupancy test
extern void unit_notify_weapon_removed(void); // 0x56ab10, next batch: weapon-removal guard
  // real signature (unit_notify_weapon_removed.c): void unit_notify_weapon_removed(int32_t object_index, int16_t new_state); Ghidra recovered 0 of 2 args at this call site
extern void unit_dispatch_scripted_event_9(int32_t param_1); // 0x56c370, next batch: scripted event dispatch
  // real signature (unit_dispatch_scripted_event_9.c): void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); Ghidra recovered 1 of 2 args at this call site
extern void unit_recompute_seat_occupants(uint32_t unit_index); // 0x56ce30, EAX
  // real signature (unit_recompute_seat_occupants.c): void unit_recompute_seat_occupants(uint32_t unit_index); Ghidra recovered 0 of 1 args at this call site
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index); // 0x56d6a0, ESI
  // real signature (unit_pick_and_ready_next_weapon.c): void unit_pick_and_ready_next_weapon(uint32_t unit_index); Ghidra recovered 0 of 1 args at this call site
extern void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); // 0x56ebd0
extern void unit_melee_attack_scan(uint32_t unit_index); // 0x56f550
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, EAX, CX
  // real signature (unit_get_weapon_object_index.c): datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); Ghidra recovered 1 of 2 args at this call site
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // UNSURE module, object_index visible
extern uint32_t actor_notify_weapon_pickup_once(void); // UNSURE module, implicit args only
extern void weapon_action_notify_for_unit(datum_index unit_index, int32_t action_code); // 0x492730, EAX, stack
extern uint32_t weapon_prevents_melee_attack(datum_index item_index); // 0x4c2ee0, ECX
extern void weapon_reset_triggers(datum_index weapon_index); // UNSURE module
extern int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index,
    int16_t category, int16_t mode); // 0x4c2f80, EAX, CX, stack
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90, next batch (signature per sibling agent's src/units/unit_update_animation_state_machine.c)
extern int16_t animation_choose_random_permutation(uint32_t flag); // UNSURE module

// Shared by both places biped_update repositions this unit relative to a parent's seat marker
// (entering a vehicle seat, and the "unit fell below its parent" recheck later in the same
// function): looks up the seat's marker_name on the parent's Unit tag, reads the marker's
// current local transform, derives a world position from the delta between the biped's own
// root-node position and that marker (offset by the tag's default root-node translation on Z
// only -- preserved exactly, not simplified), applies it via object_set_position_and_orientation,
// recombines the root node's current basis with the model's default root-node basis into the
// object's forward/up vectors, then clears the transient seat-tracking fields this reposition
// invalidates and lets the seat-occupancy/weapon-switch/seat-transition machinery catch up.
// UNSURE: parent_object_index's exact provenance at each call site (see call sites below).
static void detach_and_realign_to_parent_seat(uint32_t object_index, datum_index parent_object_index,
                                               int16_t vehicle_seat_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object *parent = ((object_header *)object_data->data)[parent_object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    unit_data *parent_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
    Unit *parent_tag = (Unit *)tag_instances[parent->definition_tag & 0xffff].data;
    UnitSeat *seat = &((UnitSeat *)parent_tag->seats.pointer)[vehicle_seat_index];
    Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    GBXModel *model = (GBXModel *)tag_instances[object_tag->model.tag_id.index].data;
    ModelNode *model_nodes = (ModelNode *)model->nodes.pointer;
    real_matrix4x3 *own_nodes = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);
    object_marker marker;
    real_point3d delta, new_position;
    real_matrix4x3 combined;

    object_get_node_local_transform(parent_object_index, seat->marker_name.string, &marker, 1);

    delta.x = own_nodes[0].position.x - marker.node_transform.position.x;
    delta.y = own_nodes[0].position.y - marker.node_transform.position.y;
    delta.z = own_nodes[0].position.z - marker.node_transform.position.z;
    new_position.x = delta.x + obj->position.x;
    new_position.y = delta.y + obj->position.y;
    new_position.z = (delta.z + obj->position.z) - model_nodes[0].default_translation.z; // preserved as-is; Z only

    if (parent_unit->driver_unit_index == object_index && parent_unit->animation_state != 0x25 &&
        obj->parent_object != k_datum_index_none) {
        unit_try_set_animation_state(obj->parent_object, 0x25);
    }

    unit->last_parent_object_index = parent_object_index;
    unit->last_seat_change_tick = game_time->game_time;
    if (unit->driver_unit_index == object_index) unit->driver_unit_index = k_datum_index_none;
    if (unit->gunner_unit_index == object_index) unit->gunner_unit_index = k_datum_index_none;

    // UNSURE: FUN_004f6610, no visible arguments in Ghidra beyond object_index
    object_snap_to_parent_marker_and_detach(object_index);

    object_set_position_and_orientation(object_index, 0, 0, &new_position);

    {
        real_matrix4x3 *own_root = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);
        // ModelNode (types/tags.h, 0x9c bytes) ends with scale / rotation / translation at
        // +0x68, +0x6c and +0x90 -- bit for bit a real_matrix4x3 (scale, forward, left, up,
        // position). Taken through &model_nodes[0].scale so the offset stays in tags.h.
        real_matrix4x3 *default_root = (real_matrix4x3 *)&model_nodes[0].scale;
        matrix4x3_multiply(own_root, default_root, &combined);
        obj->forward = combined.forward;
        obj->up = combined.up;
    }

    // Ghidra's puVar3 here is *our own* object pointer, re-derived fresh (object_data->data at
    // this unit's own header slot again); local_1c is our own tag's model.tag_id (Object+0x34):
    // a light-attachment refresh, gated on whether this unit even has a model tag assigned.
    if (*(uint32_t *)&object_tag->model.tag_id != 0xffffffff && (obj->flags & 1) != 0) {
        object_for_each_light_attachment(object_index, 0, 1);
    }
    if (*(uint32_t *)&object_tag->model.tag_id != 0xffffffff) {
        obj->flags = obj->flags & ~1u;
        // this unit's own object_header.flags |= 2 (a separate byte from object.flags above)
        ((object_header *)object_data->data)[object_index & 0xffff].flags |= 2;
    }

    unit->vehicle_seat_index = -1;
    unit->base_animation_state = 2;
    if (parent_unit->driver_unit_index == object_index) parent_unit->driver_unit_index = k_datum_index_none;
    if (parent_unit->gunner_unit_index == object_index) parent_unit->gunner_unit_index = k_datum_index_none;

    // FIXED (0x559422..0x55943d / 0x559969..0x559987): the parent's seat occupants are recomputed (EAX = the
    // parent), this unit readies its next weapon (ESI), and the state machine runs with ECX = a local request
    // {0x14, 0} (the draft passed no arguments and a NULL request).
    {
        int8_t exit_request[2] = { 0x14, 0 };

        unit_recompute_seat_occupants(parent_object_index);
        unit_pick_and_ready_next_weapon(object_index);
        unit_update_animation_state_machine(object_index, exit_request);
    }

    {
        // UNSURE: offset 0x1ea within the unit relative to *(short*)(puVar10+0x1ea)+puVar10+0x10,
        // i.e. object + (a cached signed offset) + 0x10; not identified against any documented
        // field. Preserved as a raw write of the tag's default root-node translation.
        Point3D *target = (Point3D *)((uint8_t *)obj + *(int16_t *)((uint8_t *)obj + 0x1ea) + 0x10);
        target->x = model_nodes[0].default_translation.x;
        target->y = model_nodes[0].default_translation.y;
        target->z = model_nodes[0].default_translation.z;
    }

    if (*(int16_t *)((uint8_t *)obj + 0xb4) == 0) { // object.type == biped: only reset ground-adjust state for bipeds
        unit_reset_orientation_and_find_position(object_index);
    }
    object_recalculate_bounding_radius_recursive(object_index);

    if (unit_all_seats_unoccupied(object_index) == 1) { // index in EAX
        object *vehicle = object_try_and_get(object_index, 2);
        if (vehicle != 0) {
            *(int32_t *)((uint8_t *)vehicle + 0x5ac) = game_time->game_time; // vehicle_data.network_update_tick
        }
    }

    if (game_connection_role == 1) {
        void *history = datum_get(0, player_data); // UNSURE: index argument not visible in the decompile
        if (history != 0 && *(int16_t *)((uint8_t *)history + 2) == -1) {
            *(int32_t *)((uint8_t *)history + 0x180) = 0;
            *(int32_t *)((uint8_t *)history + 0x17c) = 0;
            *(int32_t *)((uint8_t *)history + 0x1e0) = 0;
            *(int32_t *)((uint8_t *)history + 0x1dc) = 0;
        }
    }
}

// Top-level per-frame update for a biped object: while seated in a vehicle, keeps the seat
// occupancy, weapon-switch and prediction-history bookkeeping in sync with the vehicle each
// tick (and repositions relative to the seat marker if the unit has fallen too far below its
// parent); otherwise runs the normal grounded-biped tick: leveling the up-vector, driving
// facing/movement/animation-state triggers, and gating melee/evade reactions on a per-tick
// countdown.
uint32_t biped_update(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    int8_t request[2] = { 0, 0 }; // [ebp-0x4] state, [ebp-0x3] action flag (0x55910a/0x55910e)

    if (obj->network_role == 1 && obj->network_position_valid == 1 && obj->parent_object == k_datum_index_none) {
        unit_recalculate_position(object_index);
    }

    if (obj->parent_object != k_datum_index_none) {
        object *parent = ((object_header *)object_data->data)[obj->parent_object & 0xffff].data;

        if (*(int16_t *)((uint8_t *)parent + 0xb4) != 1) {
            // parent is not a vehicle: if it's a biped, nothing further to do here (local_8's
            // write on this path is provably dead, see file header); either way skip to the tail.
            goto tail;
        }

        unit_evaluate_flee_reaction(object_index); // index in EDI

        if ((unit->control_flags & 0x40) != 0) {
            // UNSURE: object_try_and_get's index argument is ECX, not visible in the decompile;
            // re-validating the vehicle we are parented to (mask 3 = biped|vehicle) is the
            // interpretation that makes the candidate.type==1 test below meaningful.
            object *candidate = object_try_and_get(obj->parent_object, 3);
            if (candidate != 0 && game_connection_role != 1 &&
                candidate->parent_object != k_datum_index_none &&
                *(int16_t *)((uint8_t *)candidate + 0x2f0) != -1) { // unit_data.vehicle_seat_index
                if (*(int16_t *)((uint8_t *)candidate + 0xb4) == 1) {
                    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
                    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
                    if (candidate->parent_object == k_datum_index_none ||
                        *(int16_t *)((uint8_t *)candidate + 0x2f0) == -1) {
                        goto network_role_check;
                    }
                    detach_and_realign_to_parent_seat(object_index, candidate->parent_object,
                                                       *(int16_t *)((uint8_t *)candidate + 0x2f0));
                network_role_check:
                    if (game_connection_role == 1) {
                        datum_index controlling_player = unit->controlling_player;
                        if (controlling_player != k_datum_index_none &&
                            (int16_t)controlling_player >= 0 &&
                            (int16_t)controlling_player < *(int16_t *)((uint8_t *)player_data + 0x20)) {
                            int32_t entry = (int32_t)*(int16_t *)((uint8_t *)player_data + 0x22) *
                                            (int16_t)controlling_player;
                            int16_t sequence = *(int16_t *)(entry + *(int32_t *)((uint8_t *)player_data + 0x34));
                            int16_t salt = (int16_t)(controlling_player >> 0x10);
                            if (sequence != 0 && (salt == 0 || sequence == salt) &&
                                *(int16_t *)(entry + *(int32_t *)((uint8_t *)player_data + 0x34) + 2) != -1 &&
                                network_client != 0) {
                                player_update_history_free_all(*(void **)(network_client + 0xf48));
                            }
                        }
                    }
                } else {
                    // candidate.type != vehicle: nothing more to do on this path
                }
            }
        } else if (unit_state_is_scripted_animation(unit) == 0) { // unit_data * in ECX
            Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
            ModelAnimations *graph = (ModelAnimations *)tag_instances[object_tag->animation_graph.tag_id.index].data;
            int8_t animation_definition_index = unit->animation_definition_index;
            uint8_t *unit_block = (uint8_t *)graph->units.pointer + animation_definition_index * 100; // UNSURE: 100 = ModelAnimationsAnimationGraphUnitSeat stride per units.h

            if (*(int32_t *)(unit_block + 0x40) > 8 && *(int16_t *)(*(int32_t *)(unit_block + 0x44) + 0x10) != -1) {
                // UNSURE: the original truncates a leftover tag-data pointer (local_c, still
                // holding this unit's own tag data from the top of the function, never
                // re-derived on this path) to 16 bits and uses it as an object index -- almost
                // certainly a decompiler artifact of dead/reused storage rather than intended
                // logic. Preserved bug-for-bug via the same stale pointer.
                {
                    void *stale_tag_data_pointer = tag_instances[obj->definition_tag & 0xffff].data;
                    uint16_t bogus_index = (uint16_t)(uint32_t)stale_tag_data_pointer;
                    object *bogus_object = ((object_header *)object_data->data)[bogus_index].data;
                    if (((unit_data *)((uint8_t *)bogus_object + k_unit_data_offset))->driver_unit_index == object_index) {
                        unit_notify_weapon_removed();
                    }
                }
                {
                    uint32_t custom_animation_index = animation_choose_random_permutation(1);
                    unit_set_custom_animation(object_index, object_tag->animation_graph.tag_id.index,
                                               custom_animation_index);
                }
                obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
                object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
                if (*(uint32_t *)&object_tag->model.tag_id != 0xffffffff) {
                    if ((obj->flags & 1) != 0) {
                        object_for_each_light_attachment(object_index, 0, 1);
                    }
                    if (*(uint32_t *)&object_tag->model.tag_id != 0xffffffff) {
                        obj->flags = obj->flags & ~1u;
                    }
                }
                unit->animation_state = 0x1b;
                actor_notify_weapon_pickup_once();
                if (parent->network_role == 0) {
                    unit_dispatch_scripted_event_9(0);
                }
            }
        }

        if (DAT_006893cc != 0 && obj->position.z < 0.0f && (parent->flags & 2) != 0 &&
            game_connection_role != 1) {
            if (unit->vehicle_seat_index != -1) { // re-validated parent+seat, see file header
                detach_and_realign_to_parent_seat(object_index, obj->parent_object, unit->vehicle_seat_index);
            }
            if (parent->network_role == 0) {
                unit_dispatch_scripted_event_9(1);
            }
            if (game_connection_role == 1) {
                datum_index controlling_player = unit->controlling_player;
                if (controlling_player != k_datum_index_none && (int16_t)controlling_player >= 0 &&
                    (int16_t)controlling_player < *(int16_t *)((uint8_t *)player_data + 0x20)) {
                    int32_t entry = (int32_t)*(int16_t *)((uint8_t *)player_data + 0x22) *
                                    (int16_t)controlling_player;
                    int16_t sequence = *(int16_t *)(entry + *(int32_t *)((uint8_t *)player_data + 0x34));
                    int16_t salt = (int16_t)(controlling_player >> 0x10);
                    if (sequence != 0 && (salt == 0 || sequence == salt) &&
                        *(int16_t *)(entry + *(int32_t *)((uint8_t *)player_data + 0x34) + 2) != -1 &&
                        network_client != 0) {
                        player_update_history_free_all(*(void **)(network_client + 0xf48));
                    }
                }
            }
        }
        goto tail;
    }

    // -- unparented biped: the normal per-tick update --
    // Ghidra: FUN_00560800() with no bound arguments -- EAX carries the Biped tag and ECX the
    // object (see that function's own register note), both of which this scope can re-derive.
    unit_update_up_vector((Biped *)tag_instances[obj->definition_tag & 0xffff].data, obj);
    if ((obj->vitality_flags & 4) != 0 || (unit->flags & 0x44) == 0) {
        unit->desired_facing_vector.k = 0.0f; // matches puVar10[0x8b] cleared unconditionally before the test
        if (vector3d_normalize_with_length(&unit->desired_facing_vector) == 0.0f) {
            unit->desired_facing_vector = *global_forward3d_pointer;
        }
    }
    switch (unit->animation_state) {
        case 0: case 2: case 3: biped->movement_state = 0; break;
        case 4: case 5: case 6: case 7: biped->movement_state = 1; break;
        default: biped->movement_state = 2; break;
    }
    if (unit->throttle.i * unit->throttle.i + unit->throttle.j * unit->throttle.j +
        unit->throttle.k * unit->throttle.k < 0.010000001f) {
        unit->throttle.i = global_origin3d_pointer->x;
        unit->throttle.j = global_origin3d_pointer->y;
        unit->throttle.k = global_origin3d_pointer->z;
    }

    biped->unknown_501 = (biped->flags & 1) ? ((biped->unknown_501 < 0x7f) ? biped->unknown_501 + 1 : biped->unknown_501) : 0;
    biped->unknown_502 = (biped->flags & 2) ? ((biped->unknown_502 < 0x7f) ? biped->unknown_502 + 1 : biped->unknown_502) : 0;

    // FIXED (0x559c29..0x559cb0): the 2-byte request is {0, control flags bit 0}; biped_update_facing gets the
    // unit (EAX) and the request; every sibling below gets the request (the draft passed a lone byte).
    request[1] = (int8_t)(*(uint8_t *)&unit->control_flags & 1);
    request[0] = 0;
    if ((obj->vitality_flags & 4) == 0) {
        biped_update_facing(object_index, request);
    }
    biped_integrate_movement_with_collision(object_index, request);

    if ((obj->vitality_flags & 4) == 0) {
        if ((biped->flags & 1) == 0) {
            if (biped->unknown_508 == -1) {
                if ((biped->flags & 2) != 0) biped_trigger_on_velocity_threshold(object_index);
            } else {
                biped_advance_frame_counter_trigger(object_index, (char *)request);
            }
        } else {
            biped_apply_idle_fidget(object_index, (uint8_t *)request); // EDI unit, stack request
        }
    } else {
        // Ghidra: FUN_0055e840() with no bound arguments; both parameters are register-carried.
        // UNSURE: the state_out pointer is taken to be the same byte its sibling calls above
        // write through.
        biped_update_idle_basis(object_index, (uint8_t *)request); // ESI unit, EDI request
    }

    if (unit_updates_suppressed != 0) goto tail;

    if (biped->unknown_505 == 0) {
        if (unit->controlling_player != k_datum_index_none && (int8_t)unit->control_flags < 0) {
            // FIXED (0x559ceb..0x559d8a): the weapon in the unit's current slot (+0x2f2), and every callee's
            // real arguments.
            datum_index weapon = unit_get_weapon_object_index(object_index, unit->current_weapon_index);
            uint32_t allowed = weapon_prevents_melee_attack(weapon);
            if (allowed == 0 && unit->zoom_level == -1) {
                unit_start_seat_overlay_animation_a(object_index, 7);
                weapon_reset_triggers(weapon);
                weapon_action_notify_for_unit(object_index, 4);
                {
                    int8_t duration = (int8_t)weapon_get_first_person_animation_time(weapon, 0xd, 0, -1);
                    biped->unknown_505 = (int8_t)(duration - (duration >> 2)); // ~75% of duration
                    biped->unknown_506 = (int8_t)(biped->unknown_505 -
                        (int8_t)weapon_get_first_person_animation_time(weapon, 0xd, 1, -1));
                }
                goto melee_countdown_tail;
            }
        }
    } else {
        if (biped->unknown_505 == biped->unknown_506) {
            unit_melee_attack_scan(object_index);
        }
        biped->unknown_505 = biped->unknown_505 - 1;
    melee_countdown_tail:
        if (unit_updates_suppressed != 0) goto tail;
    }

    unit_update_footstep_and_idle_triggers(object_index); // UNSURE: callee takes its argument in a register Ghidra could not bind; object_index is the only live candidate here
    if (unit_updates_suppressed == 0) {
        biped_check_evade_reaction(object_index);
        unit_check_fell_off_level(object_index); // UNSURE: callee takes its argument in ECX; object_index is the only live candidate here
    }

tail:
    // FIXED (0x559dd2..0x559de5): the state machine runs with ECX = the request (the draft called a
    // nonexistent twin and never ran it here).
    if (unit_update_animation_state_machine(object_index, request) == 1) {
        unit_snap_to_min_ground_height(object_index);
    }
    if ((obj->vitality_flags & 4) != 0 && (obj->flags & 0x20) != 0) {
        *(int16_t *)((uint8_t *)obj + 0xbc) = *(int16_t *)((uint8_t *)obj + 0xbc) + 1;
        return 1;
    }
    *(int16_t *)((uint8_t *)obj + 0xbc) = 0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x5590a0):

undefined4 unit_update(uint param_1)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  uint *puVar5;
  undefined *puVar6;
  char cVar7;
  undefined2 uVar8;
  short sVar9;
  uint *puVar10;
  int iVar11;
  int iVar12;
  undefined4 uVar13;
  undefined2 extraout_var;
  short sVar14;
  int iVar15;
  float10 fVar16;
  undefined1 local_ec [96];
  float local_8c;
  float local_88;
  float local_84;
  undefined1 local_7c [4];
  uint local_78;
  uint local_74;
  uint local_70;
  uint local_60;
  uint local_5c;
  uint local_58;
  uint *local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  undefined4 local_28;
  undefined4 local_24;
  float local_20;
  uint *local_1c;
  int local_18;
  undefined4 local_14;
  int local_10;
  uint local_c;
  byte local_8;
  byte local_7;

  local_10 = (param_1 & 0xffff) * 0xc;
  puVar10 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_10);
  local_c = *(uint *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_44 = puVar10;
  if (((puVar10[1] == 1) && ((char)puVar10[6] == '\x01')) && (puVar10[0x47] == 0xffffffff)) {
    FUN_00558eb0();
  }
  local_8 = 0;
  local_7 = 0;
  if (puVar10[0x47] != 0xffffffff) {
    local_18 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar10[0x47] & 0xffff) * 0xc);
    if (*(short *)(local_18 + 0xb4) != 1) {
      if (*(short *)(local_18 + 0xb4) == 0) {
        local_8 = *(byte *)(local_18 + 0x106) & 4 | 0x20;
      }
      goto LAB_00559dd2;
    }
    FUN_0055e2d0();
    if ((((puVar10[0x82] & 0x40) != 0) &&
        (puVar10 = (uint *)object_try_and_get(3), local_1c = puVar10, puVar10 != (uint *)0x0)) &&
       ((DAT_00719720 != 1 &&
        ((local_c = puVar10[0x47], local_c != 0xffffffff && ((short)puVar10[0xbc] != -1)))))) {
      if ((short)puVar10[0x2d] == 1) {
        iVar15 = *(int *)(DAT_008603b0 + 0x34);
        puVar10 = *(uint **)(iVar15 + 8 + local_10);
        local_c = puVar10[0x47];
        if ((local_c == 0xffffffff) || ((short)puVar10[0xbc] == -1)) {
LAB_005594ef:
          iVar15 = DAT_0087a480;
          if (DAT_00719720 == 1) {
LAB_00559505:
            uVar4 = puVar10[0x86];
            if (((uVar4 != 0xffffffff) && (sVar9 = (short)uVar4, -1 < sVar9)) &&
               (sVar9 < *(short *)(iVar15 + 0x20))) {
              iVar11 = (int)*(short *)(iVar15 + 0x22) * (int)sVar9;
              sVar9 = *(short *)(iVar11 + *(int *)(iVar15 + 0x34));
              if ((((sVar9 != 0) &&
                   ((sVar14 = (short)(uVar4 >> 0x10), sVar14 == 0 || (sVar9 == sVar14)))) &&
                  (*(short *)(iVar11 + *(int *)(iVar15 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0))
              {
                player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
              }
            }
          }
        }
        else {
          local_14 = *(uint **)(iVar15 + 8 + (local_c & 0xffff) * 0xc);
          iVar15 = *(int *)(iVar15 + 8 + local_10);
          iVar15 = *(short *)(iVar15 + 0x1f2) + iVar15;
          object_get_node_local_transform
                    (local_c,*(int *)(*(int *)((*local_14 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                     0x2e8) + 0x24 + (short)puVar10[0xbc] * 0x11c,local_ec,1);
          local_34 = *(float *)(iVar15 + 0x28) - local_8c;
          local_30 = *(float *)(iVar15 + 0x2c) - local_88;
          iVar11 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 +
                                                        DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                     0x14 + DAT_0087bc14) + 0xbc);
          local_1c = (uint *)(iVar11 + 0x68);
          local_28 = *(undefined4 *)(iVar11 + 0x28);
          local_2c = *(float *)(iVar15 + 0x30) - local_84;
          local_24 = *(undefined4 *)(iVar11 + 0x2c);
          local_20 = *(float *)(iVar11 + 0x30);
          if ((local_14[0xc9] == param_1) &&
             ((*(char *)((int)local_14 + 0x2a3) != '%' && (puVar10[0x47] != 0xffffffff)))) {
            unit_try_set_animation_state(puVar10[0x47],0x25);
          }
          iVar15 = DAT_006f1d6c;
          puVar10[0xcb] = local_c;
          puVar10[0xcc] = *(uint *)(iVar15 + 0xc);
          if (puVar10[0xc9] == param_1) {
            puVar10[0xc9] = 0xffffffff;
          }
          if (puVar10[0xca] == param_1) {
            puVar10[0xca] = 0xffffffff;
          }
          FUN_004f6610(param_1);
          local_40 = local_34 + (float)puVar10[0x17];
          local_3c = local_30 + (float)puVar10[0x18];
          local_38 = (local_2c + (float)puVar10[0x19]) - local_20;
          object_set_position_and_orientation(param_1,0,0);
          iVar11 = local_10;
          iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_10);
          (*(code *)PTR_matrix4x3_multiply_00696664)
                    (*(short *)(iVar15 + 0x1f2) + iVar15,local_1c,local_7c);
          puVar10[0x1d] = local_78;
          puVar10[0x1e] = local_74;
          puVar10[0x1f] = local_70;
          puVar10[0x20] = local_60;
          puVar10[0x21] = local_5c;
          iVar15 = DAT_008603b0;
          puVar10[0x22] = local_58;
          puVar3 = *(uint **)(*(int *)(iVar15 + 0x34) + 8 + iVar11);
          local_1c = *(uint **)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if ((local_1c[0xd] != 0xffffffff) && ((puVar3[4] & 1) != 0)) {
            object_for_each_light_attachment(0,1);
          }
          if (local_1c[0xd] != 0xffffffff) {
            iVar15 = *(int *)(DAT_008603b0 + 0x34);
            puVar3[4] = puVar3[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar15 + local_10 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
          *(undefined2 *)(puVar10 + 0xbc) = 0xffff;
          *(undefined1 *)((int)puVar10 + 0x2a7) = 2;
          if (local_14[0xc9] == param_1) {
            local_14[0xc9] = 0xffffffff;
          }
          if (local_14[0xca] == param_1) {
            local_14[0xca] = 0xffffffff;
          }
          FUN_0056ce30();
          FUN_0056d6a0();
          local_14._0_3_ = CONCAT12(0x14,(undefined2)local_14);
          local_14 = (uint *)(uint)(uint3)local_14;
          FUN_00565420(param_1);
          puVar2 = (undefined4 *)(*(short *)((int)puVar10 + 0x1ea) + 0x10 + (int)puVar10);
          *puVar2 = local_28;
          puVar2[1] = local_24;
          puVar2[2] = local_20;
          if ((short)puVar10[0x2d] == 0) {
            FUN_0055add0(param_1);
          }
          object_recalculate_bounding_radius_recursive(param_1);
          cVar7 = FUN_00566910();
          if ((cVar7 == '\x01') && (iVar15 = object_try_and_get(2), iVar15 != 0)) {
            *(undefined4 *)(iVar15 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
          }
          iVar15 = DAT_0087a480;
          if (DAT_00719720 == 1) {
            iVar11 = datum_get();
            if ((iVar11 != 0) && (*(short *)(iVar11 + 2) == -1)) {
              *(undefined4 *)(iVar11 + 0x180) = 0;
              *(undefined4 *)(iVar11 + 0x17c) = 0;
              *(undefined4 *)(iVar11 + 0x1e0) = 0;
              *(undefined4 *)(iVar11 + 0x1dc) = 0;
              goto LAB_005594ef;
            }
            goto LAB_00559505;
          }
        }
      }
      else {
        cVar7 = FUN_00565c60();
        if (cVar7 == '\0') {
          iVar15 = *(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          iVar11 = *(int *)(*(int *)((*(uint *)(iVar15 + 0x44) & 0xffff) * 0x20 + 0x14 +
                                    DAT_0087bc14) + 0x10);
          iVar12 = (char)puVar10[0xa8] * 100;
          if ((8 < *(int *)(iVar12 + 0x40 + iVar11)) &&
             (*(short *)(*(int *)(iVar12 + iVar11 + 0x44) + 0x10) != -1)) {
            if (*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_c & 0xffff) * 0xc) +
                         0x324) == param_1) {
              FUN_0056ab10();
            }
            uVar13 = FUN_004d6280(1);
            unit_set_custom_animation(*(undefined4 *)(iVar15 + 0x44),uVar13);
            puVar10 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_10);
            iVar15 = *(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (*(int *)(iVar15 + 0x34) != -1) {
              if ((puVar10[4] & 1) != 0) {
                object_for_each_light_attachment(0,1);
              }
              if (*(int *)(iVar15 + 0x34) != -1) {
                iVar15 = *(int *)(DAT_008603b0 + 0x34);
                puVar10[4] = puVar10[4] & 0xfffffffe;
                pbVar1 = (byte *)(iVar15 + local_10 + 2);
                *pbVar1 = *pbVar1 | 2;
              }
            }
            puVar10 = local_1c;
            *(undefined1 *)((int)local_1c + 0x2a3) = 0x1b;
            FUN_0042c370();
            if (puVar10[1] == 0) {
              FUN_0056c370(0);
            }
          }
        }
      }
    }
    if (((DAT_006893cc != '\0') && (*(float *)(local_18 + 0x88) < 0.0)) &&
       (((*(byte *)(local_18 + 0x10) & 2) != 0 && (DAT_00719720 != 1)))) {
      iVar15 = *(int *)(DAT_008603b0 + 0x34);
      puVar10 = *(uint **)(iVar15 + 8 + local_10);
      local_c = puVar10[0x47];
      if ((local_c != 0xffffffff) && ((short)puVar10[0xbc] != -1)) {
        puVar3 = *(uint **)(iVar15 + 8 + (local_c & 0xffff) * 0xc);
        iVar15 = *(int *)(iVar15 + 8 + local_10);
        iVar15 = *(short *)(iVar15 + 0x1f2) + iVar15;
        object_get_node_local_transform
                  (local_c,*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                   0x2e8) + 0x24 + (short)puVar10[0xbc] * 0x11c,local_ec,1);
        local_34 = *(float *)(iVar15 + 0x28) - local_8c;
        local_30 = *(float *)(iVar15 + 0x2c) - local_88;
        iVar11 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 +
                                                      DAT_0087bc14) + 0x34) & 0xffff) * 0x20 + 0x14
                                  + DAT_0087bc14) + 0xbc);
        local_18 = iVar11 + 0x68;
        local_28 = *(undefined4 *)(iVar11 + 0x28);
        local_2c = *(float *)(iVar15 + 0x30) - local_84;
        local_24 = *(undefined4 *)(iVar11 + 0x2c);
        local_20 = *(float *)(iVar11 + 0x30);
        if ((puVar3[0xc9] == param_1) &&
           ((*(char *)((int)puVar3 + 0x2a3) != '%' && (puVar10[0x47] != 0xffffffff)))) {
          unit_try_set_animation_state(puVar10[0x47],0x25);
        }
        iVar15 = DAT_006f1d6c;
        puVar10[0xcb] = local_c;
        puVar10[0xcc] = *(uint *)(iVar15 + 0xc);
        if (puVar10[0xc9] == param_1) {
          puVar10[0xc9] = 0xffffffff;
        }
        if (puVar10[0xca] == param_1) {
          puVar10[0xca] = 0xffffffff;
        }
        FUN_004f6610(param_1);
        local_40 = local_34 + (float)puVar10[0x17];
        local_3c = local_30 + (float)puVar10[0x18];
        local_38 = (local_2c + (float)puVar10[0x19]) - local_20;
        object_set_position_and_orientation(param_1,0,0);
        iVar11 = local_10;
        iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_10);
        (*(code *)PTR_matrix4x3_multiply_00696664)
                  (*(short *)(iVar15 + 0x1f2) + iVar15,local_18,local_7c);
        puVar10[0x1d] = local_78;
        puVar10[0x1e] = local_74;
        puVar10[0x1f] = local_70;
        puVar10[0x20] = local_60;
        puVar10[0x21] = local_5c;
        iVar15 = DAT_008603b0;
        puVar10[0x22] = local_58;
        puVar5 = *(uint **)(*(int *)(iVar15 + 0x34) + 8 + iVar11);
        local_18 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if ((*(int *)(local_18 + 0x34) != -1) && ((puVar5[4] & 1) != 0)) {
          object_for_each_light_attachment(0,1);
        }
        if (*(int *)(local_18 + 0x34) != -1) {
          iVar15 = *(int *)(DAT_008603b0 + 0x34);
          puVar5[4] = puVar5[4] & 0xfffffffe;
          pbVar1 = (byte *)(iVar15 + local_10 + 2);
          *pbVar1 = *pbVar1 | 2;
        }
        *(undefined2 *)(puVar10 + 0xbc) = 0xffff;
        *(undefined1 *)((int)puVar10 + 0x2a7) = 2;
        if (puVar3[0xc9] == param_1) {
          puVar3[0xc9] = 0xffffffff;
        }
        if (puVar3[0xca] == param_1) {
          puVar3[0xca] = 0xffffffff;
        }
        FUN_0056ce30();
        FUN_0056d6a0();
        local_14._0_3_ = CONCAT12(0x14,(undefined2)local_14);
        local_14 = (uint *)(uint)(uint3)local_14;
        FUN_00565420(param_1);
        puVar2 = (undefined4 *)(*(short *)((int)puVar10 + 0x1ea) + 0x10 + (int)puVar10);
        *puVar2 = local_28;
        puVar2[1] = local_24;
        puVar2[2] = local_20;
        if ((short)puVar10[0x2d] == 0) {
          FUN_0055add0(param_1);
        }
        object_recalculate_bounding_radius_recursive(param_1);
        cVar7 = FUN_00566910();
        if ((cVar7 == '\x01') && (iVar15 = object_try_and_get(2), iVar15 != 0)) {
          *(undefined4 *)(iVar15 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
        }
        if (((DAT_00719720 == 1) && (iVar15 = datum_get(), iVar15 != 0)) &&
           (*(short *)(iVar15 + 2) == -1)) {
          *(undefined4 *)(iVar15 + 0x180) = 0;
          *(undefined4 *)(iVar15 + 0x17c) = 0;
          *(undefined4 *)(iVar15 + 0x1e0) = 0;
          *(undefined4 *)(iVar15 + 0x1dc) = 0;
        }
      }
      if (puVar10[1] == 0) {
        FUN_0056c370(1);
      }
      if (((DAT_00719720 == 1) && (uVar4 = puVar10[0x86], uVar4 != 0xffffffff)) &&
         ((sVar9 = (short)uVar4, -1 < sVar9 && (sVar9 < *(short *)(DAT_0087a480 + 0x20))))) {
        iVar15 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar9;
        sVar9 = *(short *)(iVar15 + *(int *)(DAT_0087a480 + 0x34));
        if (((sVar9 != 0) && ((sVar14 = (short)(uVar4 >> 0x10), sVar14 == 0 || (sVar9 == sVar14))))
           && ((*(short *)(iVar15 + *(int *)(DAT_0087a480 + 0x34) + 2) != -1 && (DAT_0071c2d8 != 0))
              )) {
          player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
        }
      }
    }
    goto LAB_00559dd2;
  }
  FUN_00560800();
  if (((*(byte *)((int)puVar10 + 0x106) & 4) != 0) || ((*(byte *)(local_c + 0x2f4) & 0x44) == 0)) {
    puVar10[0x8b] = 0;
    fVar16 = (float10)vector3d_normalize_with_length();
    puVar6 = PTR_DAT_00696718;
    if ((float10)0.0 == fVar16) {
      puVar10[0x89] = *(uint *)PTR_DAT_00696718;
      puVar10[0x8a] = *(uint *)(puVar6 + 4);
      puVar10[0x8b] = *(uint *)(puVar6 + 8);
    }
  }
  switch(*(undefined1 *)((int)puVar10 + 0x2a3)) {
  case 0:
  case 2:
  case 3:
    *(undefined1 *)((int)puVar10 + 0x4d2) = 0;
    break;
  default:
    *(undefined1 *)((int)puVar10 + 0x4d2) = 2;
    break;
  case 4:
  case 5:
  case 6:
  case 7:
    *(undefined1 *)((int)puVar10 + 0x4d2) = 1;
  }
  puVar6 = PTR_DAT_00696714;
  if ((float)puVar10[0xa0] * (float)puVar10[0xa0] +
      (float)puVar10[0x9f] * (float)puVar10[0x9f] + (float)puVar10[0x9e] * (float)puVar10[0x9e] <
      0.010000001) {
    puVar10[0x9e] = *(uint *)PTR_DAT_00696714;
    puVar10[0x9f] = *(uint *)(puVar6 + 4);
    puVar10[0xa0] = *(uint *)(puVar6 + 8);
  }
  if ((puVar10[0x133] & 1) == 0) {
    *(undefined1 *)((int)puVar10 + 0x501) = 0;
  }
  else if (*(char *)((int)puVar10 + 0x501) < '\x7f') {
    *(char *)((int)puVar10 + 0x501) = *(char *)((int)puVar10 + 0x501) + '\x01';
  }
  if ((puVar10[0x133] & 2) == 0) {
    *(undefined1 *)((int)puVar10 + 0x502) = 0;
  }
  else if (*(char *)((int)puVar10 + 0x502) < '\x7f') {
    *(char *)((int)puVar10 + 0x502) = *(char *)((int)puVar10 + 0x502) + '\x01';
  }
  local_7 = (byte)puVar10[0x82] & 1;
  local_8 = 0;
  if ((*(byte *)((int)puVar10 + 0x106) & 4) == 0) {
    unit_update_facing(&local_8);
  }
  FUN_0055cfd0(param_1,&local_8);
  if ((*(byte *)((int)puVar10 + 0x106) & 4) == 0) {
    if ((puVar10[0x133] & 1) == 0) {
      if ((short)puVar10[0x142] == -1) {
        if ((puVar10[0x133] & 2) != 0) {
          FUN_0055ec20();
        }
      }
      else {
        FUN_0055eb90(&local_8);
      }
    }
    else {
      FUN_0055e940(&local_8);
    }
  }
  else {
    FUN_0055e840();
  }
  if (DAT_0071c419 != '\0') goto LAB_00559dd2;
  if (*(char *)((int)puVar10 + 0x505) == '\0') {
    if ((puVar10[0x86] != 0xffffffff) && ((char)puVar10[0x82] < '\0')) {
      uVar13 = unit_get_weapon_object_index();
      cVar7 = FUN_004c2ee0();
      if ((cVar7 == '\0') && ((char)puVar10[200] == -1)) {
        FUN_00565e00(param_1,7);
        FUN_004c4b50(uVar13);
        FUN_00492730(4);
        uVar8 = FUN_004c2f80(0,0xffffffff);
        cVar7 = (char)uVar8;
        local_18 = CONCAT22(extraout_var,(short)(cVar7 >> 2));
        *(char *)((int)puVar10 + 0x505) = cVar7;
        local_14 = (uint *)CONCAT22(uVar8,(undefined2)local_14);
        *(char *)((int)puVar10 + 0x505) = cVar7 - (cVar7 >> 2);
        cVar7 = FUN_004c2f80(1,0xffffffff);
        *(char *)((int)puVar10 + 0x506) = (local_14._2_1_ - (char)local_18) - cVar7;
        goto LAB_00559da9;
      }
    }
  }
  else {
    if (*(char *)((int)puVar10 + 0x505) == *(char *)((int)puVar10 + 0x506)) {
      unit_melee_attack_scan(param_1);
    }
    *(char *)((int)puVar10 + 0x505) = *(char *)((int)puVar10 + 0x505) + -1;
LAB_00559da9:
    if (DAT_0071c419 != '\0') goto LAB_00559dd2;
  }
  FUN_00560410();
  if (DAT_0071c419 == '\0') {
    FUN_0055e190(param_1);
    FUN_0055e4a0();
  }
LAB_00559dd2:
  sVar9 = FUN_00565420(param_1);
  if (sVar9 == 1) {
    FUN_0055ecf0(param_1);
  }
  if (((*(byte *)((int)local_44 + 0x106) & 4) != 0) && ((local_44[4] & 0x20) != 0)) {
    *(short *)(local_44 + 0x2f) = (short)local_44[0x2f] + 1;
    return 1;
  }
  *(undefined2 *)(local_44 + 0x2f) = 0;
  return 1;
}
#endif
