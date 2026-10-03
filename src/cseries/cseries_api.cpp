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

uint32_t time_query_performance_counter_ms(void)
{
    return halo::cseries::performance_clock::milliseconds();
}

void write_to_error_file(char *message, uint8_t with_timestamp)
{
    halo::cseries::error_log::write(message, with_timestamp);
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
