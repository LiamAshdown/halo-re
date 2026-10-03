#include "halo/hs/hs1_interpreter.hpp"
#include "halo/memory/api.hpp"
#include "halo/input/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

extern "C" {
extern void object_list_reference_add(datum_index header_index, datum_index object_index);
extern data_array *object_list_header_data;
extern datum_index *object_name_list;
extern int32_t object_lookup_table_get(int32_t value);
extern int32_t (*hs_type_conversion_procedures[k_hs_type_count][k_hs_type_count])(int32_t value);
extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern data_array *hs_thread_data;
extern data_array *hs_syntax_data;
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count, int16_t *expected_types, char first);
extern int16_t hs_comparison_types[2];
}

static hs_syntax_node *syntax_get(datum_index node)
{
    return (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node & halo::k_slot_mask) * 0x14);
}

namespace halo::hs {

/**
 * Casts an hs enum value to real (type conversion table entry).
 *
 * @address 0x48ab40
 */
int32_t ScriptCasts::enum_to_real(int32_t value)
{
    float result = (float)((int32_t)(int16_t)value + 1);

    return *(int32_t *)&result;
}

/**
 * Identity cast: returns the value unchanged (type conversion table entry).
 *
 * @address 0x48ab90
 */
int32_t ScriptCasts::identity(int32_t value)
{
    return value;
}

/**
 * Casts an hs long value to boolean (type conversion table entry).
 *
 * @address 0x48aab0
 */
int32_t ScriptCasts::long_to_boolean(int32_t value)
{
    return (int32_t)(((uint32_t)value & 0xffffff00u) | (value == 0));
}

/**
 * Casts an hs long value to real (type conversion table entry).
 *
 * @address 0x48ab30
 */
int32_t ScriptCasts::long_to_real(int32_t value)
{
    float result = (float)value;

    return *(int32_t *)&result;
}

/**
 * Casts an hs object name value to object list (type conversion table entry).
 *
 * @address 0x48aba0
 */
datum_index ScriptCasts::object_name_to_object_list(int32_t name_index)
{
    int16_t name = (int16_t)name_index;
    datum_index object_index;
    datum_index header_index;

    if (name < 0 || name >= 0x200) {
        return k_datum_index_none;
    }
    object_index = object_name_list[name];
    if (object_index == k_datum_index_none) {
        return k_datum_index_none;
    }
    header_index = halo::memory::datum_new(object_list_header_data);
    if (header_index != k_datum_index_none) {
        object_list_header *header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & halo::k_slot_mask) * 0x0c);

        header->count = 0;
        header->first_reference = k_datum_index_none;
    }
    object_list_reference_add(header_index, object_index);
    return header_index;
}

/**
 * Casts an hs real value to short (type conversion table entry).
 *
 * @address 0x48ab60
 */
int32_t ScriptCasts::real_to_short(int32_t value)
{
    int16_t result = (int16_t)(int32_t)*(float *)&value;

    return (int32_t)(((uint32_t)value & 0xffff0000u) | (uint16_t)result);
}

/**
 * Casts an hs short value to boolean (type conversion table entry).
 *
 * @address 0x48aad0
 */
int32_t ScriptCasts::short_to_boolean(int32_t value)
{
    return (int32_t)(((uint32_t)value & 0xffffff00u) | ((int16_t)value == 0));
}

/**
 * Casts an hs short value to real (type conversion table entry).
 *
 * @address 0x48ab10
 */
int32_t ScriptCasts::short_to_real(int32_t value)
{
    float result = (float)(int16_t)value;

    return *(int32_t *)&result;
}

/**
 * Coerces `value` from `source_type` to `dest_type`, unless the types already match, source_type is
 * "passthrough", or dest_type is in the object-name family. Ordinary types go through
 * hs_type_conversion_procedures; an object-name source goes through object_lookup_table_get.
 *
 * @address 0x48ad10
 */
int32_t ScriptCasts::coerce_value(int32_t value, hs_type_t dest_type, hs_type_t source_type)
{
    if (source_type != dest_type && source_type != _hs_type_passthrough &&
        (dest_type < 0x2b || 0x30 < dest_type)) {
        if (dest_type < 0x25 || 0x2a < dest_type) {
            value = hs_type_conversion_procedures[dest_type][source_type](value);
        } else if (0x2a < source_type && source_type < 0x31) {
            return object_lookup_table_get(value);
        }
    }
    return value;
}

/**
 * Argument-list evaluation step: receives the value of the argument just evaluated for the calling thread.
 *
 * @address 0x489d50
 */
