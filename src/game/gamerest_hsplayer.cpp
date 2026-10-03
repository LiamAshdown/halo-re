#include "halo/game/gamerest_hsplayer.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include <stdint.h>
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/api.hpp"


namespace halo::game {

/**
 * (vehicle_gunner <unit>): the unit's gunner, or none.
 *
 * @address 0x47c3a0
 */
void HsPlayerFunctions::vehicle_gunner_evaluate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        object *unit_obj = halo::objects::object_try_and_get((datum_index)arguments[0], _object_mask_unit);
        datum_index gunner = (datum_index)halo::k_dword_none;

        if (unit_obj != 0) {
            gunner = (halo::game::unit_data_of(unit_obj))->gunner_unit_index;
        }
        halo::hs::hs_thread_return((int32_t)gunner, thread_index);
    }
}

/**
 * (vehicle_test_seat <vehicle> <string> <unit>): whether the unit sits in the vehicle's seat with
 * that label.
 *
 * @address 0x47bef0
 */
void HsPlayerFunctions::vehicle_test_seat_evaluate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t seated = halo::units::unit_is_child_seated_at_named_marker((datum_index)arguments[0],
            (char *)((const char *)(uintptr_t)(uint32_t)arguments[1]), (datum_index)arguments[2]);
        halo::hs::hs_thread_return((int32_t)seated, thread_index);
    }
}

/**
 * blam-cc: EAX -> definition, ECX -> first, stack -> index, thread_index
 * Evaluates this builtin's one object argument; once ready, forwards it to unit_build_seat_occupant_zone_list and
 * returns its result from the HS thread.
 *
 * @address 0x47c310
 */
void HsPlayerFunctions::camo_screen_effect(int16_t index, uint32_t thread_index, hs_function_definition *definition, char first)
{
    int32_t *args = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);
    (void)index;

    if (args != 0) {
        int32_t result = halo::units::unit_build_seat_occupant_zone_list(args[0]);
        halo::hs::hs_thread_return(result, thread_index);
    }
}

/**
 * blam-cc: EAX -> definition, ECX -> first, stack -> index, thread_index
 * Evaluates this builtin's one (object, boolean) argument pair; once both are ready, and unless
 * the object argument is -1, sets or clears object::vitality_flags bit 0x0100 on it according to
 * the boolean, then returns from the HS thread (result unused, i.e. void).
 *
 * @address 0x47b140
 */
void HsPlayerFunctions::examine_nearby_vehicle(int16_t index, uint32_t thread_index, hs_function_definition *definition, char first)
{
    int32_t *args = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);
    (void)index;

    if (args == 0) {
        return;
    }
    if (args[0] != (int32_t)halo::k_dword_none) {
        object *target = (object *)halo::game::object_at(args[0]);
        if ((char)args[1] != 0) {
            *((uint8_t *)&target->vitality_flags + 1) |= 0x01;
            halo::hs::hs_thread_return(0, thread_index);
            return;
        }
        *((uint8_t *)&target->vitality_flags + 1) &= 0xfe;
    }
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluates function_index's one object argument; once ready, and unless it is -1, sets
 * object::vitality_flags bit 0x2000 on it (byte 0x106 bit 0x20) and returns from the HS thread.
 *
 * @address 0x47b940
 */
void HsPlayerFunctions::set_action_result(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *args = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (args != 0) {
        object *target = (object *)halo::game::object_at(args[0]);
        *((uint8_t *)&target->vitality_flags) |= 0x20;
        halo::hs::hs_thread_return(0, thread_index);
    }
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::HsPlayerFunctions::vehicle_gunner_evaluate; forwards to the C++ implementation.
 * register convention: the hs_function_definition::evaluate shape, all on the stack.
 * // blam-cc: stack -> (function_index, thread_index, first)
 * blam-cc: EAX value, ECX thread_index
 * blam-cc: ECX object_index
 *
 * @address 0x47c3a0
 */
void hs_vehicle_gunner_evaluate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::game::HsPlayerFunctions::vehicle_gunner_evaluate(function_index, thread_index, first);
}

/**
 * C entry point for halo::game::HsPlayerFunctions::vehicle_test_seat_evaluate; forwards to the C++ implementation.
 * register convention: the hs_function_definition::evaluate shape, all on the stack.
 * // blam-cc: stack -> (function_index, thread_index, first)
 * blam-cc: EAX value, ECX thread_index
 * blam-cc: stack (unit_index, seat_label), EBX child_object_index
 *
 * @address 0x47bef0
 */
void hs_vehicle_test_seat_evaluate(int16_t function_index, uint32_t thread_index, char first)
{
    halo::game::HsPlayerFunctions::vehicle_test_seat_evaluate(function_index, thread_index, first);
}

/**
 * C entry point for halo::game::HsPlayerFunctions::set_action_result; forwards to the C++ implementation.
 * register convention: none -- all three are genuine stack parameters.
 *
 * @address 0x47b940
 */
void player_set_action_result(int16_t function_index, uint32_t thread_index, char first)
{
    halo::game::HsPlayerFunctions::set_action_result(function_index, thread_index, first);
}

}
