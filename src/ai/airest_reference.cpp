#include "halo/objects/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/core/bit_cast.hpp"
#include "halo/hs/records.hpp"
#include "halo/ai/airest_reference.hpp"
#include "halo/models/api.hpp"

#include <string.h>
#include <stdint.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/core/x87.hpp"

static auto &global_down3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_down3d_pointer);
static auto &k_real_zero = halo::link::ref<float>(halo::ai::vars().k_real_zero);
static auto &k_real_one = halo::link::ref<float>(halo::ai::vars().k_real_one);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);

namespace halo::ai {

/**
 * Clears the defending state of every platoon in the reference's platoon range (hs ai_attack).
 *
 * @address 0x433200
 */
void ReferenceView::clear_defending()
{
    uint32_t packed_reference = handle;
    ai_reference_platoon_range range;

    if (packed_reference == (uint32_t)k_datum_index_none) {
        return;
    }

    halo::ai::ai_reference_expand_to_platoon_range(packed_reference, &range);

    while (range.encounter_index != -1 && range.platoon_start <= range.platoon_end) {
        encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[range.encounter_index & halo::k_slot_mask];
        encounter_platoon_state *state =
            &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + (int16_t)range.platoon_start)];
        range.platoon_start = range.platoon_start + 1;
        if (state == 0) {
            return;
        }
        state->defending = 0;
    }
}

/**
 * Behaviour of ai platoon range has available, moved unchanged from the original free function.
 *
 * @address 0x433180
 */
uint8_t ReferenceView::has_available()
{
    uint32_t packed_reference = handle;
    ai_reference_platoon_range range;

    if (packed_reference == (uint32_t)k_datum_index_none) {
        return 0;
    }

    halo::ai::ai_reference_expand_to_platoon_range(packed_reference, &range);

    for (;;) {
        encounter *enc;
        encounter_platoon_state *state;

        if (range.encounter_index == -1 || range.platoon_end < range.platoon_start) {
            return 0;
        }

        enc = &((encounter *)halo::ai::globals().encounter_data->data)[range.encounter_index & halo::k_slot_mask];
        state = &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + (int16_t)range.platoon_start)];
        range.platoon_start = range.platoon_start + 1;
        if (state == 0) {
            return 0;
        }
        if (state->defending == 0) {
            return 1;
        }
    }
}

/**
 * Sets the defending state of every platoon in the reference's platoon range (hs ai_defend).
 *
 * @address 0x433270
 */
void ReferenceView::set_defending()
{
    uint32_t packed_reference = handle;
    ai_reference_platoon_range range;

    if (packed_reference == (uint32_t)k_datum_index_none) {
        return;
    }

    halo::ai::ai_reference_expand_to_platoon_range(packed_reference, &range);

    while (range.encounter_index != -1 && range.platoon_start <= range.platoon_end) {
        encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[range.encounter_index & halo::k_slot_mask];
        encounter_platoon_state *state =
            &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + (int16_t)range.platoon_start)];
        range.platoon_start = range.platoon_start + 1;
        if (state == 0) {
            return;
        }
        state->defending = 1;
    }
}

/**
 * Sets the maneuvering state of every platoon in the reference's platoon range (hs ai_retreat).
 *
 * @address 0x4332e0
 */
void ReferenceView::set_maneuvering()
{
    uint32_t packed_reference = handle;
    ai_reference_platoon_range range;

    if (packed_reference == (uint32_t)k_datum_index_none) {
        return;
    }

    halo::ai::ai_reference_expand_to_platoon_range(packed_reference, &range);

    while (range.encounter_index != -1 && range.platoon_start <= range.platoon_end) {
        encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[range.encounter_index & halo::k_slot_mask];
        encounter_platoon_state *state =
            &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + (int16_t)range.platoon_start)];
        range.platoon_start = range.platoon_start + 1;
        if (state == 0) {
            return;
        }
        state->maneuvering = 1;
    }
}

/**
 * Stores maneuver_disabled = (flag == 0) for every platoon in the reference's platoon range (hs ai_maneuver_enable).
 *
 * @address 0x433350
 */
void ReferenceView::set_maneuver_enabled(char flag)
{
    uint32_t packed_reference = handle;
    ai_reference_platoon_range range;
    uint8_t value = (flag == 0) ? 1 : 0;

    if (packed_reference == (uint32_t)k_datum_index_none) {
        return;
    }

    halo::ai::ai_reference_expand_to_platoon_range(packed_reference, &range);

    while (range.encounter_index != -1 && range.platoon_start <= range.platoon_end) {
        encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[range.encounter_index & halo::k_slot_mask];
        encounter_platoon_state *state =
            &halo::ai::globals().platoon_states[(int16_t)(enc->first_platoon + (int16_t)range.platoon_start)];
        range.platoon_start = range.platoon_start + 1;
        if (state == 0) {
            return;
        }
        state->maneuver_disabled = value;
    }
}

/**
 * Behaviour of ai reference activate squads, moved unchanged from the original free function.
 *
 * @address 0x432b80
 */
void ReferenceView::activate_squads()
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none) {
        int32_t squad_filter = (packed_reference >> 0x1e == 2) ? (int32_t)(int8_t)(packed_reference >> 0x10) : -1;

        if (packed_reference >> 0x1e == 1) {
            halo::ai::encounter_spawn_squads(packed_reference & halo::k_slot_mask, (int32_t)(int8_t)(packed_reference >> 0x10),
                                               squad_filter);
            return;
        }
        halo::ai::encounter_spawn_squads(packed_reference & halo::k_slot_mask, -1, squad_filter);
    }
}

/**
 * Behaviour of ai reference actor iterator init cursor, moved unchanged from the original free function.
 *
 * @address 0x4369f0
 */
void ReferenceView::actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor)
{
    if (halo::ai::globals().state->actors_valid == 0) {
        return;
    }

    cursor[0] = (datum_index)encounter_index;
    cursor[1] = (datum_index)k_datum_index_none;

    if (encounter_index == -1) {
        cursor[2] = halo::ai::globals().state->first_encounterless_actor;
        return;
    }

    cursor[2] = ((encounter *)halo::ai::globals().encounter_data->data)[(uint32_t)encounter_index & halo::k_slot_mask].first_actor;
}

