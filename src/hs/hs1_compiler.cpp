#include "halo/hs/hs1_compiler.hpp"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "halo/memory/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

extern "C" {
extern int16_t string_table_index_of(const char *search, int16_t count, const char **table);
extern hs_global_reference hs_find_global_by_name(char *name);
extern char hs_parse(datum_index node_index, hs_type_t expected_type);
extern data_array *hs_syntax_data;
extern char *hs_compiled_source;
extern char *hs_type_names[k_hs_type_count];
extern uint8_t hs_blocking_forbidden;
extern uint8_t hs_set_forbidden;
extern char *hs_compile_error;
extern int32_t hs_compile_error_offset;
extern int16_t hs_script_find_by_name(char *name);
extern char *hs_script_type_names[k_hs_script_type_count];
extern Scenario *global_scenario;
extern char *hs_source_buffer_append(char *text, uint32_t length);
extern void skip_whitespace(char **cursor);
extern datum_index hs_tokenize(char **cursor);
extern char hs_parse_nonprimitive(datum_index node_index);
extern char hs_parse_primitive(datum_index node_index);
extern uint8_t hs_syntax_data_dirty;
extern int32_t hs_compiled_source_length;
extern datum_index hs_compile_expression(char *text, uint32_t length, char **error_message, char **error_offset);
extern void hs_evaluate_expression(datum_index node_index);
extern char hs_rebuild_source(void);
extern char hs_compile_source(void);
extern void hs_scripts_free(void);
extern void hs_scripts_reload(void);
extern uint8_t hs_compiling;
extern uint8_t hs_compile_release_source;
extern uint8_t hs_compiled_source_owned;
extern uint8_t hs_reload_pending;
extern datum_index global_scenario_index;
extern char hs_verify_source_offset(int32_t offset);
extern int16_t hs_find_function_by_name(char *name);
extern hs_type_t hs_global_get_type(hs_global_reference global);
extern char hs_types_are_compatible(hs_type_t destination_type, hs_type_t source_type);
extern uint8_t hs_postprocessing;
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern void hs_compile(int32_t source_length, char *source_text, char **error_message, int32_t *error_offset);
extern void console_print_error_va(uint8_t clear_first, const char *format, ...);
}

namespace halo::hs {

/**
 * Validates a (global <type> <name> <value>) declaration: shape, type keyword, name length, name collision,
 * and the initializer expression.
 *
 * @address 0x485b60
 */
char ScriptCompiler::add_global(datum_index node_index)
{
    data_array *nodes;
    hs_syntax_node *node;
    datum_index identifier_index;
    hs_syntax_node *identifier_node;
    datum_index type_index;
    hs_syntax_node *type_node;
    datum_index name_index;
    hs_syntax_node *name_node;
    datum_index value_index;
    hs_syntax_node *value_node;
    int16_t type_ordinal;
    char *name_text;
    size_t name_length;
    hs_global_reference existing;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & halo::k_slot_mask) * nodes->size);
    identifier_index = node->data.first_child;
    if (identifier_index != k_datum_index_none) {
        identifier_node = (hs_syntax_node *)((uint8_t *)nodes->data + (identifier_index & halo::k_slot_mask) * nodes->size);
        type_index = identifier_node->next_node;
        if (type_index != k_datum_index_none) {
            type_node = (hs_syntax_node *)((uint8_t *)nodes->data + (type_index & halo::k_slot_mask) * nodes->size);
            name_index = type_node->next_node;
            if (name_index != k_datum_index_none) {
                name_node = (hs_syntax_node *)((uint8_t *)nodes->data + (name_index & halo::k_slot_mask) * nodes->size);
                value_index = name_node->next_node;
                if (value_index != k_datum_index_none) {
                    value_node = (hs_syntax_node *)((uint8_t *)nodes->data + (value_index & halo::k_slot_mask) * nodes->size);
                    if (value_node->next_node == k_datum_index_none) {
                        type_ordinal = string_table_index_of(hs_compiled_source + type_node->source_offset,
                                                     k_hs_type_count, (const char **)hs_type_names);
                        if ((type_ordinal < 4) || (0x30 < type_ordinal)) {
                            hs_compile_error = (char *)"this is not a valid type.";
                            hs_compile_error_offset = type_node->source_offset;
                            return 0;
                        }
                        name_text = hs_compiled_source + name_node->source_offset;
                        name_length = strlen(name_text);
                        if ((name_length != 0) && (name_length < 0x20)) {
                            existing = hs_find_global_by_name(name_text);
                            if (existing == k_hs_global_reference_none) {
                                hs_blocking_forbidden = 1;
                                hs_set_forbidden = 1;
                                if (hs_parse(value_index, type_ordinal) != 0) {
                                    hs_compile_error = (char *)"i couldn't allocate space for this global.";
                                    hs_compile_error_offset = node->source_offset;
                                }
                                hs_blocking_forbidden = 0;
                                hs_set_forbidden = 0;
                                return 0;
                            }
                            hs_compile_error = (char *)"there is already a variable by this name.";
                            hs_compile_error_offset = name_node->source_offset;
                            return 0;
                        }
                        hs_compile_error = (char *)"i expected a global variable name less than 32 characters.";
                        hs_compile_error_offset = name_node->source_offset;
                        return 0;
                    }
                }
            }
        }
    }
    hs_compile_error = (char *)"i expected (global<type> <name> <initial value>)";
    hs_compile_error_offset = node->source_offset;
    return 0;
}

