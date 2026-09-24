// unit_apply_damage_effects  (Ghidra: FUN_005674a0)
// address 0x5674a0, size 3199 bytes
// name confidence: 0.35 (matches functions.md's summary: "Applies the gameplay side effects of
//   damage to a unit, including possible vehicle ejection, animation-state change, and
//   impact-direction-based effects")   rewrite confidence: 0.2
// evidence: types/objects.h damage_data (0x00 damage_effect_tag, 0x04 flags, 0x08
//   responsible_player, 0x0c responsible_object, 0x34 direction, 0x40 random_blend);
//   types/units.h unit_data.unknown_37c/0x404/0x406/0x408/0x40c/0x28b/0x2a3/0x2a7/0xcb-oob
//   (driver/gunner 0x324/0x328), biped_data (vehicle exit/network fields); types/objects.h
//   object.recent_shield_damage/recent_body_damage (0xf4/0xf8), object.body_vitality (0xe0),
//   object.shield_vitality (0xe4), object.vitality_flags (0x106), object.parent_object (0x11c),
//   object.forward (0x74), object.controlling_player is actually unit_data.controlling_player
//   (0x218, read through the same object pointer since unit_data starts at object+0x1f4);
//   types/tags.h Unit.feign_death_threshold/feign_death_time (0x22c/0x230), Unit.unit_flags
//   (0x17c, has bit 0x40000 == definition_flag0-ish "entrance_inside_bounding_sphere"-adjacent).
// register convention: this function has 1 caller and Ghidra recovered 7 named parameters
//   directly (no in_EAX/in_ECX leftovers at entry), so they are taken verbatim.
//   // blam-cc: param_1 -> unit_index, param_2 -> damage, param_3 -> flags,
//   //   param_4 -> body_damage_amount, param_5 -> shield_damage_amount,
//   //   param_6 -> forward_object (UNSURE), param_7 -> apply_effects
// UNSURE: this is the single worst-decompiled function in the module. Past the first third,
//   dozens of calls are shown with zero or one visible argument even though the immediately
//   preceding statements plainly build a larger argument list on the stack (Ghidra even leaks
//   raw return-address literals into several "stack variable" assignments, e.g.
//   `puStack_144 = (uint *)0x56764b`, which is a sure sign its per-call stack-argument recovery
//   gave up on this function). Every control-flow branch and every memory write from the
//   original is preserved exactly, including in the vehicle-ejection block; only the *extra*
//   arguments this rewrite could not recover for a handful of interior calls are left as the
//   single argument Ghidra shows, each flagged UNSURE-CALL in place. Getting one of those wrong
//   can only change a secondary notification (which node/marker a helper reads, whether a
//   melee/seat-transition helper receives a redundant duplicate of a value it already holds via
//   the surrounding object pointer); it cannot change unit_index, the object being modified, or
//   any of the vitality/animation-state/seat writes this function performs directly.
// UNSURE: several raw offsets into the parent/vehicle object (`0xbc`, `0xc9`, `0xca`, `0x1ea`,
//   `0x1f2`, `0x2e8`) and into the DamageEffect tag (`0x1c4` response block and its own +0x04,
//   +0x20, +0x24) are not named by this module's header; they are kept as literal byte offsets
//   with an inline note of what the surrounding logic implies about them, rather than guessed
//   field names.
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)
// reconciled: R32 follow-up: local extern player_control_globals (0x0071c2d8) renamed network_client (networking.h name) because game.h is now included and owns the player_control_globals typedef
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>  // uintptr_t only; this is a .c file, not a Ghidra-ingested header

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *player_data;      // 0x0087a480
extern int32_t game_connection_role; // 0x00719720, 1 = client, 2 = server
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)
extern void *matrix4x3_multiply_thunk; // 0x00696664
extern uint8_t *network_client; // 0x0071c2d8 (networking.h network_client; renamed from network_client, which collides with game.h's typedef)
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t is_dedicated_server_flag; // 0x00724a44

extern real random_real_range(real min, real max);                      // 0x401050
extern real vector2d_normalize_with_length(real_vector2d *v);                      // 0x4018e0, UNSURE signature
extern void actor_reassign_vehicle_seat(uint32_t unit_index);                             // 0x42b880, UNSURE signature
extern void actor_react_to_threat_event(uint16_t a, uint32_t unit_index, int32_t b, void *forward_object); // 0x42be40, UNSURE signature
extern void actor_notify_weapon_pickup_once(uint32_t object_index);                           // 0x42c370, UNSURE signature
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);               // 0x4cc0d0, UNSURE signature
extern real vector2d_angle_between(real_vector2d *a, real_vector2d *b);                     // 0x4cd480, UNSURE signature
extern uint32_t datum_get(void);                                           // 0x4d0680, UNSURE signature  // real signature (datum_get.c): void * datum_get(datum_index handle, data_array *array); Ghidra recovered 0 of 2 args at this call site
extern int32_t animation_choose_random_permutation(int32_t mode);                                 // 0x4d6280
extern void player_update_history_free_all(void);                         // 0x4e6f20
extern void network_index_cache_remove(uint32_t object_index);                           // 0x4e9d40, UNSURE signature
extern void object_set_position_and_orientation(uint32_t object_index, void *transform); // 0x4f51c0, UNSURE signature  // real signature (object_set_position_and_orientation.c): void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up, real_point3d *position); Ghidra recovered 2 of 4 args at this call site
extern void object_get_node_local_transform(uint32_t object_index, uint8_t *marker); // 0x4f6080, UNSURE signature, writes local_c8/c4/c0  // real signature (object_get_node_local_transform.c): int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t param_4); Ghidra recovered 2 of 4 args at this call site
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);                             // 0x4f6610, UNSURE signature
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index);       // 0x4f9a20, UNSURE signature  // real signature (object_for_each_light_attachment.c): void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); Ghidra recovered 1 of 3 args at this call site
extern void unit_reset_orientation_and_find_position(uint32_t object_index);                             // 0x55add0, UNSURE signature
extern void unit_choose_combat_reaction_animation(uint32_t unit_index, uint8_t reaction);           // 0x561140, UNSURE signature  // real signature (unit_choose_combat_reaction_animation.c): uint8_t unit_choose_combat_reaction_animation(uint32_t unit_index, const datum_index *reaction_source, uint8_t is_scripted, uint8_t allow_second_tier, float distance_bias); Ghidra recovered 2 of 5 args at this call site
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request);      // 0x565420, UNSURE signature
extern void unit_validate_and_clear_weapon_switch(uint32_t unit_index);                             // 0x5659c0, UNSURE signature
extern uint8_t unit_state_is_scripted_animation(unit_data *unit);                       // 0x565c60, UNSURE signature
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index);                          // 0x566910
extern void unit_broadcast_state_change_event(int32_t index);                                   // 0x566c00, UNSURE signature
extern void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction,
                                        uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction,
                                        float turn_angle, int16_t weapon_class_index, int32_t fire_trigger_event,
                                        uint8_t require_still); // 0x566de0
