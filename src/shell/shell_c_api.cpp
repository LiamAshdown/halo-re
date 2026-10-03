/**
 * C linkage shims for the shell module: one extern "C" function per original symbol, forwarding to the
 * C++ classes in namespace halo::shell (the std/hwreq view classes, Application, GameWindow,
 * CrashReporter, Config and the rest).
 */

#include "halo/shell/hwreq.hpp"
#include "halo/shell/application.hpp"
#include "halo/shell/config.hpp"
#include "halo/shell/diagnostics.hpp"
#include "halo/shell/hardware.hpp"
#include "halo/shell/system.hpp"
#include "halo/shell/window.hpp"

extern "C" {
extern int32_t config_disable_alpha_render_targets;
extern int32_t config_disable_buffering;
extern int32_t config_disable_driver_management;
extern int32_t config_disable_render_targets;
extern int32_t config_disable_specular;
extern int32_t config_enable_stop_start;
extern int32_t config_head_relative_speech;
extern int32_t config_invalid_driver;
extern int32_t config_invalid_sound_driver;
extern int32_t config_linear_texture_addressing;
extern int32_t config_linear_texture_addressing_sun;
extern int32_t config_linear_texture_addressing_zoom;
extern int32_t config_min_max_blend_op_is_broken;
extern int32_t config_old_driver;
extern int32_t config_old_sound_driver;
extern int32_t config_prototype_card;
extern int32_t config_safe_mode;
extern int32_t config_unsupported_card;
extern int32_t config_use_alternate_convolve_mask;
extern int32_t config_use_anisotropic_filter;
extern int32_t config_use_fixed_function;
extern float config_decal_z_bias;
extern float config_decal_slope_z_bias;
extern float config_transparent_decal_z_bias;
extern float config_transparent_decal_slope_z_bias;
}

