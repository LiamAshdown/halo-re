#include "halo/hs/hs1_autocomplete.hpp"
#include <string.h>
#include <stdlib.h>

extern "C" {
extern char hs_evaluate_variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count, int32_t **out_values);
extern void hs_thread_return(int32_t value, uint32_t thread_index);
extern void game_variant_list_matching_substring(uint32_t argument_count, int32_t *arguments);
extern void hs_autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride);
extern datum_index global_scenario_index;
extern Scenario *global_scenario;
extern int16_t hs_autocomplete_maximum_count;
extern char *hs_autocomplete_prefix;
extern int16_t hs_autocomplete_count;
extern char **hs_autocomplete_results;
extern uint8_t hs_gametype_flags_applicable(uint8_t flags);
extern hs_function_definition *hs_function_definitions[k_hs_function_count];
extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count];
extern tag_instance *tag_instances;
extern Globals *global_globals;
extern void hs_autocomplete_scan_candidates(char **table, int16_t end, int16_t start);
extern char *hs_script_type_names[k_hs_script_type_count];
extern char *hs_type_names[k_hs_type_count];
extern void KeyValCompareKeyA(const void *a, const void *b);
extern void hs_enumerate_special_form_names(void);
extern void hs_autocomplete_add_startup(void);
extern void *hs_autocomplete_procedures[0x12];
extern char k_empty_string[1];
extern uint16_t hs_autocomplete_gametype_mask;
extern hs_global_reference hs_find_global_by_name(char *name);
extern void hs_autocomplete_test_candidate(char *candidate);
}

static void autocomplete_offer(char *candidate)
{
    if (hs_autocomplete_count < hs_autocomplete_maximum_count &&
        _strnicmp(candidate, hs_autocomplete_prefix, (int32_t)strlen(hs_autocomplete_prefix)) == 0) {
        hs_autocomplete_results[hs_autocomplete_count] = candidate;
        hs_autocomplete_count = hs_autocomplete_count + 1;
    }
}

namespace halo::hs {

/**
 * Evaluate handler for the game_variant_list_matching_substring external hs function: decodes its single
 * argument and forwards (evaluated_count, value) to game_variant_list_matching_substring, then pops the
 * evaluation frame.
 *
 * @address 0x483060
 */
void ScriptAutocomplete::game_variant_list_matching_substring_evaluate(int16_t function_index, datum_index thread, char first)
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        game_variant_list_matching_substring(argument_count, arguments);
        hs_thread_return(0, thread);
    }
}

/**
 * Offers every command list name that matches the current autocomplete prefix.
 *
 * @address 0x483a40
 */
void ScriptAutocomplete::autocomplete_add_command_list_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->command_lists, 0, 0x60);
    }
}

/**
 * Offers every conversation name that matches the current autocomplete prefix.
 *
 * @address 0x483aa0
 */
void ScriptAutocomplete::autocomplete_add_conversation_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->ai_conversations, 0, 0x74);
    }
}

/**
 * Offers every cutscene camera point name that matches the current autocomplete prefix.
 *
 * @address 0x483b60
 */
void ScriptAutocomplete::autocomplete_add_cutscene_camera_point_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->cutscene_camera_points, 4, 0x68);
    }
}

/**
 * Offers every cutscene flag name that matches the current autocomplete prefix.
 *
 * @address 0x483b30
 */
void ScriptAutocomplete::autocomplete_add_cutscene_flag_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->cutscene_flags, 4, 0x5c);
    }
}

/**
 * Offers every cutscene title name that matches the current autocomplete prefix.
 *
 * @address 0x483b90
 */
void ScriptAutocomplete::autocomplete_add_cutscene_title_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->cutscene_titles, 4, 0x60);
    }
}

/**
 * Offers every encounter name that matches the current autocomplete prefix.
 *
 * @address 0x483a10
 */
void ScriptAutocomplete::autocomplete_add_encounter_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->encounters, 0, 0xb0);
    }
}

/**
 * Offers every function name that matches the current autocomplete prefix.
 *
 * @address 0x4838a0
 */
void ScriptAutocomplete::autocomplete_add_function_names(void)
{
    int32_t i;

    for (i = 0; i < 0x20a; i++) {
        hs_function_definition *definition = hs_function_definitions[i];

        if (hs_gametype_flags_applicable((uint8_t)definition->gametype_flags)) {
            autocomplete_offer(definition->name);
        }
    }
}

/**
 * Offers every global name that matches the current autocomplete prefix.
 *
 * @address 0x483960
 */
void ScriptAutocomplete::autocomplete_add_global_names(void)
{
    int32_t i;

    for (i = 0; i < k_hs_builtin_global_count; i++) {
        hs_global_definition *definition = hs_global_definitions[i];

        if (hs_gametype_flags_applicable((uint8_t)definition->gametype_flags)) {
            autocomplete_offer(definition->name);
        }
    }
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->globals, 0, 0x5c);
    }
}

/**
 * Offers every hud message name that matches the current autocomplete prefix.
 *
 * @address 0x483c50
 */