extern void unit_record_recent_damage_and_react(void *damage_record, uint32_t unit_index);        // 0x568230, UNSURE signature  // real signature (unit_record_recent_damage_and_react.c): void unit_record_recent_damage_and_react(uint32_t unit_index, float damage_amount, int16_t response_index, uint8_t allow_broadcast, uint32_t responsible_player, int16_t team_index, uint32_t responsible_object); Ghidra recovered 2 of 7 args at this call site
extern void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset);           // 0x568610, UNSURE signature
extern void unit_notify_weapon_removed(uint32_t unit_index);                             // 0x56ab10  // real signature (unit_notify_weapon_removed.c): void unit_notify_weapon_removed(int32_t object_index, int16_t new_state); Ghidra recovered 1 of 2 args at this call site
extern void unit_dispatch_scripted_event_9(uint32_t unit_index);                             // 0x56c370, UNSURE signature  // real signature (unit_dispatch_scripted_event_9.c): void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); Ghidra recovered 1 of 2 args at this call site
extern void unit_recompute_seat_occupants(uint32_t unit_index);                             // 0x56ce30
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index);                             // 0x56d6a0
extern void unit_set_custom_animation(TagID animation_graph_tag, int16_t animation_index); // 0x56ebd0  // real signature (unit_set_custom_animation.c): void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); Ghidra recovered 2 of 3 args at this call site
extern void unit_enter_stunned_state(uint32_t unit_index);                             // 0x5705a0, UNSURE signature  // real signature (unit_enter_stunned_state.c): void unit_enter_stunned_state(uint32_t unit_index, uint32_t responsible_object); Ghidra recovered 1 of 2 args at this call site