void ScriptFlowCommands::argument_list(uint32_t unused_param_1, uint32_t thread_index, int32_t value)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    datum_index *next_node_slot;
    int32_t *count;
    int32_t *values;
    int32_t i;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
    frame = thread_record->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    count = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    values = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 0x80;

    if ((char)value != 0) {
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & halo::k_slot_mask) * 0x14);
        *next_node_slot = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & halo::k_slot_mask) * 0x14))->next_node;
        *count = 0;
        for (i = 0; i < 0x20; i++) {
            values[i] = 0;
        }
    }

    if (*next_node_slot != k_datum_index_none && *count < 0x20) {
        hs_thread_push(*next_node_slot, thread_index, &value);
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (*next_node_slot & halo::k_slot_mask) * 0x14);
        *next_node_slot = node->next_node;
        values[*count] = value;
        *count = *count + 1;
        return;
    }
    hs_thread_return(-1, thread_index);
}

/**
 * Evaluate handler shared by the '+', '-', '*', '/', 'min' and 'max' special forms. Left-folds the special
 * form's child list: the first child's value seeds the accumulator, and each subsequent child combines into
 * it per `opcode`.
 *
 * @address 0x489250
 */
void ScriptFlowCommands::arithmetic_reduce(int16_t opcode, uint32_t thread_index, char first)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    int16_t *term_count;
    datum_index *next_node_slot;
    float *child_value;
    float *accumulator;
    float value;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
    frame = thread_record->stack;
    term_count = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    frame = thread_record->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    child_value = (float *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    accumulator = (float *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if (first != 0) {
        *term_count = 0;
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & halo::k_slot_mask) * 0x14);
        *next_node_slot = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & halo::k_slot_mask) * 0x14))->next_node;
        goto check_continue;
    }

    value = *child_value;
    if (*term_count == 0) {
        *accumulator = value;
    } else {
        switch (opcode) {
        case 7: *accumulator = value + *accumulator; break;
        case 8: *accumulator = *accumulator - value; break;
        case 9: *accumulator = value * *accumulator; break;
        case 10: *accumulator = *accumulator / value; break;
        case 0xb: if (value < *accumulator) *accumulator = value; break;
        case 0xc: if (*accumulator < value) *accumulator = value; break;
        }
    }
    *term_count = *term_count + 1;

check_continue:
    if (*next_node_slot == k_datum_index_none) {
        hs_thread_return(*(int32_t *)accumulator, thread_index);
        return;
    }
    hs_thread_push(*next_node_slot, thread_index, child_value);
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (*next_node_slot & halo::k_slot_mask) * 0x14);
    *next_node_slot = node->next_node;
}

/**
 * Evaluate handler for hs function "begin".
 *
 * @address 0x488b90
 */
void ScriptFlowCommands::begin(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
    hs_stack_frame *frame;
    datum_index *next_expression;
    int32_t *result;

    frame = thread->stack;
    next_expression = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    result = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if (first != 0) {
        *next_expression = syntax_get(syntax_get(thread->stack->syntax_node)->data.first_child)->next_node;
        *result = 0;
    }
    if (*next_expression != k_datum_index_none) {
        hs_thread_push(*next_expression, thread_index, result);
        *next_expression = syntax_get(*next_expression)->next_node;
        return;
    }
    hs_thread_return(*result, thread_index);
}

/**
 * Evaluate handler for hs function "bind" (string, string, string -> void).
 *
 * @address 0x4823e0
 */
void ScriptFlowCommands::bind(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::input::hs_bind_control((const char *)arguments[0], (const char *)arguments[1], (const char *)arguments[2]);
        hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler for 'and' (opcode 5) and 'or' (any other opcode reaching this function). Reduces the
 * special form's child list left to right, short-circuiting as soon as the running result can no longer
 * change (false for 'and', true for 'or').
 *
 * @address 0x489120
 */
void ScriptFlowCommands::boolean_and_or(int16_t opcode, uint32_t thread_index, char first)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    datum_index *next_node_slot;
    char *child_value;
    char *result;
    char is_and;
    char child_result;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218);
    frame = thread_record->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    child_value = (char *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    result = (char *)((uint8_t *)frame + 0x0e + frame->size);
    is_and = (opcode == 5);
    frame->size = frame->size + 1;

    if (first != 0) {
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
            (frame->syntax_node & halo::k_slot_mask) * 0x14);
        *next_node_slot = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node->data.first_child & halo::k_slot_mask) * 0x14))->next_node;
        *result = is_and;
        goto check_continue;
    }

    child_result = *child_value;
    if (is_and) {
        if (*result != 0 && child_result != 0) {
            child_result = 1;
        } else {
            child_result = 0;
        }
    } else {
        if (*result == 0 && child_result == 0) {
            child_result = 0;
        } else {
            child_result = 1;
        }
    }
    *result = child_result;

check_continue:
    if (*next_node_slot != k_datum_index_none && (*result != 0) == (is_and != 0)) {
        hs_thread_push(*next_node_slot, thread_index, child_value);
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (*next_node_slot & halo::k_slot_mask) * 0x14);
        *next_node_slot = node->next_node;
        return;
    }
    hs_thread_return(*result, thread_index);
}

/**
 * Evaluate handler shared by the hs comparison special forms (<, >, <=, >=, =, !=).
 *
 * @address 0x489490
 */
