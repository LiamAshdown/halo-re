#include "halo/objects/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/hs/script_globals.hpp"
#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"

namespace halo::ai {

namespace actor_notify_squad_and_flag_danger_local {
}

/**
 * If the actor controls a unit, broadcasts a "retreat/regroup" squad event (0x17, or 0x16 when alternate_event
 * is set). If raise_danger_flag is set and no higher-priority danger slot (unknown_308) is already claimed,
 * claims danger code 6 with no payload object.
 *
 * @address 0x423600
 */
void ActorView::notify_squad_and_flag_danger(uint8_t alternate_event, uint8_t raise_danger_flag)
{
    using namespace actor_notify_squad_and_flag_danger_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->unit_index != (datum_index)k_datum_index_none) {
        halo::ai::ai_communication_broadcast(0x17 - (alternate_event != 0), self->unit_index,
                                   (datum_index)k_datum_index_none, (datum_index)k_datum_index_none,
                                   (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
    }

    if (raise_danger_flag != 0 && self->pending_panic_type < 6) {
        self->pending_panic_type = 6;
        self->pending_panic_prop_index = halo::k_dword_none;
    }
}

namespace actor_notify_squad_of_threat_direction_local {
}

/**
 * If the actor controls a unit and event_kind is 2, broadcasts a category-0xb squad event with grenade_type_code
 * (0/1/2) remapped to a descending severity code (3/2/1). Either way, if the actor controls a unit, records a
 * look-at point toward `point` (at priority 4, only when event_kind is 2 and the ac
 *
 * @address 0x4234f0
 */
void ActorOps::notify_squad_of_threat_direction(const real_point3d *point, datum_index actor_index, int16_t event_kind, int16_t grenade_type_code)
{
    using namespace actor_notify_squad_of_threat_direction_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    datum_index unit_index = self->unit_index;

    if (unit_index == (datum_index)k_datum_index_none) {
        return;
    }

    if (event_kind == 2) {
        int32_t severity = -1;
        if (grenade_type_code == 0) severity = 3;
        else if (grenade_type_code == 1) severity = 2;
        else if (grenade_type_code == 2) severity = 1;
        halo::ai::ai_communication_broadcast(0xb, unit_index, (datum_index)k_datum_index_none, severity,
                                   (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
    }

    {
        real_vector3d direction;
        float length;

        direction.i = point->x - self->aim_origin.x;
        direction.j = point->y - self->aim_origin.y;
        direction.k = point->z - self->aim_origin.z;
        length = halo::math::vector3d_normalize_with_length(direction);

        if (self->awareness_level < 3 && length < actor_tag->surprise_distance && event_kind == 2) {
            halo::ai::actor_record_look_at_point(actor_index, (const uint32_t *)&direction, 4, halo::k_dword_none);
        }
        halo::ai::actor_queue_search_position(actor_index, 0, 4, &direction, halo::k_dword_none, 0, 0, halo::k_dword_none, 0, 0);
    }
}

namespace actor_notify_target_engaged_local {
}

/**
 * 0x42d340, not yet rewritten (this module). If the target prop is not a vault, the notifying actor still
 * controls a unit, and the target prop is itself a unit, broadcasts an "engaged" chatter event (5, or 4 when
 * alternate_event is set) naming the actor's unit and the target's object.
 *
 * @address 0x4220c0
 */
void TargetView::notify_target_engaged(datum_index actor_index, uint8_t alternate_event)
{
    using namespace actor_notify_target_engaged_local;
    prop *target = &((prop *)halo::ai::globals().prop_data->data)[target_prop_index & halo::k_slot_mask];
    actor *notifier = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    datum_index unit_index;

    if (target->dead == 0) {
        unit_index = notifier->unit_index;
        if (unit_index != (datum_index)k_datum_index_none && target->enemy != 0) {
            halo::ai::ai_communication_broadcast(5 - (alternate_event != 0), unit_index, target->object_index, 3,
                                       (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
        }
    }
}

namespace actor_notify_weapon_pickup_once_local {
}

/**
 * If object_index's controlling actor has not yet been notified of a weapon pickup event afterward.
 *
 * @address 0x42c370
 */
void ActorOps::notify_weapon_pickup_once(datum_index object_index)
{
    using namespace actor_notify_weapon_pickup_once_local;
    object *obj;
    unit_data *unit;
    datum_index actor_index;
    actor *a;

    obj = halo::ai::object_at(object_index);
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    actor_index = unit->actor_index;
    if (actor_index != (datum_index)k_datum_index_none) {
        a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
        if (a->vehicle_exit_forced == 0) {
            halo::ai::ai_communication_broadcast(0x25, (datum_index)k_datum_index_none,
                                        (datum_index)k_datum_index_none, 0,
                                        (datum_index)k_datum_index_none,
                                        (datum_index)k_datum_index_none, 0);
        }
        a->vehicle_exit_forced = 0;
    }
}

namespace actor_pick_dialogue_variant_a_local {
static auto &k_real_one = halo::link::ref<float>(halo::ai::vars().k_real_one);
static auto &k_random_scale_65536 = halo::link::ref<float>(halo::ai::vars().k_random_scale_65536);
static auto &actor_dialogue_variant_offset_1a = halo::link::ref<float>(halo::ai::vars().actor_dialogue_variant_offset_1a);
static auto &actor_dialogue_variant_scale_2a = halo::link::ref<float>(halo::ai::vars().actor_dialogue_variant_scale_2a);
static auto &actor_dialogue_variant_offset_2a = halo::link::ref<float>(halo::ai::vars().actor_dialogue_variant_offset_2a);
static auto &k_real_point_six = halo::link::ref<float>(halo::ai::vars().k_real_point_six);
static auto &actor_dialogue_variant_offset_3a = halo::link::ref<float>(halo::ai::vars().actor_dialogue_variant_offset_3a);
static auto &ticks_per_second = halo::link::ref<float>(halo::ai::vars().ticks_per_second);
}

/**
 * Rolls the shared PRNG for categories 1-3 (each with its own scale/offset, category 1 having no extra scale
 * factor), or falls back to a fixed default value for anything else, then converts the result to a tick count
 * clamped to [0, 255].
 *
 * @address 0x424aa0
 */
int32_t ActorOps::pick_dialogue_variant_a(int16_t category)
{
    using namespace actor_pick_dialogue_variant_a_local;
    float value = k_real_one;
    int32_t ticks;

    if (category == 1) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        value = (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536
              + actor_dialogue_variant_offset_1a;
    } else if (category == 2) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        value = (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536 * actor_dialogue_variant_scale_2a
              + actor_dialogue_variant_offset_2a;
    } else if (category == 3) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        value = (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536 * k_real_point_six
              + actor_dialogue_variant_offset_3a;
    }

    ticks = halo::x87::__ftol((double)(value * ticks_per_second));
    if (ticks > 0xff) {
        return 0xff;
    }
    return ticks;
}

namespace actor_pick_dialogue_variant_b_local {
static auto &k_real_one = halo::link::ref<float>(halo::ai::vars().k_real_one);
static auto &k_random_scale_65536 = halo::link::ref<float>(halo::ai::vars().k_random_scale_65536);
static auto &actor_dialogue_variant_scale_1b = halo::link::ref<float>(halo::ai::vars().actor_dialogue_variant_scale_1b);
static auto &actor_dialogue_variant_scale_23b = halo::link::ref<float>(halo::ai::vars().actor_dialogue_variant_scale_23b);
static auto &k_real_point_six = halo::link::ref<float>(halo::ai::vars().k_real_point_six);
static auto &ticks_per_second = halo::link::ref<float>(halo::ai::vars().ticks_per_second);
}

/**
 * Rolls the shared PRNG for category 1 (scaled by actor_dialogue_variant_scale_1b, offset by the same constant
 * the default case uses directly) or for categories 2-3 (a different scale/offset pair), otherwise falls back to
 * the fixed default value, then converts to a tick count clamped to [0, 255].
 *
 * @address 0x424b80
 */
int32_t ActorOps::pick_dialogue_variant_b(int16_t category)
{
    using namespace actor_pick_dialogue_variant_b_local;
    float value = k_real_one;
    int32_t ticks;

    if (category == 1) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        value = (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536 * actor_dialogue_variant_scale_1b
              + k_real_one;
    } else if (category > 1 && category <= 3) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        value = (float)(int32_t)(halo::math::globals().random_seed_global >> 0x10) * k_random_scale_65536 * actor_dialogue_variant_scale_23b
              + k_real_point_six;
    }

    ticks = halo::x87::__ftol((double)(value * ticks_per_second));
    if (ticks > 0xff) {
        return 0xff;
    }
    return ticks;
}

namespace actor_play_first_valid_vocalization_local {
}

/**
 * Actor AI behaviour: play first valid vocalization.
 *
 * @address 0x40e260
 */
uint8_t ActorOps::play_first_valid_vocalization(int16_t *seat_list, datum_index vehicle_index, datum_index actor_index, char *seat_name, int16_t seat_flags, int16_t count)
{
    using namespace actor_play_first_valid_vocalization_local;
    actor *act = halo::ai::actor_at(actor_index);
    int16_t local_list[16];
    uint8_t order[k_actor_mode_data_size];
    int16_t i;

    if (seat_list == 0) {
        seat_list = local_list;
        count = halo::units::unit_find_seats_matching_name_and_flags(vehicle_index, seat_name, (uint16_t)seat_flags, local_list, 16);
    }
    for (i = 0; i < count; i++) {
        int16_t seat = seat_list[i];

        if (seat == -1 || !halo::units::unit_seat_index_is_valid(act->unit_index, vehicle_index, seat)) {
            continue;
        }
        if (halo::ai::actor_build_order_investigate_encounter_point(vehicle_index, actor_index, seat, order)) {
            halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::vehicle, order);
            seat_list[i] = -1;
            return 1;
        }
    }
    return 0;
}

namespace actor_push_recognition_entry_local {
}

/**
 * Records that the actor has just recognized something at one of its encounter firing positions. The current
 * ring slot takes the type byte and the position index, the cursor wraps modulo 4, and the latched
 * recognition_position is refreshed from the scenario firing position itself. A firing_position_in
 *
 * @address 0x4141a0
 */
void ActorView::push_recognition_entry(int16_t firing_position_index, uint8_t type)
{
    using namespace actor_push_recognition_entry_local;
    actor *self;
    ScenarioEncounter *encounter_definition;
    ScenarioFiringPosition *firing_positions;
    int16_t cursor;

    if (firing_position_index == -1) {
        return;
    }

    self = halo::ai::actor_at(actor_index);

    cursor = self->recognition_cursor;
    self->recognition[cursor].type = type;
    self->recognition[cursor].firing_position_index = firing_position_index;
    self->recognition_cursor = (int16_t)((cursor + 1) % 4);

    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
                               [self->encounter_index & halo::k_slot_mask];
    firing_positions = (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;

    self->recognition_valid = 1;
    self->recognition_type = type;
    self->recognition_position.x = firing_positions[firing_position_index].position.x;
    self->recognition_position.y = firing_positions[firing_position_index].position.y;
    self->recognition_position.z = firing_positions[firing_position_index].position.z;
}

namespace actor_queue_directional_reaction_event_local {
static auto &actor_dialogue_variant_table_b = halo::link::ref<int16_t []>(halo::ai::vars().actor_dialogue_variant_table_b);
}

/**
 * Queues category-3 combat dialogue and a matching look-at/search-position update, driven either by an explicit
 * unit prop (when it is a unit) or, failing that, a supplied direction vector normalized in place. Bails out
 * with no effect if neither source is usable, or if the actor is not alert enough / h
 *
 * @address 0x422270
 */
void ActorOps::queue_directional_reaction_event(const real_vector3d *direction, datum_index target_prop_index, datum_index actor_index)
{
    using namespace actor_queue_directional_reaction_event_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    prop *target = 0;
    real_vector3d normalized;
    int have_direction = 0;
    const void *look_source = 0;
    int16_t kind;
    uint32_t payload;
    Actor *actor_tag;
    float wait_scale;
    float min_scale, max_scale;
    int32_t ticks;

    if (target_prop_index != (datum_index)k_datum_index_none) {
        target = &((prop *)halo::ai::globals().prop_data->data)[target_prop_index & halo::k_slot_mask];
    } else if (direction != 0) {
        float mag2 = direction->k * direction->k + direction->j * direction->j + direction->i * direction->i;
        if (mag2 > 0.25f) {
            float inv = -1.0f / (float)halo::libm::sqrt((double)mag2);
            normalized.i = inv * direction->i;
            normalized.j = inv * direction->j;
            normalized.k = inv * direction->k;
            have_direction = 1;
            look_source = &normalized;
        }
    }
    if (target != 0) {
        look_source = &target->direction;
    }

    self->attack_pending = 1;

    if ((target == 0 || target->enemy != 0) && self->awareness_level < 3) {
        halo::ai::actor_record_look_at_point(actor_index, (const uint32_t *)look_source, 5, target_prop_index);
        halo::ai::actor_queue_search_position(actor_index, 0, 5, (real_vector3d *)look_source,
                                    halo::k_dword_none, 0, 90, target_prop_index, 150, 0);
    }

    if (target_prop_index != (datum_index)k_datum_index_none) {
        kind = 1;
        payload = target_prop_index;
    } else {
        if (!have_direction) {
            return;
        }
        kind = 4;
        payload = 0;
    }

    actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);

    if (self->awareness_level > 1 && self->vocalization_line < 12 &&
        (self->mode != halo::ai::actor_mode::obey || self->mode_data.obey.allow_look != 0) &&
        (kind != 1 || halo::memory::datum_get(payload, halo::ai::globals().prop_data) != 0)) {
        wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 5.0f : 2.5f;

        if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
            min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
            max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
            wait_scale = halo::math::random_real_range(min_scale, max_scale) * wait_scale;
        }

        ticks = (int32_t)(wait_scale * 30.0f + 0.5f);
        if (ticks > INT16_MAX) {
            ticks = INT16_MAX;
        }

        self->vocalization_state = (int16_t)ticks;
        self->vocalization_variant = actor_dialogue_variant_table_b[self->combat_status >= 4];
        self->vocalization_line = 11;
        if (target_prop_index != (datum_index)k_datum_index_none) {
            self->vocalization_source.code = target_prop_index;
            self->vocalization_source.payload.handle = 0;
            self->vocalization_source.payload.point.y = 0.0f;
        } else {
            memcpy(&self->vocalization_source, &normalized, sizeof(real_vector3d));
        }
    }
}

namespace actor_queue_point_reaction_dialogue_local {
static auto &actor_dialogue_variant_table_g = halo::link::ref<int16_t []>(halo::ai::vars().actor_dialogue_variant_table_g);
}

/**
 * If the actor's awareness level is not exactly 1, and it is alert enough / hasn't queued too many vocalizations
 * / isn't mid-vocalization itself / hasn't recently had a category-1 event, queues category-1 dialogue with a
 * randomized duration (scaled by vitality grade and the Actor tag's event_look_time
 *
 * @address 0x422780
 */
void ActorOps::queue_point_reaction_dialogue(const real_point3d *point, datum_index actor_index)
{
    using namespace actor_queue_point_reaction_dialogue_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->awareness_level != 1) {
        Actor *actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);

        if (self->awareness_level > 1 && self->vocalization_line < 2 &&
            (self->mode != halo::ai::actor_mode::obey || self->mode_data.obey.allow_look != 0) &&
            self->flee_reason < 7) {
            float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 2.6f : 1.3f;

            if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
                float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
                float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
                wait_scale = halo::math::random_real_range(min_scale, max_scale) * wait_scale;
            }

            {
                int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f);
                if (ticks > INT16_MAX) {
                    ticks = INT16_MAX;
                }

                self->vocalization_variant = actor_dialogue_variant_table_g[self->combat_status >= 4];
                self->vocalization_line = 1;
                self->vocalization_state = (int16_t)ticks;
                self->vocalization_source.code = 3;
                self->vocalization_source.payload.point = *point;
            }
        }
    }
}