extern "C" {

msvc_std_string * string_assign_substr(msvc_std_string *this_, const msvc_std_string *right, uint32_t pos, uint32_t count)
{
    return halo::shell::StdString(this_).assign_substr(right, pos, count);
}

msvc_std_string * msvc_string_assign_n(msvc_std_string *self, const char *source, uint32_t count)
{
    return halo::shell::StdString(self).assign_n(source, count);
}

int32_t string_compare(const msvc_std_string *self, uint32_t n1, uint32_t pos, const char *s, uint32_t n2)
{
    return halo::shell::StdString(self).compare(n1, pos, s, n2);
}

msvc_std_string * string_erase(msvc_std_string *self, uint32_t pos, uint32_t count)
{
    return halo::shell::StdString(self).erase(pos, count);
}

void string_grow_reserve(msvc_std_string *self, uint32_t new_capacity, uint32_t preserve_count)
{
    halo::shell::StdString(self).grow_reserve(new_capacity, preserve_count);
}

msvc_std_string * hwreq_key_string_construct_cstr(msvc_std_string *self, const char *source)
{
    return halo::shell::StdString(self).construct_cstr(source);
}

void hwreq_key_string_destruct(msvc_std_string *self)
{
    halo::shell::StdString(self).destroy();
}

void hwreq_string_destruct(msvc_std_string *self)
{
    halo::shell::StdString(self).destroy();
}

msvc_std_string * hwreq_string_assign_cstr(msvc_std_string *dest, const char *s)
{
    return halo::shell::StdString(dest).assign_cstr(s);
}

uint8_t hwreq_map_key_less_than(const msvc_std_string *self, const msvc_std_string *other)
{
    return halo::shell::StdString(self).less_than(other);
}

hwreq_string_pair * hwreq_string_pair_construct(hwreq_string_pair *dest, const msvc_std_string *first_source, const msvc_std_string *second_source)
{
    return halo::shell::StringPair(dest).construct(first_source, second_source);
}

hwreq_string_pair * string_pair_construct_empty(hwreq_string_pair *dest, const hwreq_string_pair *source)
{
    return halo::shell::StringPair(dest).copy_construct(source);
}

void hwreq_string_pair_destruct(hwreq_string_pair *pair)
{
    halo::shell::StringPair(pair).destroy();
}

hwreq_string_pair * copy_backward_string_pair(hwreq_string_pair *first, hwreq_string_pair *last, hwreq_string_pair *dest_end)
{
    return halo::shell::StringPair::copy_backward(first, last, dest_end);
}

void destroy_range_string_pair(hwreq_string_pair *first, hwreq_string_pair *last)
{
    halo::shell::StringPair::destroy_range(first, last);
}

void fill_string_pair_range(hwreq_string_pair *first, hwreq_string_pair *last, const hwreq_string_pair *value)
{
    halo::shell::StringPair::fill_range(first, last, value);
}

hwreq_string_pair * uninit_copy_string_pair(hwreq_string_pair *source_begin, hwreq_string_pair *source_end, hwreq_string_pair *dest)
{
    return halo::shell::StringPair::uninit_copy(source_begin, source_end, dest);
}

void uninit_fill_n_string_pair(hwreq_string_pair *dest, uint32_t count, const hwreq_string_pair *value)
{
    halo::shell::StringPair::uninit_fill_n(dest, count, value);
}

int32_t hwreq_device_list_size(const msvc_std_vector *self)
{
    return halo::shell::PairVector(self).element_count();
}

void hwreq_device_list_push_back(msvc_std_vector *self, const hwreq_string_pair *value)
{
    halo::shell::PairVector(self).push_back(value);
}

hwreq_string_pair ** hwreq_pair_vector_insert(msvc_std_vector *self, hwreq_string_pair **result, hwreq_string_pair *where, const hwreq_string_pair *value)
{
    return halo::shell::PairVector(self).insert(result, where, value);
}

void hwreq_pair_vector_insert_n(msvc_std_vector *self, hwreq_string_pair *where, uint32_t count, const hwreq_string_pair *value)
{
    halo::shell::PairVector(self).insert_n(where, count, value);
}

void hwreq_property_set_upsert(hwreq_property_set *property_set, char *key, char *value)
{
    halo::shell::PropertySet(property_set).upsert(key, value);
}

void hwreq_property_set_apply(hwreq_property_set *source, hwreq_property_set *target)
{
    halo::shell::PropertySet(source).apply_to(target);
}

uint32_t hwreq_device_override_list_find(hwreq_property_set *property_set, const char *key, char *out_value, uint32_t capacity)
{
    return halo::shell::PropertySet(property_set).find_value(key, out_value, capacity);
}

void hwreq_property_set_flags_destruct(hwreq_property_set *set)
{
    halo::shell::PropertySet(set).destroy_flags();
}

void tree_iterator_increment(hwreq_map_node **iterator)
{
    halo::shell::TreeIterator(iterator).increment();
}

void tree_iterator_decrement(hwreq_map_node **iterator)
{
    halo::shell::TreeIterator(iterator).decrement();
}

hwreq_map_node * tree_find_max(hwreq_map_node *node)
{
    return halo::shell::TreeNode(node).find_max();
}

hwreq_map_node * tree_find_min(hwreq_map_node **subtree_root_left_field)
{
    return halo::shell::TreeNode::find_min(subtree_root_left_field);
}

void tree_destroy_subtree(void *map_self, hwreq_map_node *node)
{
    halo::shell::TreeNode(node).destroy_subtree();
}

hwreq_map_node * tree_head_node_allocate(void)
{
    return halo::shell::TreeNode::allocate_head();
}

hwreq_map_node * tree_node_allocate(uint32_t left, uint32_t parent, uint32_t right, uint8_t color, const hwreq_map_value_type *source)
{
    return halo::shell::TreeNode::allocate(left, parent, right, color, source);
}

hwreq_map_node * tree_lower_bound(msvc_std_map *tree, const msvc_std_string *search_key)
{
    return halo::shell::StdMap(tree).lower_bound(search_key);
}

hwreq_map_node * hwreq_map_find(msvc_std_map *map, msvc_std_string *key)
{
    return halo::shell::StdMap(map).find(key);
}

void tree_rotate_left(hwreq_map_node *x, msvc_std_map *tree)
{
    halo::shell::StdMap(tree).rotate_left(x);
}

void tree_rotate_right(hwreq_map_node *x, msvc_std_map *tree)
{
    halo::shell::StdMap(tree).rotate_right(x);
}

hwreq_map_node ** tree_splice_insert(msvc_std_map *tree, hwreq_map_node *parent, hwreq_map_node **result_holder, uint8_t insert_as_left, const hwreq_map_value_type *value)
{
    return halo::shell::StdMap(tree).splice_insert(parent, result_holder, insert_as_left, value);
}

void tree_insert_unique(msvc_std_map *tree, hwreq_tree_insert_result *result, const hwreq_map_value_type *value)
{
    halo::shell::StdMap(tree).insert_unique(result, value);
}

hwreq_map_node * tree_hint_insert_unique(msvc_std_map *tree, hwreq_map_node **result_holder, hwreq_map_node *hint, const hwreq_map_value_type *value)
{
    return halo::shell::StdMap(tree).hint_insert_unique(result_holder, hint, value);
}

hwreq_map_node ** tree_erase_one(msvc_std_map *tree, hwreq_map_node **result_holder, hwreq_map_node *erased)
{
    return halo::shell::StdMap(tree).erase_one(result_holder, erased);
}

hwreq_map_node ** tree_erase_range(hwreq_map_node **out, hwreq_map_node *first, hwreq_map_node *last, msvc_std_map *tree)
{
    return halo::shell::StdMap(tree).erase_range(out, first, last);
}

void hwreq_map_destruct(msvc_std_map *self)
{
    halo::shell::StdMap(self).destruct();
}

hwreq_property_set ** hwreq_property_set_map_index(msvc_std_string *key, msvc_std_map *map)
{
    return halo::shell::StdMap(map).index_property_set(key);
}

hwreq_parse_exception * hwreq_parse_exception_construct(hwreq_parse_exception *self, const msvc_std_string *message)
{
    return halo::shell::ParseException(self).construct(message);
}

hwreq_parse_exception * hwreq_parse_exception_copy_construct(hwreq_parse_exception *self, const hwreq_parse_exception *other)
{
    return halo::shell::ParseException(self).copy_construct(other);
}

void hwreq_parse_exception_destruct(hwreq_parse_exception *self)
{
    halo::shell::ParseException(self).destruct();
}

void hwreq_parse_exception_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag)
{
    halo::shell::ParseException(self).scalar_deleting_destruct(free_flag);
}

