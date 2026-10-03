#include "halo/networking/game_mode.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/units/records.hpp"
#include "halo/objects/record_access.hpp"
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
#include "halo/main/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/models/models.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/units/vars.hpp"

static auto &ai_update_stagger = halo::link::ref<halo::units::ai_update_stagger_state *>(halo::units::vars().ai_update_stagger);
static auto &unit_speech_fallback_index = halo::link::ref<int16_t []>(halo::units::vars().unit_speech_fallback_index);
static auto &unit_speech_priority_table = halo::link::ref<int16_t []>(halo::units::vars().unit_speech_priority_table);
static auto &unit_speech_repeat_seconds = halo::link::ref<float []>(halo::units::vars().unit_speech_repeat_seconds);
static auto &unit_base_animation_state_names = halo::link::ref<char *[6]>(halo::units::vars().unit_base_animation_state_names);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &global_down3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_down3d_pointer);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);

namespace {

ModelAnimationsAnimation &graph_animation(uint32_t graph_tag, int32_t index)
{
    return halo::objects::block_element<ModelAnimationsAnimation>(halo::objects::tag_as<ModelAnimations>(graph_tag)->animations, index);
}

ModelAnimationsAnimationGraphUnitSeat &graph_unit_seat(uint32_t graph_tag, int8_t index)
{
    return halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(halo::objects::tag_as<ModelAnimations>(graph_tag)->units, index);
}

ModelAnimationsAnimationGraphWeapon &seat_weapon(const ModelAnimationsAnimationGraphUnitSeat &seat, int8_t index)
{
    return halo::objects::block_element<ModelAnimationsAnimationGraphWeapon>(seat.weapons, index);
}

int16_t animation_in_block(const TagReflexive &block, int32_t slot)
{
    if (slot < static_cast<int32_t>(block.count)) {
        return halo::objects::block_element<int16_t>(block, slot);
    }
    return -1;
}

}

namespace halo::units {

/**
 * object_type_definition "unit" row, +0x14 column. Carves an 8-byte block for ai_update_stagger out of the
 * game-state arena and folds its size into the running allocation CRC.
 *
 * @address 0x561fe0
 */
void halo::units::unit_ai_update_stagger_allocate(void)
{
    uint8_t *block = halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor;
    int32_t size = 8;

    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 8;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    ai_update_stagger = (ai_update_stagger_state *)block;
}

/**
 * object_type_definition "unit" row, +0x1c column. Zeroes ai_update_stagger's threshold and highest fields (a
 * single dword store in the original); claimed is left as-is.
 *
 * @address 0x562020
 */
void halo::units::unit_ai_update_stagger_reset(void)
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
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    int32_t chain = *chain_value;
    int16_t index = *dialogue_index;
    int16_t result = 0;

