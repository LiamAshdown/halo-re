#include "halo/hs/records.hpp"
#include "halo/hs/hs3_parser.hpp"
#include "halo/scenario/api.hpp"
#include <stdio.h>
#include <string.h>
#include "crt.h"
#include <ctype.h>
#include <stdlib.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/game/api.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/objects/api.hpp"

static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &hs_parse_primitive_procedures = halo::link::ref<void *[k_hs_type_count]>(halo::hs::vars().hs_parse_primitive_procedures);
static auto &hs_tag_group_for_type = halo::link::ref<uint32_t [8]>(halo::hs::vars().hs_tag_group_for_type);
static auto &hs_enum_definitions = halo::link::ref<hs_enum_definition [5]>(halo::hs::vars().hs_enum_definitions);

static char parse_typed_argument(datum_index argument, hs_type_t type)
{
    hs_syntax_node *node = halo::hs::syntax_node_at(argument);
    if (node->type != 0) {
        return 1;
    }
    node->type = type;
    if ((node->flags & 1) != 0) {
        node->index_union = type;
        return halo::hs::hs_parse_primitive(argument);
    }
    return halo::hs::hs_parse_nonprimitive(argument);
}

static char hs_inspect_not_a_reference[] = "this is not a global variable reference, function call, or script call.";

namespace halo::hs::part3 {

/**
 * Recursively parses/type-checks a single syntax node against `expected_type`, dispatching to the primitive or
 * nonprimitive parser the first time it is visited (node->type == 0); returns 1 without reparsing if the node
 * was already typed.
 *
 * @address 0x486420
 */
char Parser::hs_parse(datum_index node_index, hs_type_t expected_type) const
{
    data_array *nodes;
    hs_syntax_node *node;
    char result;

    nodes = halo::hs::globals().syntax_data;
    node = halo::hs::syntax_node_at(node_index);
    result = 1;
    if (node->type == 0) {
        node->type = expected_type;
        if ((node->flags & _hs_syntax_node_primitive_bit) != 0) {
            node->index_union = expected_type;
            result = halo::hs::hs_parse_primitive(node_index);
            return result;
        }
        result = halo::hs::hs_parse_nonprimitive(node_index);
    }
    return result;
}

/**
 * 0x432320, blam-cc: EAX -> reference_string, ECX -> scenario, stack -> out_packed_reference
 *
 * @address 0x487150
 */
char Parser::parse_ai(datum_index node_index) const
{
    hs_syntax_node *node = halo::hs::syntax_node_at(node_index);
    uint8_t found = halo::ai::ai_reference_parse(halo::hs::globals().compiled_source + node->source_offset, halo::scenario::globals().scenario,
        (uint32_t *)&node->data);

    if (!found) {
        halo::hs::globals().compile_error = const_cast<char *>("this is not a valid ai encounter or squad.");
        halo::hs::globals().compile_error_offset = node->source_offset;
    }
    return found;
}

/**
 * Behaviour of the original `hs_parse_ai_command_list` function, moved unchanged into the class.
 *
 * @address 0x4871a0
 */
char Parser::parse_ai_command_list(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 0, &halo::scenario::globals().scenario->command_lists, 0x60);
}

/**
 * Behaviour of the original `hs_parse_arithmetic` function, moved unchanged into the class.
 *
 * @address 0x484ea0
 */
char Parser::parse_arithmetic(int16_t function_index, datum_index node_index) const
{
    hs_syntax_node *call = halo::hs::syntax_node_at(node_index);
    datum_index argument = halo::hs::syntax_node_at(*(datum_index *)&call->data)->next_node;
    int16_t count = 0;
    char ok = 1;

    for (; argument != halo::k_dword_none; argument = halo::hs::syntax_node_at(argument)->next_node) {
        ok = parse_typed_argument(argument, 6);
        count++;
        if (!ok) {
            break;
        }
    }
    if ((ok && count < 2) || (function_index == 0xa && count > 2)) {
        sprintf(halo::hs::globals().compile_error_buffer, "the %s call requires %s2 arguments.", halo::hs::globals().function_definitions[function_index]->name,
            function_index == 0xa ? "" : "at least ");
        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = halo::hs::syntax_node_at(node_index)->source_offset;
        return 0;
    }
    return ok;
}

/**
 * Behaviour of the original `hs_parse_begin` function, moved unchanged into the class.
 *
 * @address 0x484600
 */
char Parser::parse_begin(int16_t function_index, datum_index node_index) const
{
    hs_syntax_node *call = halo::hs::syntax_node_at(node_index);
    datum_index argument = halo::hs::syntax_node_at(*(datum_index *)&call->data)->next_node;
    int16_t count = 0;
    char ok = 1;

    while (argument != halo::k_dword_none) {
        hs_syntax_node *node = halo::hs::syntax_node_at(argument);
        datum_index next = node->next_node;

        if (function_index == 0) {
            ok = halo::hs::hs_parse(argument, next == halo::k_dword_none ? call->type : 4);
            if (next == halo::k_dword_none && call->type == 0 && ok) {
                call->type = halo::hs::syntax_node_at(argument)->type;
            }
        } else {
            ok = halo::hs::hs_parse(argument, call->type);
            if (call->type == 0 && ok) {
                call->type = halo::hs::syntax_node_at(argument)->type;
            }
        }
        argument = next;
        count++;
        if (!ok) {
            return 0;
        }
    }
    if (count < 1) {
        sprintf(halo::hs::globals().compile_error_buffer, "a statement block must contain at least one argument.",
            halo::hs::globals().function_definitions[function_index]->name);
        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = call->source_offset;
        return 0;
    }
    if (count > 0x20 && function_index == 1) {
        halo::hs::globals().compile_error = const_cast<char *>("begin_random can take a maximum of 32 arguments (matt can increase this.)");
        halo::hs::globals().compile_error_offset = call->source_offset;
        return 0;
    }
    return 1;
}

/**
 * Behaviour of the original `hs_parse_boolean` function, moved unchanged into the class.
 *
 * @address 0x486a10
 */
