#include "halo/hs/hs3_parser.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "win32.h"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

typedef struct rebuild_file_reference {
    uint32_t signature;
    uint8_t flags;
    uint8_t unknown_005;
    int16_t location;
    char path[0x100];
    void *handle;
}

rebuild_file_reference;

extern "C" {
extern uint8_t file_reference_exists(rebuild_file_reference *ref);
extern void file_enumerate_start(uint32_t flags, rebuild_file_reference *ref);
extern uint8_t file_enumerate_find_next(rebuild_file_reference *out_entry, uint32_t *out_write_time);
extern void path_append_component(char *destination, const char *component);
extern void path_remove_last_component(char *path);
extern void path_build_full(char *source, char *destination, int16_t location);
extern void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out,
    char **name_end_out, char **ext_start_out, uint8_t split_extension);
extern int32_t file_reference_compare_full_path(const void *a, const void *b);
extern datum_index global_scenario_index;
extern char *hs_compiled_source;
extern int32_t hs_compiled_source_length;
extern void hs_tokenize_primitive(char **cursor, datum_index node_index);
extern void hs_tokenize_nonprimitive(datum_index node_index, char **cursor);
extern data_array *hs_syntax_data;
extern char *hs_compile_error;
extern datum_index hs_tokenize(char **cursor);
extern void skip_whitespace(char **cursor);
extern int32_t hs_compile_error_offset;
extern char hs_space_characters[2];
extern char hs_newline_characters[2];
extern uint8_t hs_preserve_token_case;
}

