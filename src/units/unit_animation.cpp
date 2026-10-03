#include <string.h>
#include "halo/models/api.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/lcg.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "ai.h"
#include "crt.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern uint8_t *game_state_base;
extern int32_t game_state_cursor;
extern uint32_t game_state_crc;
extern halo::units::ai_update_stagger_state *ai_update_stagger;
extern data_array *object_data;
extern int16_t unit_speech_fallback_index[];
extern int16_t unit_speech_priority_table[];
extern float unit_speech_repeat_seconds[];
extern char *unit_base_animation_state_names[6];
extern data_array *actor_data;
extern void ai_refresh_unit_stimulus_and_alert(datum_index object_index, int16_t priority, int16_t stimulus_value);
extern game_time_globals *game_time;
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern real_vector3d *global_down3d_pointer;
extern uint8_t any_local_player_within_10_units(const real_point3d *query_point);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t maximum);
extern void ai_communication_record_line_played(datum_index object_index, int16_t tier, int16_t communication_line_id, int16_t conversation_line_id);
extern void console_print_va(const char *format, ...);
extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count);
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index);
extern int16_t network_game_mode;
extern void actor_notify_weapon_pickup_once(datum_index object_index);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback);
extern void object_delete_teardown(uint32_t object_index);
extern real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out);
extern void object_set_collision_enabled(uint32_t object_index, uint8_t enable);
extern real_point3d *global_zero_vector3d_pointer;
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward, datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint);
extern void ai_communication_gate_line_played(int16_t event_id, ai_communication_record *record, datum_index object_index);
extern void ai_communication_play_event_line(datum_index object_index, int16_t event_id, uint8_t force, datum_index explicit_speaker_actor_index, uint32_t *event_record);
extern void ai_propagate_communication_reaction(datum_index object_index, ai_communication_order *order);
extern void object_solve_two_bone_ik_to_marker(uint32_t object_index, char *marker_a_name, uint32_t marker_b_object_index, char *marker_b_name, uint8_t *node_base);
}

namespace halo::units {

/**
 * object_type_definition "unit" row, +0x14 column. Carves an 8-byte block for ai_update_stagger out of the
 * game-state arena and folds its size into the running allocation CRC.
 *
 * @address 0x561fe0
 */
void unit_ai_update_stagger_allocate(void)
{
    uint8_t *block = game_state_base + game_state_cursor;
    int32_t size = 8;

    game_state_cursor = game_state_cursor + 8;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    ai_update_stagger = (ai_update_stagger_state *)block;
}

/**
 * object_type_definition "unit" row, +0x1c column. Zeroes ai_update_stagger's threshold and highest fields (a
 * single dword store in the original); claimed is left as-is.
 *
 * @address 0x562020
 */
void unit_ai_update_stagger_reset(void)
{
    ai_update_stagger->threshold = 0;
    ai_update_stagger->highest = 0;
}

/**
 * Engine function unit_animation_change_priority_check.
 *
 * @address 0x560d00
 */
int32_t UnitView::animation_change_priority_check(uint8_t follow_fallback, int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_communication_hold_tick, int16_t *dialogue_index, int32_t *chain_value)
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    int32_t chain = *chain_value;
    int16_t index = *dialogue_index;
    int16_t result = 0;

    if (chain == -1 && ((unit_object *)obj)->unit.dialogue_tag_index != k_datum_index_none && index != -1) {
        uint8_t *dialogue = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(((unit_object *)obj)->unit.dialogue_tag_index)].data;