namespace actor_queue_recognized_target_dialogue_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &actor_dialogue_variant_table_c = halo::link::ref<int16_t []>(halo::ai::vars().actor_dialogue_variant_table_c);
}

/**
 * Validates the target prop via datum_get and, if it is a unit (or a vault the actor is alert enough to notice),
 * refreshes its re-notice timer as actor_queue_sighted_target_ dialogue does, then queues category-5 dialogue
 * with a randomized duration derived the same way, storing the raw target handle as
 *
 * @address 0x422550
 */
void ActorView::queue_recognized_target_dialogue(datum_index target_prop_index)
{
    using namespace actor_queue_recognized_target_dialogue_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);

    if (self->awareness_level > 1 && self->vocalization_line < 6 &&
        (self->mode != halo::ai::actor_mode::obey || self->mode_data.obey.allow_look != 0)) {
        int16_t recent = self->flee_reason;
        prop *target = (prop *)halo::memory::datum_get(target_prop_index, halo::ai::globals().prop_data);

        if (target != 0) {
            if ((target->enemy == 0 && target->dead == 0) ||
                (target->dead != 0 && self->awareness_level > 2)) {
                if (recent > 6) {
                    return;
                }
                if (target->is_parented == 0 && target->last_attention_time != -1 &&
                    (int32_t)halo::game::globals().game_time->game_time < target->last_attention_time + 600) {
                    return;
                }
                target->last_attention_time = (int32_t)halo::game::globals().game_time->game_time;
                target->interest_satisfied = (target->interest_satisfied <= target->interest)
                                          ? target->interest
                                          : target->interest_satisfied;
            }

            {
                float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 1.4f : 0.7f;

                if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
                    float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
                    float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
                    wait_scale = halo::math::random_real_range(min_scale, max_scale) * wait_scale;
                }

                int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f);
                if (ticks > INT16_MAX) {
                    ticks = INT16_MAX;
                }

                self->vocalization_state = (int16_t)ticks;
                self->vocalization_variant = actor_dialogue_variant_table_c[self->combat_status >= 4];
                self->vocalization_line = 5;
                self->vocalization_source.code = 1;
                self->vocalization_source.payload.handle = target_prop_index;
                self->vocalization_source.payload.point.y = 0.0f;
                self->vocalization_source.payload.point.z = 0.0f;
            }
        }
    }
}

