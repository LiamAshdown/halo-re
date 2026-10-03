#include "halo/hs/hs2_commands.hpp"

#include "game.h"
#include "halo/math/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

#ifdef __cplusplus
extern "C" {
#endif
extern data_array *hs_thread_data;
extern data_array *hs_syntax_data;
extern int16_t hs_type_sizes[];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern datum_index hs_thread_new(int32_t script_index, uint8_t type);
extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address);
extern void hs_thread_evaluate_step(datum_index thread_handle);
extern uint8_t hs_runtime_active;
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t hs_global_get_value(hs_global_reference reference);
extern void hs_global_write_value(hs_global_reference reference);
extern data_array *hs_globals_data;
extern data_array *object_list_header_data;
extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count];
extern Scenario *global_scenario;
extern game_time_globals *game_time;
extern datum_index hs_thread_find_by_script_index(int16_t script_index);
#ifdef __cplusplus
}
#endif

static hs_syntax_node *syntax_get(datum_index node)
{
    return (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node & halo::k_slot_mask) * 0x14);
}

static void object_list_adjust_references(hs_global_reference reference, int16_t delta)
{
    int32_t list = hs_global_get_value(reference);

    if (list != k_datum_index_none) {
        *(int16_t *)((uint8_t *)object_list_header_data->data + (list & halo::k_slot_mask) * 0xc + 4) += delta;
    }
}

static hs_thread *thread_get(datum_index thread_index)
{
    return (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
}

namespace halo::hs {

/**
 * Evaluate handler of hs function "equality"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x4893e0
 */
void FlowCommands::evaluate_equality(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t *syntax = (uint8_t *)hs_syntax_data->data;
    uint8_t *frame = *(uint8_t **)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218 + 0x10);
    uint32_t call_node = *(uint32_t *)(frame + 4) & halo::k_slot_mask;
    uint32_t name_node = *(uint32_t *)(syntax + call_node * 0x14 + 0x10) & halo::k_slot_mask;
    uint32_t first_argument = *(uint32_t *)(syntax + name_node * 0x14 + 8) & halo::k_slot_mask;
    int16_t type = *(int16_t *)(syntax + first_argument * 0x14 + 4);
    int16_t types[2];
    uint8_t *arguments;
    uint8_t equal;
    int32_t size;
    int32_t i;

    types[0] = type;
    types[1] = type;
    arguments = (uint8_t *)hs_evaluate_typed_arguments(thread_index, 2, types, first);
    if (arguments == 0) {
        return;
    }
    size = hs_type_sizes[type];
    equal = 1;
    for (i = 0; i < size; i++) {
        if (arguments[i] != arguments[4 + i]) {
            equal = 0;
            break;
        }
    }
    if (function_index == 0xe) {
        equal = (uint8_t)(equal == 0);
    }
    hs_thread_return((int32_t)equal, thread_index);
}

/**
 * Evaluate handler of hs function "expression"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x48a250
 */
int32_t FlowCommands::evaluate_expression(datum_index node)
{
    datum_index thread_handle;
    hs_thread *thread;

    if (hs_runtime_active == 0 || node == k_datum_index_none) {
        return -1;
    }
    thread_handle = hs_thread_new(-1, 2);
    if (thread_handle == k_datum_index_none) {
        return -1;
    }
    thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_handle & halo::k_slot_mask) * 0x218);
    hs_thread_push(node, thread_handle, &thread->result);
    if ((thread->flags & 1) != 0) {
        hs_thread_evaluate_step(thread_handle);
        return -1;
    }
    return thread->result;
}

/**
 * Evaluate handler of hs function "if"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x488e60
 */
void FlowCommands::evaluate_if(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
    hs_stack_frame *frame;
    int32_t *condition;
    datum_index *branch;
    int32_t *result;
    datum_index condition_node;

    frame = thread->stack;
    condition = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    branch = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    result = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    condition_node = syntax_get(syntax_get(thread->stack->syntax_node)->data.first_child)->next_node;
    if (first != 0) {
        *condition = 0;
        *branch = k_datum_index_none;
        hs_thread_push(condition_node, thread_index, condition);
        return;
    }
    if (*branch != k_datum_index_none) {
        hs_thread_return(*result, thread_index);
        return;
    }
    if (*(uint8_t *)condition != 0) {
        *branch = syntax_get(condition_node)->next_node;
    } else {
        *branch = syntax_get(syntax_get(condition_node)->next_node)->next_node;
        if (*branch == k_datum_index_none) {
            hs_thread_return(0, thread_index);
            return;
        }
    }
    hs_thread_push(*branch, thread_index, result);
}

/**
 * Evaluate handler of hs function "ignore_arguments"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47fc20
 */
