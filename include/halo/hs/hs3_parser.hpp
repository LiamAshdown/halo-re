#pragma once

#include "halo/hs/hs3_types.hpp"

namespace halo::hs::part3 {

/**
 * Recursive-descent parser that turns tokenised script nodes into typed, validated syntax trees.
 */
class Parser {
public:
    char hs_parse(datum_index node_index, hs_type_t expected_type) const;
    char parse_ai(datum_index node_index) const;
    char parse_ai_command_list(datum_index node_index) const;
    char parse_arithmetic(int16_t function_index, datum_index node_index) const;
    char parse_begin(int16_t function_index, datum_index node_index) const;
    char parse_boolean(datum_index node_index) const;
    char parse_cond(int16_t function_index, datum_index node_index) const;
    datum_index parse_cond_recursive(datum_index cond_node_index, datum_index pair_index) const;
    char parse_conversation(datum_index node_index) const;
    char parse_cutscene_camera_point(datum_index node_index) const;
    char parse_cutscene_flag(datum_index node_index) const;
    char parse_cutscene_recording(datum_index node_index) const;
    char parse_cutscene_title(datum_index node_index) const;
    char parse_device_group(datum_index node_index) const;
    char parse_function_arguments(int16_t function_index, datum_index node_index) const;
    char parse_hud_message(datum_index node_index) const;
    char parse_if(int16_t function_index, datum_index node_index) const;
    char parse_inspect(int16_t function_index, datum_index node_index) const;
    char parse_integer(datum_index node_index) const;
    char parse_logical(int16_t function_index, datum_index node_index) const;
    char parse_navpoint(datum_index node_index) const;
    char parse_nonprimitive(datum_index node_index) const;
    char parse_object(datum_index node_index) const;
    char parse_object_list(datum_index node_index) const;
    char parse_object_name(datum_index node_index) const;
    char parse_primitive(datum_index node_index) const;
    char parse_real(datum_index node_index) const;
    char parse_scenario_datum(datum_index node_index, int16_t name_offset, TagReflexive *array, int32_t stride) const;
    char parse_script(datum_index node_index) const;
    char parse_set(int16_t function_index, datum_index node_index) const;
    char parse_sleep(int16_t function_index, datum_index node_index) const;
    char parse_sleep_until(int16_t function_index, datum_index node_index) const;
    char parse_starting_profile(datum_index node_index) const;
    char parse_string(datum_index node_index) const;
    char parse_string_arguments(int16_t function_index, datum_index node_index) const;
    char parse_tag_reference(datum_index node_index) const;
    char parse_trigger_volume(datum_index node_index) const;
    char parse_two_numeric_arguments(int16_t function_index, datum_index node_index) const;
    char parse_two_object_arguments(int16_t function_index, datum_index node_index) const;
    char parse_unit(int16_t function_index, datum_index node_index) const;
    char parse_variable(datum_index node_index) const;
    char parse_wake(int16_t function_index, datum_index node_index) const;
    char report_expected_enum_values(datum_index node_index) const;
    void resolve_identifier_as_function_or_script(datum_index node_index) const;
};

/**
 * Tokenizer for script source text plus the source buffer it reads from.
 */
class SourceTokenizer {
public:
    char rebuild_source() const;
    char *source_buffer_append(char *text, uint32_t length) const;
    datum_index tokenize(char **cursor) const;
    void tokenize_nonprimitive(datum_index node_index, char **cursor) const;
    void tokenize_primitive(char **cursor, datum_index node_index) const;
    char verify_source_offset(int32_t offset) const;
    void skip_whitespace(char **cursor) const;
};

}