void ScriptAutocomplete::autocomplete_add_hud_message_names(void)
{
    datum_index hud_messages = *(datum_index *)&global_scenario->hud_messages.tag_id;

    if (hud_messages != k_datum_index_none) {
        hs_autocomplete_scan_globals((TagReflexive *)((uint8_t *)tag_instances[hud_messages & 0xffff].data + 0x20),
            0, 0x40);
    }
}

/**
 * Offers every navpoint name that matches the current autocomplete prefix.
 *
 * @address 0x483bf0
 */
void ScriptAutocomplete::autocomplete_add_navpoint_names(void)
{
    uint8_t *interface_bitmaps = global_globals->interface_bitmaps.count != 0 ?
        (uint8_t *)global_globals->interface_bitmaps.pointer : 0;
    datum_index hud_globals = *(datum_index *)(interface_bitmaps + 0x6c);

    if (hud_globals != k_datum_index_none) {
        hs_autocomplete_scan_globals((TagReflexive *)((uint8_t *)tag_instances[hud_globals & 0xffff].data + 0x160),
            0, 0x68);
    }
}

/**
 * Offers every object name that matches the current autocomplete prefix.
 *
 * @address 0x483ad0
 */
void ScriptAutocomplete::autocomplete_add_object_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->object_names, 0, 0x24);
    }
}

/**
 * Offers every recorded animation name that matches the current autocomplete prefix.
 *
 * @address 0x483bc0
 */
void ScriptAutocomplete::autocomplete_add_recorded_animation_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->recorded_animations, 0, 0x40);
    }
}

/**
 * Offers every script name that matches the current autocomplete prefix.
 *
 * @address 0x483930
 */
void ScriptAutocomplete::autocomplete_add_script_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->scripts, 0, 0x5c);
    }
}

/**
 * Offers every starting profile name that matches the current autocomplete prefix.
 *
 * @address 0x483a70
 */
void ScriptAutocomplete::autocomplete_add_starting_profile_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->player_starting_profile, 0, 0x68);
    }
}

/**
 * Adds the HS script-type keywords startup/dormant/continuous/static (every hs_script_type_names entry
 * except "stub") to the autocomplete results.
 *
 * @address 0x483860
 */
void ScriptAutocomplete::autocomplete_add_startup(void)
{
    hs_autocomplete_scan_candidates(hs_script_type_names, 5, 0);
}

/**
 * Offers every trigger volume name that matches the current autocomplete prefix.
 *
 * @address 0x483b00
 */
void ScriptAutocomplete::autocomplete_add_trigger_volume_names(void)
{
    if (global_scenario_index != k_datum_index_none) {
        hs_autocomplete_scan_globals(&global_scenario->trigger_volumes, 4, 0x60);
    }
}

/**
 * Offers every type name that matches the current autocomplete prefix.
 *
 * @address 0x483880
 */
void ScriptAutocomplete::autocomplete_add_type_names(void)
{
    hs_autocomplete_scan_candidates(hs_type_names, 0x31, 4);
}

/**
 * Gathers autocomplete candidates for `prefix` across every category selected by `category_mask` into
 * `results` (capped at maximum_count entries), sorts them, and returns the number found.
 *
 * @address 0x483c90
 */
int16_t ScriptAutocomplete::autocomplete_gather(uint32_t category_mask, char **results, char *prefix, int16_t maximum_count, uint16_t gametype_mask)
{
    void **procedure;
    int32_t remaining;
    uint8_t bit;

    hs_autocomplete_count = 0;
    hs_autocomplete_results = results;
    hs_autocomplete_prefix = prefix;
    if (prefix == 0) {
        hs_autocomplete_prefix = k_empty_string;
    }
    procedure = hs_autocomplete_procedures;
    remaining = 0x12;
    hs_autocomplete_maximum_count = maximum_count;
    hs_autocomplete_gametype_mask = gametype_mask;
    bit = 0;
    do {
        if ((category_mask & (1u << (bit & 0x1f))) != 0) {
            ((void (*)(void)) * procedure)();
        }
        bit = bit + 1;
        procedure = procedure + 1;
        remaining = remaining - 1;
    } while (remaining != 0);
    qsort(results, (size_t)hs_autocomplete_count, 4, (int (*)(const void *, const void *))KeyValCompareKeyA);
    hs_autocomplete_results = 0;
    return hs_autocomplete_count;
}

/**
 * Appends every table[start..end) entry that starts with hs_autocomplete_prefix (case-insensitively) to
 * hs_autocomplete_results, while there is room.
 *
 * @address 0x4836f0
 */
