/**
 * @file include/halo/memory/globals.hpp
 * The memory module's engine globals as one service object. The variables live at fixed addresses in the data
 * image (standalone/data) under their original link names; Globals holds a reference to each, so no other file
 * declares them.
 */
#pragma once

#include "memory.h"

namespace halo::memory {

/**
 * References to the memory module's engine variables.
 *
 * crc32_lookup_table (0x006b7b00) is built on first use and flagged by crc32_lookup_table_initialized.
 * data_packet_group_error holds the message of the last failed packet operation. bit_mask_keep[i] is
 * (1 << i) - 1 (entry 8 is 0xff) and bit_mask_clear[i] is 0xff << i.
 */
struct Globals {
    crc32_table &crc32_lookup_table;
    uint8_t &crc32_lookup_table_initialized;
    char *&data_packet_group_error;
    byte_swap_definition &packet_header_byte_swap_definition;
    uint8_t (&bit_mask_keep)[9];
    uint8_t (&bit_mask_clear)[8];
};

extern const Globals memory_globals;

/** The memory service object (single instance, constant-initialised). */
inline const Globals &globals() { return memory_globals; }

}  // namespace halo::memory
