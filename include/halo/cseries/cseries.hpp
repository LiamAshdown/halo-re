#pragma once

#include "cseries.h"

#include <cstddef>
#include <type_traits>

namespace halo::cseries {

/**
 * RSA reference MD5 context (state, bit count, 64 byte block buffer) with the three streaming steps and
 * the two helpers that hash a whole buffer to 32 lowercase hex digits. The layout matches the 0x58 byte
 * context the original code zeroes as 0x16 dwords.
 */
struct md5_context {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];

    void update(const uint8_t *input, uint32_t length);
    void finish(uint8_t *digest);
    void transform(const uint8_t *block);
    static void to_hex(const uint8_t *digest, char *out);
    static void hex_digest(const uint8_t *data, int32_t length, char *out);
};

/**
 * A 128 bit TEA key viewed over four consecutive 32 bit words. Blocks are 8 bytes; the buffer
 * operations handle a trailing partial block by re-processing the last 8 bytes.
 */
struct tea_key {
    uint32_t words[4];

    void encrypt_block(uint32_t *block) const;
    void decrypt_block(uint32_t *block) const;
    void encrypt_buffer(int32_t length, uint8_t *data) const;
    void decrypt_buffer(int32_t length, uint8_t *data) const;
};

/**
 * The engine's private quicksort for arrays of 4 byte elements ordered by a byte-returning comparator.
 */
struct dword_sort {
    static void sort(uint32_t count, int32_t *elements, qsort_dword_compare_proc compare);
    static void shortsort(int32_t *last, int32_t *first, qsort_dword_compare_proc compare);
};

/**
 * Millisecond clock built on the high-resolution performance counter.
 */
struct performance_clock {
    static uint32_t milliseconds();
};

/**
 * Thin owner of the GlobalAlloc / GlobalFree block API used for fixed memory blocks.
 */
struct global_memory {
    static void *alloc(uint32_t size);
    static void *release(void *handle);
};

/**
 * The debug.txt error log. The one-time header state lives in the fixed-address error_file_needs_header
 * global that the shell data section defines.
 */
struct error_log {
    static void write(char *message, uint8_t with_timestamp);
};

/**
 * Start-up resolution of the profile directory (the -path override or the My Games folder).
 */
struct profile_path {
    static void initialize();
};

char directory_create_recursive(char *path);
char *string_to_lowercase(char *string);
void function_do_nothing();

} // namespace halo::cseries

static_assert(sizeof(halo::cseries::md5_context) == 0x58 && std::is_standard_layout_v<halo::cseries::md5_context>);
static_assert(sizeof(halo::cseries::tea_key) == 0x10 && std::is_standard_layout_v<halo::cseries::tea_key>);
