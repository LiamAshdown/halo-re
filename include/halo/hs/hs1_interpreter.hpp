#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#include "halo/hs/hs1_command.hpp"

namespace halo::hs {

/**
 * Value casts between hs types and the generic hs value coercion entry point.
 */
class ScriptCasts {
public:
    static int32_t enum_to_real(int32_t value);
    static int32_t identity(int32_t value);
    static int32_t long_to_boolean(int32_t value);
    static int32_t long_to_real(int32_t value);
    static datum_index object_name_to_object_list(int32_t name_index);
    static int32_t real_to_short(int32_t value);
    static int32_t short_to_boolean(int32_t value);
    static int32_t short_to_real(int32_t value);
    static int32_t coerce_value(int32_t value, hs_type_t dest_type, hs_type_t source_type);
};

/**
 * Evaluate handlers of the hs control-flow and arithmetic special forms (begin, and/or, comparisons, argument lists).
 */
class ScriptFlowCommands {
public:
    static void argument_list(uint32_t unused_param_1, uint32_t thread_index, int32_t value);
    static void arithmetic_reduce(int16_t opcode, uint32_t thread_index, char first);
    static void begin(int16_t function_index, uint32_t thread_index, char first);
    static void bind(int16_t function_index, uint32_t thread_index, char first);
    static void boolean_and_or(int16_t opcode, uint32_t thread_index, char first);
    static void comparison(int16_t function_index, uint32_t thread_index, char first);

    static const ScriptCommandGroup &commands();
};

}