/**
 * Sets the encounter index to none (which ai_reference_actor_iterator_init_cursor then reads as "walk the
 * global unassigned-actor list") on any validation failure or unrecognized reference kind.
 *
 * @address 0x432650
 */
void ReferenceView::actor_iterator_new(ai_reference_actor_iterator *out_iterator)
{
    uint32_t packed_reference = handle;
    int32_t encounter_index = (int32_t)(packed_reference & halo::k_slot_mask);
    int32_t *field0 = (int32_t *)(out_iterator->unknown_00 + 0x00);
    int32_t *squad_filter = (int32_t *)(out_iterator->unknown_00 + 0x04);
    int32_t *platoon_filter = (int32_t *)(out_iterator->unknown_00 + 0x08);

    *field0 = encounter_index;

    if (halo::scenario::globals().scenario == 0 || halo::ai::globals().state->actors_valid == 0 ||
        encounter_index >= halo::scenario::globals().scenario->encounters.count) {
        *field0 = -1;
        return;
    }

    {
        uint32_t kind = packed_reference >> 0x1e;
        *platoon_filter = -1;
        *squad_filter = -1;

        if (kind != 0) {
            if (kind == 1) {
                *platoon_filter = (int8_t)(packed_reference >> 0x10);
            } else if (kind == 2) {
                *squad_filter = (int8_t)(packed_reference >> 0x10);
            } else {
                *field0 = -1;
                return;
            }
        }
    }

    halo::ai::ai_reference_actor_iterator_init_cursor(encounter_index, (datum_index *)((uint8_t *)out_iterator + 0xc));
}

/**
 * Behaviour of ai reference actor iterator next, moved unchanged from the original free function.
 *
 * @address 0x4326d0
 */
actor * ReferenceView::actor_iterator_next(ai_reference_actor_iterator *iterator)
{
    int32_t *squad_filter = (int32_t *)(iterator->unknown_00 + 0x04);
    int32_t *platoon_filter = (int32_t *)(iterator->unknown_00 + 0x08);
    actor *base = (actor *)halo::ai::globals().actor_data->data;

    for (;;) {
        datum_index next;
        actor *candidate;

        if (halo::ai::globals().state->actors_valid == 0) {
            return 0;
        }

        next = *(datum_index *)iterator->unknown_14;
        iterator->actor_index = next;
        if (next == (datum_index)k_datum_index_none) {
            return 0;
        }

        candidate = &base[next & halo::k_slot_mask];
        *(datum_index *)iterator->unknown_14 = candidate->next_in_encounter;

        if (*squad_filter == -1 || *squad_filter == candidate->squad_index) {
            if (*platoon_filter == -1 || *platoon_filter == candidate->platoon_index) {
                return candidate;
            }
        }
    }
}

/**
 * Returns the new list's header handle, or none if the reference is none or the header allocation failed.
 *
 * @address 0x432740
 */
datum_index ReferenceView::build_object_list()
{
    uint32_t packed_reference = handle;
    datum_index header_index = (datum_index)k_datum_index_none;

    if (packed_reference != (uint32_t)k_datum_index_none) {
        header_index = halo::memory::datum_new(halo::objects::globals().object_list_header_data);
        if (header_index != (datum_index)k_datum_index_none) {
            object_list_header *header =
                halo::hs::object_list_header_at(header_index);
            ai_reference_actor_iterator iterator;
            actor *a;

            header->count = 0;
            header->first_reference = (datum_index)k_datum_index_none;

            halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
            a = halo::ai::ai_reference_actor_iterator_next(&iterator);
            while (a != 0) {
                datum_index passenger;

                if (a->unit_index != (datum_index)k_datum_index_none) {
                    halo::hs::object_list_reference_add(header_index, a->unit_index);
                }

                passenger = a->cluster_unit_index;
                while (passenger != (datum_index)k_datum_index_none) {
                    object_header *passenger_header = &((object_header *)halo::objects::globals().object_data->data)[passenger & halo::k_slot_mask];
                    halo::hs::object_list_reference_add(header_index, passenger);
                    passenger = *(datum_index *)((uint8_t *)passenger_header->data + 0x1fc);
                }

                a = halo::ai::ai_reference_actor_iterator_next(&iterator);
            }
        }
    }

    return header_index;
}

/**
 * Behaviour of ai reference clear search target, moved unchanged from the original free function.
 *
 * @address 0x434c80
 */
void ReferenceView::clear_search_target()
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        a->try_to_fight_type = 0;
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai reference detach actors from encounters, moved unchanged from the original free
 * function.
 *
 * @address 0x4351c0
 */
void ReferenceView::detach_actors_from_encounters()
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;
    actor *self;

    if (packed_reference == halo::k_dword_none) {
        return;
    }

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        halo::ai::actor_movement_action_cancel(iterator.actor_index);
        halo::ai::encounter_remove_actor(iterator.actor_index, 0);
        if (halo::ai::globals().state->actors_valid != 0) {
            self = &((actor *)halo::ai::globals().actor_data->data)[iterator.actor_index & halo::k_slot_mask];
            self->next_in_encounter = halo::ai::globals().state->first_encounterless_actor;
            halo::ai::globals().state->first_encounterless_actor = iterator.actor_index;
            self->encounterless = 1;
            self->activation_delay = static_cast<int16_t>((uint16_t)(-(uint16_t)(self->active != 0) & 0x5a));
            halo::ai::actor_movement_action_cancel(iterator.actor_index);
        }
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
    halo::ai::encounters_recompute_dirty();
}

/**
 * Behaviour of ai reference expand to platoon range, moved unchanged from the original free function.
 *
 * @address 0x432420
 */