namespace halo::hs::part3 {

/**
 * Looks for the HS source on disk: "data\global_scripts.hsc" and every *.hsc in "data\<scenario tag
 * directory>\scripts" (at most 8 entries, sorted). Returns 1 when NOTHING was found -- each existing file clears
 * it.
 *
 * @address 0x483e20
 */
char SourceTokenizer::rebuild_source() const
{
    char directory_path[0x100];
    char display_name[0x100];
    char extension[0x100];
    rebuild_file_reference global_scripts;
    rebuild_file_reference scripts_directory;
    rebuild_file_reference entries[8];
    char *dir_start;
    char *ext_fallback;
    char *name_end;
    char *ext_start;
    char nothing_found = 1;
    int16_t count;
    int16_t i;

    sprintf(directory_path, "data\\%s", halo::cache::globals().tag_instances[(int16_t)global_scenario_index].path);
    sprintf(strrchr(directory_path, '\\') + 1, "scripts");

    memset(&global_scripts, 0, sizeof(global_scripts));
    global_scripts.signature = 0x66696c6f;
    global_scripts.location = -1;
    if ((global_scripts.flags & 1) != 0) {
        path_remove_last_component(global_scripts.path);
    }
    path_append_component(global_scripts.path, "data\\global_scripts.hsc");
    global_scripts.flags |= 1;
    if (file_reference_exists(&global_scripts) != 0) {
        file_reference_exists(&global_scripts);
        nothing_found = 0;
    }

    memset(&scripts_directory, 0, sizeof(scripts_directory));
    scripts_directory.signature = 0x66696c6f;
    scripts_directory.location = -1;
    path_append_component(scripts_directory.path, directory_path);
    file_enumerate_start(0, &scripts_directory);
    for (count = 0; count < 8; count++) {
        if (file_enumerate_find_next(&entries[count], 0) == 0) {
            break;
        }
    }
    qsort(entries, count, sizeof(rebuild_file_reference), (int (*)(const void *, const void *))file_reference_compare_full_path);

    for (i = 0; i < count; i++) {
        memset(display_name, 0, sizeof(display_name));
        path_build_full(entries[i].path, display_name, entries[i].location);
        path_split_components(&dir_start, display_name, &ext_fallback, &name_end, &ext_start, (uint8_t)(entries[i].flags & 1));
        extension[0] = 0;
        if (*ext_start != 0) {
            char *end = extension + strlen(extension);

            if (end != extension) {
                *end++ = '.';
                *end = 0;
            }
            strncpy(end, ext_start, 0xff - strlen(extension));
            extension[0xff] = 0;
        }
        if (strcmp(extension, "hsc") == 0) {
            file_reference_exists(&entries[i]);
            nothing_found = 0;
        }
    }
    return nothing_found;
}

/**
 * Appends `length` bytes from `text` to the shared growable hs_compiled_source buffer (allocating or growing it
 * as needed) and returns a pointer to the start of the appended region, or NULL on allocation failure.
 *
 * @address 0x4856f0
 */
char *SourceTokenizer::source_buffer_append(char *text, uint32_t length) const
{
    void *new_buffer;
    char *dest;
    char *result;
    uint32_t words;
    uint32_t tail_bytes;
    uint32_t new_size;

    new_size = (uint32_t)hs_compiled_source_length + 1 + length;
    if (hs_compiled_source == 0) {
        new_buffer = GlobalAlloc(0, new_size);
    } else {
        if (new_size == 0) {
            GlobalFree(hs_compiled_source);
            return 0;
        }
        new_buffer = GlobalReAlloc(hs_compiled_source, new_size, 2);
    }
    if (new_buffer == 0) {
        return 0;
    }
    result = (char *)new_buffer + hs_compiled_source_length;
    dest = result;
    hs_compiled_source = (char *)new_buffer;
    for (words = length >> 2; words != 0; words = words - 1) {
        *(uint32_t *)dest = *(uint32_t *)text;
        text = text + 4;
        dest = dest + 4;
    }
    for (tail_bytes = length & 3; tail_bytes != 0; tail_bytes = tail_bytes - 1) {
        *dest = *text;
        text = text + 1;
        dest = dest + 1;
    }
    hs_compiled_source_length = hs_compiled_source_length + length;
    hs_compiled_source[hs_compiled_source_length] = 0;
    return result;
}

/**
 * Allocates one syntax-node datum for the next token/expression at *cursor and dispatches to the primitive or
 * nonprimitive tokenizer depending on whether it starts with '('.
 *
 * @address 0x486120
 */
datum_index SourceTokenizer::tokenize(char **cursor) const
{
    data_array *nodes;
    datum_index index;
    hs_syntax_node *node;

    nodes = hs_syntax_data;
    index = halo::memory::datum_new(nodes);
    if (index == k_datum_index_none) {
        hs_compile_error = (char *)"i couldn't allocate a syntax node.";
        return k_datum_index_none;
    }
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (index & halo::k_slot_mask) * nodes->size);
    node->index_union = (int16_t)halo::k_word_none;
    node->next_node = k_datum_index_none;
    node->flags = 0;
    node->type = 0;
    node->flags = (uint16_t)(**cursor != '(');
    if ((node->flags & _hs_syntax_node_primitive_bit) != 0) {
        hs_tokenize_primitive(cursor, index);
        return index;
    }
    hs_tokenize_nonprimitive(index, cursor);
    return index;
}

/**
 * Tokenizes a parenthesized list expression, recursively tokenizing each child element (via hs_tokenize) until
 * the matching ')', threading them through node->data/next_node.
 *
 * @address 0x486290
 */
void SourceTokenizer::tokenize_nonprimitive(datum_index node_index, char **cursor) const
{
    data_array *nodes;
    hs_syntax_node *node;
    void *first_child_slot;
    void *child_slot;
    char *prev_cursor;
    datum_index child_index;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & halo::k_slot_mask) * nodes->size);
    node->source_offset = (int32_t)(*cursor - hs_compiled_source);
    *cursor = *cursor + 1;
    first_child_slot = &node->data;
    child_slot = first_child_slot;
    for (;;) {
        if (hs_compile_error != 0) {
            goto empty_check;
        }
        prev_cursor = *cursor;
        skip_whitespace(cursor);
        if (*cursor != prev_cursor) {
            *prev_cursor = '\0';
        }
        if (**cursor == '\0') {
            hs_compile_error = (char *)"this left parenthesis is unmatched.";
            hs_compile_error_offset = node->source_offset;
            goto empty_check;
        }
        if (**cursor == ')') {
            **cursor = '\0';
            *cursor = *cursor + 1;
            goto empty_check;
        }
        child_index = hs_tokenize(cursor);
        *(datum_index *)child_slot = child_index;
        if (child_index != k_datum_index_none) {
            child_slot = (uint8_t *)hs_syntax_data->data + 8 + (child_index & halo::k_slot_mask) * hs_syntax_data->size;
        }
        continue;
    empty_check:
        if ((child_slot == first_child_slot) && (hs_compile_error == 0)) {
            hs_compile_error = (char *)"this expression is empty.";
            hs_compile_error_offset = node->source_offset;
        }
        return;
    }
}