void FlowCommands::evaluate_ignore_arguments(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of hs function "not"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47a360
 */
void FlowCommands::evaluate_not(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    hs_thread_return((int32_t)(*(uint8_t *)&arguments[0] == 0), thread_index);
    }
}

/**
 * Evaluate handler of hs function "nothing"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x47cf20
 */
void FlowCommands::evaluate_nothing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "random"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x488c60
 */
void FlowCommands::evaluate_random(hs_thread *thread, uint32_t thread_index, char first)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_random_state *state;
    hs_syntax_node *node;
    datum_index child;
    int16_t child_count;
    uint32_t *chosen_words;
    int32_t *child_value;
    uint32_t word_count;
    uint32_t i;
    int16_t scan;
    uint32_t candidate;
    int16_t chosen_index;
    datum_index chosen_child;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
    frame = thread_record->stack;

    state = (hs_random_state *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;
    frame = thread_record->stack;
    chosen_words = (uint32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread_record->stack;
    child_value = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if (first != 0) {
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & halo::k_slot_mask) * 0x14);
        child = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & halo::k_slot_mask) * 0x14))->next_node;
        state->child_count = 0;
        while (child != k_datum_index_none) {
            node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (child & halo::k_slot_mask) * 0x14);
            child = node->next_node;
            state->child_count = state->child_count + 1;
        }
        word_count = (uint32_t)(state->child_count + 0x1f) >> 5 & 0x3fffffff;
        for (i = 0; i < word_count; i++) {
            chosen_words[i] = 0;
        }
    }

    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    scan = 0;
    child_count = state->child_count;
    chosen_index = child_count;
    if (0 < child_count) {
        do {
            candidate = (uint32_t)(scan + (int16_t)(((halo::math::globals().random_seed_global >> 0x10) *
                (uint32_t)child_count) >> 0x10)) % (uint32_t)child_count;
            chosen_index = (int16_t)candidate;
            if ((chosen_words[(int16_t)candidate >> 5] & (1 << (candidate & 0x1f))) == 0) {
                if (0 < chosen_index) {
                    uint32_t dead = candidate & halo::k_slot_mask;
                    do {
                        dead = dead - 1;
                    } while (dead != 0);
                }
                node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                    (frame->syntax_node & halo::k_slot_mask) * 0x14);
                chosen_child = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & halo::k_slot_mask) * 0x14))->next_node;
                for (i = 0; i < (uint32_t)chosen_index; i++) {
                    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                        (chosen_child & halo::k_slot_mask) * 0x14);
                    chosen_child = node->next_node;
                }
                /* 0x488e0f: `mov ebx,[esp+0x14]` -- scratch slot 3, the 4 bytes that
                   follow the chosen bitmap's first word. */
                hs_thread_push(chosen_child, thread_index, child_value);
                chosen_words[(int16_t)candidate >> 5] |= 1 << (candidate & 0x1f);
                break;
            }
            scan = scan + 1;
        } while (scan < child_count);
    }

    if (scan == state->child_count) {
        hs_thread_return(*child_value, thread_index);
    }
}

/**
 * Evaluate handler of hs function "set"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x488fd0
 */
void FlowCommands::evaluate_set(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
    hs_syntax_node *variable;
    hs_global_reference reference;
    uint16_t index;
    hs_type_t type;
    uint32_t slot;

    variable = syntax_get(syntax_get(syntax_get(thread->stack->syntax_node)->data.first_child)->next_node);
    thread->stack->size = thread->stack->size + 4;
    reference = (hs_global_reference)variable->data.global_reference;
    index = reference & k_hs_global_index_mask;
    if ((reference & k_hs_global_builtin_bit) != 0) {
        type = hs_global_definitions[index]->type;
    } else {
        type = ((ScenarioGlobal *)global_scenario->globals.pointer)[index].type;
    }

    if (first != 0) {
        if (type == 0x17) {
            object_list_adjust_references(reference, -1);
        }
        slot = (reference & k_hs_global_builtin_bit) != 0 ? index : index + k_hs_builtin_global_count;
        hs_thread_push(variable->next_node, thread_index,
            (uint8_t *)hs_globals_data->data + (slot & halo::k_slot_mask) * 8 + 4);
        return;
    }
    hs_global_write_value(reference);
    if (type == 0x17) {
        object_list_adjust_references(reference, 1);
    }
    hs_thread_return(hs_global_get_value(reference), thread_index);
}

/**
 * Evaluate handler of hs function "sleep"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x489800
 */