void ReferenceView::expand_to_platoon_range(ai_reference_platoon_range *out_range)
{
    uint32_t packed_reference = handle;
    uint32_t encounter_index = packed_reference & halo::k_slot_mask;
    ScenarioEncounter *encounter_definition;
    uint32_t kind;
    uint32_t platoon_index;

    out_range->encounter_index = (int32_t)encounter_index;

    if (halo::scenario::globals().scenario == 0 || halo::ai::globals().state->actors_valid == 0 ||
        (int32_t)halo::scenario::globals().scenario->encounters.count <= (int32_t)encounter_index) {
        out_range->encounter_index = -1;
        return;
    }

    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index];
    kind = packed_reference >> 0x1e;

    if (kind == 0) {
        out_range->platoon_start = 0;
        out_range->platoon_end = encounter_definition->platoons.count - 1;
        return;
    }
    if (kind > 2) {
        out_range->encounter_index = -1;
        return;
    }

    platoon_index = (packed_reference >> 0x10) & 0xff;
    if (kind == 2 && (int32_t)platoon_index >= encounter_definition->squads.count) {
        out_range->platoon_start = -1;
    } else {
        if (kind == 2) {
            ScenarioSquad *squad = &((ScenarioSquad *)encounter_definition->squads.pointer)[platoon_index];
            platoon_index = squad->platoon;
        }
        out_range->platoon_start = (int32_t)platoon_index;
    }

    platoon_index = out_range->platoon_start;
    if ((int32_t)platoon_index >= 0 && (int32_t)platoon_index < encounter_definition->platoons.count) {
        out_range->platoon_end = platoon_index;
        return;
    }

    out_range->encounter_index = -1;
}

/**
 * Behaviour of ai reference face starting location, moved unchanged from the original free function.
 *
 * @address 0x4349d0
 */
void ReferenceView::face_starting_location(uint8_t idle_only)
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;
    ScenarioEncounter *definition;
    ScenarioSquad *squads;
    int16_t squad_index;
    int16_t location_index;
    float facing;
    real_vector3d forward;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->unit_index != (datum_index)k_datum_index_none &&
            (idle_only == 0 ||
             (a->active_unit_index == (datum_index)k_datum_index_none &&
              halo::units::unit_test_placement_candidate(a->unit_index, global_down3d_pointer, 0, 2.0f, 0) == -1)) &&
            a->encounter_index != (datum_index)k_datum_index_none) {

            squad_index = a->squad_index;
            definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)
                [a->encounter_index & halo::k_slot_mask];
            squads = (ScenarioSquad *)definition->squads.pointer;

            location_index = halo::ai::squad_pick_random_starting_location(a->encounter_index, squad_index);
            if (location_index != -1) {
                facing = ((ScenarioActorStartingLocation *)
                    squads[squad_index].starting_locations.pointer)[location_index].facing;
                forward.k = 0.0f;
                forward.i = (float)halo::x87::fcos((double)facing);
                forward.j = (float)halo::x87::fsin((double)facing);
                halo::objects::object_set_position_and_orientation(a->unit_index, &forward, 0, 0);
                halo::objects::object_reset_velocity_and_wake(a->unit_index);
                halo::ai::actor_movement_action_stop(iterator.actor_index);
            }
        }
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai reference flee if ready, moved unchanged from the original free function.
 *
 * @address 0x434d90
 */
void ReferenceView::flee_if_ready(uint32_t readiness_param)
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        actor_mode_data mode_data;

        if (halo::ai::actor_squad_action_status_broadcast(iterator.actor_index, (int16_t)readiness_param,
                &mode_data.obey) != 0) {
            halo::ai::actor_set_mode(iterator.actor_index, halo::ai::actor_mode::obey, &mode_data);
        }
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai reference for each squad, moved unchanged from the original free function.
 *
 * @address 0x432f50
 */
void ReferenceView::for_each_squad()
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_squad_iterator iterator;
        encounter_squad_state *state;

        halo::ai::ai_reference_squad_iterator_new(packed_reference, &iterator);
        state = halo::ai::ai_reference_squad_iterator_next(&iterator);
        while (state != 0) {
            halo::ai::encounter_squad_clear_spawn_delay((datum_index)iterator.encounter_index, (int16_t)iterator.cursor);
            state = halo::ai::ai_reference_squad_iterator_next(&iterator);
        }
    }
}

/**
 * Returns 0, and leaves the out-parameters at their default (0 / untouched), for a reference this function
 * cannot resolve.
 *
 * @address 0x432f90
 */
uint32_t ReferenceView::get_stat_pair(int16_t stat_kind, int32_t *out_member_count, uint32_t *out_extra)
{
    uint32_t packed_reference = handle;
    uint32_t extra = 0;
    uint32_t result = 0;
    int32_t member_count = 0;

    if (packed_reference != (uint32_t)k_datum_index_none) {
        uint32_t kind = packed_reference >> 0x1e;
        uint32_t encounter_index = packed_reference & halo::k_slot_mask;

        if (kind == 0) {
            if ((int32_t)encounter_index < halo::scenario::globals().scenario->encounters.count) {
                encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index];
                if (stat_kind == 0) {
                    result = (uint32_t)enc->living_count;
                } else if (stat_kind == 1) {
                    result = (uint32_t)enc->swarm_count;
                } else {
                    int32_t diff = (int32_t)enc->living_count - (int32_t)enc->swarm_count;
                    result = (uint32_t)(diff & ~(diff >> 31));
                }
                member_count = enc->member_count;
                extra = halo::bit_cast<uint32_t>(enc->average_vitality);
            }
        } else if (kind == 1) {
            if ((int32_t)encounter_index < halo::scenario::globals().scenario->encounters.count) {
                encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index];
                int16_t platoon_sub_index = (int8_t)(packed_reference >> 0x10);
                if (platoon_sub_index < enc->platoon_count) {
                    encounter_platoon_state *state =
                        &halo::ai::globals().platoon_states[enc->first_platoon + platoon_sub_index];
                    if (stat_kind == 0) {
                        result = (uint32_t)state->living_count;
                        extra = *(uint32_t *)&state->average_vitality;
                        member_count = state->member_count;
                    } else if (stat_kind == 1) {
                        result = (uint32_t)state->swarm_count;
                        extra = *(uint32_t *)&state->average_vitality;
                        member_count = state->member_count;
                    } else {
                        int32_t diff;
                        extra = *(uint32_t *)&state->average_vitality;
                        member_count = state->member_count;
                        diff = (int32_t)state->living_count - (int32_t)state->swarm_count;
                        result = (uint32_t)(diff & ~(diff >> 31));
                    }
                }
            }
        } else if ((int32_t)encounter_index < halo::scenario::globals().scenario->encounters.count) {
            encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[encounter_index];
            int16_t squad_sub_index = (int8_t)(packed_reference >> 0x10);
            if (squad_sub_index < enc->squad_count) {
                encounter_squad_state *state = &halo::ai::globals().squad_states[enc->first_squad + squad_sub_index];
                if (stat_kind == 0) {
                    result = (uint32_t)state->living_count;
                    extra = (uint32_t)state->average_vitality;
                    member_count = state->member_count;
                } else if (stat_kind == 1) {
                    result = (uint32_t)state->swarm_count;
                    extra = (uint32_t)state->average_vitality;
                    member_count = state->member_count;
                } else {
                    int32_t diff;
                    extra = (uint32_t)state->average_vitality;
                    member_count = state->member_count;
                    diff = (int32_t)state->living_count - (int32_t)state->swarm_count;
                    result = (uint32_t)(diff & ~(diff >> 31));
                }
            }
        }
    }

    if (out_member_count != 0) {
        *out_member_count = member_count;
    }
    if (out_extra != 0) {
        *out_extra = extra;
    }
    return result;
}

