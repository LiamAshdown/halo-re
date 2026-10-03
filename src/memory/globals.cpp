/**
 * @file src/memory/globals.cpp
 * Binds halo::memory::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/memory/globals.hpp"
#include "halo/memory/api.hpp"

extern "C" {
extern crc32_table crc32_lookup_table;
extern uint8_t crc32_lookup_table_initialized;
extern char *data_packet_group_error;
extern byte_swap_definition packet_header_byte_swap_definition;
extern uint8_t bit_mask_keep[9];
extern uint8_t bit_mask_clear[8];
}

namespace halo::memory {

const Globals memory_globals{
    ::crc32_lookup_table,
    ::crc32_lookup_table_initialized,
    ::data_packet_group_error,
    ::packet_header_byte_swap_definition,
    ::bit_mask_keep,
    ::bit_mask_clear,
};

}  // namespace halo::memory