void hwreq_length_error_destruct(hwreq_parse_exception *self)
{
    halo::shell::ParseException(self).length_error_destruct();
}

void hwreq_length_error_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag)
{
    halo::shell::ParseException(self).length_error_scalar_deleting_destruct(free_flag);
}

void hwreq_out_of_range_destruct(hwreq_parse_exception *self)
{
    halo::shell::ParseException(self).out_of_range_destruct();
}

void hwreq_out_of_range_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag)
{
    halo::shell::ParseException(self).out_of_range_scalar_deleting_destruct(free_flag);
}

uint32_t hwreq_token_match_keyword(const char *keyword, hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).match_keyword(keyword);
}

int32_t hwreq_token_parse_hex_digit(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).parse_hex_digit();
}

uint32_t hwreq_token_parse_hex_id(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).parse_hex_id();
}

int32_t hwreq_token_parse_hex_id_byteswap(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).parse_hex_id_byteswap();
}

int32_t hwreq_token_parse_number(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).parse_number();
}

char * hwreq_token_parse_quoted_string(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).parse_quoted_string();
}

void hwreq_token_skip_line(hwreq_parser *parser)
{
    halo::shell::HwreqParser(parser).skip_line();
}

void hwreq_token_skip_whitespace(hwreq_parser *parser)
{
    halo::shell::HwreqParser(parser).skip_whitespace();
}

void hwreq_parser_report_error(hwreq_parser *parser, const char *message)
{
    halo::shell::HwreqParser(parser).report_error(message);
}

hwreq_parser * hwreq_parser_construct(hwreq_parser *this_)
{
    return halo::shell::HwreqParser(this_).construct();
}

hwreq_parser * hwreq_parser_create(void)
{
    return halo::shell::HwreqParser::create();
}

void hwreq_parser_destruct(hwreq_parser *self)
{
    halo::shell::HwreqParser(self).destruct();
}

void hwreq_parser_scalar_deleting_destructor(hwreq_parser *this_parser)
{
    halo::shell::HwreqParser(this_parser).scalar_deleting_destruct();
}

char * hwreq_parser_get_error_message(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).error_message_text();
}

uint32_t hwreq_parser_get_flag_count(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).flag_count();
}

