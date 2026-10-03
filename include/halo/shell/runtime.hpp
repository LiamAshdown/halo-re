#pragma once

#include "halo/shell/types.hpp"
#include "halo/shell/layout.hpp"

namespace halo::shell {

/**
 * View over an MSVC 7.1 std::string (inline buffer while capacity is 0xf, heap pointer above).
 */
class StdString {
public:
    explicit StdString(const msvc_std_string *value) : self(const_cast<msvc_std_string *>(value)) {}

    char *data() const
    {
        return self->capacity >= k_msvc_string_heap_capacity ? heap_pointer(*self) : self->buffer.inline_buffer;
    }

    /** The heap block of a string whose capacity exceeds the inline buffer (the 32-bit pointer the MSVC layout stores). */
    static char *heap_pointer(const msvc_std_string &string)
    {
        return reinterpret_cast<char *>(static_cast<uintptr_t>(string.buffer.heap_buffer));
    }

    static void set_heap_pointer(msvc_std_string &string, void *block)
    {
        string.buffer.heap_buffer = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block));
    }

    void init_empty() const
    {
        self->capacity = k_msvc_string_inline_capacity;
        self->size = 0;
        self->buffer.inline_buffer[0] = 0;
    }

    msvc_std_string *assign_substr(const msvc_std_string *right, uint32_t pos, uint32_t count);
    msvc_std_string *assign_n(const char *source, uint32_t count);
    int32_t compare(uint32_t n1, uint32_t pos, const char *s, uint32_t n2);
    msvc_std_string *erase(uint32_t pos, uint32_t count);
    void grow_reserve(uint32_t new_capacity, uint32_t preserve_count);
    msvc_std_string *construct_cstr(const char *source);
    void destroy();
    msvc_std_string *assign_cstr(const char *s);
    uint8_t less_than(const msvc_std_string *other);

    msvc_std_string *self;
};

/**
 * View over a std::pair<std::string, std::string> (one flag name/value of a property set).
 */
class StringPair {
public:
    explicit StringPair(const hwreq_string_pair *value) : self(const_cast<hwreq_string_pair *>(value)) {}

    hwreq_string_pair *construct(const msvc_std_string *first_source, const msvc_std_string *second_source);
    hwreq_string_pair *copy_construct(const hwreq_string_pair *source);
    void destroy();
    static hwreq_string_pair *copy_backward(hwreq_string_pair *first, hwreq_string_pair *last, hwreq_string_pair *dest_end);
    static void destroy_range(hwreq_string_pair *first, hwreq_string_pair *last);
    static void fill_range(hwreq_string_pair *first, hwreq_string_pair *last, const hwreq_string_pair *value);
    static hwreq_string_pair *uninit_copy(hwreq_string_pair *source_begin, hwreq_string_pair *source_end, hwreq_string_pair *dest);
    static void uninit_fill_n(hwreq_string_pair *dest, uint32_t count, const hwreq_string_pair *value);

    hwreq_string_pair *self;
};

/**
 * View over a std::vector<hwreq_string_pair>.
 */
class PairVector {
public:
    explicit PairVector(const msvc_std_vector *value) : self(const_cast<msvc_std_vector *>(value)) {}

    int32_t element_count();
    void push_back(const hwreq_string_pair *value);
    hwreq_string_pair **insert(hwreq_string_pair **result, hwreq_string_pair *where, const hwreq_string_pair *value);
    void insert_n(hwreq_string_pair *where, uint32_t count, const hwreq_string_pair *value);

    msvc_std_vector *self;
};

/**
 * View over a hardware requirements property set: an ordered, case-insensitive flag list.
 */
class PropertySet {
public:
    explicit PropertySet(const hwreq_property_set *value) : self(const_cast<hwreq_property_set *>(value)) {}

    void upsert(char *key, char *value);
    void apply_to(hwreq_property_set *target);
    uint32_t find_value(const char *key, char *out_value, uint32_t capacity);
    void destroy_flags();
    static hwreq_property_set *create(hwreq_parser *parser);

    hwreq_property_set *self;
};

/**
 * Position inside a std::map red-black tree; steps in place.
 */
