// actor_process_vehicle_seat_exit  (Ghidra: still FUN_0040b080; named for this rewrite)
// address 0x40b080, size 1767 bytes, 0 callers in this build (dead or reached only through a
//   function-pointer table this analysis did not resolve -- see ai_types_notes.md's identical
//   note about the also-0-caller 0x42c940 in this module)
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: phase-4 summary ("Detaches or re-seats an actor's controlled unit relative to a
//   vehicle seat, updating seat, transform, and multiplayer bookkeeping as needed"); types/ai.h
//   actor.active_unit_index (0x158), actor.first_prop (0x50), actor.unit_index (0x18),
//   actor.unknown_2ed/0x160 order_committed/0x1b0/0x280 danger_type/0x28a/0x38c/0x390/0x394;
//   types/ai.h prop.kind (0x24), prop.is_parented (0x12e), prop.is_unit (0x60),
//   prop.relationship_object_index (0x110); types/objects.h object.parent_object (0x11c),
//   object.type (0x0b4), object.definition_tag (0x000), object.network_role (0x004),
//   object.forward/up (0x074/0x080), object.nodes (0x1f0, an object_block_reference);
//   types/units.h unit_data.vehicle_seat_index (0x2f0), .controlling_player (0x218),
//   .driver_unit_index/.gunner_unit_index (0x324/0x328), .last_parent_object_index/
//   .last_seat_change_tick (0x32c/0x330), .animation_state/.base_animation_state/
//   .animation_definition_index/.animation_state_flags (0x2a3/0x2a7/0x2a0/0x298);
//   types/tags.h Object (model TagID 0x34, animation_graph TagID 0x44), Unit (seats
//   TagReflexive 0x2e4/0x2e8), UnitSeat (marker_name TagString 0x24), ModelAnimations (units
//   TagReflexive 0xc), ModelAnimationsAnimationGraphUnitSeat (animations TagReflexive 0x40);
//   types/math.h real_matrix4x3 (position 0x28, matching the GBXModel node stride of 0x34
//   this function indexes with "+0x68" == node + 2); sibling function
//   ai_reference_units_exit_vehicles.c (0x433ea0), which performs the same detach/reseat
//   sequence for a whole ai-reference list rather than a single actor and was used as a guide
//   for the tag-lookup chain (Unit tag, model tag, animation-graph tag) shared by both.
// register convention: actor index in EAX (in_EAX, unresolved register read; Ghidra shows the
//   function as taking no recognized parameters).
//   // blam-cc: EAX -> actor_index
//
// Disassembly notes (objdump -d -M intel, since this is one of the large deferred functions and
// several of its calls carry implicit register arguments Ghidra's decompiled C hides entirely):
//   - object_try_and_get is called as object_try_and_get(self->unit_index, 3): the decompiled
//     text shows only the literal 3, but 0x40b16b/0x40b170 load self->unit_index into ESI and
//     then ECX right before the call, matching the ECX/stack convention that
//     src/objects/object_try_and_get.c already establishes for this address.
//   - object_get_node_local_transform (0x4f6080) is called here with FOUR values pushed to the
//     stack (flag, marker*, marker_name, object_index, in that push order -- i.e. object_index
//     is the first/nearest-the-call parameter). Disassembling the callee's own prologue
//     (0x4f6080: `mov eax,[esp+4]`) confirms it reads its first argument off the stack, not out
//     of EAX -- so it is a plain stack (cdecl) function here. This differs from
//     src/objects/object_get_node_local_transform.c's documented "EAX/ECX/EDX + one stack
//     argument" convention; that file's convention could not be reproduced against this
//     call site or against the callee's own prologue and is flagged separately for review
//     rather than edited here (out of this rewrite's scope).
//   - object_set_position_and_orientation matches src/objects/object_set_position_and_orientation.c
//     exactly: three stack arguments (object_index, forward, up) plus the position pointer in
//     EDI. At 0x40b363 this function does `lea edi,[esp+0x4c]` immediately before the call,
//     pointing EDI at a small on-stack real_point3d it just finished computing -- that buffer is
//     `new_position` below.
//   - unit_set_custom_animation matches src/units/unit_set_custom_animation.c's confirmed
//     three-argument form (object_index in EAX, graph and animation_index on the stack), not
//     the two-argument extern the sibling file above declares for it.
//   - object_for_each_light_attachment matches src/objects/object_for_each_light_attachment.c's
//     confirmed three-argument form (object_index in EAX, two stack bools); at both call sites
//     here EAX is loaded with the rider's own object index immediately before the call.
// UNSURE: the composition of `new_position` (the buffer written through the EDI position
//   pointer). The three components are unambiguously `<some point> + rider->position`
//   (0x40b357/0x40b36b/0x40b376 add ebx+0x5c/0x60/0x64, and ebx is the rider object pointer
//   throughout this branch). Ghidra's own decompiled C reads that "some point" as
//   model_node_array[0].position (the GBXModel tag's default node-0 translation, reached
//   through the Unit tag's model TagID and then GBXModel+0xbc's node-array pointer -- see
//   model_node0_position below), which is what this rewrite uses; a component-by-component
//   re-derivation from raw ESP offsets did not cleanly agree with that reading past two levels
//   of intervening stack adjustment (the "add esp,0x10" at 0x40b2e6 and a further push at
//   0x40b34d) and was not trusted over Ghidra's own stack-slot dataflow for this specific
//   FPU-heavy stretch, which decompilers are generally more reliable at than a manual
//   byte-by-byte replay across a branch. Flagged rather than guessed further.
// UNSURE: unit_notify_weapon_removed, unit_dispatch_scripted_event_9 and actor_notify_weapon_pickup_once (FUN_0042c370) are each
//   called here with an object index that this disassembly shows arriving in ECX immediately
//   before the call (the rider's own object pointer, or the vehicle's, depending on the call
//   site) even though none of them is one of this rewrite's target addresses and none has been
//   independently re-verified from its own prologue; declared with that ECX parameter and
//   marked UNSURE rather than reusing the sibling file's zero-argument externs for them.
// UNSURE: the exact GBXModel node-array layout (a TagReflexive at GBXModel+0xb8, count/pointer
//   only) is used as a raw offset plus a real_matrix4x3 cast, since GBXModel is not fully typed
//   in types/tags.h; no new struct was added, this is just pointer arithmetic against an
//   existing type.

// REVIEW FINDING (phase-4 review pass, not fixed here -- needs a full esp-tracked trace):
// Ghidra drops three x87 subtractions this function performs, and this file inherits the loss.
// Confirmed directly in `objdump -d -M intel --start-address=0x40b270 --stop-address=0x40b390`:
//   * 0x40b275/0x40b293/0x40b2b7: `fld [edi+0x28|0x2c|0x30]` followed by
//     `fsub [esp+0xf0|0xf4|0xf8]`, where EDI is the RIDER's own node array
//     (rider + [rider+0x1f2]) and [esp+0xf0..] is the marker output that
//     object_get_node_local_transform just filled. The three values this produces are
//     rider_node0.position MINUS marker.position, a delta -- Ghidra renders the operands as a
//     plain copy of the MODEL tag's node[0] position with no subtraction at all, and this file
//     follows Ghidra.
//   * 0x40b379: the third component of the point handed to object_set_position_and_orientation
//     gets one further `fsub [esp+0x3c]` that neither Ghidra nor this file reproduces.
// The `fadd [ebx+0x5c|0x60|0x64]` at 0x40b353..0x40b376 confirms that the out-point IS
// model_node0.position + rider->position as written below, so only the two subtractions above
// are known-missing. Not reconstructed here because the esp bookkeeping across the
// `add esp,0x10` at 0x40b2e6, the stdcall at 0x40b34e and the three pushes at 0x40b35a..0x40b35e
// was not resolved well enough to say which stack slot feeds which component.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "game.h"
#include "ai.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
extern data_array *object_data;      // 0x008603b0
extern data_array *player_data;      // 0x0087a480
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t network_game_mode;    // 0x00719720
extern void *network_client;         // 0x0071c2d8, UNSURE name (matches
                                     //   ai_reference_units_exit_vehicles.c's
                                     //   object_control_local_player_b)
extern void (*matrix4x3_multiply_ptr)(void *a, void *b, void *out); // 0x00696664

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
    object_marker *marker, uint32_t flag); // 0x4f6080, UNSURE: all-stack convention, see header