void ScriptFlowCommands::comparison(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t *syntax = (uint8_t *)hs_syntax_data->data;
    uint8_t *frame = *(uint8_t **)((uint8_t *)hs_thread_data->data + (thread_index & halo::k_slot_mask) * 0x218 + 0x10);
    uint32_t call_node = *(uint32_t *)(frame + 4) & halo::k_slot_mask;
    uint32_t name_node = *(uint32_t *)(syntax + call_node * 0x14 + 0x10) & halo::k_slot_mask;
    uint32_t first_argument = *(uint32_t *)(syntax + name_node * 0x14 + 8) & halo::k_slot_mask;
    int16_t type = *(int16_t *)(syntax + first_argument * 0x14 + 4);
    int32_t *arguments;
    double a;
    float b;
    uint8_t result = 0;

    hs_comparison_types[1] = type;
    hs_comparison_types[0] = type;
    arguments = hs_evaluate_typed_arguments(thread_index, 2, hs_comparison_types, first);
    if (arguments == 0) {
        return;
    }
    if (hs_comparison_types[0] == 6) {
        a = *(float *)&arguments[0];
        b = *(float *)&arguments[1];
    } else if (hs_comparison_types[0] == 8) {
        a = (double)arguments[0];
        b = (float)arguments[1];
    } else {
        a = (double)*(int16_t *)&arguments[0];
        b = (float)*(int16_t *)&arguments[1];
    }
    switch (function_index - 15) {
    case 0: result = (uint8_t)(a > (double)b); break;
    case 1: result = (uint8_t)(a < (double)b); break;
    case 2: result = (uint8_t)(a >= (double)b); break;
    case 3: result = (uint8_t)(a <= (double)b); break;
    default: break;
    }
    hs_thread_return((int32_t)((thread_index & 0xffffff00) | result), thread_index);
}

namespace {
const ScriptCommandEntry k_script_flow_commands_entries[] = {
    {"begin", &ScriptFlowCommands::begin},
    {"bind", &ScriptFlowCommands::bind},
    {"comparison", &ScriptFlowCommands::comparison},
};
constexpr ScriptCommandGroup k_script_flow_commands_group(k_script_flow_commands_entries, sizeof(k_script_flow_commands_entries) / sizeof(k_script_flow_commands_entries[0]));
}

/**
 * Registry of the hs script commands implemented by ScriptFlowCommands, keyed by script function name.
 */
const ScriptCommandGroup &ScriptFlowCommands::commands()
{
    return k_script_flow_commands_group;
}

}

extern "C" {

int32_t hs_cast_enum_to_real(int32_t value)
{
    return halo::hs::ScriptCasts::enum_to_real(value);
}

int32_t hs_cast_identity(int32_t value)
{
    return halo::hs::ScriptCasts::identity(value);
}

int32_t hs_cast_long_to_boolean(int32_t value)
{
    return halo::hs::ScriptCasts::long_to_boolean(value);
}

int32_t hs_cast_long_to_real(int32_t value)
{
    return halo::hs::ScriptCasts::long_to_real(value);
}

datum_index hs_cast_object_name_to_object_list(int32_t name_index)
{
    return halo::hs::ScriptCasts::object_name_to_object_list(name_index);
}

int32_t hs_cast_real_to_short(int32_t value)
{
    return halo::hs::ScriptCasts::real_to_short(value);
}

int32_t hs_cast_short_to_boolean(int32_t value)
{
    return halo::hs::ScriptCasts::short_to_boolean(value);
}

int32_t hs_cast_short_to_real(int32_t value)
{
    return halo::hs::ScriptCasts::short_to_real(value);
}

int32_t hs_coerce_value(int32_t value, hs_type_t dest_type, hs_type_t source_type)
{
    return halo::hs::ScriptCasts::coerce_value(value, dest_type, source_type);
}

void hs_evaluate_argument_list(uint32_t unused_param_1, uint32_t thread_index, int32_t value)
{
    halo::hs::ScriptFlowCommands::argument_list(unused_param_1, thread_index, value);
}

void hs_evaluate_arithmetic_reduce(int16_t opcode, uint32_t thread_index, char first)
{
    halo::hs::ScriptFlowCommands::arithmetic_reduce(opcode, thread_index, first);
}

void hs_evaluate_begin(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ScriptFlowCommands::begin(function_index, thread_index, first);
}

void hs_evaluate_bind(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ScriptFlowCommands::bind(function_index, thread_index, first);
}

void hs_evaluate_boolean_and_or(int16_t opcode, uint32_t thread_index, char first)
{
    halo::hs::ScriptFlowCommands::boolean_and_or(opcode, thread_index, first);
}

void hs_evaluate_comparison(int16_t function_index, uint32_t thread_index, char first)
{
    halo::hs::ScriptFlowCommands::comparison(function_index, thread_index, first);
}

}