namespace actor_queue_search_and_relay_perception_local {
}

/**
 * Queues a priority-1 velocity-only search request from the prop, as actor_queue_velocity_search_from_prop does
 * at priority 6, then forwards the prop's owning actor's running perception priority (if positive) as a new
 * perception event on this actor.
 *
 * @address 0x4221f0
 */
void ActorOps::queue_search_and_relay_perception(datum_index prop_index, datum_index actor_index)
{
    using namespace actor_queue_search_and_relay_perception_local;
    prop *p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];
    datum_index owner_index;

    halo::ai::actor_queue_search_position(actor_index, 0, 1, (real_vector3d *)&p->direction,
                                halo::k_dword_none, 0, 90, prop_index, 150, 0);

    owner_index = p->owner_actor_index;
    if (owner_index != (datum_index)k_datum_index_none) {
        actor *owner = &((actor *)halo::ai::globals().actor_data->data)[owner_index & halo::k_slot_mask];
        if (owner->suspicion_status > 0) {
            halo::ai::actor_record_perception_event(actor_index, owner->suspicion_status, 0x1c2);
        }
    }
}

namespace actor_queue_search_position_local {
}

/**
 * stack -> surface_index, position_extra, velocity_ticks, prop_index, prop_value, prop_flag Records a candidate
 * 'investigate/search' position for the actor if its priority is at least as high as any currently queued one,
 * replacing the stored position and/or velocity (each optional) and the caller-supplied
 *
 * @address 0x421af0
 */