    if (chain == -1 && obj->unit.dialogue_tag_index != k_datum_index_none && index != -1) {
        uint8_t *dialogue = halo::objects::tag_record_bytes(obj->unit.dialogue_tag_index);

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
    if ((!test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen) || requested_priority == 0xa) && chain != -1) {
        int16_t current = obj->unit.current_speech.priority;

        if (current == 0) {
            result = 2;
        } else {
            int16_t pending = obj->unit.pending_speech.priority;
            int16_t highest = (current > pending) ? current : pending;
            int16_t table;
            uint8_t allowed = 0;

            if ((requested_priority == 2 || requested_priority == 7 || requested_priority == 10) &&
                (uint8_t)obj->unit.speech_started != 0 && obj->unit.speech_duration_ticks == 0 && requested_priority > highest) {
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
                        allowed = (uint8_t)(obj->unit.speech_tail_ticks + obj->unit.speech_duration_ticks <
                            (int16_t)(int32_t)(interval * 30.0f));
                    }
                    if (allowed) {
                        if (requested_priority > highest) {
                            result = 1;
                        } else if (requested_priority > obj->unit.pending_speech.priority) {
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
        *out_communication_hold_tick = obj->unit.communication_hold_tick;
    }
    return result;
}

/**
 * Effectively a no-op stub (single return instruction) under this name; no observable behavior to confirm or
 * refute it.
 *
 * @address 0x569450
 */
void halo::units::unit_animation_set_state(void)
{
    return;
}

/**
 * False for the animation states 0x17..0x1b, 0x1d and 0x22..0x23; true for every other state.
 *
 * @address 0x565d60
 */
uint8_t halo::units::unit_animation_state_allows_parent_ik(uint8_t *animation_block)
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
uint8_t halo::units::unit_animation_state_allows_weapon_ik(uint8_t *animation_block)
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
int32_t halo::units::unit_animation_state_from_seat_type(int16_t animation_state)
{
    switch (animation_state_id(animation_state)) {
    case unit_animation_state_id::idle:
    case unit_animation_state_id::turn_in_place_a:
    case unit_animation_state_id::turn_in_place_b:
    case unit_animation_state_id::unknown_10:
    case unit_animation_state_id::unknown_11:
    case unit_animation_state_id::unknown_12:
    case unit_animation_state_id::unknown_13:
    case unit_animation_state_id::unknown_14:
    case unit_animation_state_id::soft_landing:
    case unit_animation_state_id::hard_landing:
    case unit_animation_state_id::unknown_25:
    case unit_animation_state_id::unknown_26:
        return animation_state_value(unit_animation_state_id::ready_weapon);
    case unit_animation_state_id::move_front:
    case unit_animation_state_id::move_back:
    case unit_animation_state_id::move_left:
    case unit_animation_state_id::move_right:
    case unit_animation_state_id::hurt_move_front:
    case unit_animation_state_id::hurt_move_back:
    case unit_animation_state_id::hurt_move_left:
    case unit_animation_state_id::hurt_move_right:
    case unit_animation_state_id::unknown_0c:
    case unit_animation_state_id::unknown_0d:
    case unit_animation_state_id::unknown_0e:
    case unit_animation_state_id::unknown_0f:
        return animation_state_value(unit_animation_state_id::seat_enter);
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
uint8_t halo::units::unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state)
{
    switch (animation_state_id((int8_t)animation_block[0xb])) {
    case unit_animation_state_id::turn_in_place_a:
    case unit_animation_state_id::turn_in_place_b:
    case unit_animation_state_id::unknown_25:
    case unit_animation_state_id::unknown_26:
        return requested_state != 0;
    case unit_animation_state_id::unknown_17:
    case unit_animation_state_id::seat_enter:
    case unit_animation_state_id::seat_exit:
    case unit_animation_state_id::custom_animation:
        return 0;
    case unit_animation_state_id::unknown_18:
    case unit_animation_state_id::ready_weapon:
        return (animation_state_value(unit_animation_state_id::unknown_17) < requested_state) && (requested_state < animation_state_value(unit_animation_state_id::seat_enter));
    case unit_animation_state_id::scripted_action:
    case unit_animation_state_id::unknown_1e:
    case unit_animation_state_id::unknown_1f:
    case unit_animation_state_id::throwing_grenade:
    case unit_animation_state_id::unknown_22:
    case unit_animation_state_id::unknown_23:
    case unit_animation_state_id::unknown_27:
    case unit_animation_state_id::unknown_29:
        return requested_state == animation_state_value(unit_animation_state_id::unknown_17);
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
int16_t halo::units::unit_base_animation_state_from_name(const char *name)
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

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
        source_category = *(int16_t *)(halo::objects::tag_record_bytes(*reaction_source) + 0x1c6);
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
            near_tag_detection = *(float *)(halo::objects::tag_record_bytes(*reaction_source) + 500) >= 2.0f;
        }
        if (actor == k_datum_index_none) {
            past_distance_bias = (distance_bias + 0.2f) < recent_damage;
        } else {
            past_distance_bias = *(int16_t *)((uint8_t *)halo::ai::globals().actor_data->data + halo::datum_slot(actor) * 0x724 + 0x6e) > 2;
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
    }

    if (reaction_id != -1) {
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

    if (chain != -1) {
        halo::ai::ai_refresh_unit_stimulus_and_alert(unit_index, (int16_t)out_communication_hold_tick, (int16_t)chain);
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
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    int16_t index;
    datum_index dialogue = obj->unit.dialogue_tag_index;
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
    sound = *(int32_t *)(halo::objects::tag_record_bytes(dialogue) + index * 16 + 0x1c);
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
 * Unit-tag flag 0x40 (causes_passenger_dialogue), the unit having an actor and not being mid scripted-action, and unknown_322
 * having climbed past 120 ticks. Rate-limited to once every 15 ticks via biped_data.unknown_4f8. Broadcasts
 * one of three AI communication lines (0x26/0x27/ 0x28) depending on whether a nearby open position was found
 * and how fast the unit is turning.
 *
 * @address 0x55e2d0
 */
void UnitView::evaluate_flee_reaction()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);
    object *parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(obj->parent_object)].data;
    void *parent_tag = halo::cache::globals().tag_instances[halo::datum_slot(parent->definition_tag)].data;

    if (test_flag(((struct Unit *)parent_tag)->unit_flags, tags::unit_tag_flag::causes_passenger_dialogue) &&
        unit->actor_index != k_datum_index_none && unit->animation_state != animation_state_value(unit_animation_state_id::scripted_action) &&
        (int8_t)unit->weapon_control_idle_ticks > 0x78 && *(uint8_t *)((uint8_t *)parent + 0x4d0) > 0x1e &&
        (biped->last_falling_reaction_tick == -1 ||
         (int32_t)(biped->last_falling_reaction_tick + 0xf) < halo::game::globals().game_time->game_time)) {
        real_vector3d direction;
        real_vector3d normal;

        biped->last_falling_reaction_tick = halo::game::globals().game_time->game_time;
        if (UnitView(object_index).test_placement_candidate(global_down3d_pointer, 0, 8.0f, 0) == -1) {
            direction.i = parent->velocity.i * 60.0f;
            direction.j = parent->velocity.j * 60.0f;
            direction.k = parent->velocity.k * 60.0f - halo::physics::k_physics_gravity * 1800.0f;
            if (!(halo::math::vector3d_normalize_with_length(direction) > 0.0f) ||
                UnitView(object_index).test_placement_candidate(&direction, &normal, 8.0f, 0) == -1 ||
                !(normal.k > 0.3f)) {
                halo::ai::ai_communication_broadcast(0x28, object_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
                return;
            }
        }
        if (parent->up.k > 0.6f && halo::math::vector3d_length(parent->angular_velocity) < 0.05235988f) {
            halo::ai::ai_communication_broadcast(0x26, object_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
            return;
        }
        halo::ai::ai_communication_broadcast(0x27, object_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
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
    object *unit = reinterpret_cast<object *>(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(unit_index) * 0xc + 8));
    Biped *biped_tag = halo::objects::tag_as<Biped>(*(datum_index *)unit);
    object_marker marker;

    if ((int32_t)contact_point_index >= biped_tag->contact_point.count ||
        halo::objects::tag_handle(biped_tag->footsteps) == k_datum_index_none) {
        return;
    }
    if (!halo::game::any_local_player_within_10_units((real_point3d *)&unit->bounding_center)) {
        return;
    }
    if ((int16_t)halo::objects::object_get_node_local_transform(unit_index,
            halo::objects::block_element<BipedContactPoint>(biped_tag->contact_point, contact_point_index).marker_name.string, &marker, 1) == 0) {
        return;
    }
    halo::effects::effect_marker_environment_probe(halo::objects::tag_handle(biped_tag->footsteps), (int16_t)trigger_kind,
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    ModelAnimationsAnimation *anim = &graph_animation(obj->animation_graph, obj->animation_index);

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

    obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;

    if ((halo::units::unit_data_of(obj))->animation_state != _unit_animation_state_custom_animation) {
        return 0;
    }

    {
        int16_t frame_count = graph_animation(obj->animation_graph, obj->animation_index).frame_count;

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
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    return is_scripted_animation_state(animation_state_id(unit->animation_state)) ? 1 : 0;
}

/**
 * Maps a scripted/action command enum into the corresponding unit animation-state constant and its priority
 * class.
 *
 * Original register convention: in_CX, in_EDX.
 *
 * @address 0x5692b0
 */
int32_t halo::units::unit_map_action_command_to_animation_state(int16_t command, int16_t *out_priority)
{
    int32_t state = -1;
    switch (command) {
    case 0: state = animation_state_value(unit_animation_state_id::scripted_action); break;
    case 1: state = animation_state_value(unit_animation_state_id::unknown_20); break;
    case 2: state = animation_state_value(unit_animation_state_id::throwing_grenade); break;
    case 3: state = animation_state_value(unit_animation_state_id::unknown_22); break;
    case 4: state = animation_state_value(unit_animation_state_id::seat_exit); break;
    case 5: state = animation_state_value(unit_animation_state_id::custom_animation); break;
    case 6: state = animation_state_value(unit_animation_state_id::unknown_1e); break;
    case 7: state = animation_state_value(unit_animation_state_id::unknown_1f); break;
    case 8: state = animation_state_value(unit_animation_state_id::move_front); break;
    case 9: state = animation_state_value(unit_animation_state_id::move_back); break;
    case 10: state = animation_state_value(unit_animation_state_id::move_left); break;
    case 0xb: state = animation_state_value(unit_animation_state_id::move_right); break;
    case 0xc: state = animation_state_value(unit_animation_state_id::unknown_28); break;
    case 0xd: state = animation_state_value(unit_animation_state_id::unknown_29); break;
    }
    if (out_priority != nullptr) {
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
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
        halo::ai::ai_communication_record_line_played(unit_index, 6, (int16_t)unit->current_speech.ai_line_index, -1);
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
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
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;

    if (!UnitView(unit_index).is_seat_control_available(command)) {
        return 0;
    }

    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;
    ModelAnimations *graph = halo::objects::tag_as<ModelAnimations>(unit_tag->base.animation_graph.tag_id.index);
    ModelAnimationsAnimationGraphUnitSeat *units_block = halo::objects::block_elements<ModelAnimationsAnimationGraphUnitSeat>(graph->units);
    int8_t seat_block_index = (halo::units::unit_data_of(unit_obj))->animation_definition_index;
    int8_t weapon_index = (halo::units::unit_data_of(unit_obj))->animation_weapon_index;
    ModelAnimationsAnimationGraphWeapon &weapon_record = seat_weapon(units_block[seat_block_index], weapon_index);

    int16_t state_index = (int16_t)::halo::units::unit_map_action_command_to_animation_state(command, nullptr);
    if ((-1 < state_index) && (state_index < static_cast<int32_t>(weapon_record.animations.count))) {
        return halo::objects::block_element<int16_t>(weapon_record.animations, state_index) != -1;
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
        object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
        unit_data *unit = halo::units::unit_data_of(unit_obj);
        int16_t region = halo::models::animation_graph::find_animation_by_name(unit_index, emotion_name);
        if (region != -1) {
            unit->emotion_animation_index = region;
            return;
        }
        halo::main::console_print_va("couldn't find the emotion animation '%s'", emotion_name);
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;

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

    if (UnitView(unit_index).start_user_animation(graph_tag_id, animation_name, warn_if_missing) == 0) {
        return 0;
    }

    obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    if (frame < 0) {
        return 0;
    }

    if (frame >= static_cast<int16_t>(graph_animation(obj->animation_graph, obj->animation_index).frame_count)) {
        return 0;
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    if (command == 0) {
        unit->replacement_animation_state = 0;
        unit->overlays[0].animation_index = -1;
        return;
    }

    Object *obj_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    ModelAnimationsAnimationGraphWeapon *weapon_anim = &seat_weapon(
        graph_unit_seat(obj_tag->animation_graph.tag_id.index, unit->animation_definition_index), unit->animation_weapon_index);
    ModelAnimationsAnimationGraphWeaponType *weapon_type =
        (ModelAnimationsAnimationGraphWeaponType *)(&halo::objects::block_element<ModelAnimationsAnimationGraphWeaponType>(weapon_anim->weapon_types, unit->animation_weapon_type_index));

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
                animation_index = halo::objects::block_element<int16_t>(weapon_type->animations, raw_index);
            }
        } else if (raw_index < (int32_t)weapon_anim->animations.count) {
            animation_index = halo::objects::block_element<int16_t>(weapon_anim->animations, raw_index);
        }
    }

    if (animation_index != -1) {
        if (command != 7) {
            halo::objects::object_copy_default_node_transforms(unit_index, 6);
        }
        unit->overlays[0].animation_index = halo::models::animation_choose_random_permutation(
            halo::objects::tag_handle(obj_tag->animation_graph), animation_index, static_cast<animation_random_stream>(1));
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    if (unit->overlay_animation_state > command) {
        return;
    }

    if (is_scripted_animation_state(animation_state_id(unit->animation_state))) {
        return;
    }

    Object *obj_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    ModelAnimationsAnimationGraphWeapon *weapon_anim = &seat_weapon(
        graph_unit_seat(obj_tag->animation_graph.tag_id.index, unit->animation_definition_index), unit->animation_weapon_index);
    ModelAnimationsAnimationGraphWeaponType *weapon_type =
        (ModelAnimationsAnimationGraphWeaponType *)(&halo::objects::block_element<ModelAnimationsAnimationGraphWeaponType>(weapon_anim->weapon_types, unit->animation_weapon_type_index));

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

    if (animation_in_block(weapon_type->animations, raw_index) != -1) {
        unit->overlays[1].animation_index = halo::models::animation_choose_random_permutation(
            halo::objects::tag_handle(obj_tag->animation_graph),
            halo::objects::block_element<int16_t>(weapon_type->animations, raw_index), static_cast<animation_random_stream>(1));
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
    unit_object *unit;
    int16_t animation;

    if (unit_index == k_datum_index_none || graph_tag == k_datum_index_none) {
        return 0;
    }
    unit = reinterpret_cast<unit_object *>(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(unit_index) * 0xc + 8));
    animation = halo::models::animation_graph::find_animation_by_name(graph_tag, animation_name);
    if (animation == -1) {
        halo::main::console_print_va("the animation '%s' doesn't exist in the graph '%s'", animation_name,
            *(char **)((uint8_t *)halo::cache::globals().tag_instances + (int16_t)graph_tag * 0x20 + 0x10));
        return 0;
    }
    animation = halo::models::animation_choose_random_permutation(graph_tag, animation, (animation_random_stream)1);
    const ModelAnimationsAnimation &record = graph_animation(graph_tag, animation);
    if (record.type != 0) {
        return 0;
    }
    if ((uint8_t)unit->unit.animation_state == animation_state_value(unit_animation_state_id::custom_animation) && unit->base.animation_index != -1) {
        const ModelAnimationsAnimation &current = graph_animation(graph_tag, unit->base.animation_index);

        if (current.main_animation_index == record.main_animation_index) {
            int16_t frame_count = static_cast<int16_t>(current.key_frame_index);
            uint16_t frame = *(uint16_t *)&unit->base.animation_frame;

            if ((int32_t)(int16_t)frame + 2 == (int32_t)frame_count) {
                *(uint16_t *)&unit->base.animation_frame = (uint16_t)(frame - 1);
                return 0;
            }
            if ((int16_t)frame < frame_count) {
                return 0;
            }
        }
    }
    if (interpolate) {
        halo::objects::object_copy_default_node_transforms(unit_index, 6);
    }
    unit->unit.animation_state = animation_state_value(unit_animation_state_id::custom_animation);
    UnitView(unit_index).set_custom_animation(graph_tag, animation);
    set_flag(unit->unit.animation_state_flags, units::unit_animation_state_flag::action_active);
    halo::objects::object_recalculate_bounding_radius_recursive(unit_index);
    return 1;
}

/**
 * Engine function unit_state_is_scripted_animation.
 *
 * Original register convention: in_ECX -> unit.
 *
 * @address 0x565c60
 */
uint8_t halo::units::unit_state_is_scripted_animation(unit_data *unit)
{
    if (is_scripted_animation_state(animation_state_id(unit->animation_state))) {
        return 1;
    }
    return 0;
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
    unit_object *unit = reinterpret_cast<unit_object *>(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(unit_index) * 0xc + 8));
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)unit);
    datum_index graph = halo::objects::tag_handle(unit_tag->base.animation_graph);
    const ModelAnimationsAnimationGraphUnitSeat &unit_block = graph_unit_seat(graph, (int8_t)(uint8_t)unit->unit.animation_definition_index);
    const ModelAnimationsAnimationGraphWeapon &weapon_block = seat_weapon(unit_block, (int8_t)(uint8_t)unit->unit.animation_weapon_index);
    uint8_t no_state = (uint8_t)((uint8_t)unit->unit.animation_state == 0xff);
    uint8_t changed = 0;
    int16_t current_state = 0;
    int16_t transform_count = 0;
    int16_t seat_type;
    int16_t count;

    if (no_state || (current_state = (int8_t)(uint8_t)unit->unit.animation_state) != new_state) {
        int16_t animation = -1;

        if ((uint8_t)unit->unit.animation_state == animation_state_value(unit_animation_state_id::throwing_grenade)) {
            UnitView(unit_index).release_thrown_grenade(1);
        }
        if ((uint16_t)new_state < 0x2c && state_animations[new_state][0] == 1) {
            animation = animation_in_block(weapon_block.animations, state_animations[new_state][1]);
        } else if ((uint16_t)new_state < 0x2c && state_animations[new_state][0] == 2) {
            animation = animation_in_block(unit_block.animations, state_animations[new_state][1]);
        }
        if (animation == -1) {
            switch (animation_state_id(new_state)) {
            case unit_animation_state_id::unknown_1e:
            case unit_animation_state_id::unknown_1f:
            case unit_animation_state_id::unknown_20:
            case unit_animation_state_id::throwing_grenade:
            case unit_animation_state_id::unknown_27:
            case unit_animation_state_id::unknown_29:
                return 0;
            default:
                break;
            }
        }
        animation = halo::models::animation_choose_random_permutation(graph, animation, static_cast<animation_random_stream>(1));
        {
            object *reloaded = reinterpret_cast<object *>(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(unit_index) * 0xc + 8));

            reloaded->animation_graph = graph;
            reloaded->animation_index = animation;
            reloaded->animation_frame = 0;
        }
        current_state = (int8_t)(uint8_t)unit->unit.animation_state;
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

        if (seat_type >= 0) {
            overlay = animation_in_block(weapon_block.animations, seat_type);
        }
        unit->unit.aiming_animation_index = halo::models::animation_choose_random_permutation(graph, overlay, static_cast<animation_random_stream>(1));
        count = 6;
        if (no_state) {
            int16_t idle = animation_in_block(unit_block.animations, 9);

            unit->unit.looking_animation_index = halo::models::animation_choose_random_permutation(graph, idle, static_cast<animation_random_stream>(1));
        }
        halo::objects::object_copy_default_node_transforms(unit_index, count);
    } else if (changed) {
        halo::objects::object_copy_default_node_transforms(unit_index, transform_count);
    }
    unit->unit.animation_state = (uint8_t)new_state;
    return 1;
}

/**
 * REWRITTEN from objdump 0x569530..0x56966e. Stack: (unit, command, direction). When the unit may act and its
 * current animation set (reinterpret_cast<uint8_t *>(graph) +0x10 units, +0x5c weapons, 0xbc each;
 *
 * @address 0x569530
 */
uint8_t UnitView::try_start_scripted_action_animation(int16_t command, const real_vector2d *direction)
{
    uint32_t unit_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    Unit *unit_tag;
    int16_t priority;
    int16_t state_index;
    int16_t first_animation;
    int16_t animation;
    uint8_t *object;

    if (!UnitView(unit_index).is_seat_control_available(command)) {
        return 0;
    }
    unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)unit);
    const ModelAnimationsAnimationGraphWeapon &weapon_record = seat_weapon(
        graph_unit_seat(halo::objects::tag_handle(unit_tag->base.animation_graph), (int8_t)(uint8_t)unit->unit.animation_definition_index),
        (int8_t)(uint8_t)unit->unit.animation_weapon_index);
    state_index = (int16_t)::halo::units::unit_map_action_command_to_animation_state(command, &priority);
    if (state_index < 0) {
        return 0;
    }
    first_animation = animation_in_block(weapon_record.animations, state_index);
    if (first_animation == -1) {
        return 0;
    }
    halo::objects::object_copy_default_node_transforms(unit_index, priority);
    animation = halo::models::animation_choose_random_permutation(halo::objects::tag_handle(unit_tag->base.animation_graph), first_animation, (animation_random_stream)1);
    object = halo::objects::object_record_bytes(unit_index);
    ((struct object *)object)->animation_graph = halo::objects::tag_handle(unit_tag->base.animation_graph);
    ((struct object *)object)->animation_index = animation;
    ((struct object *)object)->animation_frame = 0;
    set_flag(unit->unit.animation_state_flags, units::unit_animation_state_flag::action_active);
    unit->unit.animation_state = animation_state_value(unit_animation_state_id::scripted_action);
    if (direction != 0 && unit->base.type == _object_type_biped && unit->base.parent_object == k_datum_index_none) {
        UnitView(unit_index).set_throw_aim_direction(direction);
    }
    return 1;
}

/**
 * Engine function unit_try_start_seat_exit_animation.
 *
 * @address 0x56c470
 */
uint8_t halo::units::unit_try_start_seat_exit_animation(uint8_t force_flag, uint32_t unit_index)
{
    unit_object *self = reinterpret_cast<unit_object *>(halo::objects::object_try_and_get(unit_index, 3));
    datum_index vehicle_index;
    Unit *self_tag;
    datum_index graph;
    int16_t exit_animation;
    uint8_t *object;
    Unit *object_tag;

    if (self == 0) {
        return 0;
    }
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client && force_flag != 1) {
        return 0;
    }
    vehicle_index = self->base.parent_object;
    if (vehicle_index == k_datum_index_none || self->unit.vehicle_seat_index == -1) {
        return 0;
    }
    if (self->base.type == _object_type_vehicle) {
        UnitView(unit_index).detach_from_seat(1, force_flag, 1);
        return 0;
    }
    if (::halo::units::unit_state_is_scripted_animation((unit_data *)(reinterpret_cast<uint8_t *>(self) + k_unit_data_offset))) {
        return 0;
    }
    self_tag = halo::objects::tag_as<Unit>(*(datum_index *)self);
    graph = halo::objects::tag_handle(self_tag->base.animation_graph);
    exit_animation = animation_in_block(graph_unit_seat(graph, (int8_t)(uint8_t)self->unit.animation_definition_index).animations, 8);
    if (exit_animation == -1) {
        return 0;
    }
    if (((struct unit_object *)halo::objects::object_record_bytes(vehicle_index))->unit.driver_unit_index == unit_index) {
        UnitView((int32_t)vehicle_index).notify_weapon_removed();
    }
    UnitView(unit_index).set_custom_animation(halo::objects::tag_handle(self_tag->base.animation_graph), halo::models::animation_choose_random_permutation(graph, exit_animation, (animation_random_stream)1));
    object = halo::objects::object_record_bytes(unit_index);
    object_tag = halo::objects::tag_as<Unit>(*(datum_index *)object);
    if ((int32_t)halo::objects::tag_handle(object_tag->base.model) != -1) {
        if (test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
            halo::objects::object_for_each_light_attachment(unit_index, 0, 1);
        }
        if ((int32_t)halo::objects::tag_handle(object_tag->base.model) != -1) {
            clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
            ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].flags |= 2;
        }
    }
    self->unit.animation_state = animation_state_value(unit_animation_state_id::seat_exit);
    halo::ai::actor_notify_weapon_pickup_once(unit_index);
    if (self->base.network_role == 0) {
        ::halo::units::unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
    }
    return 1;
}

namespace unit_update_animation_state_machine_local {

static uint8_t *state_machine_object(uint32_t object_index)
{
    return *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(object_index) * 0xc + 8);
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
    unit_object *unit = reinterpret_cast<unit_object *>(state_machine_object(unit_index));
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)unit);
    int16_t requested = request[0];
    uint16_t result = 0;
    uint8_t force = 0;
    uint16_t advance;

