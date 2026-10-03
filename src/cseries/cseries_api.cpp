#include "halo/cseries/cseries.hpp"
#include "halo/cseries/api.hpp"

using halo::cseries::md5_context;
using halo::cseries::tea_key;

namespace halo::cseries {

void md5_hex_digest(const uint8_t *data, int32_t length, char *out)
{
    halo::cseries::md5_context::hex_digest(data, length, out);
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

void write_to_error_file(char *message, uint8_t with_timestamp)
{
    halo::cseries::error_log::write(message, with_timestamp);
}

void profile_path_initialize(void)
{
    halo::cseries::profile_path::initialize();
}

}  // namespace halo::cseries

/**
 * C entry points for the vendored C GameSpy sources (src/gamespy stays C and cannot call into namespaces).
 */
extern "C" {

void md5_hex_digest(const uint8_t *data, int32_t length, char *out)
{
    halo::cseries::md5_hex_digest(data, length, out);
}

void tea_encrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key)
{
    halo::cseries::tea_encrypt_buffer(length, data, key);
}

void tea_decrypt_buffer(int32_t length, uint8_t *data, const uint32_t *key)
{
    halo::cseries::tea_decrypt_buffer(length, data, key);
}

}