void ActorView::queue_search_position(real_point3d *position, int16_t priority, real_vector3d *velocity, uint32_t surface_index, uint32_t position_extra, uint32_t velocity_ticks, uint32_t prop_index, uint32_t prop_value, uint8_t prop_flag)
{
    using namespace actor_queue_search_position_local;
    actor *self;

    self = halo::ai::actor_at(actor_index);

    if (self->awareness_level < 3 && self->search_priority <= priority) {
        self->search_priority = priority;

        if (position == (real_point3d *)0) {
            self->search_position_valid = 0;
        } else {
            self->search_position_valid = 1;
            self->search_position = *position;
            self->search_surface_index = surface_index;
            self->search_position_extra = position_extra;
        }

        if (velocity == (real_vector3d *)0) {
            self->search_velocity_valid = 0;
        } else {
            self->search_velocity_valid = 1;
            self->search_velocity = *velocity;
        }

        self->search_prop_index = prop_index;
        self->search_prop_value = prop_value;
        self->search_prop_flag = prop_flag;
        self->search_velocity_ticks = velocity_ticks;
    }
}

namespace actor_queue_secondary_action_local {
}

/**
 * Actor AI behaviour: queue secondary action.
 *
 * @address 0x417a60
 */
uint8_t ActorView::queue_secondary_action(int16_t action, const real_vector2d *direction)
{
    using namespace actor_queue_secondary_action_local;
    actor *self;

    self = halo::ai::actor_at(actor_index);
    halo::ai::actor_set_units_active(actor_index, 0);

    if (self->secondary_action != (int16_t)-1) {
        return 0;
    }
    if (self->unit_index != (datum_index)k_datum_index_none && halo::units::unit_is_in_busy_animation_state(self->unit_index)) {
        return 0;
    }

    self->secondary_action = action;
    self->secondary_action_direction = *direction;
    return 1;
}

namespace actor_queue_sighted_target_dialogue_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &actor_dialogue_variant_table_a = halo::link::ref<int16_t []>(halo::ai::vars().actor_dialogue_variant_table_a);
}

/**
 * The main "did I just notice/see this target" handler: if the target prop is not a vault, queues category-4
 * "sighted" dialogue with a randomized duration (scaled by the actor's vitality grade and the Actor tag's
 * event_look_time_modifier range), throttled by a per-target re-notice cooldown of 600 tick
 *
 * @address 0x421c20
 */
