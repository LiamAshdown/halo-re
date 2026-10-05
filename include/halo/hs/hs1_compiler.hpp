#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "hs.h"

namespace halo::hs {

/**
 * The hs script compiler front end: source compilation, expression compilation, post-processing and
 * registration of scenario scripts and globals.
 */
class ScriptCompiler {
public:
    static char add_global(datum_index node_index);
    static char add_script(datum_index node_index);
    static void compile(int32_t source_length, char *source_text, char **error_message, int32_t *error_offset);
    static char compile_and_evaluate(char *command);
    static datum_index compile_expression(char *text, uint32_t length, char **error_message, char **error_offset);
    static char compile_postprocess(char **error_message, int32_t *error_offset);
    static char compile_source(void);
};

}
