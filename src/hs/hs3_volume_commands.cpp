#include "halo/hs/hs3_commands.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

extern "C" {
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern void hs_reposition_players_outside_trigger_volume(int32_t trigger_volume_index, int32_t location_index);
extern data_array *object_data;
extern uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point);
extern char hs_object_list_test_trigger_volume(int32_t trigger_volume_index, datum_index header_index, char all_mode);
}

namespace halo::hs::part3 {

/**
 * Evaluate handler of the hs script function `volume_teleport_players_not_inside`: reads its typed arguments
 * from the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47a420
 */
void VolumeCommands::evaluate_volume_teleport_players_not_inside(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_reposition_players_outside_trigger_volume(*(uint16_t *)&arguments[0], *(uint16_t *)&arguments[1]);
    hs_thread_return(0, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t inside = 0;

        if ((uint32_t)arguments[1] != halo::k_dword_none) {
            uint8_t *object = (uint8_t *)((object_header *)object_data->data)[arguments[1] & halo::k_slot_mask].data;

            inside = scenario_trigger_volume_contains_point(*(int16_t *)&arguments[0], (real_point3d *)(object + 0xa0));
        }
        hs_thread_return((int32_t)inside, thread_index);
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint8_t)hs_object_list_test_trigger_volume(*(int16_t *)&arguments[0], (datum_index)arguments[1], 0),
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
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(uint8_t)hs_object_list_test_trigger_volume(*(int16_t *)&arguments[0], (datum_index)arguments[1], 1),
        thread_index);
    }
}

}