void ActorView::queue_sighted_target_dialogue(datum_index target_prop_index, uint8_t already_noticed)
{
    using namespace actor_queue_sighted_target_dialogue_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    prop *target = &((prop *)halo::ai::globals().prop_data->data)[target_prop_index & halo::k_slot_mask];
    Actor *actor_tag;
    prop *validated;

    if (target->dead == 0) {
        actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);

        if (self->awareness_level > 1 && self->vocalization_line < 5 &&
            (self->mode != halo::ai::actor_mode::obey || self->mode_data.obey.allow_look != 0)) {
            int16_t recent = self->flee_reason;

            validated = (prop *)halo::memory::datum_get(target_prop_index, halo::ai::globals().prop_data);
            if (validated != 0) {
                if ((validated->enemy == 0 && validated->dead == 0) ||
                    (validated->dead != 0 && self->awareness_level > 2)) {
                    if (recent <= 6) {
                        if (validated->is_parented == 0 && validated->last_attention_time != -1 &&
                            (int32_t)halo::game::globals().game_time->game_time >= validated->last_attention_time + 600) {
                            validated->last_attention_time = (int32_t)halo::game::globals().game_time->game_time;
                            validated->interest_satisfied = (validated->interest_satisfied <= validated->interest)
                                                         ? validated->interest
                                                         : validated->interest_satisfied;
                        }
                    }
                }
                if (recent <= 6) {
                    float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 1.8f : 0.9f;

                    if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
                        float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
                        float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
                        wait_scale = halo::math::random_real_range(min_scale, max_scale) * wait_scale;
                    }

                    int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f);
                    if (ticks > INT16_MAX) {
                        ticks = INT16_MAX;
                    }

                    self->vocalization_line = 4;
                    self->vocalization_state = (int16_t)ticks;
                    self->vocalization_source.code = 1;
                    self->vocalization_variant = actor_dialogue_variant_table_a[self->combat_status >= 4];
                    self->vocalization_source.payload.handle = target_prop_index;
                    self->vocalization_source.payload.point.y = 0.0f;
                    self->vocalization_source.payload.point.z = 0.0f;
                }
            }
        }

        if (target->enemy != 0) {
            float facing_dot = target->direction.z * self->facing.k
                              + target->direction.y * self->facing.j
                              + target->direction.x * self->facing.i;
            int outside_cone = facing_dot < 0.5f;
            int priority = 0;

            bool check_shared;

            if (self->combat_status == 0) {
                already_noticed = 0;
                if (self->awareness_level < 3 && target->shooting != 0 &&
                    target->distance < actor_tag->surprise_distance && priority < 4) {
                    priority = 3;
                }
                check_shared = true;
            } else if (self->combat_status < 5 || outside_cone) {
                check_shared = already_noticed == 0;
            } else {
                already_noticed = 1;
                check_shared = false;
            }

            if (check_shared) {
                bool skip_record;

                if (target->shooting == 0 || !(target->distance < actor_tag->surprise_distance)) {
                    skip_record = priority == 0;
                } else if (outside_cone) {
                    skip_record = priority > 7;
                } else {
                    skip_record = priority > 6;
                }
                if (!skip_record) {
                    halo::ai::actor_record_look_at_point(actor_index, (const uint32_t *)&target->direction, (int16_t)priority, target_prop_index);
                }
            }

            if (self->combat_status < 3 && already_noticed == 0 &&
                target->visual_perception < 2 && self->unit_index != (datum_index)k_datum_index_none) {
                halo::ai::ai_communication_broadcast(6, self->unit_index, target->object_index, 3,
                                           (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
            }
        }
    }

    if (target->is_parented != 0 && target->enemy != 0 && target->dead == 0 &&
        self->type != 15 && halo::hs::fields::medusa != 0) {
        if (self->swarm == 0) {
            datum_index unit_index = self->unit_index;
            object_header *header = &((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask];
            ((uint8_t *)header->data + 0x106)[0] |= 0x20;
        } else {
            datum_index cluster_index = self->cluster_unit_index;
            while (cluster_index != (datum_index)k_datum_index_none) {
                object_header *header = &((object_header *)halo::objects::globals().object_data->data)[cluster_index & halo::k_slot_mask];
                struct object *unit_object = header->data;
                unit_object->vitality_flags |= halo::to_bits(halo::objects::vitality_flag::unknown_20);
                cluster_index = halo::units::unit_data_of(unit_object)->swarm_next_unit_index;
            }
        }
    }
}

namespace actor_queue_velocity_search_from_prop_local {
}

/**
 * Queues a priority-6 search request for the actor carrying no explicit position but a velocity vector taken
 * from the prop's scratch field, a 90-tick duration, a 150 secondary duration, and the raw prop handle threaded
 * through prop_index.
 *
 * @address 0x4221b0
 */
void ActorOps::queue_velocity_search_from_prop(datum_index prop_index, datum_index actor_index)
{
    using namespace actor_queue_velocity_search_from_prop_local;
    prop *p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];

    halo::ai::actor_queue_search_position(actor_index, 0, 6, (real_vector3d *)&p->direction,
                                halo::k_dword_none, 0, 90, prop_index, 150, 0);
}

namespace actor_react_to_flee_point_local {
static auto &actor_dialogue_variant_table_e = halo::link::ref<int16_t []>(halo::ai::vars().actor_dialogue_variant_table_e);
}

/**
 * Computes a look direction toward `point` (relative to aim origin, falling back to facing when degenerate) and,
 * if alert enough and within surprise range, records a look-at point toward it at priority 4, then
 * unconditionally queues a matching priority-3 search position. If flee_source_object is valid
 *
 * @address 0x422c00
 */
void ActorView::react_to_flee_point(int32_t flee_source_object, const real_point3d *point)
{
    using namespace actor_react_to_flee_point_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    real_vector3d direction;
    float length;

    direction.i = point->x - self->aim_origin.x;
    direction.j = point->y - self->aim_origin.y;
    direction.k = point->z - self->aim_origin.z;
    length = halo::math::vector3d_normalize_with_length(direction);
    if ((float)halo::libm::fabs((double)length) < 0.0001f) {
        direction = self->facing;
    }

    if (self->awareness_level < 3 && length < actor_tag->surprise_distance) {
        halo::ai::actor_record_look_at_point(actor_index, (const uint32_t *)&direction, 4, halo::k_dword_none);
    }
    halo::ai::actor_queue_search_position(actor_index, 0, 3, &direction, halo::k_dword_none, 0, 90, halo::k_dword_none, 0, 0);

    if (flee_source_object != -1) {
        object *source = halo::ai::object_at(flee_source_object);
        if (halo::game::teams_are_enemies(source->owner_team , self->team) != 0) {
            halo::ai::actor_record_perception_event(actor_index, 2, 0x384);
        }
    }

    if (self->awareness_level > 1 && self->vocalization_line < 7 &&
        (self->mode != halo::ai::actor_mode::obey || self->mode_data.obey.allow_look != 0)) {
        float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 1.8f : 0.9f;

        if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
            float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
            float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
            wait_scale = halo::math::random_real_range(min_scale, max_scale) * wait_scale;
        }

        {
            int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f);
            if (ticks > INT16_MAX) {
                ticks = INT16_MAX;
            }

            self->vocalization_state = (int16_t)ticks;
            self->vocalization_line = 6;
            self->vocalization_variant = actor_dialogue_variant_table_e[self->combat_status >= 4];
            self->vocalization_source.code = 3;
            self->vocalization_source.payload.point = *point;
        }
    }
}

namespace actor_react_to_registered_danger_local {
static auto &actor_dialogue_variant_table_d = halo::link::ref<int16_t []>(halo::ai::vars().actor_dialogue_variant_table_d);
}

/**
 * If the actor already has a matching, still-live danger registered against danger_object_index, just
 * re-broadcasts a category-10 "danger" squad event; otherwise computes a look direction toward `point` (relative
 * to the actor's aim origin, falling back to its current facing when degenerate) and, if al
 *
 * @address 0x422930
 */
