/**
 * @file include/halo/cseries/api.hpp
 * Functions of the cseries module that other modules and the data tables call (namespace halo::cseries). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>



typedef uint8_t (*qsort_dword_compare_proc)(int32_t element, int32_t other);

namespace halo::cseries {

/**
 * The engine globals the cseries module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    int64_t &performance_frequency;
    uint8_t &debug_log_level;
};

Globals &globals();

void md5_hex_digest(const uint8_t *data, int32_t length, char *out);
void tea_encrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key);
void tea_decrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key);
void qsort_dword_array(uint32_t count, int32_t *elements, qsort_dword_compare_proc compare);
uint32_t time_query_performance_counter_ms(void);
void * memory_global_alloc(uint32_t size);
void * memory_global_free(void *handle);
char directory_create_recursive(char *path);
char * string_to_lowercase(char *string);
void function_do_nothing(void);
void write_to_error_file(char *message, uint8_t with_timestamp);
void profile_path_initialize(void);

}
