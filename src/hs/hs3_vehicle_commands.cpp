#include "halo/ai/records.hpp"
#include "halo/hs/hs3_commands.hpp"
#include "units.h"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"


namespace halo::hs::part3 {

/**
 * Evaluate handler of the hs script function `vehicle_driver`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c340
 */
void VehicleCommands::evaluate_vehicle_driver(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t *unit = (uint8_t *)halo::objects::object_try_and_get((datum_index)arguments[0], 3);

        halo::hs::hs_thread_return(unit != 0 ? *(int32_t *)&((unit_object *)unit)->unit.driver_unit_index : -1, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_vehicle_hover`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x4801c0
 */
void VehicleCommands::evaluate_vehicle_hover(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        datum_index vehicle = (datum_index)arguments[0];
        uint8_t hover = *(uint8_t *)&arguments[1];

        if (vehicle != k_datum_index_none) {
            uint8_t *obj = (uint8_t *)halo::ai::object_at(vehicle);

            if (hover != 0) {
                halo::objects::object_get_position((real_point3d *)(obj + 0x4fc), vehicle);
                obj[0x4cc] |= 2;
            } else {
                obj[0x4cc] &= 0xfd;
            }
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * 0x56a4c0, blam-cc: stack, stack, EAX
 *
 * @address 0x47c150
 */
void VehicleCommands::evaluate_vehicle_load_magic(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)(uint16_t)halo::units::unit_seat_candidates_from_zone_and_enter((uint32_t)arguments[0],
        (char *)arguments[1], (uint32_t)arguments[2]), thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_vehicle_riders`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c300
 */
void VehicleCommands::evaluate_vehicle_riders(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        datum_index list = halo::units::unit_build_seat_occupant_zone_list((uint32_t)arguments[0]);
        halo::hs::hs_thread_return((int32_t)list, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_vehicle_test_seat_list`: reads its typed arguments
 * from the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47be90
 */
void VehicleCommands::evaluate_vehicle_test_seat_list(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t result = halo::units::unit_named_seat_occupant_in_zone((uint32_t)arguments[0], (char *)arguments[1], (uint32_t)arguments[2]);
        halo::hs::hs_thread_return((int32_t)result, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_vehicle_unload`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47c1b0
 */
void VehicleCommands::evaluate_vehicle_unload(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t count = halo::units::unit_detach_child_at_named_seat((uint32_t)arguments[0], (char *)arguments[1]);

        halo::hs::hs_thread_return((int32_t)(uint16_t)count, thread_index);
    }
}

}