/**
 * Parses and registers a (script <type> ... <name> <body>) declaration, enforcing naming, typing, and
 * stub-override rules.
 *
 * @address 0x485d50
 */
char ScriptCompiler::add_script(datum_index node_index)
{
    data_array *nodes;
    hs_syntax_node *node;
    datum_index type_index;
    hs_syntax_node *type_node;
    int16_t script_type;
    datum_index return_type_index;
    hs_syntax_node *return_type_node;
    hs_type_t return_type;
    datum_index name_index;
    hs_syntax_node *name_node;
    datum_index body_index;
    char *name_text;
    size_t name_length;
    int16_t existing_index;
    ScenarioScript *existing_script;
    datum_index new_root;
    datum_index new_body_holder;
    hs_syntax_node *new_root_node;
    hs_syntax_node *new_body_holder_node;
    char ok;
    char *dest;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & halo::k_slot_mask) * nodes->size);
    type_index = node->data.first_child;
    if (type_index == k_datum_index_none) {
        hs_compile_error = (char *)"i expected (script <type> <name> <expression(s)>)";
        hs_compile_error_offset = node->source_offset;
        return 0;
    }
    type_node = (hs_syntax_node *)((uint8_t *)nodes->data + (type_index & halo::k_slot_mask) * nodes->size);
    script_type = string_table_index_of(hs_compiled_source + type_node->source_offset,
                                k_hs_script_type_count, (const char **)hs_script_type_names);
    if (script_type == -1) {
        hs_compile_error = (char *)"script type must be \"startup\", \"dormant\", \"continuous\", or \"static\".";
        hs_compile_error_offset = type_node->source_offset;
        return 0;
    }

    if ((script_type == _hs_script_static) || (script_type == _hs_script_stub)) {
        return_type_index = type_node->next_node;
        if (return_type_index == k_datum_index_none) {
            hs_compile_error = (char *)"i expected (script local <type> <name> <expression(s)>).";
            hs_compile_error_offset = node->source_offset;
            return 0;
        }
        return_type_node = (hs_syntax_node *)((uint8_t *)nodes->data + (return_type_index & halo::k_slot_mask) * nodes->size);
        return_type = string_table_index_of(hs_compiled_source + return_type_node->source_offset,
                                    k_hs_type_count, (const char **)hs_type_names);
        name_index = return_type_node->next_node;
        if ((return_type < 4) || (0x30 < return_type)) {
            hs_compile_error = (char *)"this is not a valid return type.";
            hs_compile_error_offset = return_type_node->source_offset;
            return 0;
        }
    } else {
        name_index = type_node->next_node;
        return_type = _hs_type_void;
    }

    if (name_index != k_datum_index_none) {
        name_node = (hs_syntax_node *)((uint8_t *)nodes->data + (name_index & halo::k_slot_mask) * nodes->size);
        body_index = name_node->next_node;
        if (body_index != k_datum_index_none) {
            name_text = hs_compiled_source + name_node->source_offset;
            name_length = strlen(name_text);
            if ((name_length != 0) && (name_length < 0x20)) {
                existing_index = hs_script_find_by_name(name_text);
                if (existing_index == -1) {
                    hs_compile_error = (char *)"i couldn't allocate a script.";
                    hs_compile_error_offset = node->source_offset;
                    return 0;
                }
                existing_script = (ScenarioScript *)global_scenario->scripts.pointer + existing_index;
                if ((existing_script->script_type == _hs_script_stub) &&
                    (existing_script->return_type == return_type) &&
                    (script_type == _hs_script_static)) {
                    new_root = halo::memory::datum_new(nodes);
                    new_body_holder = halo::memory::datum_new(nodes);
                    if ((new_root == k_datum_index_none) || (new_body_holder == k_datum_index_none)) {
                        hs_compile_error = (char *)"i couldn't allocate a syntax node.";
                        return 0;
                    }
                    nodes = hs_syntax_data;
                    new_root_node = (hs_syntax_node *)((uint8_t *)nodes->data + (new_root & halo::k_slot_mask) * nodes->size);
                    new_body_holder_node = (hs_syntax_node *)((uint8_t *)nodes->data + (new_body_holder & halo::k_slot_mask) * nodes->size);

                    new_root_node->data.first_child = new_body_holder;
                    new_root_node->next_node = k_datum_index_none;
                    new_root_node->source_offset = node->source_offset;
                    new_root_node->flags = 0;

                    new_body_holder_node->source_offset = -1;
                    new_body_holder_node->next_node = body_index;
                    new_body_holder_node->index_union = 0;
                    new_body_holder_node->flags = _hs_syntax_node_primitive_bit;
                    new_body_holder_node->type = _hs_type_function_name;

                    ok = hs_parse(new_root, return_type);
                    if (ok != 0) {
                        dest = existing_script->name.string;
                        strcpy(dest, name_text);
                        existing_script->return_type = return_type;
                        existing_script->root_expression_index = new_root;
                        existing_script->script_type = _hs_script_static;
                        return 1;
                    }
                    return 0;
                }
                if ((existing_script->script_type == _hs_script_static) &&
                    (existing_script->return_type == return_type) &&
                    (script_type == _hs_script_stub)) {
                    return 1;
                }
                hs_compile_error = (char *)"only static scripts of the same type can override stub scripts.";
                hs_compile_error_offset = node->source_offset;
                return 0;
            }
            hs_compile_error = (char *)"i expected a script name less than 32 characters.";
            hs_compile_error_offset = name_node->source_offset;
            return 0;
        }
    }

    if (script_type == _hs_script_static) {
        hs_compile_error = (char *)"i expected (script static <type> <name> <expression(s)>)";
    } else if (script_type == _hs_script_stub) {
        hs_compile_error = (char *)"i expected (script stub <type> <name> <expression(s)>)";
    } else {
        hs_compile_error = (char *)"i expected (script <type> <name> <expression(s)>)";
    }
    hs_compile_error_offset = node->source_offset;
    return 0;
}