char * hwreq_parser_get_flag_name(hwreq_parser *parser, uint32_t index)
{
    return halo::shell::HwreqParser(parser).flag_name(index);
}

char * hwreq_parser_get_flag_value(hwreq_parser *parser, uint32_t index)
{
    return halo::shell::HwreqParser(parser).flag_value(index);
}

hwreq_property_set * hwreq_parser_get_flags(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).flags_set();
}

char * hwreq_parser_get_graphics_device_name(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).graphics_device_name_text();
}

char * hwreq_parser_get_graphics_vendor_name(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).graphics_vendor_name_text();
}

uint32_t hwreq_parser_get_requirement_count(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).requirement_count();
}

char * hwreq_parser_get_requirement_name(hwreq_parser *parser, uint32_t index)
{
    return halo::shell::HwreqParser(parser).requirement_name(index);
}

char * hwreq_parser_get_requirement_value(hwreq_parser *parser, uint32_t index)
{
    return halo::shell::HwreqParser(parser).requirement_value(index);
}

char * hwreq_parser_get_sound_device_name(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).sound_device_name_text();
}

char * hwreq_parser_get_sound_vendor_name(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).sound_vendor_name_text();
}

uint8_t hwreq_parser_has_error(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).has_error();
}

hwreq_property_set * hwreq_parser_find_property_set(hwreq_parser *self, const char *name)
{
    return halo::shell::HwreqParser(self).find_property_set(name);
}

uint8_t hwreq_parser_find_requirements_section(hwreq_parser *self)
{
    return halo::shell::HwreqParser(self).find_requirements_section();
}

uint8_t hwreq_parser_parse(hwreq_parser *parser, const char *path, const shell_sound_device *sound_device, const d3d_adapter_identifier9 *adapter, const d3d_caps9 *caps, uint32_t memory, uint32_t video_memory, uint32_t cpu_speed)
{
    return halo::shell::HwreqParser(parser).parse(path, sound_device, adapter, caps, memory, video_memory, cpu_speed);
}

uint8_t hwreq_parser_parse_block(hwreq_parser *self, hwreq_property_set *target)
{
    return halo::shell::HwreqParser(self).parse_block(target);
}

const char * hwreq_parser_parse_flag_assignment(hwreq_parser *parser, hwreq_property_set *property_set)
{
    return halo::shell::HwreqParser(parser).parse_flag_assignment(property_set);
}

uint8_t hwreq_parser_parse_propertyset_directive(hwreq_parser *self)
{
    return halo::shell::HwreqParser(self).parse_propertyset_directive();
}

uint8_t hwreq_parser_parse_vendor_block(hwreq_parser *self)
{
    return halo::shell::HwreqParser(self).parse_vendor_block();
}

uint8_t hwreq_parser_parse_audiovendor_block(hwreq_parser *self)
{
    return halo::shell::HwreqParser(self).parse_audiovendor_block();
}

uint8_t hwreq_parser_scan_for_applytoall(hwreq_parser *self)
{
    return halo::shell::HwreqParser(self).scan_for_applytoall();
}

uint8_t hwreq_parser_scan_for_applytoall_or_vendor(hwreq_parser *self)
{
    return halo::shell::HwreqParser(self).scan_for_applytoall_or_vendor();
}

const char * hwreq_parser_evaluate_condition(hwreq_parser *parser, int32_t kind, uint32_t value)
{
    return halo::shell::HwreqParser(parser).evaluate_condition(kind, value);
}

const char * hwreq_d3dcaps_field_resolve(hwreq_parser *parser)
{
    return halo::shell::HwreqParser(parser).resolve_field();
}

void *exception_copy_construct(void *self_, const void *other)
{
    return halo::shell::StdException(self_).copy_construct(other);
}

void exception_destruct(void *self_)
{
    halo::shell::StdException(self_).destruct();
}

const char *__fastcall std_exception_what(uint8_t *self, void *unused_edx)
{
    (void)unused_edx;
    return halo::shell::StdRuntimeError(self).what();
}

hwreq_parse_exception *__fastcall std_length_error_copy_construct(hwreq_parse_exception *self, void *unused_edx,
    const hwreq_parse_exception *other)
{
    (void)unused_edx;
    return halo::shell::ParseException(self).length_error_copy_construct(other);
}