        for (;;) {
            chain = *(int32_t *)(dialogue + index * 16 + 0x1c);
            if (!follow_fallback || chain != -1) {
                break;
            }
            index = unit_speech_fallback_index[index];
            if (index == -1) {
                break;
            }
        }
    }
    if ((!test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen) || requested_priority == 0xa) && chain != -1) {
        int16_t current = ((unit_object *)obj)->unit.current_speech.priority;

        if (current == 0) {
            result = 2;
        } else {
            int16_t pending = ((unit_object *)obj)->unit.pending_speech.priority;
            int16_t highest = (current > pending) ? current : pending;
            int16_t table;
            uint8_t allowed = 0;

            if ((requested_priority == 2 || requested_priority == 7 || requested_priority == 10) &&
                (uint8_t)((struct unit_object *)obj)->unit.speech_started != 0 && ((unit_object *)obj)->unit.speech_duration_ticks == 0 && requested_priority > highest) {
                highest = pending;
                current = 0;
            }
            table = unit_speech_priority_table[requested_priority];
            if (table >= highest) {
                result = 3;
            } else if (requested_priority >= 7 && table >= current) {
                result = 2;
            } else if (allow_repeat) {
                float interval = unit_speech_repeat_seconds[requested_priority];

                if (interval != 0.0f) {
                    if (interval == 3.4028235e+38f) {
                        allowed = 1;
                    } else {
                        allowed = (uint8_t)(((unit_object *)obj)->unit.speech_tail_ticks + ((unit_object *)obj)->unit.speech_duration_ticks <
                            (int16_t)(int32_t)(interval * 30.0f));
                    }
                    if (allowed) {
                        if (requested_priority > highest) {
                            result = 1;
                        } else if (requested_priority > ((unit_object *)obj)->unit.pending_speech.priority) {
                            if (current == 2 || current == 7 || requested_priority == 6 || allowed) {
                                result = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    *dialogue_index = index;
    *chain_value = chain;
    if (out_communication_hold_tick != 0) {
        *out_communication_hold_tick = ((struct unit_object *)obj)->unit.communication_hold_tick;
    }
    return result;
}

/**
 * Effectively a no-op stub (single return instruction) under this name; no observable behavior to confirm or
 * refute it.
 *
 * @address 0x569450
 */
void unit_animation_set_state(void)
{
    return;
}

/**
 * False for the animation states 0x17..0x1b, 0x1d and 0x22..0x23; true for every other state.
 *
 * @address 0x565d60
 */
uint8_t unit_animation_state_allows_parent_ik(uint8_t *animation_block)
{
    int32_t state_index = (int32_t)*(int8_t *)(animation_block + 0x0b) - 0x17;

    if ((uint32_t)state_index > 0x0c) {
        return 1;
    }
    return (state_index == 5 || (state_index >= 7 && state_index <= 10)) ? 1 : 0;
}

/**
 * True when the third animation overlay is unused, unknown_2a4 is clear and the current animation state is
 * not one of the states 0x10..0x13, 0x17..0x23 and 0x27..0x29.
 *
 * @address 0x565d00
 */
uint8_t unit_animation_state_allows_weapon_ik(uint8_t *animation_block)
{
    uint8_t result = *(int16_t *)(animation_block + 0x1a) == -1;
    int32_t state_index;

    if (animation_block[0x0c] != 0) {
        result = 0;
    }
    state_index = (int32_t)*(int8_t *)(animation_block + 0x0b) - 0x10;
    if ((uint32_t)state_index <= 0x19) {
        if (!((state_index >= 4 && state_index <= 6) || (state_index >= 0x14 && state_index <= 0x16))) {
            result = 0;
        }
    }
    return result;
}

/**
 * Engine function unit_animation_state_from_seat_type.
 *
 * Original register convention: in_CX -> animation_state.
 *
 * @address 0x565da0
 */
int32_t unit_animation_state_from_seat_type(int16_t animation_state)
{
    switch (animation_state) {
    case 0: case 2: case 3:
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16:
    case 0x25: case 0x26:
        return 0x19;
    case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 0xb: case 0xc: case 0xd:
    case 0xe: case 0xf:
        return 0x1a;
    default:
        return -1;
    }
}

/**
 * Engine function unit_animation_state_is_compatible.
 *
 * Original register convention: ECX -> animation_block, DX -> requested_state.
 *
 * @address 0x565be0
 */
uint8_t unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state)
{
    switch ((int8_t)animation_block[0xb]) {
    case 2:
    case 3:
    case 0x25:
    case 0x26:
        return requested_state != 0;
    case 0x17:
    case 0x1a:
    case 0x1b:
    case 0x1c:
        return 0;
    case 0x18:
    case 0x19:
        return (0x17 < requested_state) && (requested_state < 0x1a);
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x27:
    case 0x29:
        return requested_state == 0x17;
    default:
        return 1;
    }
}

/**
 * Case-insensitively matches `name` against unit_base_animation_state_names and returns the matching
 * unit_base_animation_state, or _unit_base_animation_state_none if none matches.
 *
 * @address 0x56eb90
 */
int16_t unit_base_animation_state_from_name(const char *name)
{
    int16_t index;

    for (index = 0; index < 6; index++) {
        if (_stricmp(name, unit_base_animation_state_names[index]) == 0) {
            return index;
        }
    }
    return (int16_t)_unit_base_animation_state_none;
}

/**
 * Engine function unit_choose_combat_reaction_animation.
 *
 * @address 0x561140
 */
uint8_t UnitView::choose_combat_reaction_animation(const datum_index *reaction_source, uint8_t is_scripted, uint8_t allow_second_tier, float distance_bias)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (unit->dialogue_tag_index == k_datum_index_none) {
        return 0;
    }

    float recent_damage = obj->recent_body_damage;
    int16_t source_category = 0;
    uint8_t low_damage = recent_damage < 0.6f;
    uint8_t use_second_tier = 0;
    int32_t chain = -1;
    int32_t out_communication_hold_tick = 0;
    int16_t reaction_id;
    uint8_t success = 0;

    if (reaction_source != 0 && *reaction_source != k_datum_index_none) {
        source_category = *(int16_t *)((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*reaction_source)].data + 0x1c6);
    }

    if (!is_scripted) {
        if (unit->major_hurt_speech_delay_ticks != 0) {
            return 0;
        }
        if (source_category == 1) {
            use_second_tier = 1;
            reaction_id = 9;
        } else if (low_damage) {
            if (unit->minor_hurt_speech_delay_ticks != 0) {
                return 0;
            }
            if (unit->minor_hurt_speech_count > 2) {
                return 0;
            }
            if (unit->current_speech.priority != 0 && halo::math::random_real() >= 0.4f) {
                return 0;
            }
            reaction_id = (recent_damage <= 0.0f) * 2 + 6;
            goto have_reaction_id;
        } else {
            use_second_tier = 1;
            reaction_id = 7;
        }
    } else {
        datum_index actor;
        actor = unit->swarm_actor_index;
        if (actor == k_datum_index_none) {
            actor = unit->actor_index;
        }
        uint8_t near_tag_detection;
        uint8_t past_distance_bias;
        near_tag_detection = 0;
        past_distance_bias = 0;

        if (reaction_source != 0 && *reaction_source != k_datum_index_none) {
            near_tag_detection = *(float *)((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*reaction_source)].data + 500) >= 2.0f;
        }
        if (actor == k_datum_index_none) {
            past_distance_bias = (distance_bias + 0.2f) < recent_damage;
        } else {
            past_distance_bias = *(int16_t *)((uint8_t *)actor_data->data + halo::datum_slot(actor) * 0x724 + 0x6e) > 2;
        }

        if (source_category == 1) {
            reaction_id = 0x10;
        } else if (source_category == 7) {
            reaction_id = 0x11;
        } else if (near_tag_detection) {
            reaction_id = 0x13;
        } else if (past_distance_bias) {
            reaction_id = allow_second_tier ? 0x12 : 0xf;
        } else {
            reaction_id = 0xe;
        }
        use_second_tier = 1;
        if (reaction_id != 0xe) {
            chain = 2;
            out_communication_hold_tick = (reaction_id == 0x12) ? 4 : 1;
        }
have_reaction_id:
        if (reaction_id == -1) {
            goto done;
        }
    }

    {
        int16_t priority = is_scripted ? 10 : (use_second_tier ? 7 : 2);
        int32_t out3f0 = -1;
        int32_t commit_chain = -1;
        int32_t result = UnitView(unit_index).animation_change_priority_check(1, priority, 0, 0, &reaction_id, &commit_chain);
        if (result > 0) {
            unit_speech line = {0};
            line.priority = priority;
            line.scream_type = reaction_id;
            line.sound_tag = (datum_index)commit_chain;
            line.tail_ticks = 7;
            line.unknown_10 = -1;
            line.unknown_14 = -1;
            line.ai_line_index = -1;
            line.unknown_18 = -1;

            UnitView(unit_index).commit_speech((const unit_speech *)(&line), (int16_t)result);
            success = 1;

            if (low_damage) {
                unit->minor_hurt_speech_count = unit->minor_hurt_speech_count + 1;
                unit->minor_hurt_speech_delay_ticks = 0x1e;
                unit->minor_hurt_speech_decay_ticks = 0x16;
            } else {
                unit->major_hurt_speech_delay_ticks = 0x3c;
            }
        }
    }

done:
    if (chain != -1) {
        ai_refresh_unit_stimulus_and_alert(unit_index, (int16_t)out_communication_hold_tick, (int16_t)chain);
    }
    return success;
}

/**
 * Engine function unit_dispatch_reaction_animation.
 *
 * @address 0x5614a0
 */
uint8_t UnitView::dispatch_reaction_animation(int16_t reaction_code)
{
    int32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    int16_t index;
    datum_index dialogue = ((unit_object *)obj)->unit.dialogue_tag_index;
    int32_t sound;
    int32_t result;
    unit_speech speech;

    switch (reaction_code) {
    case 0:
        index = 0xa;
        break;
    case 1:
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        index = ((float)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale < 0.5f) ? 0x27 : 0xb;
        break;
    case 2:
        index = 0xb;
        break;
    case 3:
        index = 0xc;
        break;
    case 4:
        index = 0xd;
        break;
    case 5:
        index = 0xb7;
        break;
    default:
        return 0;
    }
    if (dialogue == k_datum_index_none) {
        return 0;
    }
    sound = *(int32_t *)((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(dialogue)].data + index * 16 + 0x1c);
    if (sound == -1) {
        return 0;
    }
    result = UnitView((uint32_t)unit_index).animation_change_priority_check(1, 9, 0, 0, &index, &sound);
    if ((int16_t)result <= 0) {
        return 0;
    }
    memset(&speech, 0, sizeof(speech));
    speech.scream_type = index;
    speech.sound_tag = (datum_index)sound;
    speech.priority = 9;
    speech.tail_ticks = 7;
    speech.unknown_10 = -1;
    speech.unknown_14 = -1;
    speech.ai_line_index = -1;
    speech.unknown_18 = -1;
    UnitView((uint32_t)unit_index).commit_speech((const unit_speech *)(&speech), (int16_t)result);
    return 1;
}

/**
 * Evaluates whether a (typically vehicle-mounted) unit should flee or evade: gated on the parent vehicle's
 * Unit-tag flag 0x40 (UNSURE), the unit having an actor and not being mid scripted-action, and unknown_322
 * having climbed past 120 ticks. Rate-limited to once every 15 ticks via biped_data.unknown_4f8. Broadcasts
 * one of three AI communication lines (0x26/0x27/ 0x28) depending on whether a nearby open position was found
 * and how fast the unit is turning.
 *
 * @address 0x55e2d0
 */
void UnitView::evaluate_flee_reaction()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    object *parent = ((object_header *)object_data->data)[halo::datum_slot(obj->parent_object)].data;
    void *parent_tag = halo::cache::globals().tag_instances[halo::datum_slot(parent->definition_tag)].data;

    if (test_flag(((struct Unit *)parent_tag)->unit_flags, tags::unit_tag_flag::causes_passenger_dialogue) &&
        unit->actor_index != k_datum_index_none && unit->animation_state != 0x1d &&
        (int8_t)unit->weapon_control_idle_ticks > 0x78 && *(uint8_t *)((uint8_t *)parent + 0x4d0) > 0x1e &&
        (biped->last_falling_reaction_tick == -1 ||
         (int32_t)(biped->last_falling_reaction_tick + 0xf) < game_time->game_time)) {
        real_vector3d direction;
        real_vector3d normal;

        biped->last_falling_reaction_tick = game_time->game_time;
        if (UnitView(object_index).test_placement_candidate(global_down3d_pointer, 0, 8.0f, 0) == -1) {
            direction.i = parent->velocity.i * 60.0f;
            direction.j = parent->velocity.j * 60.0f;
            direction.k = parent->velocity.k * 60.0f - halo::physics::k_physics_gravity * 1800.0f;
            if (!(halo::math::vector3d_normalize_with_length(direction) > 0.0f) ||
                UnitView(object_index).test_placement_candidate(&direction, &normal, 8.0f, 0) == -1 ||
                !(normal.k > 0.3f)) {
                ai_communication_broadcast(0x28, object_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
                return;
            }
        }
        if (parent->up.k > 0.6f && halo::math::vector3d_length(parent->angular_velocity) < 0.05235988f) {
            ai_communication_broadcast(0x26, object_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
            return;
        }
        ai_communication_broadcast(0x27, object_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
    }
}

/**
 * REWRITTEN (objdump 0x560590..0x560628; the draft passed no point to the proximity test and no marker name).
 * EBX = the unit, [esp+4] = the trigger kind, [esp+8] = the contact point. A contact point below the biped
 * tag's count (+0x4e8) with a footsteps tag (+0x398) set, near a local player
 * (any_local_player_within_10_units, EDX = the unit's centre +0xa0), finds its marker (contact point +0x20
 * name, 0x40 each at +0x4ec;
 *
 * @address 0x560590
 */
void UnitView::fire_animation_sound_trigger(uint32_t trigger_kind, int16_t contact_point_index)
{
    uint32_t unit_index = datum_handle;
    uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(unit_index) * 0xc + 8);
    uint8_t *biped_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)unit)].data;
    object_marker marker;

    if ((int32_t)contact_point_index >= *(int32_t *)(biped_tag + 0x4e8) ||
        *(datum_index *)(biped_tag + 0x398) == k_datum_index_none) {
        return;
    }
    if (!any_local_player_within_10_units((real_point3d *)&((struct object *)unit)->bounding_center)) {
        return;
    }
    if ((int16_t)object_get_node_local_transform(unit_index,
            (char *)(*(uint8_t **)(biped_tag + 0x4ec) + contact_point_index * 0x40 + 0x20), &marker, 1) == 0) {
        return;
    }
    halo::effects::effect_marker_environment_probe(*(datum_index *)(biped_tag + 0x398), (int16_t)trigger_kind,
        (real_point3d *)((uint8_t *)&marker + 0x60), 0);
}

