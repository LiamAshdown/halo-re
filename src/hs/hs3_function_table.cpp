#include "halo/hs/records.hpp"
#include "halo/hs/hs3_machine.hpp"
#include "crt.h"
#include <string.h>
#include <stdio.h>
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/hs/api.hpp"
#include "halo/interface/api.hpp"


namespace halo::hs::part3 {

/**
 * Looks up a script function's table index by name, first rewriting the legacy alias
 * "player_effect_set_max_rumble" to its current name "player_effect_set_max_vibrate".
 *
 * @address 0x483520
 */
int16_t FunctionTable::find_function_by_name(char *name) const
{
    char alias[32];
    int16_t index;

    strcpy(alias, "player_effect_set_max_rumble");
    if (_stricmp(name, alias) == 0) {
        name = const_cast<char *>("player_effect_set_max_vibrate");
    }
    for (index = 0; index < k_hs_function_count; index++) {
        if (_stricmp(halo::hs::globals().function_definitions[index]->name, name) == 0) {
            return index;
        }
    }
    return -1;
}

/**
 * Formats a human-readable "(name arg<type> ...)" prototype string for hs_function_definitions entry
 * `function_index` into `out`.
 *
 * @address 0x484300
 */
void FunctionTable::format_function_signature(int16_t function_index, char *out) const
{
    hs_function_definition *def;
    char *end;
    int16_t i;

    def = halo::hs::globals().function_definitions[function_index];
    sprintf(out, "(%s", def->name);
    if (def->param_info != 0) {
        end = out + strlen(out);
        sprintf(end, " %s", def->param_info);
    } else {
        for (i = 0; i < def->parameter_count; i = i + 1) {
            end = out + strlen(out);
            sprintf(end, " <%s>", halo::hs::globals().type_names[def->parameters[i]]);
        }
    }
    end = out + strlen(out);
    end[0] = ')';
    end[1] = '\0';
}

/**
 * Collects call node `node_index`'s argument-node indices into out_indices[0..required_count), reporting an
 * error if it does not have exactly required_count arguments.
 *
 * @address 0x484fb0
 */
char FunctionTable::get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index, datum_index *out_indices) const
{
    data_array *nodes;
    hs_syntax_node *node;
    datum_index child;
    int16_t count;
    char success;

    nodes = halo::hs::globals().syntax_data;
    node = halo::hs::syntax_node_at(node_index);
    child = (halo::hs::syntax_node_at(node->data.first_child))->next_node;
    success = 1;
    for (count = 0; (child != k_datum_index_none) && (count < required_count); count = count + 1) {
        out_indices[count] = child;
        child = (halo::hs::syntax_node_at(child))->next_node;
    }
    if ((count != required_count) || (child != k_datum_index_none)) {
        sprintf(halo::hs::globals().compile_error_buffer, "the %s call requires %d arguments.", function_name, (int)required_count);
        halo::hs::globals().compile_error = halo::hs::globals().compile_error_buffer;
        halo::hs::globals().compile_error_offset = node->source_offset;
        success = 0;
    }
    return success;
}

/**
 * Console command handler that prints a single HS function's formatted signature and documentation string (split
 * across multiple console lines at each '\n').
 *
 * @address 0x4841b0
 */
void FunctionTable::help_print_function(char *name) const
{
    int16_t function_index;
    char buffer[2048];
    char *newline;
    char *line;

    function_index = halo::hs::hs_find_function_by_name(name);
    if (function_index != -1) {
        halo::hs::hs_format_function_signature(function_index, buffer);
        halo::interface::chimera__console_out((ColorARGB *)0, buffer);
        strcpy(buffer, halo::hs::globals().function_definitions[function_index]->info);
        newline = strchr(buffer, '\n');
        if (newline == 0) {
            halo::interface::chimera__console_out((ColorARGB *)0, buffer);
            return;
        }
        line = buffer;
        while (line != 0) {
            if (newline == 0) {
                halo::interface::chimera__console_out((ColorARGB *)0, line);
                return;
            }
            *newline = '\0';
            halo::interface::chimera__console_out((ColorARGB *)0, line);
            line = newline + 1;
            newline = strchr(line, '\n');
        }
    }
}

/**
 * Evaluate handler of the hs script function `hs_null_with_params_evaluate`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x483430
 */
hs_type_t FunctionTable::null_with_params_evaluate(hs_syntax_node *node) const
{
    return node->type;
}

/**
 * Returns the index of the scenario script named `name`, or -1 if no scenario is loaded or no script matches.
 *
 * @address 0x4833a0
 */
int16_t FunctionTable::script_find_by_name(char *name) const
{
    ScenarioScript *scripts;
    int32_t count;
    int32_t i;

    if (halo::scenario::globals().scenario_index == k_datum_index_none) {
        return -1;
    }
    count = (int32_t)halo::scenario::globals().scenario->scripts.count;
    if (0 < count) {
        scripts = (ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer;
        for (i = 0; i < count; i++) {
            if (strcmp((char *)name, scripts[i].name.string) == 0) {
                return (int16_t)i;
            }
        }
    }
    return -1;
}

}
