#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "crt.h"
#include "hs.h"

namespace halo::hs {

/**
 * Console autocomplete candidate gathering over the hs function, global, script and tag-name tables.
 */
class ScriptAutocomplete {
public:
    static void game_variant_list_matching_substring_evaluate(int16_t function_index, datum_index thread, char first);
    static void autocomplete_add_command_list_names(void);
    static void autocomplete_add_conversation_names(void);
    static void autocomplete_add_cutscene_camera_point_names(void);
    static void autocomplete_add_cutscene_flag_names(void);
    static void autocomplete_add_cutscene_title_names(void);
    static void autocomplete_add_encounter_names(void);
    static void autocomplete_add_function_names(void);
    static void autocomplete_add_global_names(void);
    static void autocomplete_add_hud_message_names(void);
    static void autocomplete_add_navpoint_names(void);
    static void autocomplete_add_object_names(void);
    static void autocomplete_add_recorded_animation_names(void);
    static void autocomplete_add_script_names(void);
    static void autocomplete_add_starting_profile_names(void);
    static void autocomplete_add_startup(void);
    static void autocomplete_add_trigger_volume_names(void);
    static void autocomplete_add_type_names(void);
    static int16_t autocomplete_gather(uint32_t category_mask, char **results, char *prefix, int16_t maximum_count, uint16_t gametype_mask);
    static void autocomplete_scan_candidates(char **table, int16_t end, int16_t start);
    static void autocomplete_scan_globals(TagReflexive *table, int16_t name_offset, int32_t stride);
    static void autocomplete_test_candidate(char *candidate);
    static void enumerate_special_form_names(void);
};

}