/**
 * Behaviour of ai reference invoke squad callback 406f80, moved unchanged from the original free function.
 *
 * @address 0x434e60
 */
void ReferenceView::invoke_squad_callback_406f80()
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        halo::ai::actor_swarm_for_each_component(iterator.actor_index, 0, halo::ai::actor_obey_member_advance, 0,
            &halo::ai::actor_at(iterator.actor_index)->mode_data.obey);
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Sets timer_started on every squad of the reference (hs ai_timer_start).
 *
 * @address 0x432f10
 */
void ReferenceView::start_squad_timers()
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_squad_iterator iterator;
        encounter_squad_state *state;

        halo::ai::ai_reference_squad_iterator_new(packed_reference, &iterator);
        state = halo::ai::ai_reference_squad_iterator_next(&iterator);
        while (state != 0) {
            state->timer_started = 1;
            state = halo::ai::ai_reference_squad_iterator_next(&iterator);
        }
    }
}

/**
 * Behaviour of ai reference max activity stage, moved unchanged from the original free function.
 *
 * @address 0x435700
 */
int16_t ReferenceView::max_activity_stage()
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;
    int16_t best = 0;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        int16_t stage = (int16_t)halo::ai::ai_actor_get_activity_stage(iterator.actor_index);
        if (best <= stage) {
            best = stage;
        }
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
    return best;
}

/**
 * Behaviour of ai reference notify actors, moved unchanged from the original free function.
 *
 * @address 0x432bd0
 */
void ReferenceView::notify_actors(uint8_t flag)
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_actor_iterator iterator;
        actor *a;

        halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            halo::ai::actor_mark_units_and_release(flag, iterator.actor_index, 0);
            a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        }
    }
}

/**
 * A packed ai reference is the encounter in the low word, the kind in the top two bits and the sub-index
 * in bits 16..23: kind 1 = platoon, kind 2 = squad.
 *
 * @address 0x432c20
 */
void ReferenceView::notify_squad_index()
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none) {
        uint32_t kind = packed_reference >> 0x1e;
        int32_t sub_index = (int32_t)((packed_reference >> 0x10) & 0xff);
        int32_t squad_index = (kind == 2) ? sub_index : -1;
        int32_t platoon_index = (kind == 1) ? sub_index : -1;

        halo::ai::ai_release_actors_filtered((datum_index)(packed_reference & halo::k_slot_mask), platoon_index, squad_index, 0);
    }
}

/**
 * Returns whether the reference resolved to something other than "none"/unresolved.
 *
 * @address 0x432320
 */
uint8_t ReferenceView::parse(char *reference_string, Scenario *scenario, uint32_t *out_packed_reference)
{
    uint32_t packed = halo::k_dword_none;
    char *slash;

    if (_stricmp(reference_string, "none") == 0) {
        *out_packed_reference = halo::k_dword_none;
        return 1;
    }

    slash = strrchr(reference_string, '/');
    if (slash == 0) {
        int32_t encounter_index = halo::ai::scenario_find_encounter_index_by_name(scenario, reference_string);
        if (encounter_index != -1) {
            packed = (uint32_t)encounter_index & halo::k_slot_mask;
        }
    } else {
        int32_t name_length = (int32_t)(slash - reference_string);
        if (name_length < 0x20) {
            char encounter_name[32];
            int32_t encounter_index;

            strncpy(encounter_name, reference_string, name_length);
            encounter_name[name_length] = '\0';

            encounter_index = halo::ai::scenario_find_encounter_index_by_name(scenario, encounter_name);
            if (encounter_index != -1) {
                ScenarioEncounter *encounter_definition =
                    &((ScenarioEncounter *)scenario->encounters.pointer)[encounter_index];
                int32_t squad_index = halo::ai::encounter_definition_find_squad_index_by_name(encounter_definition, slash + 1);
                uint32_t high;

                if (squad_index != -1) {
                    high = ((uint32_t)squad_index & 0xff) | 0xffff8000;
                } else {
                    int32_t platoon_index =
                        halo::ai::encounter_definition_find_platoon_index_by_name(encounter_definition, slash + 1);
                    if (platoon_index == -1) {
                        *out_packed_reference = packed;
                        return packed != halo::k_dword_none;
                    }
                    high = ((uint32_t)platoon_index & 0xff) | 0x4000;
                }
                packed = (high << 0x10) | ((uint32_t)encounter_index & halo::k_slot_mask);
            }
        }
    }

    *out_packed_reference = packed;
    return packed != halo::k_dword_none;
}

/**
 * Behaviour of ai reference refill grenades, moved unchanged from the original free function.
 *
 * @address 0x434af0
 */