char Parser::parse_boolean(datum_index node_index) const
{
    hs_syntax_node *node = halo::hs::syntax_node_at(node_index);
    char *token = halo::hs::globals().compiled_source + node->source_offset;

    if (strcmp(token, "false") == 0 || strcmp(token, "off") == 0 || strcmp(token, "0") == 0) {
        node->data.boolean_value = 0;
        return 1;
    }
    if (strcmp(token, "true") == 0 || strcmp(token, "on") == 0 || strcmp(token, "1") == 0) {
        node->data.boolean_value = 1;
        return 1;
    }
    halo::hs::globals().compile_error = const_cast<char *>("i expected \"true\" or \"false\".");
    halo::hs::globals().compile_error_offset = node->source_offset;
    node->data.boolean_value = (uint8_t)node_index;
    return 0;
}

/**
 * Behaviour of the original `hs_parse_cond` function, moved unchanged into the class.
 *
 * @address 0x484b40
 */
char Parser::parse_cond(int16_t function_index, datum_index node_index) const
{
    hs_syntax_node *call = halo::hs::syntax_node_at(node_index);
    datum_index rewritten = halo::hs::hs_parse_cond_recursive(node_index, halo::hs::syntax_node_at(*(datum_index *)&call->data)->next_node);
    hs_syntax_node *replacement;
    datum_index next;
    hs_type_t type;
    int16_t identifier;

    if (rewritten == halo::k_dword_none) {
        return 0;
    }
    call = halo::hs::syntax_node_at(node_index);
    next = call->next_node;
    type = call->type;
    identifier = call->identifier;
    replacement = halo::hs::syntax_node_at(rewritten);
    replacement->next_node = next;
    *call = *replacement;
    call->identifier = identifier;
    return halo::hs::hs_parse(node_index, type);
}

/**
 * Recursively desugars a cond special-form's list of condition/result pairs into a chain of nested if syntax
 * nodes; the end-of-list node is a primitive of the cond type with value 0.
 *
 * @address 0x4848f0
 */
datum_index Parser::parse_cond_recursive(datum_index cond_node_index, datum_index pair_index) const
{
    data_array *nodes;
    datum_index new_index;
    hs_syntax_node *new_node;
    hs_syntax_node *cond_node;
    hs_syntax_node *pair_node;
    datum_index condition_index;
    hs_syntax_node *condition_node;
    datum_index new_if_index;
    datum_index replacement_index;
    hs_syntax_node *new_if_node;
    hs_syntax_node *replacement_node;
    datum_index inner_result;
    datum_index result_index;

    nodes = halo::hs::globals().syntax_data;
    new_index = halo::memory::datum_new(nodes);
    cond_node = halo::hs::syntax_node_at(cond_node_index);
    if (new_index == k_datum_index_none) {
        halo::hs::globals().compile_error = const_cast<char *>("i couldn't allocate a syntax node.");
        halo::hs::globals().compile_error_offset = cond_node->source_offset;
        return k_datum_index_none;
    }
    new_node = halo::hs::syntax_node_at(new_index);
    new_node->source_offset = cond_node->source_offset;
    new_node->flags = 0;
    new_node->next_node = k_datum_index_none;

    if (pair_index == k_datum_index_none) {

        new_node->flags = _hs_syntax_node_primitive_bit;
        new_node->index_union = cond_node->type;
        new_node->type = cond_node->type;
        new_node->data.long_value = 0;
        return new_index;
    }

    pair_node = halo::hs::syntax_node_at(pair_index);
    if ((pair_node->flags & _hs_syntax_node_primitive_bit) != 0) {
        halo::hs::globals().compile_error = const_cast<char *>("this argument to cond should be a condition/result pair");
        halo::hs::globals().compile_error_offset = pair_node->source_offset;
        return k_datum_index_none;
    }
    condition_index = pair_node->data.first_child;
    condition_node = halo::hs::syntax_node_at(condition_index);

    if ((uint32_t)(condition_node->next_node == 0) != (uint32_t)k_datum_index_none) {
        new_if_index = halo::memory::datum_new(nodes);
        replacement_index = halo::memory::datum_new(nodes);
        if ((new_if_index == k_datum_index_none) || (replacement_index == k_datum_index_none)) {
            nodes = halo::hs::globals().syntax_data;
            cond_node = halo::hs::syntax_node_at(cond_node_index);
            halo::hs::globals().compile_error = const_cast<char *>("i couldn't allocate a syntax node.");
            halo::hs::globals().compile_error_offset = cond_node->source_offset;
            return k_datum_index_none;
        }
        nodes = halo::hs::globals().syntax_data;
        new_if_node = halo::hs::syntax_node_at(new_if_index);
        replacement_node = halo::hs::syntax_node_at(replacement_index);

        inner_result = halo::hs::hs_parse_cond_recursive(cond_node_index, pair_node->next_node);
        new_if_node->next_node = inner_result;
        if (inner_result == k_datum_index_none) {
            return k_datum_index_none;
        }

        new_node->data.first_child = pair_index;
        new_node->index_union = _hs_type_function_name;

        pair_node->next_node = condition_index;
        pair_node->index_union = _hs_type_function_name;
        pair_node->type = _hs_type_function_name;
        pair_node->data.long_value = 0;
        pair_node->flags = _hs_syntax_node_primitive_bit;
        pair_node->source_offset = -1;

        new_if_node->data.first_child = replacement_index;
        new_if_node->flags = 0;
        new_if_node->source_offset = new_node->source_offset;

        nodes = halo::hs::globals().syntax_data;
        replacement_node->data.long_value = 0;
        replacement_node->index_union = 0;
        replacement_node->flags = _hs_syntax_node_primitive_bit;
        result_index = condition_node->next_node;
        replacement_node->type = _hs_type_function_name;
        replacement_node->source_offset = -1;
        replacement_node->next_node = result_index;
        condition_node->next_node = new_if_index;
        return new_index;
    }

    halo::hs::globals().compile_error = const_cast<char *>("this argument to cond needs a result.");
    halo::hs::globals().compile_error_offset = condition_node->source_offset;
    return k_datum_index_none;
}

/**
 * Behaviour of the original `hs_parse_conversation` function, moved unchanged into the class.
 *
 * @address 0x487200
 */
char Parser::parse_conversation(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 0, &halo::scenario::globals().scenario->ai_conversations, 0x74);
}

/**
 * Behaviour of the original `hs_parse_cutscene_camera_point` function, moved unchanged into the class.
 *
 * @address 0x487090
 */
char Parser::parse_cutscene_camera_point(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 4, &halo::scenario::globals().scenario->cutscene_camera_points, 0x68);
}

/**
 * Behaviour of the original `hs_parse_cutscene_flag` function, moved unchanged into the class.
 *
 * @address 0x487060
 */
