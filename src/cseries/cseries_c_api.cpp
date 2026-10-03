#include "halo/cseries/cseries.hpp"

using halo::cseries::md5_context;
using halo::cseries::tea_key;

extern "C" {

void md5_update(md5_context *context, const uint8_t *input, uint32_t length)
{
    context->update(input, length);
}

void md5_final(uint8_t *digest, md5_context *context)
{
    context->finish(digest);
}

void md5_transform(const uint8_t *block, md5_context *context)
{
    context->transform(block);
}

void md5_digest_to_hex(const uint8_t *digest, char *out)
{
    halo::cseries::md5_context::to_hex(digest, out);
}

void md5_hex_digest(const uint8_t *data, int32_t length, char *out)
{
    halo::cseries::md5_context::hex_digest(data, length, out);
}

void tea_encrypt_block(uint32_t *block, const uint32_t *key)
{
    reinterpret_cast<const tea_key *>(key)->encrypt_block(block);
}

void tea_decrypt_block(uint32_t *block, const uint32_t *key)
{
    reinterpret_cast<const tea_key *>(key)->decrypt_block(block);
}

void tea_encrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key)
{
    reinterpret_cast<const tea_key *>(key)->encrypt_buffer(length, data);
}

void tea_decrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key)
{
    reinterpret_cast<const tea_key *>(key)->decrypt_buffer(length, data);
}

void qsort_dword_array(uint32_t count, int32_t *elements, qsort_dword_compare_proc compare)
{
    halo::cseries::dword_sort::sort(count, elements, compare);
}

void qsort_dword_array_shortsort(int32_t *last, int32_t *first, qsort_dword_compare_proc compare)
{
    halo::cseries::dword_sort::shortsort(last, first, compare);
}

uint32_t time_query_performance_counter_ms(void)
{
    return halo::cseries::performance_clock::milliseconds();
}

void *memory_global_alloc(uint32_t size)
{
    return halo::cseries::global_memory::alloc(size);
}

void *memory_global_free(void *handle)
{
    return halo::cseries::global_memory::release(handle);
}

char directory_create_recursive(char *path)
{
    return halo::cseries::directory_create_recursive(path);
}

char *string_to_lowercase(char *string)
{
    return halo::cseries::string_to_lowercase(string);
}

void function_do_nothing(void)
{
    halo::cseries::function_do_nothing();
}

void write_to_error_file(char *message, uint8_t with_timestamp)
{
    halo::cseries::error_log::write(message, with_timestamp);
}

void profile_path_initialize(void)
{
    halo::cseries::profile_path::initialize();
}

} // extern "C"