/**
 * Tokenizes and type-checks every top-level expression of `source_text` (source_length bytes, the source
 * file's own recorded length) as an implicit special form, appending it to hs_compiled_source first. On the
 * first failure, reports the error message/offset relative to this source file's own position in the
 * combined buffer.
 *
 * @address 0x485770
 */
void ScriptCompiler::compile(int32_t source_length, char *source_text, char **error_message, int32_t *error_offset)
{
    char *cursor;
    char success;
    datum_index node_index;
    hs_syntax_node *node;

    cursor = hs_source_buffer_append(source_text, (uint32_t)source_length);
    if (cursor == 0) {
        *error_message = (char *)"couldn't allocate memory for compiled source.";
        return;
    }
    hs_compile_error = 0;
    *error_message = 0;
    *error_offset = 0;
    hs_compile_error_offset = -1;
    skip_whitespace(&cursor);
    do {
        if (*cursor == '\0') {
            if (success != 0) {
                return;
            }
            break;
        }
        node_index = hs_tokenize(&cursor);
        skip_whitespace(&cursor);
        if (hs_compile_error != 0) {
            break;
        }
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & halo::k_slot_mask) * hs_syntax_data->size);
        success = 1;
        if (node->type == 0) {
            node->type = _hs_type_special_form;
            if ((node->flags & _hs_syntax_node_primitive_bit) == 0) {
                success = hs_parse_nonprimitive(node_index);
            } else {
                node->index_union = _hs_type_special_form;
                success = hs_parse_primitive(node_index);
            }
        }
    } while (success != 0);
    *error_message = hs_compile_error;
    hs_syntax_data_dirty = 1;
    if (hs_compile_error_offset != -1) {
        hs_compile_error_offset = hs_compile_error_offset + (source_length - hs_compiled_source_length);
        *error_offset = hs_compile_error_offset + (int32_t)source_text;
    }
}