void FlowCommands::evaluate_sleep(uint32_t unused_param_1, uint32_t thread_index, char first)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    char *condition;
    int16_t *ticks;
    int32_t *timeout_ticks;
    int32_t *start_tick;
    int16_t *stage;
    datum_index name_node;
    datum_index condition_node;
    datum_index ticks_node;
    datum_index timeout_node;
    int32_t ticks_value;
    int32_t wake_tick;
    int32_t capped;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
    frame = thread_record->stack;
    condition = (char *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    ticks = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    timeout_ticks = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    start_tick = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    stage = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    frame = thread_record->stack;
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (frame->syntax_node & halo::k_slot_mask) * 0x14);
    /* no none-guards here: retail walks all three links unconditionally (0x4898c4..0x4898e4) */
    name_node = node->data.first_child;
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (name_node & halo::k_slot_mask) * 0x14);
    condition_node = node->next_node;
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (condition_node & halo::k_slot_mask) * 0x14);
    ticks_node = node->next_node;

    if (first != 0) {
        *condition = 0;
        *start_tick = game_time->game_time;
        *stage = 0;
        *ticks = 0x1e;
        *timeout_ticks = -1;
        if (ticks_node != k_datum_index_none) {
            hs_thread_push(ticks_node, thread_index, ticks);
            return;
        }
    }

    if (*stage == 0) {
        *stage = 1;
        if (ticks_node != k_datum_index_none) {
            node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                (ticks_node & halo::k_slot_mask) * 0x14);
            timeout_node = node->next_node;
            if (timeout_node != k_datum_index_none) {
                hs_thread_push(timeout_node, thread_index, timeout_ticks);
                return;
            }
        }
    } else if (*stage != 1) {
        return;
    }

    if (*condition == 0 &&
        (*timeout_ticks == -1 || game_time->game_time < *start_tick + *timeout_ticks)) {
        /* re-evaluate the condition into the condition slot on every wake */
        hs_thread_push(condition_node, thread_index, condition);
        ticks_value = *ticks;
        if (ticks_value < 1) {
            ticks_value = 1;
        }
        wake_tick = ticks_value + game_time->game_time;
        thread_record->wake_tick = wake_tick;
        if (*timeout_ticks == -1) {
            return;
        }
        capped = *timeout_ticks + *start_tick;
        if (capped <= wake_tick) {
            wake_tick = capped;
        }
        thread_record->wake_tick = wake_tick;
        return;
    }
    hs_thread_return(0, thread_index);
}

/**
 * Evaluate handler of hs function "sleep_ticks"; the body is the original handler moved unchanged into the command
 * group class.
 *
 * @address 0x489650
 */
void FlowCommands::evaluate_sleep_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = thread_get(thread_index);
    hs_stack_frame *frame;
    int32_t *ticks;
    int32_t *script;
    int16_t *state;
    datum_index ticks_node;
    datum_index target = thread_index;

    frame = thread->stack;
    ticks = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    script = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    state = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    ticks_node = syntax_get(syntax_get(thread->stack->syntax_node)->data.first_child)->next_node;
    if (first != 0) {
        hs_thread_push(ticks_node, thread_index, ticks);
        *state = 0;
        return;
    }
    if (*state == 0) {
        datum_index script_node = syntax_get(ticks_node)->next_node;

        *state = 1;
        if (script_node != k_datum_index_none) {
            hs_thread_push(script_node, thread_index, script);
            return;
        }
        *script = k_datum_index_none;
    }

    if (*(int16_t *)ticks != 0) {
        int16_t count = *(int16_t *)ticks;
        int16_t script_index = *(int16_t *)script;

        if (script_index != -1) {
            target = hs_thread_find_by_script_index(script_index);
        }
        if (target != k_datum_index_none) {
            hs_thread *sleeper = thread_get(target);
            int32_t wake = count < 0 ? -2 : game_time->game_time + count;
            int32_t old_wake = sleeper->wake_tick;

            if (old_wake != -1) {
                if (target != thread_index && (sleeper->flags & _hs_thread_wake_saved_bit) == 0) {
                    sleeper->flags = sleeper->flags | _hs_thread_wake_saved_bit;
                    sleeper->saved_wake_tick = old_wake;
                }
                thread_get(target)->wake_tick = wake;
            }
        }
    }
    hs_thread_return(0, thread_index);
}

/**
 * Table of the hs functions handled by FlowCommands, in source order.
 */
EvaluateCommandTable FlowCommands::commands() noexcept
{
    static constexpr EvaluateFn k_commands[] = {
        &FlowCommands::evaluate_equality,
        &FlowCommands::evaluate_if,
        &FlowCommands::evaluate_ignore_arguments,
        &FlowCommands::evaluate_not,
        &FlowCommands::evaluate_nothing,
        &FlowCommands::evaluate_random_range,
        &FlowCommands::evaluate_real_random_range,
        &FlowCommands::evaluate_set,
        &FlowCommands::evaluate_sleep_ticks,
    };
    return {k_commands, static_cast<uint32_t>(sizeof(k_commands) / sizeof(k_commands[0]))};
}

}
