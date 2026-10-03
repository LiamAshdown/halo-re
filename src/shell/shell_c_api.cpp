#include "halo/shell/hwreq.hpp"

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

}
