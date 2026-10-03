/**
 * @file src/memory/globals.cpp
 * Binds halo::memory::Globals to the engine variables the data image defines under their original link names.
 */


#include "halo/core/crt.hpp"
#include "halo/memory/globals.hpp"
#include "link/memory.hpp"
#include "halo/memory/api.hpp"

namespace halo::memory {

const Globals &Service::instance()
{
    static const Globals state{
        ::crc32_lookup_table,
        ::crc32_lookup_table_initialized,
        ::data_packet_group_error,
        ::packet_header_byte_swap_definition,
        ::bit_mask_keep,
        ::bit_mask_clear,
    };
    return state;
}

}  // namespace halo::memory
