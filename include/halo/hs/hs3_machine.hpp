#pragma once

#include "halo/hs/hs3_types.hpp"

namespace halo::hs::part3 {

/**
 * Evaluates the argument lists of a script function call one argument per step, as the call handlers do.
 */
class ArgumentEvaluator {
public:
    int32_t *typed_arguments(uint32_t thread_index, int16_t parameter_count, int16_t *expected_types, char first) const;
    char variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count, int32_t **out_values) const;
};

/**
 * The script thread stack machine: creating, pushing, stepping, returning from and restarting hs threads.
 */
class ThreadMachine {
public:
    void evaluate_step(uint32_t thread_index) const;
    datum_index find_by_script_index(int16_t script_index) const;
    datum_index find_by_script_name(char *name) const;
    datum_index create(int32_t script_index, uint8_t type) const;
    void pop_frame(uint32_t thread_index) const;
    void push(datum_index node, uint32_t thread_index, void *result_address) const;
    void restart(uint32_t thread_index) const;
    void return_value(int32_t value, uint32_t thread_index) const;
};

/**
 * Lookup and access of script globals, both builtin definitions and scenario-defined variables.
 */
class GlobalTable {
public:
    hs_global_reference find_global_by_name(char *name) const;
    char *get_name(hs_global_reference global) const;
    hs_type_t get_type(hs_global_reference global) const;
    int32_t get_value(hs_global_reference reference) const;
    void read_value(hs_global_reference reference) const;
    void write_value(hs_global_reference reference) const;
    float *sound_get_gain_reference(char *name) const;
};

/**
 * Lookup and formatting of script functions and scripts, and argument-index helpers for the parser.
 */
class FunctionTable {
public:
    int16_t find_function_by_name(char *name) const;
    void format_function_signature(int16_t function_index, char *out) const;
    char get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index, datum_index *out_indices) const;
    void help_print_function(char *name) const;
    hs_type_t null_with_params_evaluate(hs_syntax_node *node) const;
    int16_t script_find_by_name(char *name) const;
};

/**
 * Script type compatibility rules, gametype flag tests and small string predicates.
 */
class TypeRules {
public:
    char gametype_flag_satisfied(uint8_t bit_index, uint8_t flags) const;
    uint8_t gametype_flags_applicable(uint8_t flags) const;
    char string_is_empty(char *s) const;
    char type_mask_is_subset(int16_t subtype_index, int16_t supertype_index) const;
    char types_are_compatible(hs_type_t dest_type, hs_type_t source_type) const;
};

/**
 * Formatters that print a script value of a given type into a text buffer.
 */
class ValueInspector {
public:
    void inspect_boolean(int16_t type, int32_t value, char *buffer) const;
    void inspect_enum(int16_t type, int32_t value, char *buffer) const;
    void inspect_long(int16_t type, int32_t value, char *buffer) const;
    void inspect_real(int16_t type, int32_t value, char *buffer) const;
    void inspect_short(int16_t type, int32_t value, char *buffer) const;
    void inspect_string(int16_t type, int32_t value, char *buffer) const;
};

/**
 * Script runtime lifecycle: scenario script initialisation, compile and link, reload, update and garbage
 * collection.
 */
class ScriptRuntime {
public:
    void reposition_players_outside_trigger_volume(int32_t trigger_volume_index, int32_t location_index) const;
    void runtime_initialize() const;
    void runtime_update() const;
    void scenario_scripts_initialize() const;
    char scripts_compile_and_link(char restore_previous) const;
    void scripts_free() const;
    void scripts_reload() const;
    void syntax_data_byte_swap(void *element, uint8_t *data, uint32_t size) const;
    void syntax_node_garbage_collect() const;
};

}