/**
 * Engine function unit_get_animation_frames_remaining.
 *
 * Original register convention: see file header.
 *
 * @address 0x564390
 */
int32_t UnitView::get_animation_frames_remaining(int16_t *out_animation_state)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    void *graph = halo::cache::globals().tag_instances[halo::datum_slot(obj->animation_graph)].data;
    uint8_t *animations = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer;
    ModelAnimationsAnimation *anim = (ModelAnimationsAnimation *)(animations + obj->animation_index * 0xb4);

    *out_animation_state = unit->animation_state;
    return (int32_t)anim->frame_count - (int32_t)obj->animation_frame;
}

/**
 * Returns the number of frames remaining in the unit's currently playing custom animation, or 0 if none is
 * active.
 *
 * @address 0x5701b0
 */
int32_t UnitView::get_custom_animation_time_remaining()
{
    uint32_t object_index = datum_handle;
    object *obj;
    int32_t frames_remaining;

    if (object_index == k_datum_index_none) {
        return 0;
    }

    obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (((unit_data *)((uint8_t *)obj + k_unit_data_offset))->animation_state != _unit_animation_state_custom_animation) {
        return 0;
    }

    {
        uint8_t *graph_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(obj->animation_graph)].data;
        uint8_t *anim_block = *(uint8_t **)(graph_tag + 0x78);
        int16_t frame_count = *(int16_t *)(anim_block + obj->animation_index * 0xb4 + 0x22);

        frames_remaining = (int32_t)frame_count - (int32_t)obj->animation_frame - 2;
    }

    return (frames_remaining < 1) ? 0 : frames_remaining;
}

/**
 * Returns whether the unit's current weapon/vehicle-transition mode byte is one of the reserved 'busy' state
 * values.
 *
 * Original register convention: in_ECX.
 *
 * @address 0x569c90
 */
uint8_t UnitView::is_in_busy_animation_state()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return 1;
    }
    return 0;
}

/**
 * Maps a scripted/action command enum into the corresponding unit animation-state constant and its priority
 * class.
 *
 * Original register convention: in_CX, in_EDX.
 *
 * @address 0x5692b0
 */
int32_t unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority)
{
    int32_t state = -1;
    switch (command) {
    case 0: state = 0x1d; break;
    case 1: state = 0x20; break;
    case 2: state = 0x21; break;
    case 3: state = 0x22; break;
    case 4: state = 0x1b; break;
    case 5: state = 0x1c; break;
    case 6: state = 0x1e; break;
    case 7: state = 0x1f; break;
    case 8: state = 4; break;
    case 9: state = 5; break;
    case 10: state = 6; break;
    case 0xb: state = 7; break;
    case 0xc: state = 0x28; break;
    case 0xd: state = 0x29; break;
    }
    if (out_priority != (int16_t *)0) {
        switch (command) {
        case 0: case 1: case 2: case 3: case 6: case 7: case 0xc: case 0xd:
            *out_priority = 6;
            break;
        case 4: case 5: case 8: case 9: case 10: case 0xb:
            *out_priority = 3;
            return state;
        }
    }
    return state;
}

/**
 * Engine function unit_play_default_reaction_sound.
 *
 * @address 0x561030
 */
void UnitView::play_default_reaction_sound(datum_index sound_tag, datum_index sound_handle)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t dialogue_index = -1;
    int32_t chain = (int32_t)sound_tag;
    int32_t result;
    unit_speech line = {0};

    result = UnitView(unit_index).animation_change_priority_check(0, 6, 0, 0, &dialogue_index, &chain);
    line.priority = 6;
    line.scream_type = -1;
    line.sound_tag = sound_tag;
    line.tail_ticks = 0x18;
    line.unknown_10 = -1;
    line.unknown_14 = -1;
    line.ai_line_index = -1;
    line.unknown_18 = -1;
    UnitView(unit_index).commit_speech((const unit_speech *)(&line), (int16_t)(((int16_t)result > 2) ? result : 2));

    unit->speech_sound_handle = sound_handle;
    unit->speech_started = 1;
    unit->speech_delay_ticks = 0;
    if (unit->current_speech.suppress_line_record == 0) {
        ai_communication_record_line_played(unit_index, 6, (int16_t)unit->current_speech.ai_line_index, -1);
    }
}

/**
 * object_type_definition "unit" row, +0x40 column ("region damage"). Unless the unit is already dead,
 * dispatches reaction animation 4 when damage response flag 0x200 is set, otherwise 3.
 *
 * @address 0x56f1c0
 */