void ReferenceView::refill_grenades()
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;
    uint8_t *variant_data;
    uint8_t *unit;
    int32_t rolled;
    int16_t current;
    int16_t grenade_type;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->unit_index != (datum_index)k_datum_index_none) {
            variant_data = (uint8_t *)halo::cache::globals().tag_instances[a->actor_variant_tag & halo::k_slot_mask].data;
            unit = (uint8_t *)halo::ai::object_at(a->unit_index);

            ((unit_object *)unit)->base.body_vitality = (((unit_object *)unit)->base.maximum_body_vitality <= 0.0f) ? k_real_zero : k_real_one;
            ((unit_object *)unit)->base.shield_vitality = (((unit_object *)unit)->base.maximum_shield_vitality <= 0.0f) ? k_real_zero : k_real_one;

            if (*(int16_t *)(variant_data + 0x180) != -1) {
                halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
                rolled = (int32_t)((uint32_t)(((int32_t)(int16_t)(*(int16_t *)(variant_data + 0x1d2) + 1) -
                                     (int32_t)(int16_t)*(uint16_t *)(variant_data + 0x1d0)) *
                                    (int32_t)(halo::math::globals().random_seed_global >> 0x10)) >> 0x10) +
                         (int32_t)*(uint16_t *)(variant_data + 0x1d0);

                unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)
                    [a->unit_index & halo::k_slot_mask].data;
                current = (int16_t)((unit_object *)unit)->unit.current_grenade_index;
                if (current == -1) {
                    current = 0;
                } else {
                    current = (int16_t)*(int8_t *)(unit + 0x31e + current);
                }

                if (current < (int16_t)rolled) {
                    grenade_type = *(int16_t *)(variant_data + 0x180);
                    unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)
                        [a->unit_index & halo::k_slot_mask].data;
                    *(int8_t *)(unit + 0x31e + grenade_type) =
                        (int8_t)(*(int8_t *)(unit + 0x31e + grenade_type) +
                                 ((int8_t)rolled - (int8_t)current));
                    ((struct unit_object *)unit)->unit.desired_grenade_index = static_cast<int8_t>((uint8_t)grenade_type);
                    ((struct unit_object *)unit)->unit.current_grenade_index = static_cast<int8_t>((uint8_t)grenade_type);
                }
            }
        }
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai reference reset or wake awareness, moved unchanged from the original free function.
 *
 * @address 0x434500
 */
void ReferenceView::reset_or_wake_awareness(char flag)
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        if (flag == 0) {
            if (a->awareness_level == 0) {
                a->awareness_level = 2;
            }
        } else {
            a->awareness_level = 0;
            a->mode = 0;
            halo::ai::actor_clear_perceived_props(iterator.actor_index);
            halo::ai::actor_dispatch_perception_reset(iterator.actor_index);
            halo::ai::actor_set_units_active(iterator.actor_index, 0);
        }
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai reference resolve squad datum, moved unchanged from the original free function.
 *
 * @address 0x432c80
 */
int32_t ReferenceView::resolve_squad_datum()
{
    uint32_t packed_reference = handle;
    int32_t fallback = (int32_t)halo::ai::globals().state;

    if (halo::ai::globals().state->actors_valid != 0 && packed_reference != (uint32_t)k_datum_index_none) {
        if (packed_reference >> 0x1e == 2) {
            return (int32_t)halo::ai::encounter_squad_spawn_reinforcement(packed_reference & halo::k_slot_mask,
                (int16_t)((packed_reference >> 0x10) & 0xff));
        }
        if (packed_reference >> 0x1e == 1) {
            ScenarioEncounter *encounter_definition =
                &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[packed_reference & halo::k_slot_mask];
            int32_t squad_count = encounter_definition->squads.count;

            if (squad_count <= 0) {
                return (int32_t)((packed_reference & halo::k_slot_mask) * sizeof(ScenarioEncounter));
            }
            if (squad_count > 0) {
                ScenarioSquad *squads = (ScenarioSquad *)encounter_definition->squads.pointer;
                int32_t squad_index = 0;
                do {
                    if ((int32_t)squads[squad_index].platoon == (int32_t)((packed_reference >> 0x10) & 0xff)) {
                        if ((int16_t)squad_index == -1) {
                            return squad_index;
                        }
                        return (int32_t)halo::ai::encounter_squad_spawn_reinforcement(packed_reference & halo::k_slot_mask,
                            (int16_t)squad_index);
                    }
                    squad_index = squad_index + 1;
                } while (squad_index < squad_count);
                return (int32_t)squads;
            }
        }
    }
    return fallback;
}

/**
 * Behaviour of ai reference respawn all players, moved unchanged from the original free function.
 *
 * @address 0x432d90
 */
void ReferenceView::respawn_all_players()
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none) {
        data_iterator iterator;
        player *p;

        iterator.data = halo::game::globals().player_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

        p = (player *)halo::memory::data_iterator_next(&iterator);
        while (p != 0) {
            halo::ai::ai_reference_respawn_member(packed_reference, p->unit);
            p = (player *)halo::memory::data_iterator_next(&iterator);
        }
    }
}

/**
 * Behaviour of ai reference respawn member, moved unchanged from the original free function.
 *
 * @address 0x432df0
 */
void ReferenceView::respawn_member(datum_index unit_index)
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none && unit_index != (datum_index)k_datum_index_none) {
        ai_reference_actor_iterator iterator;
        actor *a;

        halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            datum_index respawned;

            if (a->encounter_index != (datum_index)k_datum_index_none) {
                encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[a->encounter_index & halo::k_slot_mask];
                enc->activation_delay = 0x96;
                halo::ai::encounter_activate(a->encounter_index);
            }

            respawned = halo::ai::actor_find_or_create_shared_prop(unit_index, iterator.actor_index, 1, 0);
            if (respawned != (datum_index)k_datum_index_none) {
                halo::ai::actor_squad_react_to_grenade(iterator.actor_index, respawned, 3);
            }

            a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        }
    }
}

/**
 * Behaviour of ai reference respawn placed members, moved unchanged from the original free function.
 *
 * @address 0x432d30
 */
void ReferenceView::respawn_placed_members(uint32_t respawn_reference)
{
    uint32_t packed_reference = handle;
    if (respawn_reference != (uint32_t)k_datum_index_none && packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_actor_iterator iterator;
        actor *a;

        halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            datum_index unit_index = a->unit_index;
            if (unit_index == (datum_index)k_datum_index_none) {
                unit_index = a->cluster_unit_index;
            }
            if (unit_index != (datum_index)k_datum_index_none) {
                halo::ai::ai_reference_respawn_member(respawn_reference, unit_index);
            }
            a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        }
    }
}

/**
 * Behaviour of ai reference set combat alert flag, moved unchanged from the original free function.
 *
 * @address 0x435af0
 */