extern void object_snap_to_parent_marker_and_detach(datum_index object_index);          // 0x4f6610, not yet rewritten
extern void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward,
    real_vector3d *up, real_point3d *position); // 0x4f51c0, EDI -> position
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20, EAX -> object_index
extern void object_recalculate_bounding_radius_recursive(datum_index object_index); // 0x4f82b0
extern void unit_try_set_animation_state(uint32_t object_index, int16_t new_state); // 0x565f90
extern uint8_t unit_state_is_scripted_animation(uint16_t *animation_state_flags); // 0x565c60,
                                     //   UNSURE: ECX -> &rider->animation_state_flags (0x298),
                                     //   re-derived here, differs from the sibling's
                                     //   zero-argument extern
extern void unit_set_custom_animation(uint32_t object_index, datum_index graph,
    int16_t animation_index); // 0x56ebd0, matches src/units/unit_set_custom_animation.c
extern int16_t animation_choose_random_permutation(int32_t flag); // 0x4d6280, matches every other call site in the tree
extern void *datum_get(void);              // 0x4d0680, no visible argument
extern void player_update_history_free_all(uint32_t player_history); // 0x4e6f20
extern void actor_notify_weapon_pickup_once(datum_index unit_index); // 0x42c370, UNSURE ECX arg,
                                     //   not yet rewritten
