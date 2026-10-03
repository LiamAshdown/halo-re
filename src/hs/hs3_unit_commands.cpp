#include "halo/hs/hs3_commands.hpp"
#include "units.h"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"

extern "C" {
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
}

namespace halo::hs::part3 {

/**
 * Evaluate handler of the hs script function `unit_aim_without_turning`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47bcb0
 */
void UnitCommands::evaluate_unit_aim_without_turning(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[arguments[0] & halo::k_slot_mask].data;

            if ((uint8_t)arguments[1]) {
                ((unit_object *)unit)->unit.flags |= 0x4000;
            } else {
                ((unit_object *)unit)->unit.flags &= ~0x4000u;
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_can_blink`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47b810
 */
void UnitCommands::evaluate_unit_can_blink(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[arguments[0] & halo::k_slot_mask].data;

            if (!(uint8_t)arguments[1]) {
                ((unit_object *)unit)->unit.flags |= 0x400000;
            } else {
                ((unit_object *)unit)->unit.flags &= ~0x400000u;
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_close`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47b8f0
 */
void UnitCommands::evaluate_unit_close(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if ((datum_index)arguments[0] != k_datum_index_none) {
        halo::units::unit_try_set_animation_state((uint32_t)arguments[0], 0x26);
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_custom_animation_at_frame`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47bbb0
 */
void UnitCommands::evaluate_unit_custom_animation_at_frame(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)halo::units::unit_set_custom_animation_frame((uint32_t)arguments[0], *(uint8_t *)&arguments[3],
        (datum_index)arguments[1], (const char *)arguments[2], *(int16_t *)&arguments[4]), thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_doesnt_drop_items`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c650
 */
void UnitCommands::evaluate_unit_doesnt_drop_items(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::units::unit_mark_zone_occupants_flag((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * 0x569d40
 *
 * @address 0x47be40
 */
void UnitCommands::evaluate_unit_enter_vehicle(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::units::unit_detach_and_enter_named_seat((uint32_t)arguments[0], (uint32_t)arguments[1], (char *)arguments[2]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_exit_vehicle`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47bfa0
 */
void UnitCommands::evaluate_unit_exit_vehicle(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::units::unit_try_exit_controlled_seat((uint32_t)arguments[0]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_get_current_flashlight_state`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c830
 */
void UnitCommands::evaluate_unit_get_current_flashlight_state(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t on = 0;

        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[arguments[0] & halo::k_slot_mask].data;

            on = (uint8_t)((((unit_object *)unit)->unit.flags >> 0x13) & 1);
        }
        hs_thread_return((int32_t)(uint8_t)(on), thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_get_custom_animation_time`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47ba00
 */
void UnitCommands::evaluate_unit_get_custom_animation_time(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint16_t)halo::units::unit_get_custom_animation_time_remaining((uint32_t)arguments[0]), thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_get_health`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c400
 */
void UnitCommands::evaluate_unit_get_health(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t *object = (uint8_t *)halo::objects::object_try_and_get((datum_index)arguments[0], halo::k_dword_none);
    float result = -1.0f;

    if (object != 0) {
        result = (object[0x106] & 4) ? 0.0f : *(float *)(object + 0xe0);
    }
    hs_thread_return(*(int32_t *)&result, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_get_shield`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c480
 */
void UnitCommands::evaluate_unit_get_shield(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t *object = (uint8_t *)halo::objects::object_try_and_get((datum_index)arguments[0], halo::k_dword_none);
    float result = -1.0f;

    if (object != 0) {
        result = (object[0x106] & 4) ? 0.0f : *(float *)(object + 0xe4);
    }
    hs_thread_return(*(int32_t *)&result, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_get_total_grenade_count`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c500
 */
void UnitCommands::evaluate_unit_get_total_grenade_count(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    uint8_t *unit = (uint8_t *)halo::objects::object_try_and_get((datum_index)arguments[0], 3);
    int16_t total = 0;

    if (unit != 0) {
        total = (int16_t)((int8_t)unit[0x31e] + (int8_t)unit[0x31f]);
    }
    hs_thread_return((int32_t)(uint16_t)total, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_has_weapon`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c580
 */
void UnitCommands::evaluate_unit_has_weapon(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t has = 0;

        if (arguments[0] != -1 && arguments[1] != -1) {
            has = halo::units::unit_has_weapon_of_type((uint32_t)arguments[0], arguments[1]);
        }
        hs_thread_return((int32_t)(uint8_t)(has), thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_unit_has_weapon_readied`: reads its typed arguments
 * from the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c5f0
 */
void UnitCommands::evaluate_unit_has_weapon_readied(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t readied = halo::units::unit_current_weapon_is_type((uint32_t)arguments[0], (datum_index)arguments[1]);
        hs_thread_return((int32_t)readied, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_unit_impervious`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c690
 */
void UnitCommands::evaluate_unit_impervious(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_set_unit_flag_800000((datum_index)arguments[0], *(char *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_is_playing_custom_animation`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47bc20
 */
void UnitCommands::evaluate_unit_is_playing_custom_animation(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t playing = 0;

        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[arguments[0] & halo::k_slot_mask].data;

            playing = (uint8_t)(unit[0x2a3] == 0x1c);
        }
        hs_thread_return((int32_t)(uint8_t)(playing), thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_kill_silent`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47b9a0
 */
void UnitCommands::evaluate_unit_kill_silent(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[arguments[0] & halo::k_slot_mask].data;

        unit[0x106] |= 0x40;
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_open`: reads its typed arguments from the calling thread and
 * hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47b8a0
 */
void UnitCommands::evaluate_unit_open(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    if ((datum_index)arguments[0] != k_datum_index_none) {
        halo::units::unit_try_set_animation_state((uint32_t)arguments[0], 0x25);
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_set_current_vitality`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c0b0
 */
void UnitCommands::evaluate_unit_set_current_vitality(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::units::unit_update_vitality_fractions((uint32_t)arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_set_desired_flashlight_state`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c7a0
 */
void UnitCommands::evaluate_unit_set_desired_flashlight_state(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if (arguments[0] != -1) {
            uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[arguments[0] & halo::k_slot_mask].data;

            ((unit_object *)unit)->unit.flags |= (uint8_t)arguments[1] ? 0x10000000 : 0x20000000;
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_set_emotion`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47bd40
 */
void UnitCommands::evaluate_unit_set_emotion(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];

    if (unit != k_datum_index_none) {
        *(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (unit & halo::k_slot_mask) * 0xc + 8) + 0x2a8) = *(uint8_t *)&arguments[1];
        halo::objects::object_copy_default_node_transforms(unit, 6);
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_set_emotion_animation`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47bf50
 */
void UnitCommands::evaluate_unit_set_emotion_animation(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::units::unit_scripting_set_emotion_animation((uint32_t)arguments[0], (const char *)arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_unit_set_enterable_by_player`: reads its typed
 * arguments from the calling thread and hands the result back through the thread, exactly as the original
 * handler did.
 *
 * @address 0x47bdb0
 */
void UnitCommands::evaluate_unit_set_enterable_by_player(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[arguments[0] & halo::k_slot_mask].data;

            if (*(uint8_t *)&arguments[1] == 0) {
                ((unit_object *)unit)->unit.flags |= 0x10000;
            } else {
                ((unit_object *)unit)->unit.flags &= 0xfffeffff;
            }
        }
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_set_maximum_vitality`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47bfe0
 */
void UnitCommands::evaluate_unit_set_maximum_vitality(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];
    float body = *(float *)&arguments[1];
    float shield = *(float *)&arguments[2];

    if (unit != k_datum_index_none &&
        (*(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (unit & halo::k_slot_mask) * 0xc + 8) + 0x106) & 4) == 0) {
        halo::objects::object_initialize_shield_stun_thresholds(unit, &body, &shield);
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_set_seat`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c260
 */
void UnitCommands::evaluate_unit_set_seat(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];

    if (unit != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (unit & halo::k_slot_mask) * 0xc + 8);

        object[0x20f] = (uint8_t)halo::units::unit_base_animation_state_from_name((const char *)arguments[1]);
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_unit_solo_player_integrated_night_vision_is_active`:
 * reads its typed arguments from the calling thread and hands the result back through the thread, exactly as the
 * original handler did.
 *
 * @address 0x47c730
 */
void UnitCommands::evaluate_unit_solo_player_integrated_night_vision_is_active(int16_t function_index, uint32_t thread_index, char first) const
{
    (void)function_index;
    (void)first;
    uint8_t active = halo::units::unit_local_player_weapon_flag_check();
    hs_thread_return((int32_t)active, thread_index);
}

/**
 * Evaluate handler of the hs script function `unit_stop_custom_animation`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47ba60
 */
void UnitCommands::evaluate_unit_stop_custom_animation(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    datum_index unit = (datum_index)arguments[0];

    if (unit != k_datum_index_none &&
        *(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (unit & halo::k_slot_mask) * 0xc + 8) + 0x2a3) == 0x1c) {
        halo::units::unit_try_set_animation_state(unit, 0);
    }
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `unit_suspended`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c6e0
 */
void UnitCommands::evaluate_unit_suspended(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::units::unit_reset_velocity_and_ground_flag((uint32_t)arguments[0], *(uint8_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `units_set_current_vitality`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c100
 */
void UnitCommands::evaluate_units_set_current_vitality(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::ai::ai_object_list_update_vitality_fractions((datum_index)arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_units_set_desired_flashlight_state`: reads its typed
 * arguments from the calling thread and hands the result back through the thread, exactly as the original
 * handler did.
 *
 * @address 0x47c750
 */
void UnitCommands::evaluate_units_set_desired_flashlight_state(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::units::unit_mark_zone_list_alt_flag((uint32_t)arguments[0], *(uint8_t *)&arguments[1]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `units_set_maximum_vitality`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c060
 */
void UnitCommands::evaluate_units_set_maximum_vitality(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::ai::ai_object_list_initialize_shield_stun_thresholds((datum_index)arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
        hs_thread_return(0, thread_index);
    }
}

}
