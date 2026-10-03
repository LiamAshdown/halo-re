/**
 * @file include/halo/shell/api.hpp
 * Functions of the shell module that other modules and the data tables call (namespace halo::shell). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>



struct d3d9_interface;
struct d3d_adapter_identifier9;
struct d3d_caps9;
struct hwreq_parse_exception;
struct hwreq_parser;
struct hwreq_property_set;
struct shell_sound_device;

namespace halo::shell {

/**
 * The engine globals the shell module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    void *&module_handle;
    int32_t &disable_alpha_render_targets;
    int32_t &disable_buffering;
    int32_t &disable_driver_management;
    int32_t &disable_render_targets;
    int32_t &disable_specular;
    int32_t &enable_stop_start;
    int32_t &head_relative_speech;
    int32_t &invalid_driver;
    int32_t &invalid_sound_driver;
    int32_t &linear_texture_addressing;
    int32_t &linear_texture_addressing_sun;
    int32_t &linear_texture_addressing_zoom;
    int32_t &min_max_blend_op_is_broken;
    int32_t &old_driver;
    int32_t &old_sound_driver;
    int32_t &prototype_card;
    int32_t &safe_mode;
    int32_t &unsupported_card;
    int32_t &use_alternate_convolve_mask;
    int32_t &use_anisotropic_filter;
    int32_t &use_fixed_function;
    int32_t &force_shader;
    int32_t &maximum_resolution;
    float &decal_z_bias;
    float &decal_slope_z_bias;
    float &transparent_decal_z_bias;
    float &transparent_decal_slope_z_bias;
};

Globals &globals();

hwreq_parse_exception * hwreq_parse_exception_copy_construct(hwreq_parse_exception *self, const hwreq_parse_exception *other);
void hwreq_parse_exception_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag);
void hwreq_length_error_destruct(hwreq_parse_exception *self);
void hwreq_length_error_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag);
void hwreq_out_of_range_destruct(hwreq_parse_exception *self);
void hwreq_out_of_range_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag);
void hwreq_parser_scalar_deleting_destructor(hwreq_parser *this_parser);
char * hwreq_parser_get_error_message(hwreq_parser *parser);
uint32_t hwreq_parser_get_flag_count(hwreq_parser *parser);
char * hwreq_parser_get_flag_name(hwreq_parser *parser, uint32_t index);
char * hwreq_parser_get_flag_value(hwreq_parser *parser, uint32_t index);
hwreq_property_set * hwreq_parser_get_flags(hwreq_parser *parser);
char * hwreq_parser_get_graphics_device_name(hwreq_parser *parser);
char * hwreq_parser_get_graphics_vendor_name(hwreq_parser *parser);
uint32_t hwreq_parser_get_requirement_count(hwreq_parser *parser);
char * hwreq_parser_get_requirement_name(hwreq_parser *parser, uint32_t index);
char * hwreq_parser_get_requirement_value(hwreq_parser *parser, uint32_t index);
char * hwreq_parser_get_sound_device_name(hwreq_parser *parser);
char * hwreq_parser_get_sound_vendor_name(hwreq_parser *parser);
uint8_t hwreq_parser_has_error(hwreq_parser *parser);
hwreq_property_set * hwreq_parser_find_property_set(hwreq_parser *self, const char *name);
uint8_t hwreq_parser_parse(hwreq_parser *parser, const char *path, const shell_sound_device *sound_device, const d3d_adapter_identifier9 *adapter, const d3d_caps9 *caps, uint32_t memory, uint32_t video_memory, uint32_t cpu_speed);
void * exception_copy_construct(void *self_, const void *other);
const char *__fastcall std_exception_what(uint8_t *self, void *unused_edx);
hwreq_parse_exception *__fastcall std_length_error_copy_construct(hwreq_parse_exception *self, void *unused_edx, const hwreq_parse_exception *other);
hwreq_parse_exception *__fastcall std_out_of_range_copy_construct(hwreq_parse_exception *self, void *unused_edx, const hwreq_parse_exception *other);
uint32_t clipboard_get_text(char *buffer, uint32_t capacity);
uint8_t command_line_check_flag(const char *flag_name, const char **out_value);
uint8_t config_compute_uma_video_memory(void);
uint8_t config_set_decal_slope_z_bias(char *value);
uint8_t config_set_decal_z_bias(char *value);
uint8_t config_set_transparent_decal_slope_z_bias(char *value);
uint8_t config_set_transparent_decal_z_bias(char *value);
uint8_t config_set_maximum_resolution(char *value);
uint8_t config_set_force_shader(const char *value);
uint8_t config_set_disable_alpha_render_targets(const char *value);
uint8_t config_set_disable_buffering(const char *value);
uint8_t config_set_disable_driver_management(const char *value);
uint8_t config_set_disable_render_targets(const char *value);
uint8_t config_set_disable_specular(const char *value);
uint8_t config_set_enable_stop_start(const char *value);
uint8_t config_set_head_relative_speech(const char *value);
uint8_t config_set_invalid_driver(const char *value);
uint8_t config_set_invalid_sound_driver(const char *value);
uint8_t config_set_linear_texture_addressing(const char *value);
uint8_t config_set_linear_texture_addressing_sun(const char *value);
uint8_t config_set_linear_texture_addressing_zoom(const char *value);
uint8_t config_set_min_max_blend_op_is_broken(const char *value);
uint8_t config_set_old_driver(const char *value);
uint8_t config_set_old_sound_driver(const char *value);
uint8_t config_set_prototype_card(const char *value);
uint8_t config_set_safe_mode(const char *value);
uint8_t config_set_unsupported_card(const char *value);
uint8_t config_set_use_alternate_convolve_mask(const char *value);
uint8_t config_set_use_anisotropic_filter(const char *value);
uint8_t config_set_use_fixed_function(const char *value);
int32_t cpu_get_type(int32_t mode);
void keystone_library_unload(void);
void os_platform_identify(void);
int32_t security_check_write_access(void);
int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal);
int32_t shell_load_localized_string(uint32_t buffer_capacity, void *module, char *buffer, uint32_t id);
char * shell_parse_config_txt(uint32_t adapter_index, d3d9_interface *d3d);
void shell_pump_windows_messages(void);
int32_t __stdcall shell_winmain(void *hInstance, void *hPrevInstance, char *lpCmdLine, int32_t nCmdShow);

}