char Parser::parse_cutscene_flag(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 4, &halo::scenario::globals().scenario->cutscene_flags, 0x5c);
}

/**
 * Behaviour of the original `hs_parse_cutscene_recording` function, moved unchanged into the class.
 *
 * @address 0x4870f0
 */
char Parser::parse_cutscene_recording(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 0, &halo::scenario::globals().scenario->recorded_animations, 0x40);
}

/**
 * Behaviour of the original `hs_parse_cutscene_title` function, moved unchanged into the class.
 *
 * @address 0x4870c0
 */
char Parser::parse_cutscene_title(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 4, &halo::scenario::globals().scenario->cutscene_titles, 0x60);
}

/**
 * Behaviour of the original `hs_parse_device_group` function, moved unchanged into the class.
 *
 * @address 0x487120
 */
char Parser::parse_device_group(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 0, &halo::scenario::globals().scenario->device_groups, 0x34);
}

/**
 * Behaviour of the original `hs_parse_function_arguments` function, moved unchanged into the class.
 *
 * @address 0x487440
 */
char Parser::parse_function_arguments(int16_t function_index, datum_index node_index) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    hs_syntax_node *node = halo::hs::syntax_node_at(node_index);
    datum_index argument = halo::hs::syntax_node_at(node->data.first_child)->next_node;
    char ok = 1;
    int16_t i = 0;

    while (i < definition->parameter_count && argument != k_datum_index_none) {
        if (halo::hs::hs_parse(argument, (hs_type_t)(uint16_t)definition->parameters[i])) {
            argument = halo::hs::syntax_node_at(argument)->next_node;
        } else {
            ok = 0;
        }
        i++;
        if (!ok) {
            return 0;
        }
    }
    if (!ok) {
        return ok;
    }
    if (i != definition->parameter_count || argument != k_datum_index_none) {
        sprintf(halo::hs::globals().compile_error_buffer, "the \"%s\" call requires exactly %d arguments.", definition->name,
            (int32_t)definition->parameter_count);
        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = halo::hs::syntax_node_at(node_index)->source_offset;
        return 0;
    }
    return 1;
}

/**
 * Behaviour of the original `hs_parse_hud_message` function, moved unchanged into the class.
 *
 * @address 0x4873c0
 */
char Parser::parse_hud_message(datum_index node_index) const
{
    datum_index hud_messages = *(datum_index *)&halo::scenario::globals().scenario->hud_messages.tag_id;

    if (hud_messages == k_datum_index_none) {
        return 0;
    }
    return halo::hs::hs_parse_scenario_datum(node_index, 0,
        (TagReflexive *)((uint8_t *)halo::cache::globals().tag_instances[hud_messages & halo::k_slot_mask].data + 0x20), 0x40);
}

/**
 * Parses/type-checks an (if <condition> <then> [<else>]) special-form syntax node.
 *
 * @address 0x484780
 */
char Parser::parse_if(int16_t function_index, datum_index node_index) const
{
    data_array *nodes;
    hs_syntax_node *node;
    hs_syntax_node *condition_node;
    hs_syntax_node *then_node;
    datum_index then_index;
    datum_index else_index;
    hs_syntax_node *else_node;
    char ok;
    hs_type_t resolved_type;

    (void)function_index;
    nodes = halo::hs::globals().syntax_data;
    node = halo::hs::syntax_node_at(node_index);

    condition_node = 0;
    then_index = k_datum_index_none;
    else_index = k_datum_index_none;
    if (node->data.first_child != k_datum_index_none) {
        condition_node = halo::hs::syntax_node_at(node->data.first_child);
        then_index = condition_node->next_node;
    }
    if ((condition_node != 0) && (then_index != k_datum_index_none)) {
        then_node = halo::hs::syntax_node_at(then_index);
        else_index = then_node->next_node;
        if ((else_index == k_datum_index_none) ||
            ((halo::hs::syntax_node_at(else_index))->next_node == k_datum_index_none)) {
            ok = halo::hs::hs_parse(node->data.first_child, _hs_type_boolean);
            if (ok == 0) {
                return 0;
            }
            ok = halo::hs::hs_parse(then_index, node->type);
            if (ok != 0) {
                if (node->type == 0) {
                    node->type = then_node->type;
                }
                if (else_index != k_datum_index_none) {
                    ok = halo::hs::hs_parse(else_index, node->type);
                    if (ok == 0) {
                        return 0;
                    }
                }
                return 1;
            }
            if (halo::hs::globals().compile_error != 0) {
                return 0;
            }
            if (node->type != 0) {
                return 0;
            }
            if (else_index == k_datum_index_none) {
                return 0;
            }
            ok = halo::hs::hs_parse(else_index, 0);
            if (ok == 0) {
                return 0;
            }
            else_node = halo::hs::syntax_node_at(else_index);
            resolved_type = else_node->type;
            node->type = resolved_type;
            return halo::hs::hs_parse(then_index, resolved_type);
        }
    }
    halo::hs::globals().compile_error = const_cast<char *>("i expected (if <condition> <then> [<else>]).");
    halo::hs::globals().compile_error_offset = node->source_offset;
    return 0;
}

/**
 * Behaviour of the original `hs_parse_inspect` function, moved unchanged into the class.
 *
 * @address 0x485460
 */
char Parser::parse_inspect(int16_t function_index, datum_index node_index) const
{
    datum_index argument;

    if (!halo::hs::hs_get_parameter_indices(halo::hs::globals().function_definitions[function_index]->name, 1, node_index, &argument)) {
        return 0;
    }
    if (halo::hs::hs_parse(argument, 0)) {
        return 1;
    }
    if (halo::hs::globals().compile_error == 0) {
        halo::hs::globals().compile_error = hs_inspect_not_a_reference;
        halo::hs::globals().compile_error_offset = halo::hs::syntax_node_at(argument)->source_offset;
    }
    return 0;
}

/**
 * Behaviour of the original `hs_parse_integer` function, moved unchanged into the class.
 *
 * @address 0x486b80
 */
