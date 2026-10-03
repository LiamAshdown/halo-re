#include "halo/memory/globals.hpp"
#include "halo/memory/memory.hpp"

#include "tags.h"
#include "halo/memory/api.hpp"

#define CRC32_POLYNOMIAL 0xedb88320u


namespace halo::memory {

/**
 * Generates the standard 256 entry reversed CRC-32 table (polynomial 0xedb88320) into this table.
 *
 * @address 0x4d0330
 */
void crc32_table_view::build()
{
    uint32_t seed;
    uint32_t value;
    int32_t bit;
    int32_t index;

    seed = 0;
    index = 0x100;
    while (index != 0) {
        bit = 8;
        value = seed;
        while (bit != 0) {
            if ((value & 1) == 0) {
                value = value >> 1;
            } else {
                value = (value >> 1) ^ CRC32_POLYNOMIAL;
            }
            bit = bit - 1;
        }
        this->entries[seed] = value;
        seed = seed + 1;
        index = index - 1;
    }
}

/**
 * Computes/continues a CRC-32 checksum over `length` bytes of `data`, folding into *crc. Builds the
 * CRC-32 lookup table on first use.
 *
 * @address 0x4d02d0
 */
void crc32_update(uint32_t *crc, const void *bytes, int32_t length)
{
    const uint8_t *data = static_cast<const uint8_t *>(bytes);
    uint32_t value;

    if (globals().crc32_lookup_table_initialized == 0) {
        halo::memory::view(&globals().crc32_lookup_table)->build();
        globals().crc32_lookup_table_initialized = 1;
    }
    value = *crc;
    if (0 < length) {
        do {
            value = (value >> 8) ^ globals().crc32_lookup_table.entries[(*data ^ value) & 0xff];
            data = data + 1;
            length = length - 1;
        } while (length != 0);
    }
    *crc = value;
}

} // namespace halo::memory