    if (unit->base.parent_object == k_datum_index_none && !test_flag(unit->base.vitality_flags, objects::vitality_flag::health_frozen)) {
        int16_t base_state = -1;

        switch ((int8_t)(uint8_t)unit->unit.seat_command) {
        case 0: base_state = 0; break;
        case 1: case 2: base_state = 1; break;
        case 3: base_state = (int16_t)(2 + (request[1] != 0)); break;
        case 4: base_state = 2; break;
        case 5: base_state = 4; break;
        case 6: base_state = 5; break;
        default: break;
        }
        if ((uint8_t)unit->unit.scripted_base_animation_state != 0xff) {
            base_state = (int8_t)(uint8_t)unit->unit.scripted_base_animation_state;
        }
        if ((unit->unit.control_flags & 0x200)) {
            base_state = 1;
        }
        if ((uint8_t)unit->unit.flaming_ticks != 0) {
            base_state = 5;
        }
        if ((int8_t)(uint8_t)unit->unit.base_animation_state != base_state && ::halo::units::unit_animation_state_is_compatible(reinterpret_cast<uint8_t *>(unit) + 0x298, requested)) {
            char *weapon_label = UnitView(unit_index).get_current_weapon_label();

            UnitView(unit_index).set_or_test_seat_and_weapon_label(unit_base_animation_state_names[base_state], weapon_label, 1);
        }
    }