char Parser::parse_integer(datum_index node_index) const
{
    hs_syntax_node *node = halo::hs::syntax_node_at(node_index);
    char *p = halo::hs::globals().compiled_source + node->source_offset;
    char valid = 1;
    int32_t value;

    if (*p == '-') {
        p++;
    }
    for (; *p != 0; p++) {
        if (!isdigit((unsigned char)*p)) {
            halo::hs::globals().compile_error = const_cast<char *>("this is not a valid integer.");
            halo::hs::globals().compile_error_offset = node->source_offset;
            valid = 0;
            break;
        }
    }
    value = atoi(halo::hs::globals().compiled_source + node->source_offset);
    if (valid && node->type != 8 && (value > 0x7fff || value < -0x8000)) {
        halo::hs::globals().compile_error = const_cast<char *>("shorts must be in the range [-32767, 32768].");
        halo::hs::globals().compile_error_offset = node->source_offset;
        valid = 0;
    }
    if (node->type == 8) {
        node->data.long_value = value;
    } else {
        node->data.short_value = (int16_t)value;
    }
    return valid;
}

/**
 * Behaviour of the original `hs_parse_logical` function, moved unchanged into the class.
 *
 * @address 0x484db0
 */
char Parser::parse_logical(int16_t function_index, datum_index node_index) const
{
    hs_syntax_node *call = halo::hs::syntax_node_at(node_index);
    datum_index argument = halo::hs::syntax_node_at(*(datum_index *)&call->data)->next_node;
    int16_t count = 0;

    for (; argument != halo::k_dword_none; argument = halo::hs::syntax_node_at(argument)->next_node) {
        char ok = parse_typed_argument(argument, 5);

        count++;
        if (!ok) {
            return 0;
        }
    }
    if (count < 2) {
        sprintf(halo::hs::globals().compile_error_buffer, "the %s call requires at least 2 arguments.", halo::hs::globals().function_definitions[function_index]->name);
        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = halo::hs::syntax_node_at(node_index)->source_offset;
        return 0;
    }
    return 1;
}

/**
 * Behaviour of the original `hs_parse_navpoint` function, moved unchanged into the class.
 *
 * @address 0x487360
 */
char Parser::parse_navpoint(datum_index node_index) const
{
    uint8_t *interface_bitmaps = global_globals->interface_bitmaps.count != 0 ?
        (uint8_t *)global_globals->interface_bitmaps.pointer : 0;
    datum_index hud_globals = *(datum_index *)(interface_bitmaps + 0x6c);

    if (hud_globals == k_datum_index_none) {
        return 0;
    }
    return halo::hs::hs_parse_scenario_datum(node_index, 0,
        (TagReflexive *)((uint8_t *)halo::cache::globals().tag_instances[hud_globals & halo::k_slot_mask].data + 0x160), 0x68);
}

/**
 * Parses a non-primitive expression token: either the "global"/"script" special forms (when the node's expected
 * type is _hs_type_special_form) or a function/script call, resolving and type-checking it against
 * hs_function_definitions or the scenario's own scripts.
 *
 * @address 0x486710
 */