void ReferenceView::set_combat_alert_flag(uint8_t new_flag)
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_actor_iterator iterator;
        actor *a;

        halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            halo::ai::actor_set_combat_alert_flag(iterator.actor_index, new_flag);
            a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        }
    }
}

/**
 * Behaviour of ai reference set search target area, moved unchanged from the original free function.
 *
 * @address 0x434d00
 */
void ReferenceView::set_search_target_area()
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        a->try_to_fight_type = 2;
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Behaviour of ai reference set search target point, moved unchanged from the original free function.
 *
 * @address 0x434cc0
 */
void ReferenceView::set_search_target_point(uint32_t reference_value)
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        a->try_to_fight_type = 1;
        a->try_to_fight_reference = reference_value;
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Stores dormancy_disabled = (flag == 0) on every squad of the reference (hs ai_allow_dormant).
 *
 * @address 0x435bc0
 */
void ReferenceView::set_squads_dormancy_allowed(char flag)
{
    uint32_t packed_reference = handle;
    ai_reference_squad_iterator iterator;
    encounter_squad_state *state;
    uint8_t value = (flag == 0);

    halo::ai::ai_reference_squad_iterator_new(packed_reference, &iterator);
    state = halo::ai::ai_reference_squad_iterator_next(&iterator);
    while (state != 0) {
        state->dormancy_disabled = value;
        state = halo::ai::ai_reference_squad_iterator_next(&iterator);
    }
}

/**
 * Stores charge_disallowed = (flag == 0) on every actor of the reference (hs ai_allow_charge).
 *
 * @address 0x434d40
 */
void ReferenceView::set_charge_allowed(char flag)
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;
    uint8_t value = (flag == 0);

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        a->charge_disallowed = value;
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Does nothing if the AI globals, the reference or the resolved squad/actor-variant/actor tag chain is not
 * valid.
 *
 * @address 0x4328c0
 */
void ReferenceView::spawn_starting_location_object(datum_index unit_index, uint32_t packed_reference)
{
    uint32_t encounter_index;
    ScenarioEncounter *encounter_definition;
    uint32_t squad_index;
    uint32_t kind;

    if (halo::ai::globals().state->actors_valid == 0 || packed_reference == (uint32_t)k_datum_index_none ||
        unit_index == (datum_index)k_datum_index_none) {
        return;
    }

    encounter_index = packed_reference & halo::k_slot_mask;
    if ((int32_t)encounter_index >= halo::scenario::globals().scenario->encounters.count) {
        return;
    }
    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index];

    squad_index = 0;
    kind = packed_reference >> 0x1e;
    if (kind == 2) {
        uint32_t requested = (packed_reference >> 0x10) & 0xff;
        squad_index = requested; // requested is always 0..255, so the original's "< 0" guard never fires
    } else if (kind == 1) {
        uint32_t target_platoon = (packed_reference >> 0x10) & 0xff;
        uint32_t candidate = 0;
        if (encounter_definition->squads.count > 0) {
            ScenarioSquad *squads = (ScenarioSquad *)encounter_definition->squads.pointer;
            do {
                if (squads[candidate].platoon == target_platoon) {
                    squad_index = candidate;
                    break;
                }
                candidate = candidate + 1;
            } while (candidate < (uint32_t)encounter_definition->squads.count);
        }
    }

    if ((int32_t)squad_index < encounter_definition->squads.count) {
        ScenarioSquad *squad = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];
        int16_t actor_palette_index = (int16_t)squad->actor_type;

        if (actor_palette_index != -1) {
            TagDependency *actor_palette_entry =
                &((TagDependency *)halo::scenario::globals().scenario->actor_palette.pointer)[actor_palette_index];
            datum_index actor_variant_tag = *(datum_index *)&actor_palette_entry->tag_id;

            if (actor_variant_tag != (datum_index)k_datum_index_none) {
                uint8_t *actor_variant_data =
                    (uint8_t *)halo::cache::globals().tag_instances[actor_variant_tag & halo::k_slot_mask].data;
                datum_index actor_definition_tag = *(datum_index *)(actor_variant_data + 0x10);

                if (actor_definition_tag != (datum_index)k_datum_index_none) {
                    Actor *actor_tag_data = halo::ai::tag_data<Actor>(actor_definition_tag);
                    uint32_t actor_tag_flags = actor_tag_data->flags;
                    char reuse_existing = (char)((actor_tag_flags >> 0x1a) & 1); // Actor.flags bit 26, "swarm"
                    char start_active =
                        (char)((encounter_definition->flags >> 4) & 1); // ScenarioEncounterFlags bit 4, "initially_braindead"

                    halo::ai::actor_new_and_attach_to_unit(reuse_existing, unit_index, actor_variant_tag, encounter_index,
                        (int16_t)squad_index, 0, (datum_index)k_datum_index_none, start_active,
                        (uint16_t)squad->initial_state, (int16_t)squad->return_state, halo::k_word_none, 0);
                    halo::ai::encounters_recompute_dirty();
                }
            }
        }
    }
}

/**
 * Behaviour of ai reference squad iterator new, moved unchanged from the original free function.
 *
 * @address 0x4324f0
 */
void ReferenceView::squad_iterator_new(ai_reference_squad_iterator *out_iterator)
{
    uint32_t packed_reference = handle;
    uint32_t encounter_index = packed_reference & halo::k_slot_mask;
    ScenarioEncounter *encounter_definition;
    uint32_t kind;

    out_iterator->encounter_index = (int32_t)encounter_index;

    if (halo::scenario::globals().scenario == 0 || halo::ai::globals().state->actors_valid == 0 ||
        (int32_t)halo::scenario::globals().scenario->encounters.count <= (int32_t)encounter_index) {
        out_iterator->encounter_index = -1;
        return;
    }

    encounter_definition = &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[encounter_index];
    kind = packed_reference >> 0x1e;

    if (kind < 2) {
        out_iterator->cursor = -1;
        out_iterator->squad_start = 0;
        out_iterator->squad_end = encounter_definition->squads.count - 1;
        if (kind != 0) {
            out_iterator->platoon_filter = (int8_t)(packed_reference >> 0x10);
            return;
        }
    } else {
        uint32_t squad_index = (int8_t)(packed_reference >> 0x10);
        if (kind != 2 || (int32_t)squad_index >= encounter_definition->squads.count) {
            out_iterator->encounter_index = -1;
            return;
        }
        out_iterator->cursor = -1;
        out_iterator->squad_end = (int32_t)squad_index;
        out_iterator->squad_start = (int32_t)squad_index;
    }
    out_iterator->platoon_filter = -1;
}