/**
 * Compiles and immediately executes a single console/script command line, auto-wrapping bare global-set or
 * bare-call syntax, and triggers a full script reload when requested.
 *
 * @address 0x484400
 */
char ScriptCompiler::compile_and_evaluate(char *command)
{
    char result;
    char buffer[1024];
    char formatted[1024];
    char *semicolon;
    char *p;
    char *space;
    int16_t mode;
    char *format;
    hs_global_reference global;
    datum_index expr;
    char *error_message;
    char *error_offset;
    char *newline;

    result = 0;
    strncpy(buffer, command, 0x400);
    buffer[1023] = 0;
    semicolon = strchr(buffer, ';');
    if (semicolon != 0) {
        buffer[0] = 0;
    }
    p = buffer;
    if (buffer[0] != 0) {
        while (isspace((unsigned char)*p)) {
            p = p + 1;
            if (*p == 0) {
                goto reload_check;
            }
        }
        hs_compiling = 1;
        hs_compiled_source = 0;
        hs_compiled_source_length = 0;
        hs_compile_release_source = 0;
        hs_syntax_data_dirty = 0;
        hs_compile_error = 0;
        mode = 0;
        if (buffer[0] != '(') {
            space = strchr(buffer, ' ');
            if (space != 0) {
                *space = 0;
            }
            global = hs_find_global_by_name(buffer);
            if (global == k_hs_global_reference_none) {
                mode = 1;
            } else {
                mode = 0;
                if (space != 0) {
                    mode = 2;
                }
            }
            if (space != 0) {
                *space = ' ';
            }
        }
        if (mode == 1) {
            format = (char *)"(%s)";
            sprintf(formatted, format, buffer);
            command = formatted;
        } else if (mode == 2) {
            format = (char *)"(set %s)";
            sprintf(formatted, format, buffer);
            command = formatted;
        }
        expr = hs_compile_expression(command, (uint32_t)strlen(command), &error_message, &error_offset);
        if (expr == k_datum_index_none) {
            if ((error_message != 0) && (error_offset != 0)) {
                newline = strchr(error_offset, '\n');
                if (newline != 0) {
                    *newline = '\0';
                }
            }
        } else {
            result = 1;
            hs_evaluate_expression(expr);
        }
        if (hs_compile_release_source != 0) {
            if (hs_syntax_data_dirty != 0) {
                halo::memory::data_delete_all(hs_syntax_data);
            }
            if (hs_compiled_source != 0) {
                GlobalFree(hs_compiled_source);
            }
        }
        if (hs_compiled_source_owned != 0) {
            GlobalFree(hs_compiled_source);
            hs_compiled_source = 0;
            hs_compiled_source_owned = 0;
        }
        hs_compiling = 0;
    }
reload_check:
    if (hs_reload_pending != 0) {
        if (hs_rebuild_source() != 0) {
            hs_compile_source();
            hs_scripts_free();
            hs_scripts_reload();
        }
        hs_reload_pending = 0;
    }
    return result;
}

/**
 * Tokenizes and wraps a single standalone expression string into an "(inspect <expr>)" syntax-node tree
 * ready for immediate evaluation, returning its root node index or k_datum_index_none on failure.
 *
 * @address 0x485540
 */