char Parser::parse_nonprimitive(datum_index node_index) const
{
    data_array *nodes;
    hs_syntax_node *node;
    hs_syntax_node *identifier_node;
    char *message;
    hs_type_t expected_type;
    int16_t resolved_index;
    hs_function_definition *function_def;
    hs_type_t actual_type;
    char compatible;
    char (*parse)(int16_t, datum_index);
    ScenarioScript *script;
    char *identifier_text;

    nodes = halo::hs::globals().syntax_data;
    node = halo::hs::syntax_node_at(node_index);
    identifier_node = halo::hs::syntax_node_at(node->data.first_child);

    if ((identifier_node->flags & _hs_syntax_node_primitive_bit) == 0) {
        message = const_cast<char *>("\"script\" or \"global\"");
        if (node->type != _hs_type_special_form) {
            message = const_cast<char *>("a function name");
        }
        sprintf(halo::hs::globals().compile_error_buffer, "i expected %s, but i got an expression.", message);
        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = identifier_node->source_offset;
        return 0;
    }

    if (node->type != _hs_type_special_form) {
        halo::hs::hs_resolve_identifier_as_function_or_script(node_index);
        resolved_index = node->index_union;
        if (resolved_index == -1) {
            halo::hs::globals().compile_error = const_cast<char *>("this is not a valid function or script name.");
            halo::hs::globals().compile_error_offset = identifier_node->source_offset;
            return 0;
        }

        if ((node->flags & _hs_syntax_node_script_call_bit) == 0) {

            function_def = halo::hs::globals().function_definitions[resolved_index];
            expected_type = node->type;
            if (expected_type != 0) {
                actual_type = function_def->return_type;
                compatible = halo::hs::hs_types_are_compatible(expected_type, actual_type);
                if (compatible == 0) {
                    sprintf(halo::hs::globals().compile_error_buffer, "i expected a %s, but this function returns a %s.",
                            halo::hs::globals().type_names[expected_type], halo::hs::globals().type_names[actual_type]);
                    halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
                    halo::hs::globals().compile_error_offset = node->source_offset;
                    return 0;
                }
            }
            if ((halo::hs::globals().blocking_forbidden != 0) && ((resolved_index == _hs_function_sleep) || (resolved_index == _hs_function_sleep_until))) {
                halo::hs::globals().compile_error = const_cast<char *>("it is illegal to block in this context.");
                halo::hs::globals().compile_error_offset = node->source_offset;
                return 0;
            }
            if ((halo::hs::globals().set_forbidden != 0) && (resolved_index == _hs_function_set)) {
                halo::hs::globals().compile_error = const_cast<char *>("it is illegal to set the value of variables in this context.");
                halo::hs::globals().compile_error_offset = node->source_offset;
                return 0;
            }
            if ((expected_type == 0) && (function_def->return_type != 3)) {
                node->type = function_def->return_type;
            }
            parse = (char (*)(int16_t, datum_index))function_def->parse;
            return parse(resolved_index, node_index);
        }

        script = (ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer + resolved_index;
        if ((script->script_type != _hs_script_static) && (script->script_type != _hs_script_stub)) {
            halo::hs::globals().compile_error = const_cast<char *>("this is not a static script.");
            halo::hs::globals().compile_error_offset = node->source_offset;
            return 0;
        }
        expected_type = node->type;
        if (expected_type != 0) {
            actual_type = script->return_type;
            compatible = halo::hs::hs_types_are_compatible(expected_type, actual_type);
            if (compatible == 0) {
                sprintf(halo::hs::globals().compile_error_buffer, "i expected a %s, but this script returns a %s.",
                        halo::hs::globals().type_names[expected_type], halo::hs::globals().type_names[actual_type]);
                halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
                halo::hs::globals().compile_error_offset = node->source_offset;
                return 0;
            }

            return 1;
        }
        node->type = script->return_type;
        return 1;
    }

    identifier_text = halo::hs::globals().compiled_source + identifier_node->source_offset;
    if (strncmp(identifier_text, "global", 7) == 0) {
        return halo::hs::hs_add_global(node_index);
    }
    if (strncmp(identifier_text, "script", 7) == 0) {
        return halo::hs::hs_add_script(node_index);
    }
    halo::hs::globals().compile_error = const_cast<char *>("i expected \"script\" or \"global\".");
    halo::hs::globals().compile_error_offset = identifier_node->source_offset;
    return 0;
}

/**
 * Behaviour of the original `hs_parse_object` function, moved unchanged into the class.
 *
 * @address 0x4872f0
 */
char Parser::parse_object(datum_index node_index) const
{
    hs_syntax_node *node = halo::hs::syntax_node_at(node_index);
    char result;

    if (strcmp(halo::hs::globals().compiled_source + node->source_offset, "none") == 0) {
        node->data.long_value = -1;
        return 1;
    }
    node->type = node->type + 6;
    node->index_union = node->type;
    result = halo::hs::hs_parse_object_name(node_index);
    node->type = node->type - 6;
    return result;
}

/**
 * Behaviour of the original `hs_parse_object_list` function, moved unchanged into the class.
 *
 * @address 0x487400
 */
char Parser::parse_object_list(datum_index node_index) const
{
    hs_syntax_node *node = halo::hs::syntax_node_at(node_index);
    char result;

    node->index_union = 0x2b;
    node->type = 0x2b;
    result = halo::hs::hs_parse_object_name(node_index);
    node->type = 0x17;
    return result;
}

/**
 * Parses an object-name token: resolves it against Scenario::object_names, then checks that the matched entry's
 * object_type is a member of the object family the node's expected type demands (e.g. a "vehicle_name" node
 * requires the vehicle bit). Stores the matched index on success.
 *
 * @address 0x487230
 */
char Parser::parse_object_name(datum_index node_index) const
{
    hs_syntax_node *node;
    Scenario *scenario;
    ScenarioObjectName *object_names;
    int16_t match_index;

    scenario = halo::scenario::globals().scenario;
    node = halo::hs::syntax_node_at(node_index);

    match_index = halo::scenario::scenario_object_name_find_index(scenario, halo::hs::globals().compiled_source + node->source_offset);
    if (match_index == -1) {
        halo::hs::globals().compile_error = const_cast<char *>("this is not a valid object name.");
        halo::hs::globals().compile_error_offset = node->source_offset;
        return 0;
    }

    object_names = (ScenarioObjectName *)scenario->object_names.pointer;
    if (halo::hs::globals().object_type_masks[node->type - _hs_type_object_name] &
        (1 << (object_names[match_index].object_type & 0x1f))) {
        node->data.short_value = match_index;
        return 1;
    }

    sprintf(halo::hs::globals().compile_error_buffer, "this is not an object of type %s.", halo::hs::globals().type_names[node->type]);
    halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
    halo::hs::globals().compile_error_offset = node->source_offset;
    return 0;
}

/**
 * Parses/type-checks a primitive (non-list) syntax node as either a variable reference (tried first, unless
 * postprocessing a non-global) or a typed constant via hs_parse_primitive_procedures.
 *
 * @address 0x486480
 */
char Parser::parse_primitive(datum_index node_index) const
{
    hs_syntax_node *node;
    hs_type_t node_type;
    char result;
    char (*procedure)(datum_index);

    node = halo::hs::syntax_node_at(node_index);
    node_type = node->type;
    result = 0;

    if (node_type == _hs_type_special_form) {
        halo::hs::globals().compile_error = const_cast<char *>("i expected a script or variable definition.");
        halo::hs::globals().compile_error_offset = node->source_offset;
        return result;
    }
    if (node_type == _hs_type_void) {
        halo::hs::globals().compile_error = const_cast<char *>("the value of this expression (in a <void> slot) can never be used.");
        halo::hs::globals().compile_error_offset = node->source_offset;
        return result;
    }

    if ((halo::hs::globals().postprocessing == 0) || ((node->flags & _hs_syntax_node_global_bit) != 0)) {
        result = halo::hs::hs_parse_variable(node_index);
        if (result != 0) {
            return result;
        }
    }
    node_type = node->type;
    if ((node_type != 0) && (halo::hs::globals().compile_error == 0) &&
        ((halo::hs::globals().postprocessing == 0) || ((node->flags & _hs_syntax_node_global_bit) == 0))) {
        procedure = (char (*)(datum_index))hs_parse_primitive_procedures[node_type];
        if (procedure != 0) {
            result = procedure(node_index);
            return result;
        }
        sprintf(halo::hs::globals().compile_error_buffer, "expressions of type %s are currently unsupported.", halo::hs::globals().type_names[node_type]);
        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = node->source_offset;
        result = 0;
    }
    return result;
}

/**
 * Validates and parses a floating point literal token from the script source text into the node's real value
 * field, reporting an error (but still storing whatever atof makes of the text) if it is not a valid real
 * number.
 *
 * @address 0x486ae0
 */
char Parser::parse_real(datum_index node_index) const
{
    hs_syntax_node *node;
    char *p;
    char c;
    char valid;
    char has_dot;

    node = halo::hs::syntax_node_at(node_index);
    p = halo::hs::globals().compiled_source + node->source_offset;
    has_dot = 0;
    valid = 1;
    if (*p == '-') {
        p = p + 1;
    }
    c = *p;
    for (;;) {
        if (c == '\0') {
            node->data.real_value = (float)atof(halo::hs::globals().compiled_source + node->source_offset);
            return valid;
        }
        if (!isdigit((unsigned char)c)) {
            if ((has_dot != 0) || (*p != '.')) {
                halo::hs::globals().compile_error = const_cast<char *>("this is not a valid real number.");
                halo::hs::globals().compile_error_offset = node->source_offset;
                valid = 0;
                node->data.real_value = (float)atof(halo::hs::globals().compiled_source + node->source_offset);
                return valid;
            }
            has_dot = 1;
        }
        c = p[1];
        p = p + 1;
    }
    node->data.real_value = (float)atof(halo::hs::globals().compiled_source + node->source_offset);
    return valid;
}

/**
 * Generic parser for a quoted name token against an arbitrary-stride scenario tag-block array: linearly scans
 * `array` (an element `count` and a `pointer` to the first element, laid out like a TagReflexive) comparing each
 * element's TagString name (at `name_offset` bytes into the element) case-insensitively against the token text.
 * On a match, stores the element's index in the node and returns 1 (success). On no match, formats a "this is
 * not a valid %s name" compiler error and returns 0.
 *
 * @address 0x486f90
 */
char Parser::parse_scenario_datum(datum_index node_index, int16_t name_offset, TagReflexive *array, int32_t stride) const
{
    hs_syntax_node *node;
    int16_t i;
    char *element;

    node = halo::hs::syntax_node_at(node_index);

    for (i = 0; i < (int16_t)array->count; i++) {
        element = (char *)array->pointer + i * stride;
        if (_stricmp(element + name_offset, halo::hs::globals().compiled_source + node->source_offset) == 0) {
            node->data.long_value = i;
            return 1;
        }
    }

    sprintf(halo::hs::globals().compile_error_buffer, "this is not a valid %s name", halo::hs::globals().type_names[node->type]);
    halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
    halo::hs::globals().compile_error_offset = node->source_offset;
    return 0;
}

/**
 * Behaviour of the original `hs_parse_script` function, moved unchanged into the class.
 *
 * @address 0x486c80
 */
char Parser::parse_script(datum_index node_index) const
{
    hs_syntax_node *node = halo::hs::syntax_node_at(node_index);
    int16_t script_index = halo::hs::hs_script_find_by_name(halo::hs::globals().compiled_source + node->source_offset);

    if (script_index != -1) {
        node->data.short_value = script_index;
        return 1;
    }
    halo::hs::globals().compile_error = const_cast<char *>("this is not a valid script name.");
    halo::hs::globals().compile_error_offset = node->source_offset;
    return 0;
}

/**
 * Parses/type-checks a (set <global> <value>) special-form syntax node.
 *
 * @address 0x484be0
 */
char Parser::parse_set(int16_t function_index, datum_index node_index) const
{
    data_array *nodes;
    hs_syntax_node *node;
    datum_index variable_index;
    hs_syntax_node *variable_node;
    datum_index value_index;
    hs_syntax_node *value_node;
    hs_syntax_node *extra_node;
    hs_global_reference global;
    hs_type_t global_type;
    char compatible;
    char ok;

    (void)function_index;
    nodes = halo::hs::globals().syntax_data;
    node = halo::hs::syntax_node_at(node_index);
    variable_index = node->data.first_child;
    if (variable_index == k_datum_index_none) {
        halo::hs::globals().compile_error = const_cast<char *>("i expected a variable to set and a value.");
        halo::hs::globals().compile_error_offset = node->source_offset;
        return 0;
    }
    variable_node = halo::hs::syntax_node_at(variable_index);
    value_index = variable_node->next_node;
    if (value_index == k_datum_index_none) {
        halo::hs::globals().compile_error = const_cast<char *>("i expected an assignment value.");
        halo::hs::globals().compile_error_offset = node->source_offset;
        return 0;
    }
    value_node = halo::hs::syntax_node_at(value_index);
    if (value_node->next_node != k_datum_index_none) {
        extra_node = halo::hs::syntax_node_at(value_node->next_node);
        halo::hs::globals().compile_error = const_cast<char *>("i didn't expect this argument.");
        halo::hs::globals().compile_error_offset = extra_node->source_offset;
        return 0;
    }

    global = halo::hs::hs_find_global_by_name(halo::hs::globals().compiled_source + variable_node->source_offset);
    if (global == k_hs_global_reference_none) {
        halo::hs::globals().compile_error = const_cast<char *>("this is not a valid global variable.");
        halo::hs::globals().compile_error_offset = variable_node->source_offset;
        return 0;
    }
    global_type = halo::hs::hs_global_get_type(global);
    variable_node->type = global_type;
    if ((node->type != 0) &&
        ((compatible = halo::hs::hs_types_are_compatible(node->type, global_type)), compatible == 0)) {
        sprintf(halo::hs::globals().compile_error_buffer,
                "you cannot pass the result of this set (type %s) to a function that expects type %s.",
                halo::hs::globals().type_names[global_type], halo::hs::globals().type_names[node->type]);
        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = node->source_offset;
        return 0;
    }
    halo::hs::hs_parse_variable(variable_index);
    if (node->type == 0) {
        node->type = variable_node->type;
    }
    ok = halo::hs::hs_parse(value_index, variable_node->type);
    return ok != 0;
}

/**
 * Behaviour of the original `hs_parse_sleep` function, moved unchanged into the class.
 *
 * @address 0x485280
 */
char Parser::parse_sleep(int16_t function_index, datum_index node_index) const
{
    hs_syntax_node *call = halo::hs::syntax_node_at(node_index);
    datum_index time = halo::hs::syntax_node_at(*(datum_index *)&call->data)->next_node;
    datum_index script;

    if (time == halo::k_dword_none) {
        halo::hs::globals().compile_error = const_cast<char *>("the sleep call requires a time and, optionally, a script name.");
        halo::hs::globals().compile_error_offset = call->source_offset;
        return 0;
    }
    if (!halo::hs::hs_parse(time, 7)) {
        return 0;
    }
    script = halo::hs::syntax_node_at(time)->next_node;
    if (script != halo::k_dword_none && !halo::hs::hs_parse(script, 0xa)) {
        return 0;
    }
    return 1;
}

/**
 * Behaviour of the original `hs_parse_sleep_until` function, moved unchanged into the class.
 *
 * @address 0x485310
 */
char Parser::parse_sleep_until(int16_t function_index, datum_index node_index) const
{
    hs_syntax_node *call = halo::hs::syntax_node_at(node_index);
    datum_index condition = halo::hs::syntax_node_at(*(datum_index *)&call->data)->next_node;
    datum_index period;
    datum_index timeout;
    char ok;

    if (condition == halo::k_dword_none) {
        halo::hs::globals().compile_error = const_cast<char *>("the sleep_until call requires a condition and, optionally, a period.");
        halo::hs::globals().compile_error_offset = call->source_offset;
        return 0;
    }
    period = halo::hs::syntax_node_at(condition)->next_node;
    ok = halo::hs::hs_parse(condition, 5);
    if (!ok || period == halo::k_dword_none) {
        return ok;
    }
    timeout = halo::hs::syntax_node_at(period)->next_node;
    ok = halo::hs::hs_parse(period, 7);
    if (!ok || timeout == halo::k_dword_none) {
        return ok;
    }
    return halo::hs::hs_parse(timeout, 8);
}

/**
 * Behaviour of the original `hs_parse_starting_profile` function, moved unchanged into the class.
 *
 * @address 0x4871d0
 */
char Parser::parse_starting_profile(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 0, &halo::scenario::globals().scenario->player_starting_profile, 0x68);
}