void UnitView::region_damage_reaction(uint32_t unused, uint32_t flags)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    (void)unused;

    if (!test_flag(obj->vitality_flags, objects::vitality_flag::health_frozen)) {
        UnitView((int32_t)object_index).dispatch_reaction_animation((int16_t)(((flags & 0x200) != 0) + 3));
    }
}

/**
 * Checks whether the animation corresponding to a given scripted action currently exists for the unit's type.
 *
 * Original register convention: EAX -> unit_index, ECX -> command.
 *
 * @address 0x569470
 */
uint8_t UnitView::scripted_action_animation_exists(int16_t command)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;

    if (!UnitView(unit_index).is_seat_control_available(command)) {
        return 0;
    }

    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;
    uint8_t *graph = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(unit_tag->base.animation_graph.tag_id.index)].data;
    uint8_t *units_block = *(uint8_t **)&((ModelAnimations *)graph)->units.pointer;
    int8_t seat_block_index = ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->animation_definition_index;
    uint8_t *weapons_array = *(uint8_t **)(units_block + 0x5c + seat_block_index * 100);
    int8_t weapon_index = ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->animation_weapon_index;
    uint8_t *weapon_record = weapons_array + weapon_index * 0xbc;

    int16_t state_index = (int16_t)::halo::units::unit_map_action_command_to_animation_state(command, (int16_t *)0);
    if ((-1 < state_index) && (state_index < *(int32_t *)(weapon_record + 0x98))) {
        int16_t animation = *(int16_t *)(*(int32_t *)(weapon_record + 0x9c) + state_index * 2);
        return animation != -1;
    }
    return 0;
}

/**
 * Engine function unit_scripting_set_emotion_animation.
 *
 * @address 0x569cf0
 */
void UnitView::scripting_set_emotion_animation(const char *emotion_name)
{
    uint32_t unit_index = datum_handle;
    if (unit_index != k_datum_index_none) {
        object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        int16_t region = halo::models::animation_graph_find_animation_by_name(unit_index, emotion_name);
        if (region != -1) {
            unit->emotion_animation_index = region;
            return;
        }
        console_print_va("couldn't find the emotion animation '%s'", emotion_name);
    }
    return;
}

/**
 * Sets the unit's active custom animation (graph handle and animation index) and resets its playback frame to
 * zero.
 *
 * @address 0x56ebd0
 */
void UnitView::set_custom_animation(datum_index graph, int16_t animation_index)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    obj->animation_graph = graph;
    obj->animation_index = animation_index;
    obj->animation_frame = 0;
}

/**
 * Sets the current playback frame of the unit's active custom animation, if valid (i.e. if
 * unit_start_user_animation reports the animation is already active/continuing, and the frame falls within
 * the graph's animation length).
 *
 * @address 0x570220
 */
uint8_t UnitView::set_custom_animation_frame(uint8_t warn_if_missing, datum_index graph_tag_id, const char *animation_name, int16_t frame)
{
    uint32_t unit_index = datum_handle;
    object *obj;
    uint8_t *graph_tag;

    if (UnitView(unit_index).start_user_animation(graph_tag_id, animation_name, warn_if_missing) == 0) {
        return 0;
    }

    obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    if (frame < 0) {
        return 0;
    }

    graph_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(obj->animation_graph)].data;
    {
        uint8_t *anim_block = *(uint8_t **)(graph_tag + 0x78);
        int16_t frame_count = *(int16_t *)(anim_block + obj->animation_index * 0xb4 + 0x22);
        if (frame >= frame_count) {
            return 0;
        }
    }

    obj->animation_frame = frame;
    return 1;
}

/**
 * FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original
 * never reads EAX; unit_index arrive(s) on the stack (2 stack argument(s)).
 *
 * Original register convention: see file header.
 *
 * @address 0x565e00
 */
void UnitView::start_seat_overlay_animation_a(int16_t command)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (command == 0) {
        unit->replacement_animation_state = 0;
        unit->overlays[0].animation_index = -1;
        return;
    }

    Object *obj_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    void *graph = halo::cache::globals().tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)&((ModelAnimations *)graph)->units.pointer;
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);
    ModelAnimationsAnimationGraphWeapon *weapon_anim =
        (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)unit_seat->weapons.pointer +
                                                 unit->animation_weapon_index * 0xbc);
    ModelAnimationsAnimationGraphWeaponType *weapon_type =
        (ModelAnimationsAnimationGraphWeaponType *)((uint8_t *)weapon_anim->weapon_types.pointer +
                                                      unit->animation_weapon_type_index * 0x3c);

    int16_t raw_index = -1;
    uint8_t via_weapon_type = 0;

    switch (command) {
    case 1: raw_index = 0x15; break;
    case 2: raw_index = 0x16; break;
    case 3: raw_index = 0x17; break;
    case 4: raw_index = 0x18; break;
    case 5: raw_index = 0; via_weapon_type = 1; break;
    case 6: raw_index = 1; via_weapon_type = 1; break;
    case 7: raw_index = 8; via_weapon_type = 1; break;
    case 8: raw_index = 0x14; break;
    case 9: raw_index = 9; via_weapon_type = 1; break;
    default: raw_index = -2; break;
    }

    int16_t animation_index = -1;
    if (raw_index != -2) {
        if (via_weapon_type) {
            if (raw_index < (int32_t)weapon_type->animations.count) {
                animation_index = *(int16_t *)((uint8_t *)weapon_type->animations.pointer + raw_index * 2);
            }
        } else if (raw_index < (int32_t)weapon_anim->animations.count) {
            animation_index = *(int16_t *)((uint8_t *)weapon_anim->animations.pointer + raw_index * 2);
        }
    }

    if (animation_index != -1) {
        if (command != 7) {
            object_copy_default_node_transforms(unit_index, 6);
        }
        unit->overlays[0].animation_index = halo::models::animation_choose_random_permutation(
            *(datum_index *)&obj_tag->animation_graph.tag_id, animation_index, static_cast<animation_random_stream>(1));
        unit->overlays[0].frame = 0;
        unit->replacement_animation_state = (int8_t)command;
    }
}

/**
 * Engine function unit_start_seat_overlay_animation_b.
 *
 * Original register convention: see file header.
 *
 * @address 0x566410
 */
void UnitView::start_seat_overlay_animation_b(int16_t command)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (unit->overlay_animation_state > command) {
        return;
    }

    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return;
    default:
        break;
    }

    Object *obj_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    void *graph = halo::cache::globals().tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)&((ModelAnimations *)graph)->units.pointer;
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);
    ModelAnimationsAnimationGraphWeapon *weapon_anim =
        (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)unit_seat->weapons.pointer +
                                                 unit->animation_weapon_index * 0xbc);
    ModelAnimationsAnimationGraphWeaponType *weapon_type =
        (ModelAnimationsAnimationGraphWeaponType *)((uint8_t *)weapon_anim->weapon_types.pointer +
                                                      unit->animation_weapon_type_index * 0x3c);

    int16_t raw_index;
    switch (command) {
    case 1: raw_index = 4; break;
    case 2: raw_index = 5; break;
    case 3: raw_index = 6; break;
    case 4: raw_index = 7; break;
    case 5: raw_index = 2; break;
    case 6: raw_index = 3; break;
    default: return;
    }

    if (raw_index < (int32_t)weapon_type->animations.count &&
        *(int16_t *)((uint8_t *)weapon_type->animations.pointer + raw_index * 2) != -1) {
        unit->overlays[1].animation_index = halo::models::animation_choose_random_permutation(
            *(datum_index *)&obj_tag->animation_graph.tag_id,
            *(int16_t *)((uint8_t *)weapon_type->animations.pointer + raw_index * 2), static_cast<animation_random_stream>(1));
        unit->overlays[1].frame = 0;
        unit->overlay_animation_state = (int8_t)command;
    }
}