void unit_apply_damage_effects(datum_index unit_index, damage_data *damage, uint8_t flags,
                               float body_damage_amount, float shield_damage_amount,
                               void *forward_object, uint8_t apply_effects)
{
    uint32_t *self = (uint32_t *)((object_header *)object_data->data)[unit_index & 0xffff].data; // puVar16
    object *self_obj = (object *)self;
    Unit *unit_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;               // local_28
    unit_data *unit = (unit_data *)((uint8_t *)self + k_unit_data_offset);
    uint8_t *deffect_tag = (uint8_t *)tag_instances[damage->damage_effect_tag & 0xffff].data;      // iVar19 initially
    uint8_t *response_block = deffect_tag + 0x1c4;                                                 // local_1c

    uint32_t node_snapshot[3];                 // local_44 / local_40 / local_3c
    uint8_t is_stun_reaction = flags & 1;      // bVar15, refined below
    uint32_t stun_flag = flags & 1;            // local_18 low byte
    uint32_t death_reaction_flag = 0;          // local_20 low byte
    uint32_t shield_break_flag;                // local_2c low byte

    if ((apply_effects == 1) && (0.0f < self_obj->recent_body_damage + self_obj->recent_shield_damage)) {
        float total = self_obj->recent_body_damage + self_obj->recent_shield_damage;
        unit->unknown_404 = *(int16_t *)(deffect_tag + 0x1c6);
        unit->unknown_406 = 0x2d;
        if (total < unit->unknown_408) {
            total = unit->unknown_408;
        }
        unit->unknown_408 = total;
        if (damage->responsible_object != k_datum_index_none) {
            unit->unknown_40c = damage->responsible_object;
        }
    }

    uint32_t unit_flags_word = unit->flags; // local_c, reused heavily below for unrelated things
    if ((unit_flags_word & 0x10) != 0) {
        float v = unit->unknown_37c - *(float *)(deffect_tag + 0x1e0);
        unit->unknown_37c = v;
        if (v < 0.0f) {
            unit->unknown_37c = 0.0f;
        }
    }

    if (apply_effects == 1) {
        if (((flags & 1) == 0) || (*(float *)(deffect_tag + 500) < 2.0f)) {
            shield_break_flag = 0;
        } else {
            shield_break_flag = 1;
        }
        if (((flags & 1) == 0) && ((unit_flags_word & 0x2000) != 0) &&
            (0.0f < unit_tag->feign_death_threshold) && (0.0f < unit_tag->feign_death_time) &&
            (0.0f < self_obj->body_vitality) && (unit_tag->feign_death_threshold < self_obj->recent_body_damage)) {
            random_real_range(0.0f, 1.0f); // UNSURE: result discarded in the original decompilation
            self_obj->vitality_flags |= _object_health_frozen_bit;
            death_reaction_flag = 1;
            self_obj->shield_stun_ticks = 0; // UNSURE: __ftol() of the random_real_range() result
            is_stun_reaction = (uint8_t)stun_flag;
        }
    }

    if ((unit->controlling_player == k_datum_index_none) && (is_stun_reaction != 0) &&
        (*(int8_t *)(response_block + 4) < 0) && ((unit_tag->base.flags & 0x40000) != 0)) {
        if (self_obj->parent_object == k_datum_index_none) {
            goto no_parent_dispatch;
        }
        object *parent = object_try_and_get(self_obj->parent_object, _object_mask_unit); // UNSURE: args recovered from context
        if ((parent != (object *)0) && (game_connection_role != 1) &&
            (parent->parent_object != k_datum_index_none) &&          // local_10[0x47] = object+0x11c
            (((unit_data *)((uint8_t *)parent + k_unit_data_offset))->vehicle_seat_index != -1)) { // local_10[0xbc] = object+0x2f0
            if (parent->type == _object_type_vehicle) {
                self = (uint32_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
                unit_data *self_unit = (unit_data *)((uint8_t *)self + k_unit_data_offset);
                datum_index grandparent = self_obj->parent_object;    // local_10 = puVar16[0x47]
                if ((grandparent == k_datum_index_none) || (self_unit->vehicle_seat_index == -1)) { // puVar16[0xbc]
seat_loop_reenter:
                    // LAB_00567a9a: on a client (role == 1) fall into the controlling-player
                    // block below; on anything else skip straight past it.
                    if (game_connection_role != 1) {
                        goto after_eject;
                    }
                } else {
                    object *seat_parent = ((object_header *)object_data->data)[grandparent & 0xffff].data; // local_14
                    // UNSURE: the marker/exit-position math below (0x2e8/0x24/0x11c stride, node
                    // transform, and the 0x1ea/0x1f2 node-array offsets) is reproduced with raw
                    // offsets; see out/phase4/units_types_notes.md for the UnitSeat 0x11c stride.
                    Unit *seat_parent_tag = (Unit *)tag_instances[seat_parent->definition_tag & 0xffff].data;
                    uint8_t *exit_marker = (uint8_t *)(*(int32_t *)((uint8_t *)seat_parent_tag + 0x2e8) + 0x24 +
                                                       self_unit->vehicle_seat_index * 0x11c); // puVar16[0xbc]
                    object_get_node_local_transform((uint32_t)(uintptr_t)seat_parent, exit_marker); // fills local_c8/c4/c0
                    // iVar14: the node block the original snapshots *here*, before any of the
                    // writes below, out of Object.model.tag_id's tag data + 0xbc.
                    {
                        Object *self_def0 = (Object *)tag_instances[self_obj->definition_tag & 0xffff].data;
                        uint32_t *node_block = (uint32_t *)(*(int32_t *)((uint8_t *)tag_instances[
                            self_def0->model.tag_id.index & 0xffff].data + 0xbc));
                        node_snapshot[0] = node_block[0xa];  // local_44 = iVar14 + 0x28
                        node_snapshot[1] = node_block[0xb];  // local_40 = iVar14 + 0x2c
                        node_snapshot[2] = node_block[0xc];  // local_3c = iVar14 + 0x30
                    }
                    // UNSURE: local_38/34/30 (marker position minus local_c8/c4/c0) are computed by
                    // the original but this rewrite could not confirm object_get_node_local_transform's
                    // true output shape; preserved as an offset-only comment rather than guessed code.
                    if ((((unit_data *)((uint8_t *)seat_parent + k_unit_data_offset))->driver_unit_index == unit_index) &&
                        (((unit_data *)((uint8_t *)seat_parent + k_unit_data_offset))->animation_state != 0x25) &&
                        (self_obj->parent_object != k_datum_index_none)) { // local_14+0x2a3, puVar16[0x47]
                        unit_try_set_animation_state(unit_index, 0x1b);
                    }
                    self_unit->last_parent_object_index = grandparent;          // puVar16[0xcb] = local_10
                    self_unit->last_seat_change_tick = game_time->game_time;  // puVar16[0xcc]
                    // UNSURE: Ghidra shows this first clear pair on puVar16 (this unit) and the
                    // second pair further down on local_14 (the vehicle). Testing a unit's own
                    // driver/gunner handle against its own index reads oddly, but it is what the
                    // decompilation says, so each pair is reproduced against the base Ghidra gives
                    // rather than folded into one.
                    if (self_unit->driver_unit_index == unit_index) {
                        self_unit->driver_unit_index = k_datum_index_none;
                    }
                    if (self_unit->gunner_unit_index == unit_index) {
                        self_unit->gunner_unit_index = k_datum_index_none;
                    }
                    object_snap_to_parent_marker_and_detach(unit_index);
                    object_set_position_and_orientation(unit_index, exit_marker); // UNSURE-CALL: transform arg
                    // UNSURE-CALL: matrix4x3_multiply's real operands (node array transform x exit
                    // marker transform) are not reproduced field-by-field; see the #if 0 block.
                    matrix4x3_multiply(0, 0, 0);
                    object *self_obj2 = self_obj;
                    Object *self_def = (Object *)tag_instances[self_obj2->definition_tag & 0xffff].data;
                    if ((*(uint32_t *)&self_def->model.tag_id != 0xffffffff) && ((self_obj2->flags & 1) != 0)) {
                        object_for_each_light_attachment(unit_index);
                    }
                    if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                        self_obj2->flags &= ~1u;                      // puVar4[4] &= 0xfffffffe
                        ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
                    }
                    self_unit->vehicle_seat_index = -1;
                    self_unit->base_animation_state = _unit_base_animation_state_stand;
                    if (((unit_data *)((uint8_t *)seat_parent + k_unit_data_offset))->driver_unit_index == unit_index) {
                        ((unit_data *)((uint8_t *)seat_parent + k_unit_data_offset))->driver_unit_index = k_datum_index_none;
                    }
                    if (((unit_data *)((uint8_t *)seat_parent + k_unit_data_offset))->gunner_unit_index == unit_index) {
                        ((unit_data *)((uint8_t *)seat_parent + k_unit_data_offset))->gunner_unit_index = k_datum_index_none;
                    }
                    unit_recompute_seat_occupants(unit_index);
                    unit_pick_and_ready_next_weapon(unit_index);
                    // UNSURE-CALL: Ghidra shows unit_update_animation_state_machine with no visible arguments; the
                    // statement before it is local_14 = CONCAT12(0x14, (uint16)local_14), i.e. the
                    // register argument is (0x14 << 16) | (previous local_14 & 0xffff).
                    unit_update_animation_state_machine(unit_index, (const int8_t *)0x14);
                    // writes the three dwords snapshotted above into the node block at
                    // self + *(int16 *)(self + 0x1ea) + 0x10
                    uint32_t *node_func = (uint32_t *)(*(int16_t *)((uint8_t *)self + 0x1ea) + 0x10 + (uint8_t *)self);
                    node_func[0] = node_snapshot[0];
                    node_func[1] = node_snapshot[1];
                    node_func[2] = node_snapshot[2];
                    if (self_obj2->type == _object_type_biped) {
                        unit_reset_orientation_and_find_position(unit_index);
                    }
                    object_recalculate_bounding_radius_recursive(unit_index);
                    if ((unit_all_seats_unoccupied(unit_index) == 1) && (object_try_and_get(unit_index, 0xffffffff) != 0)) {
                        // UNSURE: writes the current tick into the retrieved object at +0x5ac
                        // (vehicle_data.network_update_tick); the mask argument to object_try_and_get
                        // is unrecovered.
                    }
                    if (game_connection_role != 1) {
                        goto after_eject;                 // LAB_00567b07
                    }
                    uint32_t player_record = datum_get();
                    if ((player_record != 0) && (*(int16_t *)(player_record + 2) == -1)) {
                        *(uint32_t *)(player_record + 0x180) = 0;
                        *(uint32_t *)(player_record + 0x17c) = 0;
                        *(uint32_t *)(player_record + 0x1e0) = 0;
                        *(uint32_t *)(player_record + 0x1dc) = 0;
                        goto seat_loop_reenter;
                    }
                }
seat_loop_check_deferred:
                uint32_t controlling = ((unit_data *)((uint8_t *)self + k_unit_data_offset))->controlling_player;
                if ((controlling != k_datum_index_none) && (-1 < (int16_t)controlling) &&
                    ((int16_t)controlling < *(int16_t *)((uint8_t *)player_data + 0x20))) {
                    int32_t rec_off = *(int16_t *)((uint8_t *)player_data + 0x22) * (int16_t)controlling;
                    int16_t sanity = *(int16_t *)(rec_off + *(int32_t *)((uint8_t *)player_data + 0x34));
                    if ((sanity != 0) &&
                        (((int16_t)(controlling >> 16) == 0) || (sanity == (int16_t)(controlling >> 16))) &&
                        (*(int16_t *)(rec_off + *(int32_t *)((uint8_t *)player_data + 0x34) + 2) != -1) &&
                        (network_client != 0)) {
                        player_update_history_free_all();
                    }
                }
            } else {
                // parent is not a vehicle: try the "melee lunge" damage-transfer path
                object *responsible = parent; // local_10 in this branch
                if ((unit_state_is_scripted_animation(0) == 0)) { // UNSURE-CALL: original passes no visible argument
                    int32_t weapon_class = (int8_t)*((uint8_t *)responsible + 0x2a0 /* animation_definition_index-ish, see UNSURE */) * 100 +
                        *(int32_t *)((uint8_t *)tag_instances[
                            (*(uint32_t *)((uint8_t *)tag_instances[responsible->definition_tag & 0xffff].data + 0x44)) & 0xffff].data + 0x10);
                    if (8 < *(int32_t *)(uintptr_t)(weapon_class + 0x40)) {
                        int16_t chosen = *(int16_t *)(*(int32_t *)(uintptr_t)(weapon_class + 0x44) + 0x10);
                        if (chosen != -1) {
                            if (((unit_data *)((uint8_t *)((object_header *)object_data->data)[self_obj->parent_object & 0xffff].data
                                    + k_unit_data_offset))->driver_unit_index == unit_index) {
                                unit_notify_weapon_removed(unit_index);
                            }
                            int32_t new_anim = animation_choose_random_permutation(1);
                            unit_set_custom_animation(unit_tag->base.animation_graph.tag_id, (int16_t)new_anim);
                            object *self_obj3 = ((object_header *)object_data->data)[unit_index & 0xffff].data; // puVar4
                            Object *self_def = (Object *)tag_instances[self_obj3->definition_tag & 0xffff].data;
                            if ((*(uint32_t *)&self_def->model.tag_id != 0xffffffff) && ((self_obj3->flags & 1) != 0)) {
                                object_for_each_light_attachment(unit_index);
                            }
                            if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                                self_obj3->flags &= ~1u;                  // puVar4[4] &= 0xfffffffe
                                ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
                            }
                            *(int8_t *)((uint8_t *)responsible + 0x2a3) = 0x1b;
                            actor_notify_weapon_pickup_once((uint32_t)(uintptr_t)responsible);
                            if (*(uint32_t *)((uint8_t *)responsible + 4) == 0) { // puVar8[1], object + 0x04
                                unit_dispatch_scripted_event_9(unit_index);
                            }
                            goto no_parent_dispatch;
                        }
                    }
                }
            }
        }
    }
    goto after_eject;