/**
 * Behaviour of the original `hs_parse_string` function, moved unchanged into the class.
 *
 * @address 0x486c50
 */
char Parser::parse_string(datum_index node_index) const
{
    hs_syntax_node *node = halo::hs::syntax_node_at(node_index);
    node->data.string_value = (int32_t)(halo::hs::globals().compiled_source + node->source_offset);
    return 1;
}

/**
 * Behaviour of the original `hs_parse_string_arguments` function, moved unchanged into the class.
 *
 * @address 0x487530
 */
char Parser::parse_string_arguments(int16_t function_index, datum_index node_index) const
{
    datum_index argument = halo::hs::syntax_node_at(*(datum_index *)&halo::hs::syntax_node_at(node_index)->data)->next_node;

    for (; argument != halo::k_dword_none; argument = halo::hs::syntax_node_at(argument)->next_node) {
        if (!parse_typed_argument(argument, 9)) {
            return 0;
        }
    }
    return 1;
}

/**
 * Looks up a parsed enum/keyword token by exact name in Scenario::references and stores its TagID if found and
 * its tag group matches the group expected for the node's type. Always returns 1; a non-match
 * simply leaves the node's data untouched.
 *
 * @address 0x486ce0
 */
char Parser::parse_tag_reference(datum_index node_index) const
{
    hs_syntax_node *node;
    int32_t count;
    int32_t i;
    ScenarioReference *reference;
    char *token_text;

    node = halo::hs::syntax_node_at(node_index);
    count = (int32_t)halo::scenario::globals().scenario->references.count;
    token_text = halo::hs::globals().compiled_source + node->source_offset;
    for (i = 0; i < count; i = i + 1) {
        reference = (ScenarioReference *)halo::scenario::globals().scenario->references.pointer + i;
        if ((strcmp((char *)reference->reference.path_pointer, token_text) == 0) &&
            (reference->reference.tag_fourcc == hs_tag_group_for_type[node->type - 0x18])) {
            node->data.tag_reference = halo::objects::tag_handle(reference->reference);
            return 1;
        }
    }
    return 1;
}