hwreq_parse_exception *__fastcall std_out_of_range_copy_construct(hwreq_parse_exception *self, void *unused_edx,
    const hwreq_parse_exception *other)
{
    (void)unused_edx;
    return halo::shell::ParseException(self).out_of_range_copy_construct(other);
}

void string_throw_length_error(void)
{
    halo::shell::StdThrow::string_too_long();
}

void string_throw_out_of_range(void)
{
    halo::shell::StdThrow::string_out_of_range();
}

void hwreq_pair_vector_throw_length_error(void)
{
    halo::shell::StdThrow::vector_too_long();
}

void hwreq_vector_throw_out_of_range(void)
{
    halo::shell::StdThrow::vector_subscript();
}

void hwreq_map_node_key_destruct(hwreq_map_node *node)
{
    halo::shell::StdString(&node->key).destroy();
}

uint32_t clipboard_get_text(char *buffer, uint32_t capacity)
{
    return halo::shell::Clipboard::get_text(buffer, capacity);
}

uint8_t command_line_check_flag(const char *flag_name, const char **out_value)
{
    return halo::shell::CommandLine::has_flag(flag_name, out_value);
}

char **command_line_parse_to_argv(char *command_line, int32_t *out_count)
{
    return halo::shell::CommandLine::parse_to_argv(command_line, out_count);
}

uint8_t compute_sha1_hash(const uint8_t *data, uint32_t length, uint8_t *digest_out)
{
    return halo::shell::Sha1::hash(data, length, digest_out);
}

uint8_t compute_sha1_hash_first_qword(const uint8_t *data, uint32_t length, uint32_t *output)
{
    return halo::shell::Sha1::hash_first_qword(data, length, output);
}

uint8_t config_compute_uma_video_memory(void)
{
    return halo::shell::Config::compute_uma_video_memory();
}

void config_reset_system_requirements(void)
{
    halo::shell::Config::reset_system_requirements();
}

uint8_t config_set_decal_slope_z_bias(char *value)
{
    return halo::shell::Config::set_float(&config_decal_slope_z_bias, value);
}

uint8_t config_set_decal_z_bias(char *value)
{
    return halo::shell::Config::set_float(&config_decal_z_bias, value);
}

uint8_t config_set_transparent_decal_slope_z_bias(char *value)
{
    return halo::shell::Config::set_float(&config_transparent_decal_slope_z_bias, value);
}

uint8_t config_set_transparent_decal_z_bias(char *value)
{
    return halo::shell::Config::set_float(&config_transparent_decal_z_bias, value);
}

uint8_t config_set_maximum_resolution(char *value)
{
    return halo::shell::Config::set_maximum_resolution(value);
}

uint8_t config_set_force_shader(const char *value)
{
    return halo::shell::Config::set_force_shader(value);
}

uint8_t config_set_disable_alpha_render_targets(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_alpha_render_targets);
}

uint8_t config_set_disable_buffering(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_buffering);
}

uint8_t config_set_disable_driver_management(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_driver_management);
}

uint8_t config_set_disable_render_targets(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_render_targets);
}

uint8_t config_set_disable_specular(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_disable_specular);
}

uint8_t config_set_enable_stop_start(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_enable_stop_start);
}

uint8_t config_set_head_relative_speech(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_head_relative_speech);
}

uint8_t config_set_invalid_driver(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_invalid_driver);
}

uint8_t config_set_invalid_sound_driver(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_invalid_sound_driver);
}

uint8_t config_set_linear_texture_addressing(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_linear_texture_addressing);
}

uint8_t config_set_linear_texture_addressing_sun(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_linear_texture_addressing_sun);
}

uint8_t config_set_linear_texture_addressing_zoom(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_linear_texture_addressing_zoom);
}

uint8_t config_set_min_max_blend_op_is_broken(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_min_max_blend_op_is_broken);
}

uint8_t config_set_old_driver(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_old_driver);
}

uint8_t config_set_old_sound_driver(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_old_sound_driver);
}

uint8_t config_set_prototype_card(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_prototype_card);
}

uint8_t config_set_safe_mode(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_safe_mode);
}

uint8_t config_set_unsupported_card(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_unsupported_card);
}

uint8_t config_set_use_alternate_convolve_mask(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_use_alternate_convolve_mask);
}