/**
 * Tokenizes a single primitive value (quoted string or bare word) starting at *cursor into the syntax node
 * `node_index`, recording its source_offset and advancing *cursor past it.
 *
 * @address 0x4861b0
 */
void SourceTokenizer::tokenize_primitive(char **cursor, datum_index node_index) const
{
    hs_syntax_node *node;
    char *start;
    char *p;
    char c;
    int16_t i;

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & halo::k_slot_mask) * hs_syntax_data->size);
    start = *cursor;
    if (*start == '"') {
        *cursor = start + 1;
        node->source_offset = (int32_t)(start + 1 - hs_compiled_source);
        c = **cursor;
        while ((c != '\0') && (**cursor != '"')) {
            p = *cursor + 1;
            *cursor = p;
            c = *p;
        }
        if (**cursor == '\0') {
            hs_compile_error = (char *)"this quoted constant is unterminated.";
            hs_compile_error_offset = node->source_offset - 1;
        }
        **cursor = '\0';
        *cursor = *cursor + 1;
    } else {
        node->source_offset = (int32_t)(start - hs_compiled_source);
        if (**cursor != '\0') {
            for (;;) {
                c = **cursor;
                if ((c == ')') || (c == ';')) {
                    break;
                }
                i = 0;
                do {
                    if (c == hs_space_characters[i]) {
                        goto done;
                    }
                    i = i + 1;
                } while (i < 2);
                i = 0;
                do {
                    if (c == hs_newline_characters[i]) {
                        goto done;
                    }
                    i = i + 1;
                } while (i < 2);
                p = *cursor + 1;
                *cursor = p;
                if (*p == '\0') {
                    break;
                }
            }
        }
    }
done:
    if (hs_preserve_token_case == 0) {
        halo::cseries::string_to_lowercase(hs_compiled_source + node->source_offset);
    }
}

/**
 * Validates that `offset` still falls within [0, hs_compiled_source_length); flags a recompile-needed compile
 * error otherwise.
 *
 * @address 0x4858a0
 */
char SourceTokenizer::verify_source_offset(int32_t offset) const
{
    char valid;

    valid = 1;
    if ((offset < 0) || (hs_compiled_source_length <= offset)) {
        hs_compile_error = (char *)"bad source offset (you need to recompile.)";
        valid = 0;
    }
    return valid;
}

/**
 * Advances *cursor past whitespace and both line (";...") and block (";*...*;") HS comments.
 *
 * @address 0x486350
 */
void SourceTokenizer::skip_whitespace(char **cursor) const
{
    int16_t state;
    char *p;
    char c;
    int16_t i;

    state = 0;
top:
    if (state == 0) {
        p = *cursor;
        c = *p;
        if (c == ';') {
            *cursor = p + 1;
            state = 1;
            if (p[1] == '*') {
                state = 2;
                *cursor = p + 2;
            }
            goto top;
        }
        i = 0;
        for (;;) {
            if (c == hs_space_characters[i]) {
                goto found;
            }
            i = i + 1;
            if (!(i < 2)) {
                break;
            }
        }
        i = 0;
        while (c != hs_newline_characters[i]) {
            i = i + 1;
            if (1 < i) {
                return;
            }
        }
    found:
        *cursor = p + 1;
    } else if (state == 1) {
        p = *cursor;
        if (*p == '\0') {
            return;
        }
        i = 0;
        do {
            if (*p == hs_newline_characters[i]) {
                state = 0;
                break;
            }
            i = i + 1;
        } while (i < 2);
        *cursor = p + 1;
    } else {
        p = *cursor;
        if (*p == '\0') {
            hs_compile_error = (char *)"unterminated comment.";
            return;
        }
        if ((*p == '*') && (p[1] == ';')) {
            state = 0;
            *cursor = p + 1;
        }
        *cursor = *cursor + 1;
    }
    if (state == 3) {
        return;
    }
    goto top;
}

}