/**
 * Parses a trigger_volume-typed token: resolves the quoted name against Scenario::trigger_volumes.
 *
 * @address 0x487030
 */
char Parser::parse_trigger_volume(datum_index node_index) const
{
    return halo::hs::hs_parse_scenario_datum(node_index, 4, &halo::scenario::globals().scenario->trigger_volumes, 0x60);
}

/**
 * Requires exactly two arguments and parses them so both resolve to the same type: the second argument's
 * expected type follows whichever of the two parses first, falling back to _hs_type_real for the first argument
 * if neither parses without an expected type.
 *
 * @address 0x485060
 */
char Parser::parse_two_numeric_arguments(int16_t function_index, datum_index node_index) const
{
    datum_index arguments[2];
    char ok;
    char result;
    hs_type_t type;

    result = 0;
    ok = halo::hs::hs_get_parameter_indices(halo::hs::globals().function_definitions[function_index]->name, 2, node_index, arguments);
    if (ok != 0) {
        ok = halo::hs::hs_parse(arguments[0], 0);
        if (ok == 0) {
            if (halo::hs::globals().compile_error == 0) {
                ok = halo::hs::hs_parse(arguments[1], 0);
                if (ok == 0) {
                    if (halo::hs::globals().compile_error != 0) {
                        return 0;
                    }
                    ok = halo::hs::hs_parse(arguments[0], _hs_type_real);
                    if (ok == 0) {
                        return 0;
                    }
                    type = _hs_type_real;
                    arguments[0] = arguments[1];
                } else {
                    type = (halo::hs::syntax_node_at(arguments[1]))->type;
                }
                ok = halo::hs::hs_parse(arguments[0], type);
                if (ok != 0) {
                    result = 1;
                }
            }
        } else {
            type = (halo::hs::syntax_node_at(arguments[0]))->type;
            ok = halo::hs::hs_parse(arguments[1], type);
            if (ok != 0) {
                return 1;
            }
        }
    }
    return result;
}

/**
 * Requires exactly two arguments; if the first parses to a type in [0x20,0x25) or [6,9), the second is parsed
 * against that same type, and vice versa if only the second qualifies; otherwise both are parsed against
 * _hs_type_real.
 *
 * @address 0x485150
 */
char Parser::parse_two_object_arguments(int16_t function_index, datum_index node_index) const
{
    datum_index arguments[2];
    char ok;
    hs_type_t type;

    ok = halo::hs::hs_get_parameter_indices(halo::hs::globals().function_definitions[function_index]->name, 2, node_index, arguments);
    if (ok == 0) {
        return 0;
    }
    ok = halo::hs::hs_parse(arguments[0], 0);
    if (ok != 0) {
        type = (halo::hs::syntax_node_at(arguments[0]))->type;
        if (((0x1f < type) && (type < 0x25)) || ((5 < type) && (type < 9))) {
            ok = halo::hs::hs_parse(arguments[1], type);
            return ok != 0;
        }
    }
    if (halo::hs::globals().compile_error != 0) {
        return 0;
    }
    ok = halo::hs::hs_parse(arguments[1], 0);
    if (ok != 0) {
        type = (halo::hs::syntax_node_at(arguments[1]))->type;
        if (((0x1f < type) && (type < 0x25)) || ((5 < type) && (type < 9))) {
            ok = halo::hs::hs_parse(arguments[0], type);
            return ok != 0;
        }
    }
    if (halo::hs::globals().compile_error != 0) {
        return 0;
    }
    ok = halo::hs::hs_parse(arguments[0], _hs_type_real);
    if (ok == 0) {
        return 0;
    }
    type = _hs_type_real;
    arguments[0] = arguments[1];
    ok = halo::hs::hs_parse(arguments[0], type);
    return ok != 0;
}

/**
 * Behaviour of the original `hs_parse_unit` function, moved unchanged into the class.
 *
 * @address 0x4854f0
 */
char Parser::parse_unit(int16_t function_index, datum_index node_index) const
{
    datum_index argument;

    if (!halo::hs::hs_get_parameter_indices(halo::hs::globals().function_definitions[function_index]->name, 1, node_index, &argument)) {
        return 0;
    }
    return halo::hs::hs_parse(argument, 0x25);
}