uint8_t config_set_use_anisotropic_filter(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_use_anisotropic_filter);
}

uint8_t config_set_use_fixed_function(const char *value)
{
    (void)value;
    return halo::shell::Config::set_flag(&config_use_fixed_function);
}
int32_t cpu_get_type(int32_t mode)
{
    return halo::shell::Cpu::get_type(mode);
}

int32_t cpu_query_identification(void)
{
    return halo::shell::Cpu::query_identification();
}

int32_t __stdcall dialog_center_on_screen(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    return halo::shell::DialogCentering::procedure(hwnd, message, wparam, lparam);
}

uint8_t engine_initialize_subsystems(void)
{
    return halo::shell::EngineLifecycle::initialize();
}

void engine_shutdown_subsystems(void)
{
    halo::shell::EngineLifecycle::shutdown();
}

int32_t __stdcall exception_filter_crash_reporter(win32_exception_pointers *exception_pointers)
{
    return halo::shell::CrashReporter::current().handle_exception(exception_pointers);
}

int32_t extract_product_id_digits(const char *product_id)
{
    return halo::shell::ProductId::extract_digits(product_id);
}

void game_single_instance_check(int32_t mode)
{
    halo::shell::SingleInstance::check(mode);
}

void hex_string_to_bytes(uint8_t *dest, const char *source)
{
    halo::shell::HexParser::to_bytes(dest, source);
}

int32_t hex_string_to_uint(char *string)
{
    return halo::shell::HexParser::to_uint(string);
}

void keystone_library_load(void)
{
    halo::shell::KeystoneLibrary::load();
}

void keystone_library_unload(void)
{
    halo::shell::KeystoneLibrary::unload();
}

void os_platform_identify(void)
{
    halo::shell::OperatingSystem::identify();
}

void security_check_cleanup(void *descriptor, void *acl, void *sid, void *thread_token, void *impersonation_token,
                            void *sentinel)
{
    halo::shell::SecurityResources::release(descriptor, acl, sid, thread_token, impersonation_token, sentinel);
}

int32_t security_check_write_access(void)
{
    return halo::shell::WriteAccessCheck::run();
}

char *shell_build_product_id_string(void)
{
    return halo::shell::ProductId::build_string();
}

int32_t shell_check_previous_run_crash(void)
{
    return halo::shell::ExitFlag::previous_run_crashed();
}

void shell_detect_hardware_specs(void)
{
    halo::shell::HardwareProbe::current().detect();
}

int32_t __stdcall shell_display_adapter_enumerate_callback(void *guid, char *description, char *driver_name,
                                                           void *context, void *monitor)
{
    return halo::shell::Win32HardwareProbe::enumerate_display_adapter(guid, description, driver_name, context, monitor);
}

int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal)
{
    return halo::shell::FatalError::show(resource_id, help_text_or_id, is_fatal);
}

void shell_handle_activate_app(uint8_t inactive)
{
    halo::shell::GameWindow::handle_activate_app(inactive);
}

void shell_init_localization_strings(void)
{
    halo::shell::Localization::initialize();
}

int32_t shell_load_localized_string(uint32_t buffer_capacity, void *module, char *buffer, uint32_t id)
{
    return halo::shell::Localization::load_localized_string(buffer_capacity, module, buffer, id);
}

int32_t shell_load_string_resource(uint32_t id, uint16_t language, uint32_t buffer_capacity, void *module, char *buffer)
{
    return halo::shell::Localization::load_string_resource(id, language, buffer_capacity, module, buffer);
}

char *shell_parse_config_txt(uint32_t adapter_index, d3d9_interface *d3d)
{
    return halo::shell::ConfigLoader::parse(adapter_index, d3d);
}

void shell_pump_windows_messages(void)
{
    halo::shell::GameWindow::pump_messages();
}

void shell_registry_set_exit_flag_clean(void)
{
    halo::shell::ExitFlag::set_clean();
}

int32_t __stdcall shell_window_procedure(HWND hwnd, uint32_t message, uint32_t wparam, int32_t lparam)
{
    return halo::shell::GameWindow::procedure(hwnd, message, wparam, lparam);
}

int32_t __stdcall shell_winmain(void *hInstance, void *hPrevInstance, char *lpCmdLine, int32_t nCmdShow)
{
    return halo::shell::Application::winmain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

}