datum_index ScriptCompiler::compile_expression(char *text, uint32_t length, char **error_message, char **error_offset)
{
    int32_t start;
    uint8_t *dest;
    char *src;
    uint32_t words;
    uint32_t tail_bytes;
    char *cursor;
    datum_index expr_index;
    hs_syntax_node *expr_node;
    data_array *nodes;
    datum_index wrap_index;
    datum_index inspect_index;
    hs_syntax_node *wrap_node;
    hs_syntax_node *inspect_node;
    char ok;

    if ((int32_t)length < k_hs_maximum_expression_length) {
        if (global_scenario_index == k_datum_index_none) {
            start = 0;
            hs_compiled_source = (char *)GlobalAlloc(0, length + 1);
            hs_compiled_source_owned = 1;
        } else {
            hs_compiled_source = (char *)global_scenario->script_string_data.pointer;
            start = (int32_t)global_scenario->script_string_data.size - 0x400;
        }
        dest = (uint8_t *)hs_compiled_source + start;
        src = text;
        for (words = length >> 2; words != 0; words = words - 1) {
            *(uint32_t *)dest = *(uint32_t *)src;
            src = src + 4;
            dest = dest + 4;
        }
        for (tail_bytes = length & 3; tail_bytes != 0; tail_bytes = tail_bytes - 1) {
            *dest = *src;
            src = src + 1;
            dest = dest + 1;
        }
        hs_compiled_source_length = (int32_t)length + start;
        hs_compiled_source[hs_compiled_source_length] = 0;
        hs_compile_error = 0;
        *error_message = 0;
        *error_offset = 0;
        hs_compile_error_offset = -1;
        cursor = hs_compiled_source + start;
        skip_whitespace(&cursor);
        if (*cursor != '\0') {
            expr_index = hs_tokenize(&cursor);
            if (hs_compile_error == 0) {
                wrap_index = halo::memory::datum_new(hs_syntax_data);
                inspect_index = halo::memory::datum_new(hs_syntax_data);
                if ((wrap_index != k_datum_index_none) && (inspect_index != k_datum_index_none)) {
                    nodes = hs_syntax_data;
                    expr_node = (hs_syntax_node *)((uint8_t *)nodes->data + (expr_index & halo::k_slot_mask) * nodes->size);
                    wrap_node = (hs_syntax_node *)((uint8_t *)nodes->data + (wrap_index & halo::k_slot_mask) * nodes->size);
                    inspect_node = (hs_syntax_node *)((uint8_t *)nodes->data + (inspect_index & halo::k_slot_mask) * nodes->size);

                    wrap_node->data.first_child = inspect_index;
                    wrap_node->next_node = k_datum_index_none;
                    wrap_node->source_offset = expr_node->source_offset;
                    wrap_node->flags = 0;

                    inspect_node->next_node = expr_index;
                    inspect_node->source_offset = -1;
                    inspect_node->index_union = _hs_function_inspect;
                    inspect_node->flags = _hs_syntax_node_primitive_bit;
                    inspect_node->type = _hs_type_function_name;

                    ok = hs_parse(wrap_index, _hs_type_void);
                    if (ok != 0) {
                        return wrap_index;
                    }
                }
            }
            *error_message = hs_compile_error;
            if (hs_compile_error_offset != -1) {
                hs_compile_error_offset = hs_compile_error_offset - start;
                *error_offset = hs_compile_error_offset + text;
            }
        }
    }
    return k_datum_index_none;
}

/**
 * Second compilation pass: for every live hs_syntax_node, re-resolves function/script/global references by
 * name or index (as loaded from a possibly stale scenario tag) and re-validates its type against how it is
 * used, reporting the first inconsistency found.
 *
 * @address 0x4858c0
 */