void ActorOps::react_to_registered_danger(const real_point3d *point, datum_index actor_index, int32_t danger_object_index)
{
    using namespace actor_react_to_registered_danger_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    real_vector3d direction;

    if (self->danger_type >= 1 && self->danger_object_index == danger_object_index && self->danger_reaction_ticks >= 1) {
        halo::ai::ai_communication_broadcast(10, self->unit_index, (datum_index)k_datum_index_none, (datum_index)k_datum_index_none,
                                   (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
    } else {
        float length;

        direction.i = point->x - self->aim_origin.x;
        direction.j = point->y - self->aim_origin.y;
        direction.k = point->z - self->aim_origin.z;
        length = halo::math::vector3d_normalize_with_length(direction);
        if ((float)halo::libm::fabs((double)length) < 0.0001f) {
            direction = self->facing;
        }

        if (self->awareness_level < 3 && length < actor_tag->surprise_distance) {
            halo::ai::actor_record_look_at_point(actor_index, (const uint32_t *)&direction, 2, halo::k_dword_none);
        }
        halo::ai::actor_queue_search_position(actor_index, 0, 3, &direction, halo::k_dword_none, 0, 90, halo::k_dword_none, 0, 0);
    }

    if (self->awareness_level > 1 && self->vocalization_line < 4 &&
        (self->mode != halo::ai::actor_mode::obey || self->mode_data.obey.allow_look != 0) && self->flee_reason < 7) {
        float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 1.8f : 0.9f;

        if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
            float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
            float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
            wait_scale = halo::math::random_real_range(min_scale, max_scale) * wait_scale;
        }

        {
            int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f);
            if (ticks > INT16_MAX) {
                ticks = INT16_MAX;
            }

            self->vocalization_variant = actor_dialogue_variant_table_d[self->combat_status >= 4];
            self->vocalization_state = (int16_t)ticks;
            self->vocalization_line = 3;
            self->vocalization_source.code = 3;
            self->vocalization_source.payload.point = *point;
        }
    }
}

namespace actor_react_to_seen_target_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &actor_dialogue_variant_table_f = halo::link::ref<int16_t []>(halo::ai::vars().actor_dialogue_variant_table_f);
}

/**
 * If the target prop is not itself a unit, relays perception/search state to it the underlying object, either
 * notifies a hostile controlling player's presence or, for an AI-controlled unit whose actor is alert enough,
 * forwards to actor_forward_target_object_reference. If the target prop IS a unit, ins
 *
 * @address 0x422ec0
 */
void ActorView::react_to_seen_target(datum_index target_prop_index)
{
    using namespace actor_react_to_seen_target_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_tag;
    prop *target = &((prop *)halo::ai::globals().prop_data->data)[target_prop_index & halo::k_slot_mask];

    if (target->enemy == 0) {
        object *tracked = halo::ai::object_at(target->object_index);
        unit_data *unit = (unit_data *)((uint8_t *)tracked + k_unit_data_offset);

        halo::ai::actor_queue_search_and_relay_perception(target_prop_index, actor_index);

        if (unit->controlling_player != (datum_index)k_datum_index_none) {
            uint8_t *player = (uint8_t *)halo::game::globals().player_data->data + (unit->controlling_player & halo::k_slot_mask) * 0x200;
            int32_t unknown_40 = static_cast<int32_t>(((struct player *)player)->observer_target);
            int32_t unknown_44 = ((struct player *)player)->observer_state;

            if (unknown_40 != -1 && (int32_t)halo::game::globals().game_time->game_time <= unknown_44 + 0x5a) {
                object *player_unit = halo::ai::object_at(unknown_40);
                if (halo::game::teams_are_enemies(player_unit->owner_team , self->team) != 0) {
                    halo::ai::actor_target_data_acquire(actor_index, (datum_index)unknown_40, k_datum_index_none, k_datum_index_none);
                }
            }
        } else if (unit->actor_index != (datum_index)k_datum_index_none) {
            actor *controller = &((actor *)halo::ai::globals().actor_data->data)[unit->actor_index & halo::k_slot_mask];
            if (controller->combat_status >= 4) {
                halo::ai::actor_forward_target_object_reference(unit->actor_index, actor_index);
            }
        }
    } else {
        halo::ai::actor_queue_search_position(actor_index, 0, 6, (real_vector3d *)&target->direction,
                                    halo::k_dword_none, 0, 90, target_prop_index, 150, 0);
    }

    actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);

    if (self->awareness_level > 1 && self->vocalization_line < 8 &&
        (self->mode != halo::ai::actor_mode::obey || self->mode_data.obey.allow_look != 0)) {
        int16_t recent = self->flee_reason;

        prop *validated = (prop *)halo::memory::datum_get(target_prop_index, halo::ai::globals().prop_data);
        if (validated != 0) {
            if ((validated->enemy == 0 && validated->dead == 0) ||
                (validated->dead != 0 && self->awareness_level > 2)) {
                if (recent > 6) {
                    return;
                }
                if (validated->is_parented == 0 && validated->last_attention_time != -1 &&
                    (int32_t)halo::game::globals().game_time->game_time < validated->last_attention_time + 600) {
                    return;
                }
                validated->last_attention_time = (int32_t)halo::game::globals().game_time->game_time;
                validated->interest_satisfied = (validated->interest_satisfied <= validated->interest)
                                             ? validated->interest
                                             : validated->interest_satisfied;
            }

            {
                float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 1.8f : 0.9f;

                if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
                    float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
                    float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
                    wait_scale = halo::math::random_real_range(min_scale, max_scale) * wait_scale;
                }

                int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f);
                if (ticks > INT16_MAX) {
                    ticks = INT16_MAX;
                }

                self->vocalization_state = (int16_t)ticks;
                self->vocalization_variant = actor_dialogue_variant_table_f[self->combat_status >= 4];
                self->vocalization_line = 7;
                self->vocalization_source.code = 1;
                self->vocalization_source.payload.handle = target_prop_index;
                self->vocalization_source.payload.point.y = 0.0f;
                self->vocalization_source.payload.point.z = 0.0f;
            }
        }
    }
}

namespace actor_record_look_at_point_local {
}

