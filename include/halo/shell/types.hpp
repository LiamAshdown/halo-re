#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "crt.h"
#include "win32.h"

/**
 * std::logic_error family object as the hardware requirements parser builds it: the vtable pointer selects
 * logic_error, length_error or out_of_range, followed by the std::exception state and the message string.
 */
typedef struct hwreq_parse_exception {
    uint32_t vtable;
    uint32_t dofree;
    uint32_t legacy_what;
    msvc_std_string message;
} hwreq_parse_exception;

/**
 * Value type of the std::map<std::string, hwreq_property_set *> the parser keeps: key plus pointer.
 */
typedef struct hwreq_map_value_type {
    msvc_std_string key;
    uint32_t value;
} hwreq_map_value_type;

/**
 * Result of a unique insert into the property set map: the node and whether it was newly added.
 */
typedef struct hwreq_tree_insert_result {
    hwreq_map_node *node;
    uint8_t inserted;
} hwreq_tree_insert_result;

static_assert(sizeof(hwreq_parse_exception) == 0x28, "hwreq_parse_exception layout");
static_assert(sizeof(hwreq_map_value_type) == 0x20, "hwreq_map_value_type layout");
static_assert(sizeof(msvc_std_string) == 0x1c, "msvc_std_string layout");
static_assert(sizeof(msvc_std_vector) == 0x10, "msvc_std_vector layout");
static_assert(sizeof(msvc_std_map) == 0x0c, "msvc_std_map layout");
static_assert(sizeof(hwreq_map_node) == 0x30, "hwreq_map_node layout");
static_assert(sizeof(hwreq_string_pair) == 0x38, "hwreq_string_pair layout");
static_assert(sizeof(hwreq_property_set) == 0x14, "hwreq_property_set layout");
static_assert(sizeof(hwreq_parser) == k_hwreq_parser_size, "hwreq_parser layout");

namespace halo::shell {

/**
 * Length of a NUL terminated string, as the original code open-coded it.
 */
inline uint32_t cstr_length(const char *text)
{
    const char *p = text;
    while (*p != '\0') {
        p++;
    }
    return (uint32_t)(p - text);
}

}