char ScriptCompiler::compile_postprocess(char **error_message, int32_t *error_offset)
{
    data_array *nodes;
    char success;
    datum_index current;
    hs_syntax_node *node;
    hs_type_t node_type;
    int32_t stale_script_pointer;
    int32_t next_start;
    int16_t next_index;
    hs_syntax_node *scan;
    char valid_offset;
    hs_syntax_node *function_name_node;
    int16_t function_index;

    nodes = hs_syntax_data;
    hs_compiled_source = (char *)global_scenario->script_string_data.pointer;
    hs_compiled_source_length = (int32_t)global_scenario->script_string_data.size - 0x400;
    success = 1;
    hs_compile_error = 0;
    hs_postprocessing = 1;
    *error_message = 0;
    *error_offset = 0;
    current = halo::memory::datum_next(-1, nodes);

    do {
        if (current == k_datum_index_none) {
            if (success != 0) {
                hs_compiled_source = 0;
                hs_compile_error = 0;
                hs_postprocessing = 0;
                return success;
            }
            break;
        }
        node = (hs_syntax_node *)((uint8_t *)nodes->data + (current & halo::k_slot_mask) * nodes->size);
        node_type = node->type;

        if ((node_type < 4) || (0x30 < node_type)) {
            if (node_type != 2) {
                hs_compile_error = (char *)"missing type (you need to recompile scripts.)";
                goto fail;
            }
            goto advance;
        }

        if ((node->flags & _hs_syntax_node_primitive_bit) == 0) {
            if ((node->flags & _hs_syntax_node_script_call_bit) != 0) {
                node_type = node->index_union;
                if ((((-1 < node_type) && (node_type < (int32_t)global_scenario->scripts.count)) &&
                     (stale_script_pointer = node_type * 0x5c + (int32_t)global_scenario->scripts.pointer,
                      *(int16_t *)(stale_script_pointer + 0x20) == _hs_script_static)) ||
                    (*(int16_t *)(stale_script_pointer + 0x20) == _hs_script_stub)) {
                    node_type = *(int16_t *)(stale_script_pointer + 0x22);
                    goto resolved;
                }
                hs_compile_error = (char *)"bad script index (you need to recompile.)";
                goto fail;
            }
            if (node->data.first_child == k_datum_index_none) {
                hs_compile_error = (char *)"corrupt syntax tree (you need to recompile scripts.)";
                goto fail;
            }
            function_name_node = (hs_syntax_node *)((uint8_t *)nodes->data + (node->data.first_child & halo::k_slot_mask) * nodes->size);
            if (function_name_node->type != 2) {
                hs_compile_error = (char *)"corrupt syntax tree (you need to recompile scripts.)";
                goto fail;
            }
            valid_offset = hs_verify_source_offset(function_name_node->source_offset);
            if (valid_offset == 0) {
                goto fail;
            }
            function_index = hs_find_function_by_name(hs_compiled_source + function_name_node->source_offset);
            nodes = hs_syntax_data;
            if (function_index == -1) {
                hs_compile_error = (char *)"missing function (you need to recompile scripts.)";
                goto fail;
            }
            node->index_union = function_index;
            node_type = hs_function_definitions[function_index]->return_type;
            goto resolved;
        }

        if ((node_type < 9) && ((node->flags & _hs_syntax_node_global_bit) == 0)) {
            goto recheck_global;
        }
        valid_offset = 1;
        if ((node->source_offset < 0) || (hs_compiled_source_length <= node->source_offset)) {
            hs_compile_error = (char *)"bad source offset (you need to recompile.)";
            valid_offset = 0;
        }
        success = 0;
        if (valid_offset != 0) {
            success = hs_parse_primitive(current);
            nodes = hs_syntax_data;
            goto recheck_global;
        }
        goto use_index_union;

    recheck_global:
        if ((success == 0) || ((node->flags & _hs_syntax_node_global_bit) == 0)) {
            goto use_index_union;
        }
        node_type = hs_global_get_type(node->data.global_reference);
        goto resolved;

    use_index_union:
        node_type = node->index_union;

    resolved:
        if (success != 0) {
            if ((((node_type < 4) || (0x30 < node_type)) && (node_type != 3)) ||
                ((success = hs_types_are_compatible(node->type, node_type)), success == 0)) {
                hs_compile_error = (char *)"type is inconsistent with usage (you need to recompile scripts.)";
                goto fail;
            }
            success = 1;
        }
        goto advance;

    fail:
        success = 0;

    advance:
        next_start = (int32_t)(current & halo::k_slot_mask) + 1;
        current = k_datum_index_none;
        next_index = (int16_t)next_start;
        if (0 <= next_index && next_index < nodes->last_index) {
            scan = (hs_syntax_node *)((uint8_t *)nodes->data + (int32_t)next_index * nodes->size);
            for (;;) {
                if (scan->identifier != 0) {
                    current = ((uint32_t)(uint16_t)scan->identifier << 16) | (uint16_t)next_index;
                    break;
                }
                next_start = next_start + 1;
                scan = (hs_syntax_node *)((uint8_t *)scan + nodes->size);
                next_index = (int16_t)next_start;
                if (nodes->last_index <= next_index) {
                    break;
                }
            }
        }
    } while (success != 0);

    *error_message = hs_compile_error;
    if (hs_compile_error_offset != -1) {
        *error_offset = hs_compile_error_offset + (int32_t)hs_compiled_source;
    }
    hs_compiled_source = 0;
    hs_compile_error = 0;
    hs_postprocessing = 0;
    return 0;
}