/**
 * Registers a new look-at point of interest for the actor if `priority` outranks whatever is currently recorded
 * at actor.look_at_priority. `point`, when non-NULL, is copied verbatim (three dwords) into the trailing fields
 * and the "point present" flag is set to 1; when NULL, only that flag is cleared t
 *
 * @address 0x421bc0
 */
void ActorView::record_look_at_point(const uint32_t *point, int16_t priority, uint32_t data)
{
    using namespace actor_record_look_at_point_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->look_at_priority < priority) {
        self->look_at_priority = priority;
        self->look_at_reference = data;
        if (point == 0) {
            self->look_at_has_point = 0;
            return;
        }
        self->look_at_has_point = 1;
        memcpy(&self->look_at_point, point, sizeof(real_point3d));
    }
}

namespace actor_record_perception_event_local {
}

/**
 * Records a pending perception event for the actor to be picked up by actor_update_awareness_level: a
 * higher-priority event replaces the current one outright, an equal-priority event keeps the larger of the two
 * data values, and a lower-priority event is dropped.
 *
 * @address 0x422070
 */
void ActorView::record_perception_event(int16_t event, int32_t data)
{
    using namespace actor_record_perception_event_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->perception_event < event) {
        self->perception_event = event;
        self->perception_event_data = data;
    } else if (self->perception_event == event) {
        if (self->perception_event_data <= data) {
            self->perception_event_data = data;
        }
    }
}

namespace actor_scan_allies_for_backup_request_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}

/**
 * ai_group_bucket_entry now lives in types/ai.h (folded from this file). Per-tick scan of every prop (perceived
 * object) this actor is tracking. For each one that belongs to (is owned by) another actor and looks like a live
 * target, checks whether that owning ally currently has an outstanding "call for
 *
 * @address 0x420ec0
 */
void ActorView::scan_allies_for_backup_request()
{
    using namespace actor_scan_allies_for_backup_request_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_def = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    prop *props = (prop *)halo::ai::globals().prop_data->data;

    ai_group_bucket_entry buckets[16];
    int16_t bucket_count = 0;

    datum_index next = self->first_prop;
    datum_index current;

    while (current = next, current != k_datum_index_none) {
        prop *p = &props[current & halo::k_slot_mask];
        uint8_t priority;

        next = p->next_in_actor;
        priority = halo::ai::actor_target_get_backup_priority(current);

        if (priority > 0) {
            int16_t idx = halo::ai::ai_group_bucket_find_or_add(buckets, (int32_t)p->object_index,
                                                       &bucket_count, 16);
            if (idx != -1) {
                if (buckets[idx].priority < (int16_t)priority) {
                    buckets[idx].prop_index = (int32_t)current;
                    buckets[idx].key = (int32_t)p->object_index;
                    buckets[idx].prop = p;
                    buckets[idx].priority = (int16_t)priority;
                }
            }
        } else if (2 <= p->state && p->state <= 3 && !p->enemy &&
                   p->owner_actor_index != k_datum_index_none && p->distance < 8.0f) {
            actor *owner = &((actor *)halo::ai::globals().actor_data->data)[p->owner_actor_index & halo::k_slot_mask];

            if (owner->retreat_timer != 0 && owner->retreat_prop_index != k_datum_index_none &&
                (self->retreat_end_time == k_datum_index_none ||
                 owner->retreat_start_time >= self->retreat_end_time)) {
                prop *requested = &props[owner->retreat_prop_index & halo::k_slot_mask];
                datum_index own_prop_index = halo::ai::actor_find_prop_for_object(requested->object_index, actor_index);

                if (own_prop_index != k_datum_index_none) {
                    prop *own_prop = &props[own_prop_index & halo::k_slot_mask];

                    if (2 <= own_prop->state && own_prop->state <= 3 && own_prop->engaged) {
                        int16_t idx = halo::ai::ai_group_bucket_find_or_add(
                            buckets, (int32_t)requested->object_index, &bucket_count, 16);
                        if (idx != -1) {
                            float dist_sq = requested->distance * requested->distance;

                            buckets[idx].retreating_friend_count++;
                            if (dist_sq < buckets[idx].nearest_friend_distance_squared) {
                                buckets[idx].nearest_friend_distance_squared = dist_sq;
                                buckets[idx].nearest_friend_actor_index = (int32_t)p->owner_actor_index;
                            }
                            if (buckets[idx].prop_index == k_datum_index_none) {
                                buckets[idx].prop_index = (int32_t)own_prop_index;
                                buckets[idx].key = (int32_t)own_prop->object_index;
                                buckets[idx].prop = own_prop;
                            }
                        }
                    }
                }
            }
        }
    }

    if (bucket_count > 0) {
        int16_t i;
        for (i = 0; i < bucket_count; i++) {
            ai_group_bucket_entry *b = &buckets[i];
            prop *claimant = b->prop;
            int16_t trigger = actor_def->unreachable_danger_trigger;
            uint8_t flagged = 0;

            if (claimant->is_vehicle_gunner != 0 || claimant->is_vehicle_driver != 0) {
                trigger = actor_def->vehicle_danger_trigger;
            }
            if (claimant->is_parented) {
                int16_t player_trigger = actor_def->player_danger_trigger;
                if (player_trigger > 0 && trigger > player_trigger) {
                    trigger = player_trigger;
                }
            }

            if (trigger > 0 && b->priority >= trigger) {
                if (!claimant->is_parented) {
                    claimant->shots_fired = 0x16;
                } else {
                    flagged = 1;
                }
            } else if (claimant->is_parented) {
                claimant->shots_fired = 0x16;
            }

            if (claimant->shots_fired > 0) {
                if (claimant->shots_hit == 0) {
                    claimant->danger_trigger_ticks = (int16_t)(halo::math::random_real_range(
                        actor_def->danger_trigger_time[0], actor_def->danger_trigger_time[1]) *
                        30.0f);
                }
                claimant->shots_fired--;
                claimant->shots_hit++;
            }

            if (claimant->sighted_ticks >= 0x2d ||b->priority >= 4) {
                if (claimant->danger_trigger_ticks > 0 &&
                    claimant->shots_hit >= claimant->danger_trigger_ticks) {
                    if (b->priority < 7) b->priority = 7;
                }
                if (flagged) {
                    if (b->priority < 8) b->priority = 8;
                }
                if (actor_def->friends_killed_trigger > 0 &&
                    claimant->friends_killed >= actor_def->friends_killed_trigger) {
                    if (b->priority < 9) b->priority = 9;
                }
                if (actor_def->friends_retreating_trigger > 0 &&
                    b->retreating_friend_count >= actor_def->friends_retreating_trigger) {
                    if (b->priority < 6) b->priority = 6;
                }
            }
        }
    }

    if (self->retreat_timer > 0) {
        self->retreat_timer--;
        if (self->retreat_timer == 0) {
            self->retreat_end_time = halo::game::globals().game_time->game_time;
        }
        return;
    }

    {
        int16_t best_priority = 5;
        int32_t best_prop = k_datum_index_none;
        int16_t i;

        for (i = 0; i < bucket_count; i++) {
            if (buckets[i].priority > best_priority &&
                buckets[i].prop_index != k_datum_index_none) {
                best_priority = buckets[i].priority;
                best_prop = buckets[i].prop_index;
            }
        }

        if (best_prop != k_datum_index_none) {
            self->retreat_timer = (int16_t)(halo::math::random_real_range(
                actor_def->retreat_time[0], actor_def->retreat_time[1]) * 30.0f);
            self->retreat_prop_index = (datum_index)best_prop;
            self->retreat_start_time = halo::game::globals().game_time->game_time;
        }
    }
}