/**
 * Parses/type-checks a bare-word syntax node as a reference to a global variable (builtin or scenario-defined),
 * adopting the global's type if the node had no expected type yet, or reporting a type mismatch against one it
 * did.
 *
 * @address 0x486560
 */
char Parser::parse_variable(datum_index node_index) const
{
    hs_syntax_node *node;
    hs_global_reference global;
    hs_type_t global_type;
    hs_type_t node_type;
    char compatible;
    char *global_name;

    node = halo::hs::syntax_node_at(node_index);
    global = halo::hs::hs_find_global_by_name(halo::hs::globals().compiled_source + node->source_offset);
    node->data.global_reference = (int16_t)global;
    if ((int16_t)global == -1) {
        if (halo::hs::globals().postprocessing != 0) {
            halo::hs::globals().compile_error = const_cast<char *>("this is not a valid variable name.");
            halo::hs::globals().compile_error_offset = node->source_offset;
        }
        return 0;
    }

    if ((global & k_hs_global_builtin_bit) == 0) {
        global_type = ((ScenarioGlobal *)halo::scenario::globals().scenario->globals.pointer)[global & k_hs_global_index_mask].type;
    } else {
        global_type = halo::hs::globals().global_definitions[global & k_hs_global_index_mask]->type;
    }

    node_type = node->type;
    if (node_type != 0) {
        compatible = halo::hs::hs_types_are_compatible(node_type, global_type);
        if (compatible == 0) {
            global_name = halo::hs::hs_global_get_name(global);
            sprintf(halo::hs::globals().compile_error_buffer, "i expected a value of type %s, but the variable %s has type %s",
                    halo::hs::globals().type_names[node_type], global_name, halo::hs::globals().type_names[global_type]);
            halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
            halo::hs::globals().compile_error_offset = node->source_offset;
            return 0;
        }

    } else {
        node->type = global_type;
    }
    node->flags = node->flags | _hs_syntax_node_global_bit;
    return 1;
}

/**
 * Behaviour of the original `hs_parse_wake` function, moved unchanged into the class.
 *
 * @address 0x4853c0
 */
char Parser::parse_wake(int16_t function_index, datum_index node_index) const
{
    datum_index argument;
    hs_syntax_node *node;
    ScenarioScript *script;

    if (!halo::hs::hs_get_parameter_indices(halo::hs::globals().function_definitions[function_index]->name, 1, node_index, &argument)) {
        return 0;
    }
    node = halo::hs::syntax_node_at(argument);
    if (!halo::hs::hs_parse(argument, 0xa)) {
        return 0;
    }
    script = (ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer + node->data.short_value;
    if (script->script_type == 3 || script->script_type == 4) {
        halo::hs::globals().compile_error = const_cast<char *>("this static script cannot be awakened.");
        halo::hs::globals().compile_error_offset = node->source_offset;
        return 0;
    }
    return 1;
}

/**
 * Parses an enum-typed primitive token: looks the token's text up (case-insensitively) in the name list of its
 * expected enum type (hs_type 0x20..0x24: game_difficulty, team, ai_default_state, actor_type, hud_corner). On a
 * match, stores the matched index in the node and returns 1. On no match, formats a `"%s must be "a", "b" or
 * "c"."` compiler error into hs_compile_error_buffer, points hs_compile_error at it, records the node's source
 * offset as hs_compile_error_offset, and returns 0.
 *
 * @address 0x486dc0
 */
char Parser::report_expected_enum_values(datum_index node_index) const
{
    hs_syntax_node *node;
    hs_enum_definition *def;
    int16_t match_index;
    int16_t last_index;
    int16_t i;

    node = halo::hs::syntax_node_at(node_index);
    def = &hs_enum_definitions[node->type - 0x20];

    match_index = 0;
    if (0 < def->count) {
        do {
            if (_stricmp(halo::hs::globals().compiled_source + node->source_offset, def->names[match_index]) == 0)
                break;
            match_index = match_index + 1;
        } while (match_index < def->count);
    }

    if (match_index == def->count) {
        sprintf(halo::hs::globals().compile_error_buffer, "%s must be ", halo::hs::globals().type_names[node->type]);
        last_index = 0;
        if (0 < def->count - 1) {
            for (i = 0; i < def->count - 1; i++) {
                strcat(halo::hs::globals().compile_error_buffer, "\"");
                strcat(halo::hs::globals().compile_error_buffer, def->names[i]);
                strcat(halo::hs::globals().compile_error_buffer, "\", ");
                last_index = i + 1;
            }
        }
        if (1 < def->count) {
            strcat(halo::hs::globals().compile_error_buffer, "or ");
        }
        strcat(halo::hs::globals().compile_error_buffer, "\"");
        strcat(halo::hs::globals().compile_error_buffer, def->names[last_index]);
        strcat(halo::hs::globals().compile_error_buffer, "\".");

        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = node->source_offset;
        node->data.short_value = last_index;
        return 0;
    }

    node->data.short_value = match_index;
    return 1;
}

/**
 * Resolves the identifier at the head of call node `node_index` to either a builtin function index or a
 * user-script index, caching the result on both the call node and the identifier node so a second visit skips
 * the name lookup entirely.
 *
 * @address 0x486680
 */
void Parser::resolve_identifier_as_function_or_script(datum_index node_index) const
{
    data_array *nodes;
    hs_syntax_node *node;
    hs_syntax_node *identifier_node;
    int16_t function_index;
    int16_t script_index;

    nodes = halo::hs::globals().syntax_data;
    node = halo::hs::syntax_node_at(node_index);
    identifier_node = halo::hs::syntax_node_at(node->data.first_child);
    if (identifier_node->type != _hs_type_function_name) {
        function_index = halo::hs::hs_find_function_by_name(halo::hs::globals().compiled_source + identifier_node->source_offset);
        node->index_union = function_index;
        identifier_node->type = _hs_type_function_name;
        if (node->index_union == -1) {
            script_index = halo::hs::hs_script_find_by_name(halo::hs::globals().compiled_source + identifier_node->source_offset);
            node->index_union = script_index;
            if (script_index != -1) {
                node->flags = node->flags | _hs_syntax_node_script_call_bit;
            }
        }
        identifier_node->index_union = node->index_union;
        return;
    }
    node->index_union = identifier_node->index_union;
}

}