/**
 * Recompiles every registered HS script source file in turn, truncating each error message at its first
 * newline, and prints a success message if every one compiled cleanly. Frees the compiled-source buffer
 * according to hs_compile_release_source / hs_compiled_source_owned.
 *
 * @address 0x484090
 */
char ScriptCompiler::compile_source(void)
{
    Scenario *scenario;
    char all_ok;
    int16_t i;
    int32_t count;
    ScenarioSourceFile *source_files;
    char *error_message;
    int32_t error_offset;
    char *newline;

    scenario = global_scenario;
    all_ok = 1;
    hs_compiling = 1;
    hs_compiled_source = 0;
    hs_compiled_source_length = 0;
    hs_compile_release_source = 1;
    hs_syntax_data_dirty = 0;
    hs_compile_error = 0;
    halo::memory::data_delete_all(hs_syntax_data);
    i = 0;
    count = (int32_t)scenario->source_files.count;
    if (0 < count) {
        source_files = (ScenarioSourceFile *)scenario->source_files.pointer;
        do {
            hs_compile((int32_t)source_files[i].source.size, (char *)source_files[i].source.pointer,
                       &error_message, &error_offset);
            if (error_message != 0) {
                if (error_offset != 0) {
                    newline = strchr((char *)error_offset, '\n');
                    if (newline != 0) {
                        *newline = '\0';
                    }
                }
                all_ok = 0;
            }
            i = i + 1;
        } while (i < count);
        if (all_ok == 0) {
            goto cleanup;
        }
    }
    console_print_error_va(0, "scripts successfully compiled.");
cleanup:
    if (hs_compile_release_source != 0) {
        if (hs_syntax_data_dirty != 0) {
            halo::memory::data_delete_all(hs_syntax_data);
        }
        if (hs_compiled_source != 0) {
            GlobalFree(hs_compiled_source);
        }
    }
    if (hs_compiled_source_owned != 0) {
        GlobalFree(hs_compiled_source);
        hs_compiled_source = 0;
        hs_compiled_source_owned = 0;
    }
    hs_compiling = 0;
    return all_ok;
}

}

extern "C" {

char hs_add_global(datum_index node_index)
{
    return halo::hs::ScriptCompiler::add_global(node_index);
}

char hs_add_script(datum_index node_index)
{
    return halo::hs::ScriptCompiler::add_script(node_index);
}

void hs_compile(int32_t source_length, char *source_text, char **error_message, int32_t *error_offset)
{
    halo::hs::ScriptCompiler::compile(source_length, source_text, error_message, error_offset);
}

char hs_compile_and_evaluate(char *command)
{
    return halo::hs::ScriptCompiler::compile_and_evaluate(command);
}

datum_index hs_compile_expression(char *text, uint32_t length, char **error_message, char **error_offset)
{
    return halo::hs::ScriptCompiler::compile_expression(text, length, error_message, error_offset);
}

char hs_compile_postprocess(char **error_message, int32_t *error_offset)
{
    return halo::hs::ScriptCompiler::compile_postprocess(error_message, error_offset);
}

char hs_compile_source(void)
{
    return halo::hs::ScriptCompiler::compile_source();
}

}