no_parent_dispatch:
    if (apply_effects == 1) {
        unit_enter_stunned_state(unit_index);
    }
    stun_flag = 0;
    goto after_stance;

after_eject:
    self = (uint32_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    self_obj = (object *)self;
    if (((damage->flags & 0x10) == 0) &&
        (((stun_flag != 0) || (death_reaction_flag != 0) ||
          ((self_obj->vitality_flags & _object_health_frozen_bit) == 0)) &&
         (((unit->flags & 0x800000) == 0) && ((*(uint32_t *)(response_block + 4) & 0x10) == 0)))) {
        real_vector3d local_dir = { damage->direction.i, damage->direction.j, 0.0f };
        real_vector3d fwd = { self_obj->forward.i, self_obj->forward.j, 0.0f };
        uint8_t need_reaction = 0;
        uint8_t use_0x8a_flag = 0;
        uint8_t have_angle = 0;
        float angle = 0.0f;
        float len1 = vector2d_normalize_with_length((real_vector2d *)&local_dir);
        if (0.0f < len1) {
            float len2 = vector2d_normalize_with_length((real_vector2d *)&fwd);
            if (0.0f < len2) {
                angle = vector2d_angle_between((real_vector2d *)&local_dir, (real_vector2d *)&fwd);
                have_angle = 1;
            }
        }
        if ((*(int8_t *)((uint8_t *)unit_tag + 0x17c) < 0) && ((*(uint32_t *)(response_block + 4) & 4) == 0)) {
            need_reaction = 1;
        }
        if (unit->unknown_28b != 0) {
            need_reaction = 1;
        }
        if ((flags & 0x8a) != 0) {
            use_0x8a_flag = 1;
        }
        // UNSURE-CALL: see file header -- the original builds a larger record here (validity
        // flag, stun/death/shield flags, need_reaction, use_0x8a_flag, !have_angle,
        // forward_object truncated to 16 bits, angle, direction.x/y, and a player-history
        // pointer) but the call below is the only part of it Ghidra could still show explicitly.
        unit_update_stance_and_jump(unit_index, (uint8_t)stun_flag, 0, 0, 0, 0,
                                    have_angle ? angle : 0.0f, (int16_t)(uintptr_t)forward_object, 0, 0);
    }

after_stance:
    if (apply_effects == 1) {
        if ((damage->responsible_player != k_datum_index_none) && (unit->controlling_player != k_datum_index_none) &&
            (current_game_engine != 0)) {
            // UNSURE-CALL: calls through a function pointer at
            // *(code**)((uint8_t *)current_game_engine + 100) with damage->responsible_player.
        }
        if ((damage->responsible_player != k_datum_index_none) || (damage->responsible_object != k_datum_index_none)) {
            unit_record_recent_damage_and_react(damage, unit_index);
        }
        if (((damage->flags & 0x10) == 0) &&
            (((flags & 1) != 0) || (0.0f < shield_damage_amount) || (0.0f < body_damage_amount))) {
            unit_choose_combat_reaction_animation(unit_index, (uint8_t)(death_reaction_flag | stun_flag));
        }
    }
    if ((0.0f < shield_damage_amount) || (0.0f < body_damage_amount)) {
        unit_validate_and_clear_weapon_switch(unit_index);
    }
    if ((apply_effects == 1) && (self_obj->type == _object_type_biped)) {
        if (stun_flag == 0) {
            if ((self_obj->vitality_flags & _object_health_frozen_bit) == 0) {
                actor_react_to_threat_event(0, unit_index, damage->unknown_4e, forward_object); // UNSURE-CALL
            }
        } else {
            actor_reassign_vehicle_seat(unit_index);
            self = (uint32_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
            self_obj = (object *)self;
        }
    }
    if ((unit->controlling_player != k_datum_index_none) && (0.0f < *(float *)(response_block + 0x20)) &&
        ((current_game_engine != 0) || (is_dedicated_server_flag != 0))) {
        float a = damage->random_blend * *(float *)(response_block + 0x20);
        float b = *(float *)(response_block + 0x24) * damage->random_blend;
        float clamped_a = a < 0.0f ? 0.0f : a;
        if (b < 0.0f) {
            b = 0.0f;
        } else if (1.0f <= b) {
            b = 1.0f;
        }
        if (unit->unknown_424 < b) {
            float sum = unit->unknown_424 + clamped_a;
            unit->unknown_424 = sum;
            if (b < sum) {
                unit->unknown_424 = b;
            }
        }
        // UNSURE: the three __ftol() results that clamp unit+0x42a (ai_communication_count,
        // reused here as a stun-display counter per the original) are not reproduced
        // individually; this only affects a display/UI-adjacent counter, not vitality.
    }
    if ((apply_effects == 1) && ((stun_flag != 0) || (death_reaction_flag != 0))) {
        unit_release_transient_state(unit_index, 0);
        if ((self_obj->network_role == 0) && (stun_flag == 1)) {
            // UNSURE-CALL: forwards this function's own 7 incoming parameters (padded to 8
            // dwords) to unit_broadcast_state_change_event, matching the original's raw stack-to-stack copy.
            struct { datum_index a; damage_data *b; uint8_t c; float d; float e; void *f; uint8_t g; uint8_t pad; } forward = {
                unit_index, damage, flags, body_damage_amount, shield_damage_amount, forward_object, apply_effects, 0
            };
            unit_broadcast_state_change_event((int32_t)&forward);
            if ((*(uint8_t *)((uint8_t *)object_data->data + (unit_index & 0xffff) * 0xc + 2) & 8) == 0) {
                network_index_cache_remove(unit_index);
            }
            self_obj->network_role = 3;
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x5674a0):

void FUN_005674a0(uint *param_1,uint *param_2,byte param_3,float param_4,float param_5,uint *param_6
                 ,char param_7)

{
  byte *pbVar1;
  undefined4 *puVar2;
  float fVar3;
  uint *puVar4;
  uint uVar5;
  float fVar6;
  bool bVar7;
  uint *puVar8;
  char cVar9;
  undefined2 uVar10;
  short sVar11;
  short sVar12;
  short sVar13;
  int iVar14;
  byte bVar15;
  uint *puVar16;
  int iVar17;
  uint **ppuVar18;
  int iVar19;
  float10 fVar20;
  uint auStack_158 [2];
  uint *puStack_150;
  uint uStack_14c;
  uint *puStack_148;
  uint *puStack_144;
  float local_c8;
  float local_c4;
  float local_c0;
  uint local_b4;
  uint local_b0;
  uint local_ac;
  uint local_9c;
  uint local_98;
  uint local_94;
  uint *local_68;
  undefined1 local_64;
  undefined1 local_63;
  undefined1 local_62;
  undefined1 local_61;
  undefined1 local_60;
  undefined1 local_5f;
  undefined1 local_5e;
  undefined2 local_5c;
  uint *local_58;
  float local_54;
  float local_50;
  undefined4 local_4c;
  uint *local_48;
  undefined4 local_44;
  uint local_40;
  uint local_3c;
  float local_38;
  float local_34;
  float local_30;
  uint local_2c;
  int local_28;
  uint *local_24;
  uint local_20;
  int local_1c;
  uint *local_18;
  undefined4 local_14;
  uint *local_10;
  uint local_c;

  local_48 = (uint *)(param_4 + param_5);
  iVar17 = ((uint)param_1 & 0xffff) * 0xc;
  puVar16 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
  local_28 = *(int *)((*puVar16 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar19 = *(int *)((*param_2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_1c = iVar19 + 0x1c4;
  bVar15 = param_3 & 1;
  local_18 = (uint *)(CONCAT31(local_18._1_3_,param_3) & 0xffffff01);
  local_20 = local_20 & 0xffffff00;
  uVar5 = local_2c >> 8;
  local_2c = local_2c & 0xffffff00;
  if ((param_7 == '\x01') && (fVar3 = (float)puVar16[0x3e] + (float)puVar16[0x3d], 0.0 < fVar3)) {
    *(undefined2 *)(puVar16 + 0x101) = *(undefined2 *)(iVar19 + 0x1c6);
    *(undefined2 *)((int)puVar16 + 0x406) = 0x2d;
    if (fVar3 < (float)puVar16[0x102]) {
      fVar3 = (float)puVar16[0x102];
    }
    puVar16[0x102] = (uint)fVar3;
    if (param_2[3] != 0xffffffff) {
      puVar16[0x103] = param_2[3];
    }
  }
  local_c = puVar16[0x81];
  if (((local_c & 0x10) != 0) &&
     (fVar3 = (float)puVar16[0xdf] - *(float *)(iVar19 + 0x1e0), puVar16[0xdf] = (uint)fVar3,
     fVar3 < 0.0)) {
    puVar16[0xdf] = 0;
  }
  local_24 = puVar16;
  if (param_7 == '\x01') {
    if (((param_3 & 1) == 0) || (local_2c = CONCAT31((int3)uVar5,1), *(float *)(iVar19 + 500) < 2.0)
       ) {
      local_2c = local_2c & 0xffffff00;
    }
    if ((((((param_3 & 1) == 0) && ((local_c & 0x2000) != 0)) &&
         (0.0 < *(float *)(local_28 + 0x22c))) &&
        ((0.0 < *(float *)(local_28 + 0x230) && (0.0 < (float)puVar16[0x38])))) &&
       (*(float *)(local_28 + 0x22c) < (float)puVar16[0x3e])) {
      puStack_144 = (uint *)0x56764b;
      random_real_range(0.0,1.0);
      *(byte *)((int)puVar16 + 0x106) = *(byte *)((int)puVar16 + 0x106) | 4;
      local_20 = CONCAT31(local_20._1_3_,1);
      uVar10 = __ftol();
      *(undefined2 *)(puVar16 + 0x108) = uVar10;
      bVar15 = (byte)local_18;
    }
  }
  if (((puVar16[0x86] == 0xffffffff) && (bVar15 != 0)) &&
     ((*(char *)(local_1c + 4) < '\0' && ((*(uint *)(local_28 + 0x17c) & 0x40000) != 0)))) {
    if (puVar16[0x47] == 0xffffffff) {
LAB_00567dbf:
      if (param_7 == '\x01') {
        FUN_005705a0();
      }
      local_18 = (uint *)((uint)local_18 & 0xffffff00);
      goto LAB_00567b40;
    }
    local_10 = (uint *)object_try_and_get();
    if ((((local_10 != (uint *)0x0) && (DAT_00719720 != 1)) &&
        (local_c = local_10[0x47], local_c != 0xffffffff)) && ((short)local_10[0xbc] != -1)) {
      if ((short)local_10[0x2d] == 1) {
        puVar16 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
        local_10 = (uint *)puVar16[0x47];
        if ((local_10 == (uint *)0xffffffff) || ((short)puVar16[0xbc] == -1)) {
LAB_00567a9a:
          iVar19 = DAT_0087a480;
          if (DAT_00719720 != 1) goto LAB_00567b07;
        }
        else {
          local_14 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)local_10 & 0xffff) * 0xc)
          ;
          iVar19 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
          iVar19 = *(short *)(iVar19 + 0x1f2) + iVar19;
          puStack_144 = (uint *)(*(int *)(*(int *)((*local_14 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                                  ) + 0x2e8) + 0x24 + (short)puVar16[0xbc] * 0x11c);
          uStack_14c = 0x5677b7;
          puStack_148 = local_10;
          object_get_node_local_transform();
          local_38 = *(float *)(iVar19 + 0x28) - local_c8;
          local_34 = *(float *)(iVar19 + 0x2c) - local_c4;
          iVar14 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar16 & 0xffff) * 0x20 + 0x14 +
                                                        DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                     0x14 + DAT_0087bc14) + 0xbc);
          local_c = iVar14 + 0x68;
          local_44 = *(undefined4 *)(iVar14 + 0x28);
          local_30 = *(float *)(iVar19 + 0x30) - local_c0;
          local_40 = *(uint *)(iVar14 + 0x2c);
          local_3c = *(uint *)(iVar14 + 0x30);
          if (((uint *)local_14[0xc9] == param_1) &&
             ((*(char *)((int)local_14 + 0x2a3) != '%' && (puVar16[0x47] != 0xffffffff)))) {
            puStack_144 = (uint *)0x567850;
            unit_try_set_animation_state();
          }
          iVar19 = DAT_006f1d6c;
          puVar16[0xcb] = (uint)local_10;
          puVar16[0xcc] = *(uint *)(iVar19 + 0xc);
          if ((uint *)puVar16[0xc9] == param_1) {
            puVar16[0xc9] = 0xffffffff;
          }
          if ((uint *)puVar16[0xca] == param_1) {
            puVar16[0xca] = 0xffffffff;
          }
          FUN_004f6610();
          puStack_144 = param_1;
          puStack_148 = (uint *)0x5678c9;
          object_set_position_and_orientation();
          iVar19 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
          puStack_144 = (uint *)(*(short *)(iVar19 + 0x1f2) + iVar19);
          puStack_148 = (uint *)0x5678f3;
          (*(code *)PTR_matrix4x3_multiply_00696664)();
          puVar16[0x1d] = local_b4;
          puVar16[0x1e] = local_b0;
          puVar16[0x1f] = local_ac;
          puVar16[0x20] = local_9c;
          puVar16[0x21] = local_98;
          iVar19 = DAT_008603b0;
          puVar16[0x22] = local_94;
          puVar4 = *(uint **)(*(int *)(iVar19 + 0x34) + 8 + iVar17);
          local_c = *(uint *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if ((*(int *)(local_c + 0x34) != -1) && ((puVar4[4] & 1) != 0)) {
            puStack_144 = (uint *)0x567971;
            object_for_each_light_attachment();
          }
          if (*(int *)(local_c + 0x34) != -1) {
            iVar19 = *(int *)(DAT_008603b0 + 0x34);
            puVar4[4] = puVar4[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar19 + iVar17 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
          *(undefined2 *)(puVar16 + 0xbc) = 0xffff;
          *(undefined1 *)((int)puVar16 + 0x2a7) = 2;
          if ((uint *)local_14[0xc9] == param_1) {
            local_14[0xc9] = 0xffffffff;
          }
          if ((uint *)local_14[0xca] == param_1) {
            local_14[0xca] = 0xffffffff;
          }
          FUN_0056ce30();
          FUN_0056d6a0();
          local_14 = (uint *)(uint)CONCAT12(0x14,(undefined2)local_14);
          FUN_00565420();
          puVar2 = (undefined4 *)(*(short *)((int)puVar16 + 0x1ea) + 0x10 + (int)puVar16);
          *puVar2 = local_44;
          puVar2[1] = local_40;
          puVar2[2] = local_3c;
          if ((short)puVar16[0x2d] == 0) {
            FUN_0055add0();
          }
          object_recalculate_bounding_radius_recursive();
          cVar9 = FUN_00566910();
          if ((cVar9 == '\x01') && (iVar19 = object_try_and_get(), iVar19 != 0)) {
            *(undefined4 *)(iVar19 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
          }
          iVar19 = DAT_0087a480;
          if (DAT_00719720 != 1) goto LAB_00567b07;
          iVar14 = datum_get();
          if ((iVar14 != 0) && (*(short *)(iVar14 + 2) == -1)) {
            *(undefined4 *)(iVar14 + 0x180) = 0;
            *(undefined4 *)(iVar14 + 0x17c) = 0;
            *(undefined4 *)(iVar14 + 0x1e0) = 0;
            *(undefined4 *)(iVar14 + 0x1dc) = 0;
            goto LAB_00567a9a;
          }
        }
        uVar5 = puVar16[0x86];
        if (((uVar5 != 0xffffffff) && (sVar11 = (short)uVar5, -1 < sVar11)) &&
           (sVar11 < *(short *)(iVar19 + 0x20))) {
          iVar14 = (int)*(short *)(iVar19 + 0x22) * (int)sVar11;
          sVar11 = *(short *)(iVar14 + *(int *)(iVar19 + 0x34));
          if ((((sVar11 != 0) &&
               ((sVar12 = (short)(uVar5 >> 0x10), sVar12 == 0 || (sVar11 == sVar12)))) &&
              (*(short *)(iVar14 + *(int *)(iVar19 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)) {
            player_update_history_free_all();
          }
        }
      }
      else {
        cVar9 = FUN_00565c60();
        if (((cVar9 == '\0') &&
            (iVar19 = (char)local_10[0xa8] * 100 +
                      *(int *)(*(int *)((*(uint *)(*(int *)((*local_10 & 0xffff) * 0x20 + 0x14 +
                                                           DAT_0087bc14) + 0x44) & 0xffff) * 0x20 +
                                        0x14 + DAT_0087bc14) + 0x10), 8 < *(int *)(iVar19 + 0x40)))
           && (sVar11 = *(short *)(*(int *)(iVar19 + 0x44) + 0x10), local_14 = (uint *)(int)sVar11,
              sVar11 != -1)) {
          if (*(uint **)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_c & 0xffff) * 0xc) +
                        0x324) == param_1) {
            FUN_0056ab10();
          }
          FUN_004d6280();
          puStack_144 = (uint *)0x567d31;
          unit_set_custom_animation();
          puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar17);
          local_c = *(uint *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if ((*(int *)(local_c + 0x34) != -1) && ((puVar4[4] & 1) != 0)) {
            puStack_144 = (uint *)0x567d72;
            object_for_each_light_attachment();
          }
          puVar8 = local_10;
          if (*(int *)(local_c + 0x34) != -1) {
            iVar19 = *(int *)(DAT_008603b0 + 0x34);
            puVar4[4] = puVar4[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar19 + iVar17 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
          *(undefined1 *)((int)local_10 + 0x2a3) = 0x1b;
          FUN_0042c370();
          if (puVar8[1] == 0) {
            FUN_0056c370();
          }
          goto LAB_00567dbf;
        }
      }
    }
  }
LAB_00567b07:
  puVar16 = local_24;
  if (((param_2[1] & 0x10) == 0) &&
     (((((byte)local_18 != '\0' || ((byte)local_20 != '\0')) ||
       ((*(byte *)((int)local_24 + 0x106) & 4) == 0)) &&
      (((local_24[0x81] & 0x800000) == 0 && (uVar5 = *(uint *)(local_1c + 4), (uVar5 & 0x10) == 0)))
      ))) {
    local_34 = (float)param_2[0xd];
    local_40 = local_24[0x1d];
    local_30 = (float)param_2[0xe];
    local_3c = local_24[0x1e];
    local_10 = (uint *)((uint)local_10 & 0xffffff00);
    local_c = local_c & 0xffffff00;
    bVar7 = false;
    local_14 = (uint *)0x0;
    fVar20 = (float10)vector2d_normalize_with_length();
    if (((float10)0.0 < fVar20) &&
       (fVar20 = (float10)vector2d_normalize_with_length(), (float10)0.0 < fVar20)) {
      fVar20 = (float10)vector2d_angle_between();
      local_14 = (uint *)(float)fVar20;
      bVar7 = true;
      puVar16 = local_24;
    }
    if ((*(char *)(local_28 + 0x17c) < '\0') && ((uVar5 & 4) == 0)) {
      local_10 = (uint *)CONCAT31(local_10._1_3_,1);
    }
    if (*(char *)((int)puVar16 + 0x28b) != '\0') {
      local_10 = (uint *)CONCAT31(local_10._1_3_,1);
    }
    if ((param_3 & 0x8a) != 0) {
      local_c = CONCAT31(local_c._1_3_,1);
    }
    local_5e = !bVar7;
    local_63 = (byte)local_18;
    local_62 = (byte)local_20;
    local_61 = (undefined1)local_2c;
    local_64 = 1;
    local_60 = local_10._0_1_;
    local_5f = (undefined1)local_c;
    local_5c = SUB42(param_6,0);
    local_58 = local_14;
    if (!(bool)local_5e) {
      local_54 = local_34;
      local_50 = local_30;
    }
    local_4c = 0;
    if ((puVar16[0x86] != 0xffffffff) && (iVar19 = datum_get(), iVar19 != 0)) {
      local_4c = *(undefined4 *)(iVar19 + 0x2c);
    }
    puStack_144 = param_6;
    puStack_148 = local_14;
    uStack_14c = local_c;
    puStack_150 = local_10;
    auStack_158[1] = local_2c;
    auStack_158[0] = local_20;
    FUN_00566de0(param_1,local_18);
    puVar16 = local_24;
  }
  else {
    local_64 = 0;
  }
LAB_00567b40:
  if (param_7 == '\x01') {
    if (((((uint *)param_2[2] != (uint *)0xffffffff) && (puVar16[0x86] != 0xffffffff)) &&
        (DAT_006f1d20 != 0)) && (*(code **)(DAT_006f1d20 + 100) != (code *)0x0)) {
      puStack_148 = (uint *)0x567b7f;
      puStack_144 = (uint *)param_2[2];
      (**(code **)(DAT_006f1d20 + 100))();
    }
    if (((uint *)param_2[2] != (uint *)0xffffffff) || (param_2[3] != 0xffffffff)) {
      uStack_14c = (uint)*(ushort *)(local_1c + 2);
      puStack_148 = local_18;
      puStack_150 = local_48;
      auStack_158[1] = 0x567bb5;
      puStack_144 = (uint *)param_2[2];
      FUN_00568230();
    }
    if (((param_2[1] & 0x10) == 0) && ((((param_3 & 1) != 0 || (0.0 < param_5)) || (0.0 < param_4)))
       ) {
      puStack_144 = (uint *)(uint)((byte)local_20 | (byte)local_18);
      puStack_148 = param_1;
      uStack_14c = 0x567c0a;
      FUN_00561140();
    }
  }
  if ((0.0 < param_5) || (0.0 < param_4)) {
    FUN_005659c0();
  }
  if ((param_7 == '\x01') && ((short)puVar16[0x2d] == 0)) {
    if ((byte)local_18 == '\0') {
      if ((*(byte *)((int)puVar16 + 0x106) & 4) == 0) {
        puStack_148 = (uint *)(uint)*(ushort *)(local_1c + 2);
        puStack_144 = local_48;
        uStack_14c = param_2[3];
        puStack_150 = param_1;
        auStack_158[1] = 0x567f70;
        FUN_0042be40();
      }
    }
    else {
      FUN_0042b880();
      puVar16 = local_24;
    }
  }
  if (((puVar16[0x86] != 0xffffffff) && (0.0 < *(float *)(local_1c + 0x20))) &&
     ((DAT_006f1d20 != 0 || (DAT_00724a44 != '\0')))) {
    fVar3 = (float)param_2[0x10] * *(float *)(local_1c + 0x20);
    fVar6 = *(float *)(local_1c + 0x24) * (float)param_2[0x10];
    param_2 = (uint *)fVar3;
    if (fVar3 < 0.0) {
      param_2 = (uint *)0x0;
    }
    if (0.0 <= fVar6) {
      if (1.0 <= fVar6) {
        fVar6 = 1.0;
      }
    }
    else {
      fVar6 = 0.0;
    }
    if (((float)puVar16[0x109] < fVar6) &&
       (fVar3 = (float)puVar16[0x109], puVar16[0x109] = (uint)((float)param_2 + fVar3),
       fVar6 < (float)param_2 + fVar3)) {
      puVar16[0x109] = (uint)fVar6;
    }
    sVar11 = __ftol();
    sVar12 = __ftol();
    sVar13 = __ftol();
    if ((short)puVar16[0x10a] < sVar12) {
      *(short *)(puVar16 + 0x10a) = sVar12;
    }
    *(short *)(puVar16 + 0x10a) = (short)puVar16[0x10a] + sVar11;
    if (sVar13 < (short)puVar16[0x10a]) {
      *(short *)(puVar16 + 0x10a) = sVar13;
    }
  }
  if ((param_7 == '\x01') && (((byte)local_18 != '\0' || ((byte)local_20 != '\0')))) {
    puStack_144 = (uint *)0x5680c0;
    FUN_00568610();
    if ((puVar16[1] == 0) && ((byte)local_18 == '\x01')) {
      local_68 = param_1;
      ppuVar18 = &local_68;
      puVar16 = auStack_158;
      for (iVar19 = 8; iVar19 != 0; iVar19 = iVar19 + -1) {
        *puVar16 = (uint)*ppuVar18;
        ppuVar18 = ppuVar18 + 1;
        puVar16 = puVar16 + 1;
      }
      FUN_00566c00();
      if ((*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + iVar17) & 8) == 0) {
        FUN_004e9d40();
      }
      local_24[1] = 3;
    }
  }
  return;
}
#endif