    if (unit->unit.overlays[2].animation_index != -1 &&
        ::halo::units::unit_reset_light_effect((animation_state *)&unit->unit.overlays[2], halo::objects::tag_handle(unit_tag->base.animation_graph), unit_index) == 2) {
        unit->unit.overlays[2].animation_index = -1;
    }

    if (unit->base.animation_index != -1) {
        advance = ::halo::units::unit_reset_light_effect((animation_state *)&unit->base.animation_index, unit->base.animation_graph, unit_index);
        if (advance == 1) {
            switch (animation_state_id((int8_t)(uint8_t)unit->unit.animation_state)) {
            case unit_animation_state_id::unknown_1e:
            case unit_animation_state_id::unknown_1f:
            case unit_animation_state_id::unknown_29:
                UnitView(unit_index).cause_melee_damage(0, k_datum_index_none, -1, -1, -1, 0);
                break;
            case unit_animation_state_id::throwing_grenade:
                UnitView(unit_index).release_thrown_grenade(0);
                break;
            default:
                break;
            }
        } else if (advance == 2) {
            switch ((int8_t)(uint8_t)unit->unit.animation_state) {
            case 0x19: {
                uint8_t delete_now = 0;

                if ((uint8_t)unit_tag->unit_flags & 2) {
                    if ((uint8_t)unit->base.flags & 0x20) {
                        delete_now = 1;
                    } else if (unit->base.type == _object_type_biped) {
                        uint8_t *biped = state_machine_object(unit_index);
                        Biped *biped_tag = halo::objects::tag_as<Biped>(*(datum_index *)biped);

                        if (!(biped[0x4cc] & 1) || test_flag(biped_tag->biped_flags, tags::biped_tag_flag::has_no_dying_airborne)) {
                            delete_now = 1;
                        }
                    }
                }
                if (delete_now) {
                    halo::objects::object_delete_teardown(unit_index);
                    UnitView(unit_index).pick_random_spawned_actor_count();
                    break;
                }
                if (unit->base.type == _object_type_biped) {
                    UnitView(unit_index).reset_ground_adjust_state();
                }
                set_flag(unit->unit.animation_state_flags, units::unit_animation_state_flag::unknown_4);
                unit->base.animation_frame -= 1;
                break;
            }
            case 0x1a: {
                datum_index parent_index = unit->base.parent_object;
                unit_object *parent = reinterpret_cast<unit_object *>(state_machine_object(parent_index));
                Unit *parent_tag = halo::objects::tag_as<Unit>(*(datum_index *)parent);
                uint8_t seat_flags = (uint8_t)halo::objects::block_element<UnitSeat>(parent_tag->seats, unit->unit.vehicle_seat_index).flags;

                halo::objects::object_set_collision_enabled(unit_index, (uint8_t)(~seat_flags & 1));
                if (parent->unit.driver_unit_index == unit_index) {
                    halo::units::UnitView((int32_t)unit->base.parent_object).notify_weapon_removed_dup();
                }
                break;
            }
            case 0x1b: {
                void *model = halo::cache::globals().tag_instances[halo::objects::tag_handle(unit_tag->base.model) & 0xffff].data;
                real_vector3d delta;
                real_matrix4x3 world;
                real_matrix4x3 *matrix;

                halo::models::animation_graph::get_frame_delta(unit->base.animation_frame, &graph_animation(unit->base.animation_graph, unit->base.animation_index), &delta, reinterpret_cast<GBXModel *>(model));
                matrix = halo::objects::object_get_world_matrix(unit_index, &world);
                halo::math::matrix4x3_transform_vector(delta, delta, *matrix);
                UnitView(unit_index).detach_from_seat(1, 1, 1);
                unit->base.velocity.i = delta.i + unit->base.velocity.i;
                unit->base.velocity.j = delta.j + unit->base.velocity.j;
                unit->base.velocity.k = delta.k + unit->base.velocity.k;
                break;
            }
            case 0x25: case 0x26:
                unit->base.animation_frame -= 1;
                break;
            case 0x27:
                result = 1;
                requested = 0x28;
                break;
            default:
                break;
            }
            if (!::halo::units::unit_state_allows_control(reinterpret_cast<uint8_t *>(unit) + 0x298)) {
                force = 1;
            }
        }
    }

