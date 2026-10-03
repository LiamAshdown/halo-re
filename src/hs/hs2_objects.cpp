#include "halo/objects/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/tags/flags.hpp"
#include "halo/ai/flags.hpp"
#include "halo/hs/records.hpp"
#include "halo/ai/records.hpp"
#include "halo/hs/hs2_commands.hpp"

#include "objects.h"
#include "units.h"
#include "game.h"
#include "cache.h"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/ai/api.hpp"

static auto &magic_seat_animation_state_0069fde0 = halo::link::ref<int16_t>(halo::hs::vars().magic_seat_animation_state_0069fde0);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &hs_object_type_masks = halo::link::ref<int16_t []>(halo::hs::vars().hs_object_type_masks);

namespace halo::hs {

/**
 * Evaluate handler of hs function "effect_new"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a9c0
 */
void ObjectCommands::evaluate_effect_new(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_effect_spawn_at_location(halo::hs::argument_short(arguments[1]), (uint32_t)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "effect_new_on_object_marker"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47aa10
 */
void ObjectCommands::evaluate_effect_new_on_object_marker(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_effect_spawn_on_marker((datum_index)arguments[1], (datum_index)arguments[0], halo::hs::argument_string(arguments[2]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "magic_melee_attack"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47c2d0
 */
void ObjectCommands::evaluate_magic_melee_attack(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t *player = (uint8_t *)halo::game::globals().player_data->data;

    halo::units::unit_try_ready_weapon(((struct player *)player)->unit, 0, 0);
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "magic_seat_name"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47c210
 */
void ObjectCommands::evaluate_magic_seat_name(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        magic_seat_animation_state_0069fde0 = halo::units::unit_base_animation_state_from_name(halo::hs::argument_string(arguments[0]));
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_beautify"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b4b0
 */
void ObjectCommands::evaluate_object_beautify(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index object_index = (datum_index)arguments[0];

    if (object_index != k_datum_index_none) {
        uint32_t *flags = (uint32_t *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index)) + 0x10);

        if (halo::hs::argument_byte(arguments[1])) {
            *flags |= 0x400000;
        } else {
            *flags &= 0xffbfffff;
        }
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_can_take_damage"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b470
 */
void ObjectCommands::evaluate_object_can_take_damage(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::objects::object_hash_clear_flag_bit3((uint32_t)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_cannot_take_damage"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b430
 */
void ObjectCommands::evaluate_object_cannot_take_damage(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::objects::object_hash_set_flag_bit3((uint32_t)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_cast"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x489c80
 */
void ObjectCommands::evaluate_object_cast(int16_t function_index, uint32_t thread_index, char first)
{
    hs_stack_frame *frame = halo::hs::thread_at(thread_index)->stack;
    datum_index *slot = halo::hs::frame_scratch<datum_index>(frame);
    datum_index object_index;

    frame->size = (int16_t)(frame->size + 4);
    if (first) {
        hs_syntax_node *call_node = halo::hs::syntax_node_at(frame->syntax_node);
        hs_syntax_node *name_node = halo::hs::syntax_node_at(call_node->data.first_child);

        halo::hs::hs_thread_push(name_node->next_node, thread_index, slot);
        return;
    }
    object_index = *slot;
    if (object_index != k_datum_index_none) {
        unit_object *object = (unit_object *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index)));

        if ((int32_t)hs_object_type_masks[(int16_t)(function_index - 0x16)] & (1 << (static_cast<uint8_t>(object->base.type) & 0x1f))) {
            halo::hs::hs_thread_return((int32_t)object_index, thread_index);
            return;
        }
    }
    halo::hs::hs_thread_return(-1, thread_index);
}

/**
 * Evaluate handler of hs function "object_create"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a5b0
 */
void ObjectCommands::evaluate_object_create(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    int16_t name = halo::hs::argument_short(arguments[0]);

    if (name != -1 && (name < 0 || name >= 0x200 || halo::objects::globals().object_name_list[name] == k_datum_index_none)) {
        halo::objects::object_new_from_scenario_name(name);
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_create_anew"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a670
 */
void ObjectCommands::evaluate_object_create_anew(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_object_name_cache_validate(halo::hs::argument_short(arguments[0]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_create_anew_containing"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a710
 */
void ObjectCommands::evaluate_object_create_anew_containing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_object_names_for_each((void (*)(int32_t))halo::hs::hs_object_name_cache_validate, (uint32_t)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_create_containing"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a6c0
 */
void ObjectCommands::evaluate_object_create_containing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_object_names_for_each(halo::hs::hs_object_create_name_index_if_absent, (uint32_t)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_destroy"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a610
 */
void ObjectCommands::evaluate_object_destroy(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index object_index = (datum_index)arguments[0];

    if (object_index != k_datum_index_none && !halo::hs::hs_object_hierarchy_test(object_index)) {
        halo::objects::object_delete(object_index);
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_destroy_all"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a7b0
 */
void ObjectCommands::evaluate_object_destroy_all(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::hs_object_runtime_cleanup();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "object_destroy_containing"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a760
 */
void ObjectCommands::evaluate_object_destroy_containing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_object_names_for_each(halo::hs::hs_object_name_destroy, (uint32_t)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_pvs_clear"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b680
 */
void ObjectCommands::evaluate_object_pvs_clear(int16_t function_index, uint32_t thread_index, char first)
{
    halo::objects::globals().object_globals->ambient_cluster_mode = 0;
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "object_pvs_set_camera"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b640
 */
void ObjectCommands::evaluate_object_pvs_set_camera(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::objects::objects_set_ambient_cluster_override(halo::hs::argument_short(arguments[0]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_pvs_set_object"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b5d0
 */
void ObjectCommands::evaluate_object_pvs_set_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index object_index = (datum_index)arguments[0];

    if (object_index == k_datum_index_none) {
        halo::objects::globals().object_globals->ambient_cluster_mode = 0;
    } else {
        *(datum_index *)&halo::objects::globals().object_globals->ambient_cluster_index = object_index;
        halo::objects::globals().object_globals->ambient_cluster_mode = 1;
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_set_collideable"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b240
 */
void ObjectCommands::evaluate_object_set_collideable(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            uint8_t *object = (uint8_t *)halo::ai::object_at(arguments[0]);


            if (halo::hs::argument_byte(arguments[1]) == 0) {
                ((struct object *)object)->flags |= halo::to_bits(halo::objects::object_flag::unknown_1000000);
            } else {
                ((struct object *)object)->flags &= 0xfeffffff;
            }
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_set_facing"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a810
 */
void ObjectCommands::evaluate_object_set_facing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_object_detach_and_place_at_location(halo::hs::argument_short(arguments[1]), (datum_index)arguments[0], 0, 1);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_set_melee_attack_inhibited"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b1b0
 */
void ObjectCommands::evaluate_object_set_melee_attack_inhibited(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            uint8_t *object = (uint8_t *)halo::ai::object_at(arguments[0]);

            if (halo::hs::argument_byte(arguments[1]) != 0) {
                object[0x106] |= 0x80;
            } else {
                object[0x106] &= 0x7f;
            }
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_set_permutation"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a8b0
 */
void ObjectCommands::evaluate_object_set_permutation(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_object_set_permutation_by_name((datum_index)arguments[0], (void *)arguments[2], halo::hs::argument_string(arguments[1]));
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_set_ranged_attack_inhibited"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b130
 */
void ObjectCommands::evaluate_object_set_ranged_attack_inhibited(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        if ((uint32_t)arguments[0] != halo::k_dword_none) {
            uint8_t *object = (uint8_t *)halo::ai::object_at(arguments[0]);

            if (halo::hs::argument_byte(arguments[1]) != 0) {
                object[0x107] |= 1;
            } else {
                object[0x107] &= 0xfe;
            }
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_set_scale"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b2c0
 */
void ObjectCommands::evaluate_object_set_scale(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::objects::object_set_scale_and_refresh_nodes((uint32_t)arguments[0], halo::hs::argument_real(arguments[1]),
        (int16_t)halo::hs::argument_ushort(arguments[2]));
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_set_shield"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a860
 */
void ObjectCommands::evaluate_object_set_shield(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_object_set_health_fraction((datum_index)arguments[0], halo::hs::argument_real(arguments[1]));
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_teleport"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a7c0
 */
void ObjectCommands::evaluate_object_teleport(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_object_detach_and_place_at_location(halo::hs::argument_short(arguments[1]), (datum_index)arguments[0], 1, 1);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "object_type_predict"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b570
 */
void ObjectCommands::evaluate_object_type_predict(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index tag = (datum_index)arguments[0];

    if (tag != k_datum_index_none) {
        halo::cache::predicted_resource_list_touch((TagReflexive *)((uint8_t *)halo::cache::globals().tag_instances[tag & halo::k_slot_mask].data + 0x170));
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "objects_attach"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b310
 */
void ObjectCommands::evaluate_objects_attach(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index parent = (datum_index)arguments[0];
    datum_index child = (datum_index)arguments[2];

    if (parent != k_datum_index_none && child != k_datum_index_none &&
        *(datum_index *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(child)) + 0x11c) ==
            k_datum_index_none) {
        halo::objects::object_reorient_relative_to_marker(parent, halo::hs::argument_string(arguments[1]), child, halo::hs::argument_string(arguments[3]));
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "objects_can_see_flag"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ab60
 */
void ObjectCommands::evaluate_objects_can_see_flag(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)(uint8_t)halo::hs::hs_object_list_any_angle_match_gated((datum_index)arguments[0],
        halo::hs::argument_short(arguments[1]), halo::hs::argument_real(arguments[2])), thread_index);
    }
}

/**
 * Evaluate handler of hs function "objects_can_see_object"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47ab00
 */
void ObjectCommands::evaluate_objects_can_see_object(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_thread_return((int32_t)(uint8_t)halo::hs::hs_object_list_any_angle_match((datum_index)arguments[0], (datum_index)arguments[1],
        halo::hs::argument_real(arguments[2])), thread_index);
    }
}

/**
 * Evaluate handler of hs function "objects_delete_by_definition"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47abc0
 */
void ObjectCommands::evaluate_objects_delete_by_definition(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_objects_delete_by_type((uint32_t)arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "objects_detach"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b390
 */
void ObjectCommands::evaluate_objects_detach(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    datum_index parent = (datum_index)arguments[0];
    datum_index child = (datum_index)arguments[1];

    if (parent != k_datum_index_none && child != k_datum_index_none &&
        *(datum_index *)(reinterpret_cast<uint8_t *>(halo::ai::object_at(child)) + 0x11c) == parent) {
        halo::objects::object_snap_to_parent_marker_and_detach(child);
    }
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "objects_dump_memory"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b230
 */
void ObjectCommands::evaluate_objects_dump_memory(int16_t function_index, uint32_t thread_index, char first)
{
    halo::objects::objects_dump_memory();
    halo::hs::hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "objects_predict"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b530
 */
void ObjectCommands::evaluate_objects_predict(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
    halo::hs::hs_object_list_for_each((datum_index)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "scenery_animation_start"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b760
 */
void ObjectCommands::evaluate_scenery_animation_start(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::objects::object_start_animation((uint32_t)arguments[0], (datum_index)arguments[1], halo::hs::argument_string(arguments[2]), 0);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "scenery_animation_start_at_frame"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b7b0
 */
void ObjectCommands::evaluate_scenery_animation_start_at_frame(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::objects::object_start_animation((uint32_t)arguments[0], (datum_index)arguments[1], halo::hs::argument_string(arguments[2]),
                               (int16_t)halo::hs::argument_ushort(arguments[3]));
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "scenery_get_animation_time"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47b700
 */
void ObjectCommands::evaluate_scenery_get_animation_time(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        definition->parameters, first);

    if (arguments != 0) {
        halo::hs::hs_thread_return((int32_t)(uint16_t)(halo::objects::object_animation_get_frames_remaining((uint32_t)arguments[0])), thread_index);
    }
}

/**
 * Table of the hs functions handled by ObjectCommands, in source order.
 */
EvaluateCommandTable ObjectCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &ObjectCommands::evaluate_effect_new,
        &ObjectCommands::evaluate_effect_new_on_object_marker,
        &ObjectCommands::evaluate_magic_melee_attack,
        &ObjectCommands::evaluate_magic_seat_name,
        &ObjectCommands::evaluate_object_beautify,
        &ObjectCommands::evaluate_object_can_take_damage,
        &ObjectCommands::evaluate_object_cannot_take_damage,
        &ObjectCommands::evaluate_object_cast,
        &ObjectCommands::evaluate_object_create,
        &ObjectCommands::evaluate_object_create_anew,
        &ObjectCommands::evaluate_object_create_anew_containing,
        &ObjectCommands::evaluate_object_create_containing,
        &ObjectCommands::evaluate_object_destroy,
        &ObjectCommands::evaluate_object_destroy_all,
        &ObjectCommands::evaluate_object_destroy_containing,
        &ObjectCommands::evaluate_object_pvs_clear,
        &ObjectCommands::evaluate_object_pvs_set_camera,
        &ObjectCommands::evaluate_object_pvs_set_object,
        &ObjectCommands::evaluate_object_set_collideable,
        &ObjectCommands::evaluate_object_set_facing,
        &ObjectCommands::evaluate_object_set_melee_attack_inhibited,
        &ObjectCommands::evaluate_object_set_permutation,
        &ObjectCommands::evaluate_object_set_ranged_attack_inhibited,
        &ObjectCommands::evaluate_object_set_scale,
        &ObjectCommands::evaluate_object_set_shield,
        &ObjectCommands::evaluate_object_teleport,
        &ObjectCommands::evaluate_object_type_predict,
        &ObjectCommands::evaluate_objects_attach,
        &ObjectCommands::evaluate_objects_can_see_flag,
        &ObjectCommands::evaluate_objects_can_see_object,
        &ObjectCommands::evaluate_objects_delete_by_definition,
        &ObjectCommands::evaluate_objects_detach,
        &ObjectCommands::evaluate_objects_dump_memory,
        &ObjectCommands::evaluate_objects_predict,
        &ObjectCommands::evaluate_scenery_animation_start,
        &ObjectCommands::evaluate_scenery_animation_start_at_frame,
        &ObjectCommands::evaluate_scenery_get_animation_time,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