/**
 * Behaviour of ai reference squad iterator next, moved unchanged from the original free function.
 *
 * @address 0x4325b0
 */
encounter_squad_state * ReferenceView::squad_iterator_next(ai_reference_squad_iterator *iterator)
{
    encounter *enc;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squads;

    if (iterator->encounter_index == -1) {
        return 0;
    }

    enc = &((encounter *)halo::ai::globals().encounter_data->data)[iterator->encounter_index & halo::k_slot_mask];
    encounter_definition =
        &((ScenarioEncounter *)halo::scenario::globals().scenario->encounters.pointer)[iterator->encounter_index & halo::k_slot_mask];
    squads = (ScenarioSquad *)encounter_definition->squads.pointer;

    if (iterator->squad_start > iterator->squad_end) {
        return 0;
    }

    for (;;) {
        iterator->cursor = iterator->squad_start;
        iterator->squad_start = iterator->squad_start + 1;
        if (iterator->platoon_filter == -1 || squads[iterator->cursor].platoon == iterator->platoon_filter) {
            break;
        }
        if (iterator->squad_end < iterator->squad_start) {
            return 0;
        }
    }

    return &halo::ai::globals().squad_states[enc->first_squad + iterator->cursor];
}

/**
 * Stores the value as automatic_migration on every squad of the reference (hs ai_automatic_migration_target).
 *
 * @address 0x435ab0
 */
void ReferenceView::squad_set_automatic_migration(uint8_t value)
{
    uint32_t packed_reference = handle;
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_squad_iterator iterator;
        encounter_squad_state *state;

        halo::ai::ai_reference_squad_iterator_new(packed_reference, &iterator);
        state = halo::ai::ai_reference_squad_iterator_next(&iterator);
        while (state != 0) {
            state->automatic_migration = value;
            state = halo::ai::ai_reference_squad_iterator_next(&iterator);
        }
    }
}

namespace {

static void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = halo::ai::object_bytes(object_index);
    unit_object *vehicle = (unit_object *)halo::ai::object_bytes(vehicle_index);
    uint8_t *nodes = self + ((struct object *)self)->nodes.offset;
    uint8_t *seat = *(uint8_t **)(halo::ai::tag_bytes(vehicle->base.definition_tag) + 0x2e8) + *(int16_t *)(self + 0x2f0) * 0x11c;
    uint8_t *model_nodes;
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    halo::objects::object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &marker, 1);
    offset.x = *(float *)(nodes + 0x28) - marker.node_transform.position.x;
    offset.y = *(float *)(nodes + 0x2c) - marker.node_transform.position.y;
    offset.z = *(float *)(nodes + 0x30) - marker.node_transform.position.z;
    model_nodes = *(uint8_t **)(halo::ai::tag_bytes(*(datum_index *)(halo::ai::tag_bytes(*(datum_index *)self) + 0x34)) + 0xbc);
    default_translation = *(real_point3d *)(model_nodes + 0x28);
    if (((vehicle_object *)vehicle)->unit.driver_unit_index == object_index && vehicle->unit.animation_state != 0x25 &&
        ((struct object *)self)->parent_object != k_datum_index_none) {
        halo::units::unit_try_set_animation_state(((struct object *)self)->parent_object, 0x25);
    }
    *(datum_index *)(self + 0x32c) = vehicle_index;
    *(int32_t *)(self + 0x330) = halo::game::globals().game_time->game_time;
    if (*(datum_index *)(self + 0x324) == object_index) {
        *(datum_index *)(self + 0x324) = k_datum_index_none;
    }
    if (*(datum_index *)(self + 0x328) == object_index) {
        *(datum_index *)(self + 0x328) = k_datum_index_none;
    }
    halo::objects::object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + ((struct object *)self)->position.x;
    position.y = offset.y + ((struct object *)self)->position.y;
    position.z = offset.z + ((struct object *)self)->position.z - default_translation.z;
    halo::objects::object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = halo::ai::object_bytes(object_index);

        halo::math::matrix4x3_multiply((real_matrix4x3 *)(reloaded + ((struct object *)reloaded)->nodes.offset),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)&((struct object *)self)->forward.i = basis.forward;
    *(real_vector3d *)&((struct object *)self)->up.i = basis.up;
    {
        unit_object *object = (unit_object *)halo::ai::object_bytes(object_index);
        uint8_t *object_tag = halo::ai::tag_bytes(object->base.definition_tag);

        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1 && (static_cast<uint8_t>(object->base.flags) & 1) != 0) {
            halo::objects::object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
            ((struct object *)object)->flags &= ~halo::to_bits(halo::objects::object_flag::no_collision);
            halo::ai::object_header_at(object_index).flags |= 2;
        }
    }
    *(int16_t *)(self + 0x2f0) = -1;
    self[0x2a7] = 2;
    if (((vehicle_object *)vehicle)->unit.driver_unit_index == object_index) {
        ((vehicle_object *)vehicle)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((vehicle_object *)vehicle)->unit.gunner_unit_index == object_index) {
        ((vehicle_object *)vehicle)->unit.gunner_unit_index = k_datum_index_none;
    }
    halo::units::unit_recompute_seat_occupants(vehicle_index);
    halo::units::unit_pick_and_ready_next_weapon(object_index);
    {
        int8_t request[2] = { 0x14, 0 };

        halo::units::unit_update_animation_state_machine(object_index, request);
    }
    *(real_point3d *)(self + ((struct object *)self)->node_function_values.offset + 0x10) = default_translation;
    if (((struct object *)self)->type == 0) {
        halo::units::unit_reset_orientation_and_find_position(object_index, vehicle_index); // EDI = the seat parent
    }
    halo::objects::object_recalculate_bounding_radius_recursive(object_index);
    if (halo::units::unit_all_seats_unoccupied(vehicle_index) == 1) {
        uint8_t *empty = (uint8_t *)halo::objects::object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = halo::game::globals().game_time->game_time;
        }
    }
    if (halo::networking::globals().game_mode == 1) {
        uint8_t *player = (uint8_t *)halo::memory::datum_get(*(datum_index *)(self + 0x218), halo::game::globals().player_data);

        if (player != 0 && ((struct player *)player)->local_player_index == -1) {
            ((struct player *)player)->position_updates.read_index = 0;
            ((struct player *)player)->position_updates.write_index = 0;
            ((struct player *)player)->vehicle_updates.read_index = 0;
            ((struct player *)player)->vehicle_updates.write_index = 0;
        }
    }
}