class TreeIterator {
public:
    explicit TreeIterator(hwreq_map_node **value) : self(value) {}

    void increment();
    void decrement();

    hwreq_map_node **self;
};

/**
 * View over one node of a std::map<std::string, hwreq_property_set *> red-black tree.
 */
class TreeNode {
public:
    explicit TreeNode(const hwreq_map_node *value) : self(const_cast<hwreq_map_node *>(value)) {}

    hwreq_map_node *find_max();
    static hwreq_map_node *find_min(hwreq_map_node **subtree_root_left_field);
    void destroy_subtree();
    static hwreq_map_node *allocate_head();
    static hwreq_map_node *allocate(uint32_t left, uint32_t parent, uint32_t right, uint8_t color, const hwreq_map_value_type *source);
    int32_t compare_key(const msvc_std_string *search_key);

    hwreq_map_node *self;
};

/**
 * View over a std::map<std::string, hwreq_property_set *> (red-black tree with a head sentinel).
 */
class StdMap {
public:
    explicit StdMap(const msvc_std_map *value) : self(const_cast<msvc_std_map *>(value)) {}

    hwreq_map_node *lower_bound(const msvc_std_string *search_key);
    hwreq_map_node *find(msvc_std_string *key);
    void rotate_left(hwreq_map_node *x);
    void rotate_right(hwreq_map_node *x);
    hwreq_map_node **splice_insert(hwreq_map_node *parent, hwreq_map_node **result_holder, uint8_t insert_as_left, const hwreq_map_value_type *value);
    void insert_unique(hwreq_tree_insert_result *result, const hwreq_map_value_type *value);
    hwreq_map_node *hint_insert_unique(hwreq_map_node **result_holder, hwreq_map_node *hint, const hwreq_map_value_type *value);
    hwreq_map_node **erase_one(hwreq_map_node **result_holder, hwreq_map_node *erased);
    hwreq_map_node **erase_range(hwreq_map_node **out, hwreq_map_node *first, hwreq_map_node *last);
    void destruct();
    hwreq_property_set **index_property_set(msvc_std_string *key);

    msvc_std_map *self;
};

/**
 * View over the std::logic_error family object the hwreq parser throws (vtable chooses the kind).
 */
class ParseException {
public:
    explicit ParseException(const hwreq_parse_exception *value) : self(const_cast<hwreq_parse_exception *>(value)) {}

    hwreq_parse_exception *length_error_copy_construct(const hwreq_parse_exception *other);
    hwreq_parse_exception *out_of_range_copy_construct(const hwreq_parse_exception *other);

    hwreq_parse_exception *construct(const msvc_std_string *message);
    hwreq_parse_exception *copy_construct(const hwreq_parse_exception *other);
    void destruct();
    void scalar_deleting_destruct(uint8_t free_flag);
    void length_error_destruct();
    void length_error_scalar_deleting_destruct(uint8_t free_flag);
    void out_of_range_destruct();
    void out_of_range_scalar_deleting_destruct(uint8_t free_flag);

    hwreq_parse_exception *self;
};

/**
 * View over a std::exception base object (vtable, message pointer and the owns-message flag).
 */
class StdException {
public:
    explicit StdException(void *value) : self((std_exception *)value) {}

    void *copy_construct(const void *other);
    void destruct();

    std_exception *self;
};

/**
 * View over a std::runtime_error style object: the std::exception base followed by the message string.
 */
class StdRuntimeError {
public:
    explicit StdRuntimeError(void *value) : self((std_runtime_error *)value) {}

    const char *what() const;

    std_runtime_error *self;
};

/**
 * Raises the std exceptions the statically linked container code throws. Each call builds a
 * logic_error-family object with the fixed message and hands it to the C++ runtime; none return.
 */
class StdThrow {
public:
    [[noreturn]] static void string_out_of_range();
    [[noreturn]] static void string_too_long();
    [[noreturn]] static void vector_too_long();
    [[noreturn]] static void vector_subscript();
    [[noreturn]] static void invalid_map_iterator();
    [[noreturn]] static void map_too_long();
};

}