    if (unit->unit.overlays[0].animation_index != -1 &&
        ::halo::units::unit_reset_light_effect((animation_state *)&unit->unit.overlays, halo::objects::tag_handle(unit_tag->base.animation_graph), unit_index) == 2) {
        uint8_t *reloaded;

        halo::objects::object_copy_default_node_transforms(unit_index, 6);
        reloaded = state_machine_object(unit_index);
        reloaded[0x2a4] = 0;
        *(int16_t *)(reloaded + 0x2aa) = -1;
    }

    if (unit->unit.overlays[1].animation_index != -1) {
        advance = ::halo::units::unit_reset_light_effect((animation_state *)&unit->unit.overlays[1], halo::objects::tag_handle(unit_tag->base.animation_graph), unit_index);
        if ((advance == 2 || advance == 4) && ((int8_t)(uint8_t)unit->unit.animation_state < 3 || (int8_t)(uint8_t)unit->unit.animation_state > 4)) {
            unit->unit.overlay_animation_state = 0;
            unit->unit.overlays[1].animation_index = -1;
        }
    }

    if (force || (requested != (int8_t)(uint8_t)unit->unit.animation_state && ::halo::units::unit_animation_state_is_compatible(reinterpret_cast<uint8_t *>(unit) + 0x298, requested))) {
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
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));

    if ((uint8_t)(obj->unit.flags >> 8) & 0x1) {
        UnitView(unit_index).choose_dialogue_variant();
        clear_flag(obj->unit.flags, units::unit_flag::permutation_dirty);
    }
    if (obj->unit.minor_hurt_speech_decay_ticks > 0) {
        int16_t value = (int16_t)(obj->unit.minor_hurt_speech_decay_ticks - 1);

        obj->unit.minor_hurt_speech_decay_ticks = value;
        if (value == 0 && obj->unit.minor_hurt_speech_count > 0) {
            obj->unit.minor_hurt_speech_count = (int16_t)(obj->unit.minor_hurt_speech_count - 1);
            obj->unit.minor_hurt_speech_decay_ticks = 0x16;
        }
    }
    count_down(reinterpret_cast<uint8_t *>(obj) + 0x3ec);
    count_down(reinterpret_cast<uint8_t *>(obj) + 0x3ec);
    if (obj->unit.current_speech.priority > 0) {
        if (obj->unit.speech_delay_ticks > 0) {
            obj->unit.speech_delay_ticks = (int16_t)(obj->unit.speech_delay_ticks - 1);
        } else {
            if ((uint8_t)obj->unit.speech_started == 0) {
                object_marker marker;
                Point3D position;
                Vector3D forward;
                int16_t node = 0;

                if ((int16_t)halo::objects::object_get_node_local_transform(unit_index, (char *)"head", &marker, 1) != 0) {
                    position = *(Point3D *)&marker.transform.position;
                    forward = *(Vector3D *)&marker.transform.forward;
                    node = marker.node_index;
                } else {
                    position = *(Point3D *)global_zero_vector3d_pointer;
                    forward = *(Vector3D *)halo::math::globals().global_forward3d_pointer;
                }
                if (obj->unit.current_speech.sound_tag != k_datum_index_none) {
                    obj->unit.speech_sound_handle = halo::sound::sound_start_at_object_marker(unit_index, &position, &forward,
                        obj->unit.current_speech.sound_tag, node, 1.0f, 0);
                }
                halo::ai::ai_communication_gate_line_played(obj->unit.current_speech.priority, (ai_communication_record *)&obj->unit.current_speech.unknown_10,
                    unit_index);
                obj->unit.speech_started = 1;
            }
            count_down(reinterpret_cast<uint8_t *>(obj) + 0x3fc);
            if (obj->unit.speech_duration_ticks > 0) {
                int16_t value = (int16_t)(obj->unit.speech_duration_ticks - 1);

                obj->unit.speech_duration_ticks = value;
                if (value == 0) {
                    obj->unit.speech_sound_handle = k_datum_index_none;
                }
            } else {
                if ((uint8_t)obj->unit.speech_finished == 0) {
                    halo::ai::ai_communication_play_event_line(unit_index, (int16_t)*(uint16_t *)&obj->unit.current_speech.scream_type, 0, k_datum_index_none,
                        (uint32_t *)&obj->unit.current_speech.unknown_10);
                    obj->unit.speech_finished = 1;
                }
                count_down(reinterpret_cast<uint8_t *>(obj) + 0x3fe);
                if (obj->unit.speech_tail_ticks == 0) {
                    obj->unit.speech_lipsync_ticks = 0;
                }
            }
        }
    }
    if (obj->unit.speech_lipsync_ticks == 0 && (uint8_t)obj->unit.speech_lipsync_stopped == 0) {
        halo::ai::ai_propagate_communication_reaction(unit_index, (ai_communication_order *)&obj->unit.current_speech.unknown_10);
        obj->unit.speech_lipsync_stopped = 1;
    }
    if (obj->unit.current_speech.priority > 0 && obj->unit.speech_duration_ticks == 0 && obj->unit.speech_tail_ticks == 0) {
        obj->unit.current_speech.priority = 0;
    }
    if (obj->unit.current_speech.priority == 0 && obj->unit.pending_speech.priority > 0) {
        UnitView(unit_index).commit_speech((const unit_speech *)(reinterpret_cast<uint8_t *>(obj) + 0x3b8), 3);
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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);
    int32_t is_turning = 0;
    int32_t is_moving_fast = 0;

    switch (animation_state_id(unit->animation_state)) {
    case unit_animation_state_id::turn_in_place_a:
    case unit_animation_state_id::turn_in_place_b:
        is_turning = 1;
        break;
    case unit_animation_state_id::move_front:
    case unit_animation_state_id::move_back:
    case unit_animation_state_id::move_left:
    case unit_animation_state_id::move_right:
        if (0.25f < unit->throttle.k * unit->throttle.k + unit->throttle.j * unit->throttle.j +
                        unit->throttle.i * unit->throttle.i) {
            is_moving_fast = 1;
        }
        break;
    default:
        break;
    }

    if (obj->animation_index != -1) {
        ModelAnimationsAnimation *anim = &graph_animation(obj->animation_graph, obj->animation_index);

        if (is_turning) {
            if (obj->animation_frame == 0) {
                UnitView(unit_index).fire_animation_sound_trigger(3, 0);
                UnitView(unit_index).fire_animation_sound_trigger(3, 1);
            }
        } else if (is_moving_fast &&
                   (anim->left_foot_frame_index != 0 || anim->right_foot_frame_index != 0)) {
            if ((uint16_t)obj->animation_frame == (uint16_t)(uint8_t)anim->left_foot_frame_index) {
                UnitView(unit_index).fire_animation_sound_trigger(unit->base_animation_state == _unit_base_animation_state_stand, 0);
            } else if ((uint16_t)obj->animation_frame == (uint16_t)(uint8_t)anim->right_foot_frame_index) {
                UnitView(unit_index).fire_animation_sound_trigger(unit->base_animation_state == _unit_base_animation_state_stand, 1);
            }
        }
    }

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
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Unit *tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    if (test_flag(tag->unit_flags, tags::unit_tag_flag::simple_creature) || unit->animation_definition_index == -1) {
        return;
    }

    {
        const ModelAnimationsAnimationGraphUnitSeat &unit_record = graph_unit_seat(halo::objects::tag_handle(tag->base.animation_graph), unit->animation_definition_index);
        const ModelAnimationsAnimationGraphWeapon &weapon_record = seat_weapon(unit_record, unit->animation_weapon_index);

        if (obj->parent_object != k_datum_index_none && ::halo::units::unit_animation_state_allows_parent_ik((uint8_t *)&unit->animation_state_flags) != 0) {
            int32_t count = static_cast<int32_t>(unit_record.ik_points.count);
            int32_t i;
            for (i = 0; i < count; i++) {
                ModelAnimationsAnimationGraphUnitSeatikPoint &entry = halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeatikPoint>(unit_record.ik_points, i);
                halo::objects::object_solve_two_bone_ik_to_marker(object_index, entry.marker.string,
                    obj->parent_object, entry.attach_to_marker.string, (uint8_t *)node_base);
            }
        }

        if (unit->current_weapon_index != -1 && ::halo::units::unit_animation_state_allows_weapon_ik((uint8_t *)&unit->animation_state_flags) != 0) {
            int32_t count = static_cast<int32_t>(weapon_record.ik_point.count);
            int32_t i;
            for (i = 0; i < count; i++) {
                object *fresh_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
                unit_data *fresh_unit = halo::units::unit_data_of(fresh_obj);
                ModelAnimationsAnimationGraphUnitSeatikPoint &entry = halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeatikPoint>(weapon_record.ik_point, i);
                int16_t current_weapon = fresh_unit->current_weapon_index;
                datum_index weapon_handle = k_datum_index_none;
                if (current_weapon != -1) {
                    weapon_handle = fresh_unit->weapons[current_weapon];
                }
                halo::objects::object_solve_two_bone_ik_to_marker(object_index, entry.marker.string,
                    weapon_handle, entry.attach_to_marker.string, (uint8_t *)node_base);
            }
            clear_flag(unit->animation_state_flags, units::unit_animation_state_flag::action_active);
        }
    }
}

}