extern void unit_recompute_seat_occupants(void);                       // 0x56ce30, no visible argument
extern void unit_pick_and_ready_next_weapon(void);                       // 0x56d6a0, no visible argument
extern void unit_update_animation_state_machine(datum_index object_index, uint8_t *request); // 0x565420, UNSURE:
                                     //   ECX -> a 2-byte {0x14,0x00} local record built at the
                                     //   call site; not yet rewritten
extern void unit_reset_orientation_and_find_position(datum_index object_index);   // 0x55add0, not yet rewritten
extern uint8_t unit_all_seats_unoccupied(void);                    // 0x566910, no visible argument
extern void unit_notify_weapon_removed(object *vehicle);            // 0x56ab10, UNSURE ECX arg, not yet rewritten
extern void unit_dispatch_scripted_event_9(datum_index unit_index, int32_t a); // 0x56c370, UNSURE ECX arg,
                                     //   not yet rewritten

// blam-cc: EAX -> actor_index
// Re-evaluates whether the actor's controlled unit should still be treated as seated. If the
// actor has no active controlled unit at all, or already has a recognized prop tracking that
// unit and no pending order/danger reason to force a re-seat, this only clears the actor's
// "just despawned" flag and returns. Otherwise it fetches the controlled unit and, if it is
// still seated in a vehicle: for a vehicle-type rider (e.g. a turret occupying another
// vehicle's seat) it detaches the rider from its parent, snaps it to a computed world position
// and orientation, restores its light-attachment/animation bookkeeping, and updates the
// multiplayer camera-history record for whichever side (client or host) is running; for a
// biped rider it instead starts the seat's built-in exit animation when one is defined.
// Returns 1 only when the biped exit-animation path actually started an exit.
uint8_t actor_process_vehicle_seat_exit(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t found_matching_prop;
    uint8_t reseat_pending;
    uint8_t result = 0;

    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        self->unknown_2ed = 0;
        return 0;
    }

    // Look for a prop already tracking the active unit as a recognized, parented, unit-type
    // prop (kind 2 or 3). The prop itself, if found, is not otherwise used here -- only its
    // presence matters.
    found_matching_prop = 0;
    {
        datum_index prop_index = self->first_prop;
        while (prop_index != (datum_index)k_datum_index_none) {
            prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
            datum_index next = p->next_in_actor;
            if (1 < p->kind && p->kind < 4 && p->is_parented != 0 && p->is_unit != 0 &&
                p->relationship_object_index == (int32_t)self->active_unit_index) {
                found_matching_prop = 1;
                break;
            }
            prop_index = next;
        }
    }
    reseat_pending = found_matching_prop;

    if (self->unknown_2ed != 0) {
        found_matching_prop = 1;
    }

    if (self->order_committed == 0 ||
        (self->unknown_1b0 == -1 &&
         (self->danger_type != 2 || self->unknown_28a == 0))) {
        result = 0;
        if (!found_matching_prop) {
            self->unknown_2ed = 0;
            return result;
        }
    } else {
        reseat_pending = 1;
    }

    {
        datum_index rider_index = self->unit_index;
        object *rider;

        self->unknown_38c = reseat_pending;
        rider = object_try_and_get(rider_index, 3); // blam-cc: ECX -> rider_index, stack -> 3

        if (rider != 0 && network_game_mode != 1 &&
            rider->parent_object != (datum_index)k_datum_index_none &&
            ((unit_data *)((uint8_t *)rider + k_unit_data_offset))->vehicle_seat_index != -1) {

            unit_data *rider_unit = (unit_data *)((uint8_t *)rider + k_unit_data_offset);

            if (rider->type == _object_type_vehicle) {
                // Reload the rider (same object, redundant re-fetch matching the original).
                rider = ((object_header *)object_data->data)[rider_index & 0xffff].data;
                rider_unit = (unit_data *)((uint8_t *)rider + k_unit_data_offset);

                datum_index vehicle_index = rider->parent_object;

                if (vehicle_index != (datum_index)k_datum_index_none &&
                    rider_unit->vehicle_seat_index != -1) {
                    object *vehicle = ((object_header *)object_data->data)[vehicle_index & 0xffff].data;
                    unit_data *vehicle_unit = (unit_data *)((uint8_t *)vehicle + k_unit_data_offset);
                    Object *vehicle_unit_tag = (Object *)tag_instances[vehicle->definition_tag & 0xffff].data;
                    UnitSeat *seat = &((UnitSeat *)((Unit *)vehicle_unit_tag)->seats.pointer)[rider_unit->vehicle_seat_index];
                    object_marker marker_out;

                    object_get_node_local_transform(vehicle_index, seat->marker_name.string, &marker_out, 1);

                    Object *rider_unit_tag = (Object *)tag_instances[rider->definition_tag & 0xffff].data;
                    datum_index model_tag_id = *(datum_index *)&rider_unit_tag->model.tag_id;
                    void *model_tag = tag_instances[model_tag_id & 0xffff].data;
                    // GBXModel.nodes is a TagReflexive at +0xb8 (count/pointer/definition); the
                    // pointer field is at +0xbc. See file header UNSURE note.
                    real_matrix4x3 *model_nodes = *(real_matrix4x3 **)((uint8_t *)model_tag + 0xbc);
                    real_point3d model_node0_position = model_nodes[0].position;

                    if (vehicle_unit->driver_unit_index == rider_index &&
                        vehicle_unit->animation_state != 0x25 &&
                        vehicle_index != (datum_index)k_datum_index_none) {
                        unit_try_set_animation_state(vehicle_index, 0x25);
                    }

                    rider_unit->last_parent_object_index = vehicle_index;
                    rider_unit->last_seat_change_tick = game_time->game_time;
                    if (rider_unit->driver_unit_index == rider_index) {
                        rider_unit->driver_unit_index = (datum_index)k_datum_index_none;
                    }
                    if (rider_unit->gunner_unit_index == rider_index) {
                        rider_unit->gunner_unit_index = (datum_index)k_datum_index_none;
                    }

                    object_snap_to_parent_marker_and_detach(rider_index);

                    {
                        // UNSURE: see file header note on new_position's exact composition.
                        real_point3d new_position;
                        new_position.x = model_node0_position.x + rider->position.x;
                        new_position.y = model_node0_position.y + rider->position.y;
                        new_position.z = model_node0_position.z + rider->position.z;
                        object_set_position_and_orientation(rider_index, 0, 0, &new_position); // blam-cc: EDI -> &new_position
                    }

                    {
                        real_matrix4x3 *rider_current_nodes =
                            (real_matrix4x3 *)((uint8_t *)rider + rider->nodes.offset);
                        real_matrix4x3 result_transform;
                        (*matrix4x3_multiply_ptr)(rider_current_nodes, model_nodes + 2, &result_transform);
                        rider = ((object_header *)object_data->data)[rider_index & 0xffff].data; // reloaded, matching the original
                        rider_unit = (unit_data *)((uint8_t *)rider + k_unit_data_offset);
                        rider->forward = result_transform.forward;
                        rider->up = result_transform.up;
                    }

                    rider_unit_tag = (Object *)tag_instances[rider->definition_tag & 0xffff].data;
                    model_tag_id = *(datum_index *)&rider_unit_tag->model.tag_id;
                    if (model_tag_id != (datum_index)k_datum_index_none) {
                        if ((rider->flags & 1) != 0) { // UNSURE: object flags bit 0x1, see
                                                       //   object_for_each_light_attachment's own
                                                       //   note on its gating bit
                            object_for_each_light_attachment(rider_index, 0, 1);
                        }
                        if (model_tag_id != (datum_index)k_datum_index_none) {
                            rider->flags &= ~1u;
                            ((object_header *)object_data->data)[rider_index & 0xffff].flags |= 2;
                        }
                    }

                    rider_unit->vehicle_seat_index = -1;
                    rider_unit->base_animation_state = 2;
                    if (vehicle_unit->driver_unit_index == rider_index) {
                        vehicle_unit->driver_unit_index = (datum_index)k_datum_index_none;
                    }
                    if (vehicle_unit->gunner_unit_index == rider_index) {
                        vehicle_unit->gunner_unit_index = (datum_index)k_datum_index_none;
                    }

                    unit_recompute_seat_occupants();
                    unit_pick_and_ready_next_weapon();
                    {
                        uint8_t local_request[2];
                        local_request[0] = 0x14;
                        local_request[1] = 0;
                        unit_update_animation_state_machine(rider_index, local_request);
                    }

                    // Restore the seat marker's raw position into the rider's own node array
                    // (object.node_function_values, an object_block_reference at +0x1e8).
                    {
                        real_point3d *stored_marker_position =
                            (real_point3d *)((uint8_t *)rider + 0x10 + rider->node_function_values.offset);
                        *stored_marker_position = marker_out.node_transform.position;
                    }

                    if (rider->type == _object_type_biped) {
                        unit_reset_orientation_and_find_position(rider_index);
                    }
                    object_recalculate_bounding_radius_recursive(rider_index);

                    if (unit_all_seats_unoccupied() == 1) {
                        object *local_player_object = object_try_and_get((datum_index)k_datum_index_none, 2); // UNSURE object_index, see file header
                        if (local_player_object != 0) {
                            *(int32_t *)((uint8_t *)local_player_object + 0x5ac) = game_time->game_time;
                        }
                    }
                }

                if (network_game_mode == 1) {
                    player *history_player = (player *)datum_get();
                    if (history_player != 0 && history_player->local_player_index == -1) {
                        // UNSURE: which player record this clears is not named further; the
                        // four fields are raw offsets in the original (0x180/0x17c/0x1e0/0x1dc).
                        *(uint32_t *)((uint8_t *)history_player + 0x180) = 0;
                        *(uint32_t *)((uint8_t *)history_player + 0x17c) = 0;
                        *(uint32_t *)((uint8_t *)history_player + 0x1e0) = 0;
                        *(uint32_t *)((uint8_t *)history_player + 0x1dc) = 0;
                    }
                }
                // Always run when network_game_mode == 1, whether or not the zero-fields step
                // above ran (the original re-tests the same condition and falls into this same
                // block either way -- see file header).
                if (network_game_mode == 1) {
                    datum_index controlling_player = rider_unit->controlling_player;
                    if (controlling_player != (datum_index)k_datum_index_none &&
                        (int16_t)controlling_player >= 0 &&
                        (int16_t)controlling_player < player_data->maximum_count) {
                        player *p = &((player *)player_data->data)[(int16_t)controlling_player];
                        int16_t salt = (int16_t)(controlling_player >> 16);
                        if (p->identifier != 0 && (salt == 0 || p->identifier == salt) &&
                            p->local_player_index != -1 && network_client != 0) {
                            player_update_history_free_all(*(uint32_t *)((uint8_t *)network_client + 0xf48));
                        }
                    }
                }

            } else {
                // Biped rider: trigger the seat's built-in exit animation, if the animation
                // graph defines one for this seat, instead of a full detach.
                if (unit_state_is_scripted_animation(&rider_unit->animation_state_flags) == 0) {
                    Object *rider_unit_tag = (Object *)tag_instances[rider->definition_tag & 0xffff].data;
                    datum_index animation_graph_tag_id = *(datum_index *)&rider_unit_tag->animation_graph.tag_id;
                    void *animation_graph_tag = tag_instances[animation_graph_tag_id & 0xffff].data;
                    // ModelAnimations.units is a TagReflexive at +0xc; its pointer field is +0x10
                    // (types/units.h's own comment on animation_definition_index cites this).
                    uint8_t *unit_seat_anims = *(uint8_t **)((uint8_t *)animation_graph_tag + 0x10);
                    int32_t seat_record_offset = (int32_t)rider_unit->animation_definition_index * 0x64;

                    // ModelAnimationsAnimationGraphUnitSeat.animations is a TagReflexive at +0x40.
                    if (8 < *(int32_t *)(unit_seat_anims + seat_record_offset + 0x40) &&
                        *(int16_t *)(*(uint32_t *)(unit_seat_anims + seat_record_offset + 0x44) + 0x10) != -1) {

                        object *vehicle = ((object_header *)object_data->data)[rider->parent_object & 0xffff].data;
                        unit_data *vehicle_unit = (unit_data *)((uint8_t *)vehicle + k_unit_data_offset);
                        if (vehicle_unit->driver_unit_index == rider_index) {
                            unit_notify_weapon_removed(vehicle); // blam-cc: ECX -> vehicle, UNSURE
                        }

                        int16_t animation_value = animation_choose_random_permutation(1);
                        unit_set_custom_animation(rider_index, animation_graph_tag_id, animation_value);

                        rider = ((object_header *)object_data->data)[rider_index & 0xffff].data; // reloaded
                        rider_unit = (unit_data *)((uint8_t *)rider + k_unit_data_offset);
                        datum_index model_tag_id;
                        rider_unit_tag = (Object *)tag_instances[rider->definition_tag & 0xffff].data;
                        model_tag_id = *(datum_index *)&rider_unit_tag->model.tag_id;
                        if (model_tag_id != (datum_index)k_datum_index_none) {
                            if ((rider->flags & 1) != 0) {
                                object_for_each_light_attachment(rider_index, 0, 1);
                            }
                            if (model_tag_id != (datum_index)k_datum_index_none) {
                                rider->flags &= ~1u;
                                ((object_header *)object_data->data)[rider_index & 0xffff].flags |= 2;
                            }
                        }

                        rider_unit->animation_state = 0x1b; // _unit_animation_state_seat_exit
                        actor_notify_weapon_pickup_once(rider_index); // blam-cc: ECX -> rider_index, UNSURE
                        if (rider->network_role == 0) {
                            unit_dispatch_scripted_event_9(rider_index, 0); // blam-cc: ECX -> rider_index, UNSURE
                        }

                        self->unknown_390 = self->active_unit_index;
                        self->unknown_394 = (datum_index)(game_time->game_time + 0xb4); // UNSURE:
                            // field is declared datum_index in types/ai.h, but this write is a
                            // plain tick deadline, not a datum handle
                        result = 1;
                    }
                }
            }
        }

        self->unknown_38c = 0;
    }

    self->unknown_2ed = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x40b080) -- full listing via `python tools/pack.py 0x40b080`:

undefined1 FUN_0040b080(void)

{
  byte *pbVar1;
  int iVar2;
  undefined4 *puVar3;
  uint uVar4;
  uint *puVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  uint *puVar8;
  bool bVar9;
  char cVar10;
  undefined1 uVar11;
  uint in_EAX;
  int iVar12;
  uint *puVar13;
  int iVar14;
  undefined4 uVar15;
  short sVar16;
  uint uVar17;
  int iVar18;
  short sVar19;
  undefined1 local_ea;
  undefined1 local_e8;
  undefined1 local_b0 [4];
  uint uStack_ac;
  uint uStack_a8;
  uint uStack_a4;
  uint uStack_94;
  uint uStack_90;
  uint uStack_8c;
  undefined1 local_78 [116];

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_ea = 0;
  uVar11 = 0;
  if (*(int *)(iVar2 + 0x158) == -1) goto LAB_0040b759;
  uVar17 = *(uint *)(iVar2 + 0x50);
  bVar9 = false;
  local_e8 = 0;
  do {
    if (uVar17 == 0xffffffff) goto LAB_0040b11e;
    iVar12 = (uVar17 & 0xffff) * 0x138;
    uVar17 = *(uint *)(iVar12 + 8 + *(int *)(DAT_008802c0 + 0x34));
    iVar12 = iVar12 + *(int *)(DAT_008802c0 + 0x34);
  } while ((((*(short *)(iVar12 + 0x24) < 2) || (3 < *(short *)(iVar12 + 0x24))) ||
           (*(char *)(iVar12 + 0x12e) == '\0')) ||
          ((*(char *)(iVar12 + 0x60) == '\0' ||
           (*(int *)(iVar12 + 0x110) != *(int *)(iVar2 + 0x158)))));
  bVar9 = true;
  local_e8 = 1;
LAB_0040b11e:
  if (*(char *)(iVar2 + 0x2ed) != '\0') {
    bVar9 = true;
  }
  if ((*(char *)(iVar2 + 0x160) == '\0') ||
     ((*(int *)(iVar2 + 0x1b0) == -1 &&
      ((*(short *)(iVar2 + 0x280) != 2 || (*(char *)(iVar2 + 0x28a) == '\0')))))) {
    uVar11 = local_ea;
    if (!bVar9) goto LAB_0040b759;
  }
  else {
    local_e8 = 1;
  }
  uVar17 = *(uint *)(iVar2 + 0x18);
  *(undefined1 *)(iVar2 + 0x38c) = local_e8;
  puVar13 = (uint *)object_try_and_get(3);
  if ((((puVar13 != (uint *)0x0) && (DAT_00719720 != 1)) &&
      (uVar4 = puVar13[0x47], uVar4 != 0xffffffff)) && ((short)puVar13[0xbc] != -1)) {
    if ((short)puVar13[0x2d] == 1) {
      iVar18 = (uVar17 & 0xffff) * 0xc;
      puVar13 = *(uint **)(iVar18 + 8 + *(int *)(DAT_008603b0 + 0x34));
      uVar4 = puVar13[0x47];
      iVar12 = DAT_0087a480;
      if ((uVar4 == 0xffffffff) || ((short)puVar13[0xbc] == -1)) {
LAB_0040b559:
        if (DAT_00719720 == 1) {
LAB_0040b567:
          uVar17 = puVar13[0x86];
          if (((uVar17 != 0xffffffff) && (sVar19 = (short)uVar17, -1 < sVar19)) &&
             (sVar19 < *(short *)(iVar12 + 0x20))) {
            iVar18 = (int)*(short *)(iVar12 + 0x22) * (int)sVar19;
            sVar19 = *(short *)(iVar18 + *(int *)(iVar12 + 0x34));
            if ((((sVar19 != 0) &&
                 ((sVar16 = (short)(uVar17 >> 0x10), sVar16 == 0 || (sVar19 == sVar16)))) &&
                (*(short *)(iVar18 + *(int *)(iVar12 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)) {
              player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
            }
          }
        }
      }
      else {
        puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
        object_get_node_local_transform
                  (uVar4,*(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)
                         + 0x24 + (short)puVar13[0xbc] * 0x11c,local_78,1);
        iVar12 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar13 & 0xffff) * 0x20 + 0x14 +
                                                      DAT_0087bc14) + 0x34) & 0xffff) * 0x20 + 0x14
                                  + DAT_0087bc14) + 0xbc);
        uVar15 = *(undefined4 *)(iVar12 + 0x28);
        uVar6 = *(undefined4 *)(iVar12 + 0x2c);
        uVar7 = *(undefined4 *)(iVar12 + 0x30);
        if ((puVar5[0xc9] == uVar17) &&
           ((*(char *)((int)puVar5 + 0x2a3) != '%' && (puVar13[0x47] != 0xffffffff)))) {
          unit_try_set_animation_state(puVar13[0x47],0x25);
        }
        iVar14 = DAT_006f1d6c;
        puVar13[0xcb] = uVar4;
        puVar13[0xcc] = *(uint *)(iVar14 + 0xc);
        if (puVar13[0xc9] == uVar17) {
          puVar13[0xc9] = 0xffffffff;
        }
        if (puVar13[0xca] == uVar17) {
          puVar13[0xca] = 0xffffffff;
        }
        FUN_004f6610(uVar17);
        object_set_position_and_orientation(uVar17,0,0);
        iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar18);
        (*(code *)PTR_matrix4x3_multiply_00696664)
                  (*(short *)(iVar14 + 0x1f2) + iVar14,iVar12 + 0x68,local_b0);
        puVar13[0x1d] = uStack_ac;
        puVar13[0x1e] = uStack_a8;
        puVar13[0x1f] = uStack_a4;
        puVar13[0x20] = uStack_94;
        puVar13[0x21] = uStack_90;
        iVar12 = DAT_008603b0;
        puVar13[0x22] = uStack_8c;
        puVar8 = *(uint **)(*(int *)(iVar12 + 0x34) + 8 + iVar18);
        iVar12 = *(int *)((*puVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (*(int *)(iVar12 + 0x34) != -1) {
          if ((puVar8[4] & 1) != 0) {
            object_for_each_light_attachment(0,1);
          }
          if (*(int *)(iVar12 + 0x34) != -1) {
            iVar12 = *(int *)(DAT_008603b0 + 0x34);
            puVar8[4] = puVar8[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar12 + iVar18 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
        }
        *(undefined2 *)(puVar13 + 0xbc) = 0xffff;
        *(undefined1 *)((int)puVar13 + 0x2a7) = 2;
        if (puVar5[0xc9] == uVar17) {
          puVar5[0xc9] = 0xffffffff;
        }
        if (puVar5[0xca] == uVar17) {
          puVar5[0xca] = 0xffffffff;
        }
        FUN_0056ce30();
        FUN_0056d6a0();
        FUN_00565420(uVar17);
        puVar3 = (undefined4 *)(*(short *)((int)puVar13 + 0x1ea) + 0x10 + (int)puVar13);
        *puVar3 = uVar15;
        puVar3[1] = uVar6;
        puVar3[2] = uVar7;
        if ((short)puVar13[0x2d] == 0) {
          FUN_0055add0(uVar17);
        }
        object_recalculate_bounding_radius_recursive(uVar17);
        cVar10 = FUN_00566910();
        if ((cVar10 == '\x01') && (iVar12 = object_try_and_get(2), iVar12 != 0)) {
          *(undefined4 *)(iVar12 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
        }
        iVar12 = DAT_0087a480;
        if (DAT_00719720 == 1) {
          iVar18 = datum_get();
          if ((iVar18 != 0) && (*(short *)(iVar18 + 2) == -1)) {
            *(undefined4 *)(iVar18 + 0x180) = 0;
            *(undefined4 *)(iVar18 + 0x17c) = 0;
            *(undefined4 *)(iVar18 + 0x1e0) = 0;
            *(undefined4 *)(iVar18 + 0x1dc) = 0;
            goto LAB_0040b559;
          }
          goto LAB_0040b567;
        }
      }
    }
    else {
      cVar10 = FUN_00565c60();
      if (cVar10 == '\0') {
        iVar12 = *(int *)((*puVar13 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        iVar18 = *(int *)(*(int *)((*(uint *)(iVar12 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                         + 0x10);
        iVar14 = (char)puVar13[0xa8] * 100;
        if ((8 < *(int *)(iVar14 + 0x40 + iVar18)) &&
           (*(short *)(*(int *)(iVar14 + iVar18 + 0x44) + 0x10) != -1)) {
          if (*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) + 0x324
                       ) == uVar17) {
            FUN_0056ab10();
          }
          uVar15 = FUN_004d6280(1);
          unit_set_custom_animation(*(undefined4 *)(iVar12 + 0x44),uVar15);
          iVar18 = (uVar17 & 0xffff) * 0xc;
          puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar18);
          iVar12 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if (*(int *)(iVar12 + 0x34) != -1) {
            if ((puVar5[4] & 1) != 0) {
              object_for_each_light_attachment(0,1);
            }
            if (*(int *)(iVar12 + 0x34) != -1) {
              iVar12 = *(int *)(DAT_008603b0 + 0x34);
              puVar5[4] = puVar5[4] & 0xfffffffe;
              pbVar1 = (byte *)(iVar12 + iVar18 + 2);
              *pbVar1 = *pbVar1 | 2;
            }
          }
          *(undefined1 *)((int)puVar13 + 0x2a3) = 0x1b;
          FUN_0042c370();
          if (puVar13[1] == 0) {
            FUN_0056c370(0);
          }
          iVar12 = DAT_006f1d6c;
          *(undefined4 *)(iVar2 + 0x390) = *(undefined4 *)(iVar2 + 0x158);
          *(int *)(iVar2 + 0x394) = *(int *)(iVar12 + 0xc) + 0xb4;
          local_ea = 1;
        }
      }
    }
  }
  *(undefined1 *)(iVar2 + 0x38c) = 0;
  uVar11 = local_ea;
LAB_0040b759:
  *(undefined1 *)(iVar2 + 0x2ed) = 0;
  return uVar11;
}
#endif