namespace actor_scan_ally_death_panic_reaction_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}

/**
 * If the target prop is not a unit, the actor's tag allows group panic, and the actor's own panic cooldown
 * (unknown_39c) has elapsed, rolls (and exposure-scales) a chance against Actor.friend_killed_panic_chance; when
 * it succeeds and no higher-priority danger is already claimed, claims danger code 2 f
 *
 * @address 0x4233d0
 */
void TargetView::scan_ally_death_panic_reaction(datum_index actor_index)
{
    using namespace actor_scan_ally_death_panic_reaction_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    Actor *actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    prop *target = &((prop *)halo::ai::globals().prop_data->data)[target_prop_index & halo::k_slot_mask];

    if (target->enemy == 0 && halo::has(static_cast<halo::tags::actor_more_tag_flag>(actor_tag->more_flags), halo::tags::actor_more_tag_flag::panic_in_groups)  &&
        self->panic_cooldown_time < (int32_t)halo::game::globals().game_time->game_time) {
        float chance = actor_tag->friend_killed_panic_chance;

        if (!halo::ai::actor_scale_value_by_ally_exposure(actor_index, &chance) && !(halo::math::random_real() < chance)) {
            return;
        }

        if (self->pending_panic_type < 3) {
            datum_index owner_actor_index = target->owner_actor_index;
            uint32_t payload = self->target_unit_index;

            if (owner_actor_index != (datum_index)k_datum_index_none) {
                actor *owner = &((actor *)halo::ai::globals().actor_data->data)[owner_actor_index & halo::k_slot_mask];
                if (owner->mode == halo::ai::actor_mode::flee) {
                    uint32_t killer_prop = owner->mode_data.flee.reference;
                    if (killer_prop != (uint32_t)k_datum_index_none) {
                        prop *killer = &((prop *)halo::ai::globals().prop_data->data)[killer_prop & halo::k_slot_mask];
                        payload = halo::ai::actor_find_prop_for_object(killer->object_index, actor_index);
                    }
                }

                self->pending_panic_type = 2;
                self->pending_panic_prop_index = payload;
            }
        }
    }
}

namespace actor_scan_backup_and_panic_reaction_local {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}

/**
 * Per-perception-tick reaction for a non-unit target prop: marks the actor "has scanned a prop this cycle"
 * (unknown_8d), and if the target's owning actor type matches this actor's Actor.leader_type, randomly rolls (a
 * global PRNG advance) against Actor.leader_killed_panic_chance to raise danger code 8
 *
 * @address 0x423220
 */
void TargetView::scan_backup_and_panic_reaction(datum_index actor_index)
{
    using namespace actor_scan_backup_and_panic_reaction_local;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    prop *target = &((prop *)halo::ai::globals().prop_data->data)[target_prop_index & halo::k_slot_mask];
    Actor *actor_tag = halo::ai::tag_data<Actor>(self->actor_definition_tag);
    datum_index relevant;

    self->witnessed_death = 1;

    if (target->enemy != 0) {
        return;
    }

    relevant = halo::ai::actor_get_relevant_squad_member_target(actor_index, target_prop_index, 1);

    if (target->actor_type == actor_tag->leader_type && self->pending_panic_type < 8) {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        if ((float)(halo::math::globals().random_seed_global >> 0x10) * 1.5259022e-05f < actor_tag->leader_killed_panic_chance) {
            self->pending_panic_type = 8;
            self->pending_panic_prop_index = relevant;
        }
    }

    if (target->distance >= 8.0f) {
        return;
    }
    if (relevant == (datum_index)k_datum_index_none) {
        return;
    }

    {
        prop *ally = &((prop *)halo::ai::globals().prop_data->data)[relevant & halo::k_slot_mask];

        if (ally->enemy != 0) {
            if (ally->visual_perception > 0 && (int8_t)ally->aiming_at_actor_class <= 2) {
                float chance = actor_tag->friend_killed_panic_chance;
                int roll_ok;

                if (halo::has(static_cast<halo::tags::actor_more_tag_flag>(actor_tag->more_flags), halo::tags::actor_more_tag_flag::panic_in_groups)  &&
                    self->panic_cooldown_time < (int32_t)halo::game::globals().game_time->game_time &&
                    halo::ai::actor_scale_value_by_ally_exposure(actor_index, &chance)) {
                    roll_ok = 1;
                } else {
                    roll_ok = halo::math::random_real() < chance;
                }

                if (roll_ok && self->pending_panic_type < 3) {
                    self->pending_panic_type = 3;
                    self->pending_panic_prop_index = relevant;
                }
            }

            if (ally->engaged != 0) {
                ally->friends_killed = ally->friends_killed + 1;
                ally->friends_killed_timer = 0x2ee;
            }
        }
    }
}

}