/**
 * Engine function unit_start_user_animation.
 *
 * @address 0x5702a0
 */
uint8_t UnitView::start_user_animation(datum_index graph_tag, const char *animation_name, uint8_t interpolate)
{
    uint32_t unit_index = datum_handle;
    uint8_t *unit;
    uint8_t *animations;
    uint8_t *record;
    int16_t animation;

    if (unit_index == k_datum_index_none || graph_tag == k_datum_index_none) {
        return 0;
    }
    unit = *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(unit_index) * 0xc + 8);
    animation = halo::models::animation_graph_find_animation_by_name(graph_tag, animation_name);
    if (animation == -1) {
        console_print_va("the animation '%s' doesn't exist in the graph '%s'", animation_name,
            *(char **)((uint8_t *)halo::cache::globals().tag_instances + (int16_t)graph_tag * 0x20 + 0x10));
        return 0;
    }
    animation = halo::models::animation_choose_random_permutation(graph_tag, animation, (animation_random_stream)1);
    animations = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(graph_tag)].data + 0x78);
    record = animations + animation * 0xb4;
    if (*(int16_t *)(record + 0x20) != 0) {
        return 0;
    }
    if ((uint8_t)((struct unit_object *)unit)->unit.animation_state == 0x1c && ((unit_object *)unit)->base.animation_index != -1) {
        uint8_t *current = animations + ((unit_object *)unit)->base.animation_index * 0xb4;

        if (*(int16_t *)(current + 0x42) == *(int16_t *)(record + 0x42)) {
            int16_t frame_count = *(int16_t *)(current + 0x34);
            uint16_t frame = *(uint16_t *)&((unit_object *)unit)->base.animation_frame;

            if ((int32_t)(int16_t)frame + 2 == (int32_t)frame_count) {
                *(uint16_t *)&((unit_object *)unit)->base.animation_frame = (uint16_t)(frame - 1);
                return 0;
            }
            if ((int16_t)frame < frame_count) {
                return 0;
            }
        }
    }
    if (interpolate) {
        object_copy_default_node_transforms(unit_index, 6);
    }
    ((struct unit_object *)unit)->unit.animation_state = 0x1c;
    UnitView(unit_index).set_custom_animation(graph_tag, animation);
    set_flag(((struct unit_object *)unit)->unit.animation_state_flags, units::unit_animation_state_flag::action_active);
    object_recalculate_bounding_radius_recursive(unit_index);
    return 1;
}

/**
 * Engine function unit_state_is_scripted_animation.
 *
 * Original register convention: in_ECX -> unit.
 *
 * @address 0x565c60
 */
uint8_t unit_state_is_scripted_animation(unit_data *unit)
{
    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b:
    case 0x1d: case 0x1e: case 0x1f: case 0x20: case 0x21: case 0x22: case 0x23:
    case 0x27: case 0x29:
        return 1;
    default:
        return 0;
    }
}

namespace unit_try_set_animation_state_local {

static const uint8_t state_animations[0x2c][2] = {
    { 1, 0x00 }, { 1, 0x01 }, { 1, 0x02 }, { 1, 0x03 }, { 1, 0x08 }, { 1, 0x09 }, { 1, 0x0a }, { 1, 0x0b },
    { 1, 0x23 }, { 1, 0x24 }, { 1, 0x25 }, { 1, 0x26 }, { 1, 0x0c }, { 1, 0x0d }, { 1, 0x0e }, { 1, 0x0f },
    { 2, 0x17 }, { 2, 0x18 }, { 2, 0x19 }, { 2, 0x1a }, { 1, 0x10 }, { 1, 0x11 }, { 1, 0x12 }, { 0, 0 },
    { 2, 0x00 }, { 2, 0x01 }, { 0, 0 },    { 0, 0 },    { 0, 0 },    { 0, 0 },    { 1, 0x27 }, { 1, 0x2a },
    { 1, 0x2e }, { 1, 0x14 }, { 1, 0x2c }, { 1, 0x2d }, { 1, 0x2f }, { 2, 0x1b }, { 2, 0x1c }, { 1, 0x30 },
    { 1, 0x31 }, { 1, 0x32 }, { 0, 0 },    { 2, 0x1d },
};

static int16_t block_animation(const uint8_t *block, int32_t count_offset, int32_t slot)
{
    if (slot < *(int32_t *)(block + count_offset)) {
        return (*(int16_t **)(block + count_offset + 4))[slot];
    }
    return -1;
}

}

/**
 * Engine function unit_try_set_animation_state.
 *
 * @address 0x565f90
 */
