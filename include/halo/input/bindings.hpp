#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::input {

/**
 * Control binding table, last-used-binding cache, bind/unbind commands, rebind capture and device default profiles.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct Bindings {
    static void control_binding_table_initialize(void);
    static uint8_t control_binding_table_query(int32_t target, int32_t raw_id);
    static void control_binding_table_update_a(void);
    static void control_binding_table_update_b(void);
    static uint32_t control_word_extract_field(uint32_t which_word, uint32_t field_index);
    static uint8_t apply_control_binding(control_binding_descriptor *binding, int32_t action_index);
    static void apply_named_device_default_profile(uint16_t *device_name);
    static void bind_capture_reset(void);
    static void bind_scan_set_active(uint8_t enable_scan);
    static void clear_control_binding(control_binding_descriptor *binding);
    static uint32_t device_default_profile_tag_find(input_guid device_guid, void *out_profile);
    static uint8_t get_last_used_binding(int16_t action, control_binding_descriptor *out);
    static void last_used_binding_copy(int16_t action, control_binding_descriptor *source);
    static void last_used_binding_set(int16_t action, int16_t device_type, int16_t device_index, int16_t input_kind, int16_t input_index, int32_t direction);
    static uint8_t parse_device_binding_string(char *device_class_name, char *name, control_binding_descriptor *out_binding);
    static uint8_t profile_copy_bindings_by_device(int32_t category, saved_player_profile *dst, saved_player_profile *src);
    static uint8_t refresh_last_used_binding(int32_t device_class, int16_t action);
    static void scan_any_bound_input(void);
    static void test_input_device_defaults_find(char *device_id_ansi);
    static void control_binding_table_register_single(int32_t target, int32_t selector, int32_t raw_id, uint32_t raw_value);
    static void hs_bind_control(const char *device_class_name, const char *input_name, const char *action_name);
    static void hs_unbind_control(const char *device_class_name, const char *input_name);
};

}
