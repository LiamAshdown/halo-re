#include "halo/hs/records.hpp"
#include "halo/ai/records.hpp"
#include "halo/hs/hs3_commands.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/ai/api.hpp"


namespace halo::hs::part3 {

/**
 * Evaluate handler of the hs script function `volume_teleport_players_not_inside`: reads its typed arguments
 * from the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47a420
 */
void VolumeCommands::evaluate_volume_teleport_players_not_inside(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_reposition_players_outside_trigger_volume(halo::hs::argument_ushort(arguments[0]), halo::hs::argument_ushort(arguments[1]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `hs_evaluate_volume_test_object`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47a470
 */
void VolumeCommands::evaluate_volume_test_object(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        uint8_t inside = 0;

        if ((uint32_t)arguments[1] != halo::k_dword_none) {
            unit_object *object = (unit_object *)halo::ai::object_at(arguments[1]);

            inside = halo::scenario::scenario_query::trigger_volume_contains_point(halo::hs::argument_short(arguments[0]), &object->base.bounding_center);
        }
        halo::hs::hs_thread_return((int32_t)inside, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `volume_test_objects`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47a4f0
 */
void VolumeCommands::evaluate_volume_test_objects(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)(uint8_t)halo::hs::hs_object_list_test_trigger_volume(halo::hs::argument_short(arguments[0]), (datum_index)arguments[1], 0),
        thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `volume_test_objects_all`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47a550
 */
void VolumeCommands::evaluate_volume_test_objects_all(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)(uint8_t)halo::hs::hs_object_list_test_trigger_volume(halo::hs::argument_short(arguments[0]), (datum_index)arguments[1], 1),
        thread_index);
    }
}

}