uint8_t UnitView::try_set_animation_state(int16_t new_state)
{
    using namespace unit_try_set_animation_state_local;
    uint32_t unit_index = datum_handle;
    uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(unit_index) * 0xc + 8);
    uint8_t *unit_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)unit)].data;
    datum_index graph = *(datum_index *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id;
    uint8_t *graph_data = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(graph)].data;
    uint8_t *unit_block = *(uint8_t **)(graph_data + 0x10) + (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_definition_index * 0x64;
    uint8_t *weapon_block = *(uint8_t **)(unit_block + 0x5c) + (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_weapon_index * 0xbc;
    uint8_t no_state = (uint8_t)((uint8_t)((struct unit_object *)unit)->unit.animation_state == 0xff);
    uint8_t changed = 0;
    int16_t current_state = 0;
    int16_t transform_count = 0;
    int16_t seat_type;
    int16_t count;

    if (no_state || (current_state = (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state) != new_state) {
        int16_t animation = -1;

        if ((uint8_t)((struct unit_object *)unit)->unit.animation_state == 0x21) {
            UnitView(unit_index).release_thrown_grenade(1);
        }
        if ((uint16_t)new_state < 0x2c && state_animations[new_state][0] == 1) {
            animation = block_animation(weapon_block, 0x98, state_animations[new_state][1]);
        } else if ((uint16_t)new_state < 0x2c && state_animations[new_state][0] == 2) {
            animation = block_animation(unit_block, 0x40, state_animations[new_state][1]);
        }
        if (animation == -1) {
            switch (new_state) {
            case 0x1e: case 0x1f: case 0x20: case 0x21: case 0x27: case 0x29:
                return 0;
            default:
                break;
            }
        }
        animation = halo::models::animation_choose_random_permutation(graph, animation, static_cast<animation_random_stream>(1));
        {
            uint8_t *reloaded = *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(unit_index) * 0xc + 8);

            ((struct object *)reloaded)->animation_graph = graph;
            ((struct object *)reloaded)->animation_index = animation;
            ((struct object *)reloaded)->animation_frame = 0;
        }
        current_state = (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state;
        transform_count = 6;
        if ((new_state == 0 || new_state == 2 || new_state == 3) &&
            (current_state == 0 || current_state == 2 || current_state == 3)) {
            transform_count = 1;
        }
        if (new_state == 0x16 || new_state == 0x15) {
            transform_count = 2;
        }
        changed = 1;
    }

    seat_type = (int16_t)::halo::units::unit_animation_state_from_seat_type(new_state);
    if (no_state || seat_type != (int16_t)::halo::units::unit_animation_state_from_seat_type(current_state)) {
        int16_t overlay = -1;

        if (seat_type >= 0 && seat_type < *(int32_t *)(weapon_block + 0x98)) {
            overlay = (*(int16_t **)(weapon_block + 0x9c))[seat_type];
        }
        ((unit_object *)unit)->unit.aiming_animation_index = halo::models::animation_choose_random_permutation(graph, overlay, static_cast<animation_random_stream>(1));
        count = 6;
        if (no_state) {
            int16_t idle = (*(int32_t *)(unit_block + 0x40) > 9) ? (*(int16_t **)(unit_block + 0x44))[9] : -1;

            ((struct unit_object *)unit)->unit.looking_animation_index = halo::models::animation_choose_random_permutation(graph, idle, static_cast<animation_random_stream>(1));
        }
        object_copy_default_node_transforms(unit_index, count);
    } else if (changed) {
        object_copy_default_node_transforms(unit_index, transform_count);
    }
    ((struct unit_object *)unit)->unit.animation_state = (uint8_t)new_state;
    return 1;
}

/**
 * REWRITTEN from objdump 0x569530..0x56966e. Stack: (unit, command, direction). When the unit may act and its
 * current animation set (graph +0x10 units, +0x5c weapons, 0xbc each;
 *
 * @address 0x569530
 */
uint8_t UnitView::try_start_scripted_action_animation(int16_t command, const real_vector2d *direction)
{
    uint32_t unit_index = datum_handle;
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *unit_tag;
    uint8_t *weapon_record;
    int16_t priority;
    int16_t state_index;
    int16_t first_animation;
    int16_t animation;
    uint8_t *object;

    if (!UnitView(unit_index).is_seat_control_available(command)) {
        return 0;
    }
    unit_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)unit)].data;
    {
        uint8_t *graph = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id & 0xffff].data;
        uint8_t *units_block = *(uint8_t **)&((ModelAnimations *)graph)->units.pointer;
        uint8_t *weapons = *(uint8_t **)(units_block + (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_definition_index * 0x64 + 0x5c);

        weapon_record = weapons + (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_weapon_index * 0xbc;
    }
    state_index = (int16_t)::halo::units::unit_map_action_command_to_animation_state(command, &priority);
    if (state_index < 0 || (int32_t)state_index >= *(int32_t *)(weapon_record + 0x98)) {
        return 0;
    }
    first_animation = (*(int16_t **)(weapon_record + 0x9c))[state_index];
    if (first_animation == -1) {
        return 0;
    }
    object_copy_default_node_transforms(unit_index, priority);
    animation = halo::models::animation_choose_random_permutation(*(datum_index *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id, first_animation, (animation_random_stream)1);
    object = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    ((struct object *)object)->animation_graph = *(datum_index *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id;
    ((struct object *)object)->animation_index = animation;
    ((struct object *)object)->animation_frame = 0;
    set_flag(((struct unit_object *)unit)->unit.animation_state_flags, units::unit_animation_state_flag::action_active);
    ((struct unit_object *)unit)->unit.animation_state = 0x1d;
    if (direction != 0 && ((unit_object *)unit)->base.type == 0 && ((unit_object *)unit)->base.parent_object == k_datum_index_none) {
        UnitView(unit_index).set_throw_aim_direction(direction);
    }
    return 1;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[halo::datum_slot((h))].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot((t))].data)
/**
 * Engine function unit_try_start_seat_exit_animation.
 *
 * @address 0x56c470
 */
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
        UnitView(unit_index).detach_from_seat(1, force_flag, 1);
        return 0;
    }
    if (::halo::units::unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
        return 0;
    }
    self_tag = TAG_DATA(*(datum_index *)self);
    graph = *(datum_index *)&((struct Unit *)self_tag)->base.animation_graph.tag_id;
    seat_block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)(uint8_t)((struct unit_object *)self)->unit.animation_definition_index * 0x64;
    if (!(*(int32_t *)(seat_block + 0x40) > 8) || (exit_animation = (*(int16_t **)(seat_block + 0x44))[8]) == -1) {
        return 0;
    }
    if (((struct unit_object *)OBJECT_DATA(vehicle_index))->unit.driver_unit_index == unit_index) {
        UnitView((int32_t)vehicle_index).notify_weapon_removed();
    }
    UnitView(unit_index).set_custom_animation(*(datum_index *)&((struct Unit *)self_tag)->base.animation_graph.tag_id, halo::models::animation_choose_random_permutation(graph, exit_animation, (animation_random_stream)1));
    object = OBJECT_DATA(unit_index);
    object_tag = TAG_DATA(*(datum_index *)object);
    if (*(int32_t *)&((struct Unit *)object_tag)->base.model.tag_id != -1) {
        if (test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
            object_for_each_light_attachment(unit_index, 0, 1);
        }
        if (*(int32_t *)&((struct Unit *)object_tag)->base.model.tag_id != -1) {
            clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
            ((object_header *)object_data->data)[halo::datum_slot(unit_index)].flags |= 2;
        }
    }
    ((struct unit_object *)self)->unit.animation_state = 0x1b;
    actor_notify_weapon_pickup_once(unit_index);
    if (((unit_object *)self)->base.network_role == 0) {
        ::halo::units::unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
    }
    return 1;
}
#undef OBJECT_DATA
#undef TAG_DATA

namespace unit_update_animation_state_machine_local {

static uint8_t *state_machine_object(uint32_t object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(object_index) * 0xc + 8);
}

}

/**
 * Engine function unit_update_animation_state_machine.
 *
 * @address 0x565420
 */