void ScriptAutocomplete::autocomplete_scan_candidates(char **table, int16_t end, int16_t start)
{
    char **entry;
    uint16_t remaining;
    char *candidate;
    int32_t prefix_length;

    if (start < end) {
        entry = table + start;
        remaining = (uint16_t)(end - start);
        do {
            candidate = *entry;
            if (hs_autocomplete_count < hs_autocomplete_maximum_count) {
                prefix_length = (int32_t)strlen(hs_autocomplete_prefix);
                if (_strnicmp(candidate, hs_autocomplete_prefix, prefix_length) == 0) {
                    hs_autocomplete_results[hs_autocomplete_count] = candidate;
                    hs_autocomplete_count = hs_autocomplete_count + 1;
                }
            }
            entry = entry + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }
}

/**
 * Scans a (count, pointer) table of stride-`stride` records, testing the name field at `name_offset` within
 * each record as an autocomplete candidate: unconditionally if it is not a known global at all, or if it is
 * a builtin whose gametype flags allow the current gametype.
 *
 * @address 0x483770
 */
void ScriptAutocomplete::autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride)
{
    int32_t count;
    int32_t i;
    char *candidate;
    int16_t global_index;
    char applicable;
    int32_t prefix_length;

    count = (int32_t)table->count;
    if (0 < count) {
        for (i = 0; i < count; i = i + 1) {
            candidate = (char *)((int32_t)name_offset + i * stride + (int32_t)table->pointer);
            global_index = (int16_t)hs_find_global_by_name(candidate);
            if (((global_index == -1) ||
                 ((global_index < 0) &&
                  (hs_global_definitions[global_index] != 0) &&
                  (applicable = (char)hs_gametype_flags_applicable(
                      (uint8_t)hs_global_definitions[global_index]->gametype_flags), applicable != 0))) &&
                (hs_autocomplete_count < hs_autocomplete_maximum_count)) {
                prefix_length = (int32_t)strlen(hs_autocomplete_prefix);
                if (_strnicmp(candidate, hs_autocomplete_prefix, prefix_length) == 0) {
                    hs_autocomplete_results[hs_autocomplete_count] = candidate;
                    hs_autocomplete_count = hs_autocomplete_count + 1;
                }
            }
        }
    }
}

/**
 * Appends `candidate` to hs_autocomplete_results if it starts with hs_autocomplete_prefix
 * (case-insensitively) and the results buffer is not already full.
 *
 * @address 0x483690
 */
void ScriptAutocomplete::autocomplete_test_candidate(char *candidate)
{
    int32_t prefix_length;

    if (hs_autocomplete_count < hs_autocomplete_maximum_count) {
        prefix_length = (int32_t)strlen(hs_autocomplete_prefix);
        if (_strnicmp(candidate, hs_autocomplete_prefix, prefix_length) == 0) {
            hs_autocomplete_results[hs_autocomplete_count] = candidate;
            hs_autocomplete_count = hs_autocomplete_count + 1;
        }
    }
}

/**
 * , this batch Adds the "script" and "global" special-form keywords to the autocomplete results if they
 * match the current prefix.
 *
 * @address 0x483840
 */
void ScriptAutocomplete::enumerate_special_form_names(void)
{
    hs_autocomplete_test_candidate((char *)"script");
    hs_autocomplete_test_candidate((char *)"global");
}

}

extern "C" {

void game_variant_list_matching_substring_evaluate(int16_t function_index, datum_index thread, char first)
{
    halo::hs::ScriptAutocomplete::game_variant_list_matching_substring_evaluate(function_index, thread, first);
}

void hs_autocomplete_add_command_list_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_command_list_names();
}

void hs_autocomplete_add_conversation_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_conversation_names();
}

void hs_autocomplete_add_cutscene_camera_point_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_cutscene_camera_point_names();
}

void hs_autocomplete_add_cutscene_flag_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_cutscene_flag_names();
}

void hs_autocomplete_add_cutscene_title_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_cutscene_title_names();
}

void hs_autocomplete_add_encounter_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_encounter_names();
}

void hs_autocomplete_add_function_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_function_names();
}

void hs_autocomplete_add_global_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_global_names();
}

void hs_autocomplete_add_hud_message_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_hud_message_names();
}

void hs_autocomplete_add_navpoint_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_navpoint_names();
}

void hs_autocomplete_add_object_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_object_names();
}

void hs_autocomplete_add_recorded_animation_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_recorded_animation_names();
}

void hs_autocomplete_add_script_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_script_names();
}

void hs_autocomplete_add_starting_profile_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_starting_profile_names();
}

void hs_autocomplete_add_startup(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_startup();
}

void hs_autocomplete_add_trigger_volume_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_trigger_volume_names();
}

void hs_autocomplete_add_type_names(void)
{
    halo::hs::ScriptAutocomplete::autocomplete_add_type_names();
}

int16_t hs_autocomplete_gather(uint32_t category_mask, char **results, char *prefix, int16_t maximum_count, uint16_t gametype_mask)
{
    return halo::hs::ScriptAutocomplete::autocomplete_gather(category_mask, results, prefix, maximum_count, gametype_mask);
}

void hs_autocomplete_scan_candidates(char **table, int16_t end, int16_t start)
{
    halo::hs::ScriptAutocomplete::autocomplete_scan_candidates(table, end, start);
}

void hs_autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride)
{
    halo::hs::ScriptAutocomplete::autocomplete_scan_globals(table, name_offset, stride);
}

void hs_autocomplete_test_candidate(char *candidate)
{
    halo::hs::ScriptAutocomplete::autocomplete_test_candidate(candidate);
}

void hs_enumerate_special_form_names(void)
{
    halo::hs::ScriptAutocomplete::enumerate_special_form_names();
}

}