static void biped_free_local_player_history(uint8_t *self)
{
    datum_index player_index = *(datum_index *)(self + 0x218);
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    uint8_t *player;

    if (halo::networking::globals().game_mode != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= *(int16_t *)((uint8_t *)halo::game::globals().player_data + 0x20)) {
        return;
    }
    player = (uint8_t *)halo::game::globals().player_data->data + *(int16_t *)((uint8_t *)halo::game::globals().player_data + 0x22) * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || ((struct player *)player)->local_player_index == -1) {
        return;
    }
    if (halo::networking::globals().client != 0) {
        halo::networking::player_update_history_free_all((player_update_history *)(*(void **)&halo::networking::globals().client->update_history));
    }
}

}

/**
 * Behaviour of ai reference units exit vehicles, moved unchanged from the original free function.
 *
 * @address 0x433ea0
 */
void ReferenceView::units_exit_vehicles()
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *actor_record;

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    for (actor_record = halo::ai::ai_reference_actor_iterator_next(&iterator); actor_record != 0;
         actor_record = halo::ai::ai_reference_actor_iterator_next(&iterator)) {
        datum_index unit_index = actor_record->unit_index;
        int16_t index = (int16_t)unit_index;
        int16_t salt = (int16_t)(unit_index >> 16);
        uint8_t *header;
        uint8_t *self;
        datum_index vehicle_index;

        if (actor_record->active_unit_index == k_datum_index_none ||
            unit_index == k_datum_index_none || index < 0 || index >= halo::objects::globals().object_data->maximum_count) {
            continue;
        }
        header = (uint8_t *)halo::objects::globals().object_data->data + halo::objects::globals().object_data->size * index;
        if (*(int16_t *)header == 0 || (salt != 0 && *(int16_t *)header != salt) ||
            ((1u << (header[3] & 0x1f)) & 3) == 0) {
            continue;
        }
        self = *(uint8_t **)(header + 0x8);
        if (self == 0 || halo::networking::globals().game_mode == 1 ||
            (vehicle_index = ((struct object *)self)->parent_object) == k_datum_index_none ||
            *(int16_t *)(self + 0x2f0) == -1) {
            continue;
        }
        if (((struct object *)self)->type == 1) {
            uint8_t *me = halo::ai::object_bytes(unit_index);

            if (((struct object *)me)->parent_object != k_datum_index_none && *(int16_t *)(me + 0x2f0) != -1) {
                biped_detach_from_seat(unit_index, ((struct object *)me)->parent_object);
            }
            biped_free_local_player_history(me);
        } else if (!halo::units::unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
            uint8_t *self_tag = halo::ai::tag_bytes(*(datum_index *)self);
            datum_index graph = *(datum_index *)(self_tag + 0x44);
            uint8_t *seat_block = *(uint8_t **)(halo::ai::tag_bytes(graph) + 0x10) + (int8_t)self[0x2a0] * 0x64;

            if (*(int32_t *)(seat_block + 0x40) > 8 && (*(int16_t **)(seat_block + 0x44))[8] != -1) {
                int16_t exit_animation = (*(int16_t **)(seat_block + 0x44))[8];
                unit_object *object;
                uint8_t *object_tag;

                if (*(datum_index *)(halo::ai::object_bytes(vehicle_index) + 0x324) == unit_index) {
                    halo::units::unit_notify_weapon_removed((int32_t)vehicle_index);
                }
                halo::units::unit_set_custom_animation(unit_index, *(datum_index *)(self_tag + 0x44),
                    halo::models::animation_choose_random_permutation(graph, exit_animation, static_cast<animation_random_stream>(1)));
                object = (unit_object *)halo::ai::object_bytes(unit_index);
                object_tag = halo::ai::tag_bytes(object->base.definition_tag);
                if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                    if ((static_cast<uint8_t>(object->base.flags) & 1) != 0) {
                        halo::objects::object_for_each_light_attachment(unit_index, 0, 1);
                    }
                    if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                        ((struct object *)object)->flags &= ~halo::to_bits(halo::objects::object_flag::no_collision);
                        halo::ai::object_header_at(unit_index).flags |= 2;
                    }
                }
                self[0x2a3] = 0x1b;
                halo::ai::actor_notify_weapon_pickup_once(unit_index);
                if (((struct object *)self)->network_role == 0) {
                    halo::units::unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
                }
            }
        }
    }
}


/**
 * Behaviour of squad members assign team and request order, moved unchanged from the original free
 * function.
 *
 * @address 0x435590
 */
void ReferenceView::assign_team_and_request_order(int16_t value)
{
    uint32_t packed_reference = handle;
    ai_reference_actor_iterator iterator;
    actor *a;
    int16_t grade;

    if (value < 0 || 0xc <= value) {
        return;
    }

    halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        grade = actor_mode_definitions[
            ((actor *)halo::ai::globals().actor_data->data)[iterator.actor_index & halo::k_slot_mask].mode].combat_grade;
        a->standing_order_request = value;
        if (a->combat_status == 0 && (grade == 0 || grade == 1 || grade == 2)) {
            halo::ai::actor_process_order_request(iterator.actor_index, (uint16_t)halo::k_dword_none);
        }
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
    }
}

/**
 * Behaviour of squad members request order, moved unchanged from the original free function.
 *
 * @address 0x435630
 */
void ReferenceView::request_order(int16_t order_code)
{
    uint32_t packed_reference = handle;
    if (order_code >= 0 && order_code < 0xc) {
        ai_reference_actor_iterator iterator;
        actor *a;

        halo::ai::ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            halo::ai::actor_process_order_request(iterator.actor_index, (uint16_t)order_code);
            a = halo::ai::ai_reference_actor_iterator_next(&iterator);
        }
    }
}


}