uint16_t UnitView::update_animation_state_machine(const int8_t *request)
{
    using namespace unit_update_animation_state_machine_local;
    uint32_t unit_index = datum_handle;
    uint8_t *unit = state_machine_object(unit_index);
    uint8_t *unit_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)unit)].data;
    int16_t requested = request[0];
    uint16_t result = 0;
    uint8_t force = 0;
    uint16_t advance;

    if (((unit_object *)unit)->base.parent_object == k_datum_index_none && !test_flag(((struct object *)unit)->vitality_flags, objects::vitality_flag::health_frozen)) {
        int16_t base_state = -1;

        switch ((int8_t)(uint8_t)((struct unit_object *)unit)->unit.seat_command) {
        case 0: base_state = 0; break;
        case 1: case 2: base_state = 1; break;
        case 3: base_state = (int16_t)(2 + (request[1] != 0)); break;
        case 4: base_state = 2; break;
        case 5: base_state = 4; break;
        case 6: base_state = 5; break;
        default: break;
        }
        if ((uint8_t)((struct unit_object *)unit)->unit.scripted_base_animation_state != 0xff) {
            base_state = (int8_t)(uint8_t)((struct unit_object *)unit)->unit.scripted_base_animation_state;
        }
        if (((unit_object *)unit)->unit.control_flags & 0x200) {
            base_state = 1;
        }
        if ((uint8_t)((struct unit_object *)unit)->unit.flaming_ticks != 0) {
            base_state = 5;
        }
        if ((int8_t)(uint8_t)((struct unit_object *)unit)->unit.base_animation_state != base_state && ::halo::units::unit_animation_state_is_compatible(unit + 0x298, requested)) {
            char *weapon_label = UnitView(unit_index).get_current_weapon_label();

            UnitView(unit_index).set_or_test_seat_and_weapon_label(unit_base_animation_state_names[base_state], weapon_label, 1);
        }
    }

    if (((struct unit_object *)unit)->unit.overlays[2].animation_index != -1 &&
        ::halo::units::unit_reset_light_effect((animation_state *)&((struct unit_object *)unit)->unit.overlays[2], *(uint32_t *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id, unit_index) == 2) {
        ((struct unit_object *)unit)->unit.overlays[2].animation_index = -1;
    }

    if (((unit_object *)unit)->base.animation_index != -1) {
        advance = ::halo::units::unit_reset_light_effect((animation_state *)&((struct object *)unit)->animation_index, *(uint32_t *)&((unit_object *)unit)->base.animation_graph, unit_index);
        if (advance == 1) {
            switch ((int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state) {
            case 0x1e: case 0x1f: case 0x29:
                UnitView(unit_index).cause_melee_damage(0, k_datum_index_none, -1, -1, -1, 0);
                break;
            case 0x21:
                UnitView(unit_index).release_thrown_grenade(0);
                break;
            default:
                break;
            }
        } else if (advance == 2) {
            switch ((int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state) {
            case 0x19: {
                uint8_t delete_now = 0;

                if ((uint8_t)((struct Unit *)unit_tag)->unit_flags & 2) {
                    if ((uint8_t)((struct object *)unit)->flags & 0x20) {
                        delete_now = 1;
                    } else if (((unit_object *)unit)->base.type == 0) {
                        uint8_t *biped = state_machine_object(unit_index);
                        uint8_t *biped_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)biped)].data;

                        if (!(biped[0x4cc] & 1) || (*(uint32_t *)(biped_tag + 0x2f4) & 0x400)) {
                            delete_now = 1;
                        }
                    }
                }
                if (delete_now) {
                    object_delete_teardown(unit_index);
                    UnitView(unit_index).pick_random_spawned_actor_count();
                    break;
                }
                if (((unit_object *)unit)->base.type == 0) {
                    UnitView(unit_index).reset_ground_adjust_state();
                }
                set_flag(((struct unit_object *)unit)->unit.animation_state_flags, units::unit_animation_state_flag::unknown_4);
                ((unit_object *)unit)->base.animation_frame -= 1;
                break;
            }
            case 0x1a: {
                datum_index parent_index = ((unit_object *)unit)->base.parent_object;
                uint8_t *parent = state_machine_object(parent_index);
                uint8_t *parent_tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)parent)].data;
                uint8_t seat_flags = *((uint8_t *)((struct Unit *)parent_tag)->seats.pointer + ((unit_object *)unit)->unit.vehicle_seat_index * 0x11c);

                object_set_collision_enabled(unit_index, (uint8_t)(~seat_flags & 1));
                if (((struct unit_object *)parent)->unit.driver_unit_index == unit_index) {
                    ::unit_notify_weapon_removed_dup((int32_t)((unit_object *)unit)->base.parent_object);
                }
                break;
            }
            case 0x1b: {
                uint8_t *animations =
                    *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(((unit_object *)unit)->base.animation_graph)].data + 0x78);
                void *model = halo::cache::globals().tag_instances[*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id & 0xffff].data;
                real_vector3d delta;
                real_matrix4x3 world;
                real_matrix4x3 *matrix;

                halo::models::model_animation_get_frame_delta(((unit_object *)unit)->base.animation_frame,
                    reinterpret_cast<ModelAnimationsAnimation *>(animations + ((unit_object *)unit)->base.animation_index * 0xb4), &delta, reinterpret_cast<GBXModel *>(model));
                matrix = object_get_world_matrix(unit_index, &world);
                halo::math::matrix4x3_transform_vector(delta, delta, *matrix);
                UnitView(unit_index).detach_from_seat(1, 1, 1);
                ((unit_object *)unit)->base.velocity.i = delta.i + ((unit_object *)unit)->base.velocity.i;
                ((unit_object *)unit)->base.velocity.j = delta.j + ((unit_object *)unit)->base.velocity.j;
                ((unit_object *)unit)->base.velocity.k = delta.k + ((unit_object *)unit)->base.velocity.k;
                break;
            }
            case 0x25: case 0x26:
                ((unit_object *)unit)->base.animation_frame -= 1;
                break;
            case 0x27:
                result = 1;
                requested = 0x28;
                break;
            default:
                break;
            }
            if (!::halo::units::unit_state_allows_control(unit + 0x298)) {
                force = 1;
            }
        }
    }

    if (((struct unit_object *)unit)->unit.overlays[0].animation_index != -1 &&
        ::halo::units::unit_reset_light_effect((animation_state *)&((struct unit_object *)unit)->unit.overlays, *(uint32_t *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id, unit_index) == 2) {
        uint8_t *reloaded;

        object_copy_default_node_transforms(unit_index, 6);
        reloaded = state_machine_object(unit_index);
        reloaded[0x2a4] = 0;
        *(int16_t *)(reloaded + 0x2aa) = -1;
    }

    if (((struct unit_object *)unit)->unit.overlays[1].animation_index != -1) {
        advance = ::halo::units::unit_reset_light_effect((animation_state *)&((struct unit_object *)unit)->unit.overlays[1], *(uint32_t *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id, unit_index);
        if ((advance == 2 || advance == 4) && ((int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state < 3 || (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state > 4)) {
            ((struct unit_object *)unit)->unit.overlay_animation_state = 0;
            ((struct unit_object *)unit)->unit.overlays[1].animation_index = -1;
        }
    }

    if (force || (requested != (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state && ::halo::units::unit_animation_state_is_compatible(unit + 0x298, requested))) {
        UnitView(unit_index).try_set_animation_state(requested);
    }
    return result;
}

namespace unit_update_animation_timers_local {

static void count_down(uint8_t *field)
{
    int16_t value = *(int16_t *)field;

    if (value > 0) {
        *(int16_t *)field = (int16_t)(value - 1);
    }
}

}

/**
 * Engine function unit_update_animation_timers.
 *
 * @address 0x561620
 */
void UnitView::update_animation_timers()
{
    using namespace unit_update_animation_timers_local;
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;

    if ((uint8_t)(((struct unit_object *)obj)->unit.flags >> 8) & 0x1) {
        UnitView(unit_index).choose_dialogue_variant();
        clear_flag(((unit_object *)obj)->unit.flags, units::unit_flag::permutation_dirty);
    }
    if (((struct unit_object *)obj)->unit.minor_hurt_speech_decay_ticks > 0) {
        int16_t value = (int16_t)(((struct unit_object *)obj)->unit.minor_hurt_speech_decay_ticks - 1);

        ((struct unit_object *)obj)->unit.minor_hurt_speech_decay_ticks = value;
        if (value == 0 && ((struct unit_object *)obj)->unit.minor_hurt_speech_count > 0) {
            ((struct unit_object *)obj)->unit.minor_hurt_speech_count = (int16_t)(((struct unit_object *)obj)->unit.minor_hurt_speech_count - 1);
            ((struct unit_object *)obj)->unit.minor_hurt_speech_decay_ticks = 0x16;
        }
    }
    count_down(obj + 0x3ec);
    count_down(obj + 0x3ec);
    if (((unit_object *)obj)->unit.current_speech.priority > 0) {
        if (((unit_object *)obj)->unit.speech_delay_ticks > 0) {
            ((unit_object *)obj)->unit.speech_delay_ticks = (int16_t)(((unit_object *)obj)->unit.speech_delay_ticks - 1);
            goto tail;
        }
        if ((uint8_t)((struct unit_object *)obj)->unit.speech_started == 0) {
            object_marker marker;
            Point3D position;
            Vector3D forward;
            int16_t node = 0;

            if ((int16_t)object_get_node_local_transform(unit_index, (char *)"head", &marker, 1) != 0) {
                uint8_t *raw = (uint8_t *)&marker;

                position = *(Point3D *)(raw + 0x2c);
                forward = *(Vector3D *)(raw + 0x8);
                node = *(int16_t *)raw;
            } else {
                position = *(Point3D *)global_zero_vector3d_pointer;
                forward = *(Vector3D *)halo::math::globals().global_forward3d_pointer;
            }
            if (((unit_object *)obj)->unit.current_speech.sound_tag != k_datum_index_none) {
                ((unit_object *)obj)->unit.speech_sound_handle = halo::sound::sound_start_at_object_marker(unit_index, &position, &forward,
                    ((unit_object *)obj)->unit.current_speech.sound_tag, node, 1.0f, 0);
            }
            ai_communication_gate_line_played(((unit_object *)obj)->unit.current_speech.priority, (ai_communication_record *)&((struct unit_object *)obj)->unit.current_speech.unknown_10,
                unit_index);
            ((struct unit_object *)obj)->unit.speech_started = 1;
        }
        count_down(obj + 0x3fc);
        if (((unit_object *)obj)->unit.speech_duration_ticks > 0) {
            int16_t value = (int16_t)(((unit_object *)obj)->unit.speech_duration_ticks - 1);

            ((unit_object *)obj)->unit.speech_duration_ticks = value;
            if (value == 0) {
                ((unit_object *)obj)->unit.speech_sound_handle = k_datum_index_none;
            }
            goto tail;
        }
        if ((uint8_t)((struct unit_object *)obj)->unit.speech_finished == 0) {
            ai_communication_play_event_line(unit_index, (int16_t)*(uint16_t *)&((unit_object *)obj)->unit.current_speech.scream_type, 0, k_datum_index_none,
                (uint32_t *)&((struct unit_object *)obj)->unit.current_speech.unknown_10);
            ((struct unit_object *)obj)->unit.speech_finished = 1;
        }
        count_down(obj + 0x3fe);
        if (((unit_object *)obj)->unit.speech_tail_ticks == 0) {
            ((unit_object *)obj)->unit.speech_lipsync_ticks = 0;
        }
    }
tail:
    if (((unit_object *)obj)->unit.speech_lipsync_ticks == 0 && (uint8_t)((struct unit_object *)obj)->unit.speech_lipsync_stopped == 0) {
        ai_propagate_communication_reaction(unit_index, (ai_communication_order *)&((struct unit_object *)obj)->unit.current_speech.unknown_10);
        ((struct unit_object *)obj)->unit.speech_lipsync_stopped = 1;
    }
    if (((unit_object *)obj)->unit.current_speech.priority > 0 && ((unit_object *)obj)->unit.speech_duration_ticks == 0 && ((unit_object *)obj)->unit.speech_tail_ticks == 0) {
        ((unit_object *)obj)->unit.current_speech.priority = 0;
    }
    if (((unit_object *)obj)->unit.current_speech.priority == 0 && ((unit_object *)obj)->unit.pending_speech.priority > 0) {
        UnitView(unit_index).commit_speech((const unit_speech *)(obj + 0x3b8), 3);
    }
}

/**
 * Engine function unit_update_footstep_and_idle_triggers.
 *
 * Original register convention: in_EAX -> unit_index.
 *
 * @address 0x560410
 */
void UnitView::update_footstep_and_idle_triggers()
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    int32_t is_turning = 0;
    int32_t is_moving_fast = 0;

    switch (unit->animation_state) {
    case 2:
    case 3:
        is_turning = 1;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        if (0.25f < unit->throttle.k * unit->throttle.k + unit->throttle.j * unit->throttle.j +
                        unit->throttle.i * unit->throttle.i) {
            is_moving_fast = 1;
        }
        break;
    default:
        break;
    }

    if (obj->animation_index != -1) {
        void *graph = halo::cache::globals().tag_instances[halo::datum_slot(obj->animation_graph)].data;
        uint8_t *animations = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer;
        ModelAnimationsAnimation *anim = (ModelAnimationsAnimation *)(animations + obj->animation_index * 0xb4);

        if (is_turning) {
            if (obj->animation_frame == 0) {
                UnitView(unit_index).fire_animation_sound_trigger(3, 0);
                UnitView(unit_index).fire_animation_sound_trigger(3, 1);
            }
        } else if (is_moving_fast &&
                   (anim->left_foot_frame_index != 0 || anim->right_foot_frame_index != 0)) {
            int16_t foot;
            if ((uint16_t)obj->animation_frame == (uint16_t)(uint8_t)anim->left_foot_frame_index) {
                foot = 0;
            } else if ((uint16_t)obj->animation_frame != (uint16_t)(uint8_t)anim->right_foot_frame_index) {
                goto idle_timeout;
            } else {
                foot = 1;
            }
            UnitView(unit_index).fire_animation_sound_trigger(unit->base_animation_state == 2, foot);
        }
    }

idle_timeout:
    if (biped->movement_state == 0) {
        if (biped->stop_moving_ticks < 1) {
            return;
        }
        biped->stop_moving_ticks = biped->stop_moving_ticks + 1;
        if (biped->stop_moving_ticks < 4) {
            return;
        }
        UnitView(unit_index).fire_animation_sound_trigger(3, 0);
        UnitView(unit_index).fire_animation_sound_trigger(3, 1);
        biped->stop_moving_ticks = 0;
    } else if (biped->movement_state == 1) {
        biped->stop_moving_ticks = 1;
        return;
    } else {
        biped->stop_moving_ticks = 0;
    }
}

/**
 * object_type_definition "unit" row, +0x4c column. Solves the unit's own IK chains (from its animation-graph
 * "unit block" record) and, when it is holding a weapon, that weapon's IK chains (from the graph's "weapon
 * block" record), then clears the "weapon IK dirty" flag. FIXED (register inputs, objdump): the original
 * never reads ECX as an input (it overwrites or only saves it); those parameters arrive on the stack (2 stack
 * argument(s) read).
 *
 * @address 0x5643f0
 */
void UnitView::update_ik_detail_nodes(void *node_base)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Unit *tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (test_flag(tag->unit_flags, tags::unit_tag_flag::simple_creature) || unit->animation_definition_index == -1) {
        return;
    }

    {
        tag_instance *graph = &halo::cache::globals().tag_instances[*(int32_t *)&tag->base.animation_graph.tag_id & 0xffff];
        uint8_t *unit_block = *(uint8_t **)((uint8_t *)graph->data + 0x10);
        uint8_t *unit_record = unit_block + (int32_t)unit->animation_definition_index * 100;
        uint8_t *weapon_block = *(uint8_t **)(unit_record + 0x5c);
        uint8_t *weapon_record = weapon_block + (int32_t)unit->animation_weapon_index * 0xbc;

        if (obj->parent_object != k_datum_index_none && ::halo::units::unit_animation_state_allows_parent_ik((uint8_t *)&unit->animation_state_flags) != 0) {
            int32_t count = *(int32_t *)(unit_record + 0x4c);
            uint8_t *table = *(uint8_t **)(unit_record + 0x50);
            int32_t i;
            for (i = 0; i < count; i++) {
                uint8_t *entry = table + i * 0x40;
                object_solve_two_bone_ik_to_marker(object_index, (char *)entry,
                    obj->parent_object, (char *)(entry + 0x20), (uint8_t *)node_base);
            }
        }

        if (unit->current_weapon_index != -1 && ::halo::units::unit_animation_state_allows_weapon_ik((uint8_t *)&unit->animation_state_flags) != 0) {
            int32_t count = *(int32_t *)(weapon_record + 0xa4);
            uint8_t *table = *(uint8_t **)(weapon_record + 0xa8);
            int32_t i;
            for (i = 0; i < count; i++) {
                object *fresh_obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
                unit_data *fresh_unit = (unit_data *)((uint8_t *)fresh_obj + k_unit_data_offset);
                uint8_t *entry = table + i * 0x40;
                int16_t current_weapon = fresh_unit->current_weapon_index;
                datum_index weapon_handle = k_datum_index_none;
                if (current_weapon != -1) {
                    weapon_handle = fresh_unit->weapons[current_weapon];
                }
                object_solve_two_bone_ik_to_marker(object_index, (char *)entry,
                    weapon_handle, (char *)(entry + 0x20), (uint8_t *)node_base);
            }
            clear_flag(unit->animation_state_flags, units::unit_animation_state_flag::action_active);
        }
    }
}

}
